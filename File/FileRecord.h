//
// Created by elliot on 9/20/26.
//

#ifndef HEYSTACK_FILERECORD_H
#define HEYSTACK_FILERECORD_H
#include <cstdint>

#include "FileFlags.h"

/**
 * FileRecord represents the indexed files
 * for the SystemIndex.
 */
struct FileRecord {
    /**
     * Offset into the global string byte arena.
     */
    uint32_t name_offset;

    /**
     * Parent directory id
     */
    RecordId parent_id;

    /**
     * File size in bytes (0 for dir)
     */
    uint32_t size_bytes;

    /**
     * File Flags
     */
    FileFlags flags;
};


#endif //HEYSTACK_FILERECORD_H
