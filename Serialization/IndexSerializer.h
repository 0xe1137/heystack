//
// Created by elliot on 9/20/26.
//

#ifndef HEYSTACK_INDEXSERIALIZER_H
#define HEYSTACK_INDEXSERIALIZER_H

#include <string_view>
#include <cstdint>

// Forward Declaration
class SystemIndex;

#pragma pack(push, 1)
struct IndexHeader {
    char magic[4] = {'H', 'E', 'Y', 'S'};

    /**
     * Binary format version.
     */
    uint32_t version = 1;

    /**
     * Number of serialized FileRecords
     */
    uint32_t record_count;

    /**
     * Number of bytes in the string arena.
     */
    uint64_t arena_size;
};
#pragma pack(pop)

/**
 * Handles serialization to write/read index to/from disk.
 */
class IndexSerializer {
public:
    static bool saveToDisk(const SystemIndex& index, std::string_view filepath);

    static bool loadFromDisk(SystemIndex& index, std::string_view filepath);
};


#endif //HEYSTACK_INDEXSERIALIZER_H
