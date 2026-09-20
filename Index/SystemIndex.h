//
// Created by elliot on 9/20/26.
//

#ifndef HEYSTACK_SYSTEMINDEX_H
#define HEYSTACK_SYSTEMINDEX_H

#include <string>
#include <string_view>
#include <shared_mutex>
#include <unordered_map>
#include <vector>

#include "StringViewHash.h"
#include "../File/FileRecord.h"

/**
 * SystemIndex represents the core filesystem index,
 * holding the string arena of file names, the FileRecord vector,
 * and a mapping of string names to their byte offset in the string arena.
 */
class SystemIndex {

private:
    mutable std::shared_mutex index_mutex;

    /**
     * @brief vector of every indexed FileRecord
     */
    std::vector<FileRecord> records;

    /**
     * @brief Path hash to RecordId secondary index
     *
     * FSEvents can sometimes trigger a massive amount of updates,
     * so this is an O(1) lookup to retrieve the RecordId.
     *
     * @note Multimap on the very-rare chance that two paths collide / have the same hash.
     */
    std::unordered_multimap<uint64_t, RecordId> path_hash_to_id;

    /**
     * @brief  Index of the next unused record slot.
     *
     * Protected by index_mutex. Mutations occur while holding exclusive ownership
     * of the index.
     */
    size_t next_record_idx{1};

    /**
     * @brief Flat byte array for all deduplicated filenames.
     */
    std::vector<char> string_arena;

    /**
     * @brief Maps deduplicated strings to their byte offset in the string arena.
     */
    std::unordered_map<std::string, uint32_t, StringViewHash, std::equal_to<>> string_map;

    /** Internal Helpers
     * Unlocked (lock is acquired prior to invoking them).
     */
    RecordId addRecordUnlocked(uint32_t name_offset, uint64_t path_hash, RecordId parent, uint32_t size, FileFlags flags);
    uint32_t internStringUnlocked(std::string_view name);

    /** TODO: change to std::optional<std::string> for out-of-bounds case / INVALID_ID. */
    std::string getPathUnlocked(RecordId id) const;
    RecordId findRecordByPathUnlocked(std::string_view path) const;


public:
    /**
     * @brief Constructs a new System Index.
     *
     * Pre-allocates the internal records vector to hold 1 million records.
     */
    SystemIndex() {
        records.resize(1'000'000);
        string_arena.reserve(100 * 1024 * 1024);
        records[0] = {0, INVALID_ID, 0, FileFlags::Dir};
    }

    /**
     * @brief Adds a record to the index.
     *
     * Thread-safe. acquires exclusive ownership of index_mutex.
     *
     * @param name The deduplicated filename in the string_arena
     * @param full_path The full file path.
     * @param parent The RecordId of the parent directory
     * @param size The size of the file in bytes (0 for dir)
     * @param flags  FileFlags bitmask indicating file type & properties.
     * @return RecordId Unique id of the inserted record.
     */
    RecordId addRecord(std::string_view name, std::string_view full_path, RecordId parent, uint32_t size, FileFlags flags);

    /**
     * @brief Interns a string into the contiguous string_arena and returns its byte offset.
     *
     * Checks the deduplicated hash map for an existing occurrence of the string.
     * If present, returns the pre-existing byte offset.
     * Otherwise, appends the string with a null-terminator to the global byte arena, updates the hash map,
     * and returns the newly allocated starting offset.
     *
     * @note Thread-safe. Acquires a lock on `index_mutex` to synchronize access to the shared hash map
     * and the string arena.
     *
     * @param name A std::string_view representing the file or directory name.
     * @return uint32_t byte offset of the  string in the string arena.
     */
    uint32_t internString(std::string_view name);

    /**
     *
     * @return total number of records in the index.
     */
    size_t getTotalRecords() const {
        std::shared_lock lock(index_mutex);
        return next_record_idx;
    }

    /**
     * Resolves the path by walking backwards through the topology up to the root,
     * collecting the Parent Ids.
     *
     * It then iterates forward to assemble the strings from the arena, appending
     * directory separators.
     *
     * @todo platform-independent directory separators.
     *
     * @param id The unique identifier (RecordId) of the file or directory.
     * @return A std::string containing the absolute path. Returns "/" if the RecordId is root.
     */
    std::string getPath(RecordId id) const;

    /**
     * @brief Performs a basic linear search across the indexed files.
     *
     * Iterates through the records vector and performs a ranked substring match against the string arena.
     * @param query The substring to search for (case-insensitive)
     * @param limit The maximum number of results to return (default 25).
     * @return std::vector<RecordId> List of IDs matching the query.
     */
    std::vector<RecordId> search(const std::string_view query, size_t limit = 25) const;

    /**
     * @brief Locates a record's unique ID using its full absolute path.
     *
     * Performs a reverse-lookup by extracting the base filename from the provided path
     * and linearly scanning the records vector. It only constructs the full path via getPath()
     * when it finds a base filename collision..
     * @param path The full absolute file or directory path
     * @return RecordId The unique identifier of the matching record, or INVALID_ID if the path does not exist
     * in the active index.
     */
    RecordId findRecordByPath(std::string_view path) const;

    static uint64_t hashPath(std::string_view path) noexcept;
};

/**
 * Holds temporary scores during the search partial-order match.
 */
struct ScoredMatch {
    RecordId id;
    int score;
};

#endif //HEYSTACK_SYSTEMINDEX_H
