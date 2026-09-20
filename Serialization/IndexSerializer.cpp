//
// Created by elliot on 9/20/26.
//

#include "IndexSerializer.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <mutex>
#include <shared_mutex>
#include <vector>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../Index/SystemIndex.h"

namespace {
    constexpr uint32_t CURRENT_INDEX_VERSION = 1;

    bool validMagic(const IndexHeader& header) {
        return header.magic[0] == 'H'
        && header.magic[1] == 'E'
        && header.magic[2] == 'Y'
        && header.magic[3] == 'S';
    }
}

bool IndexSerializer::saveToDisk(const SystemIndex &index, std::string_view filepath) {
    std::shared_lock lock(index.index_mutex);

    const size_t total = index.next_record_idx;

    if (total > std::numeric_limits<uint32_t>::max()) return false;

    std::vector<RecordId> new_ids(total, INVALID_ID);

    uint32_t valid_count = 0;

    for (size_t i = 0; i < total; ++i) {
        if ((index.records[i].flags & FileFlags::Deleted) == FileFlags::None) {
            new_ids[i] = valid_count++;
        }
    }

    std::ofstream out(
        std::string{filepath},
        std::ios::binary | std::ios::trunc
    );

    if (!out) return false;

    IndexHeader header{};

    header.version = CURRENT_INDEX_VERSION;
    header.record_count = valid_count;
    header.arena_size = static_cast<uint64_t>(index.string_arena.size());

    out.write(
      reinterpret_cast<const char*>(&header),
      sizeof(IndexHeader)
    );

    if (!out) return false;

    for (size_t i = 0; i < total; ++i) {
        const FileRecord& record = index.records[i];

        if ((record.flags & FileFlags::Deleted) != FileFlags::None) {
            continue;
        }

        FileRecord compacted = record;

        // Record IDs change during compaction, so parent id must be updated.
        if (compacted.parent_id != INVALID_ID) {
            compacted.parent_id = new_ids[compacted.parent_id];
        }

        out.write(
            reinterpret_cast<const char*>(&compacted),
            sizeof(FileRecord));

        if (!out) return false;
    }

    // string arena
    if (!index.string_arena.empty()) {
        out.write(
          index.string_arena.data(),
          static_cast<std::streamsize>(
              index.string_arena.size())
        );
    }

    return static_cast<bool>(out);
}

bool IndexSerializer::loadFromDisk(SystemIndex &index, std::string_view filepath) {
    const std::string path{filepath};

    const int fd = ::open(path.c_str(), O_RDONLY);

    if (fd < 0) return false;

    struct stat sb{};

    if (::fstat(fd, &sb) == -1) {
        ::close(fd);
        return false;
    }

    if (sb.st_size < static_cast<off_t>(sizeof(IndexHeader))) {
        ::close(fd);
        return false;
    }

    void* mapped = ::mmap(
        nullptr,
        static_cast<size_t>(sb.st_size),
        PROT_READ,
        MAP_PRIVATE,
        fd,
        0
    );

    ::close(fd);

    if (mapped == MAP_FAILED) return false;

    const auto* bytes = static_cast<const char *>(mapped);

    const auto* header = reinterpret_cast<const IndexHeader*>(bytes);

    if (!validMagic(*header)) {
        std::cerr << "Invalid / Corrupted Binary data. Missing magic header.\n";
        ::munmap(mapped, sb.st_size);
        return false;
    }

    if (header->version != CURRENT_INDEX_VERSION) {
        std::cerr
            << "Unsupported index version: "
            << header->version
            << '\n';

        ::munmap(mapped, sb.st_size);
        return false;
    }

    const uint64_t records_size = static_cast<uint64_t>(header->record_count) * sizeof(FileRecord);
    const uint64_t expected_file_size = sizeof(IndexHeader) + records_size + header->arena_size;

    if (expected_file_size > static_cast<uint64_t>(sb.st_size)) {
        std::cerr << "Index file is truncated.\n";

        ::munmap(mapped, sb.st_size);
        return false;
    }

    // mmap layout:
    //
    // [Header]
    // [FileRecord, ...]
    // [String Arena]
    //

    const auto* disk_records =
        reinterpret_cast<const FileRecord*>(
          bytes + sizeof(IndexHeader)
        );

    const char* disk_arena =
        bytes + sizeof(IndexHeader)
        + records_size;

    // BEGNI CRITICAL SECTION
    {
        std::unique_lock lock(index.index_mutex);

        const size_t required_capacity = std::max<size_t>(
            1'000'000,
            header->record_count
        );

        index.records.clear();
        index.records.resize(required_capacity);

        std::copy(
            disk_records,
            disk_records + header->record_count,
            index.records.begin()
        );

        index.next_record_idx = header->record_count;

        index.string_arena.assign(disk_arena, disk_arena + header->arena_size);

        index.string_map.clear();
        index.string_map.reserve(header->record_count);

        uint32_t offset = 0;

        while (offset < index.string_arena.size()) {
            const char* string = &index.string_arena[offset];

            std::string_view value{string};

            if (!value.empty()) {
                index.string_map.emplace(
                    std::string{value},
                    offset);
            }

            offset += static_cast<uint32_t>(value.size() + 1);
        }

        // Rebuild full-path hash lookup
        index.path_hash_to_id.clear();
        index.path_hash_to_id.reserve(
            header->record_count);

        for (RecordId id = 1; id < header->record_count; ++id) {
            const FileRecord& record = index.records[id];

            index.path_hash_to_id.emplace(record.path_hash, id);
        }
    }
    // END CRITICAL SECTION

    ::munmap(mapped, static_cast<size_t>(sb.st_size));

    return true;
}
