//
// Created by elliot on 9/20/26.
//

#ifndef HEYSTACK_FILEFLAGS_H
#define HEYSTACK_FILEFLAGS_H

#include <cstdint>
#include <type_traits>

using RecordId = uint32_t;
constexpr RecordId ROOT_ID = 0;

constexpr RecordId INVALID_ID = 0xFFFFFFFF;

enum class FileFlags : uint8_t {
    None = 0,
    File = 1 << 0,
    Dir = 1 << 1,
    Hidden = 1 << 2,
    Symlink = 1 << 3,
    Deleted = 1 << 4,
};

constexpr FileFlags operator|(FileFlags a, FileFlags b) {
    return static_cast<FileFlags>(
        static_cast<std::underlying_type_t<FileFlags>>(a) |
        static_cast<std::underlying_type_t<FileFlags>>(b));
}

constexpr FileFlags operator&(FileFlags a, FileFlags b) {
    return static_cast<FileFlags>(
        static_cast<std::underlying_type_t<FileFlags>>(a) &
        static_cast<std::underlying_type_t<FileFlags>>(b));
}

constexpr FileFlags& operator|=(FileFlags& a, FileFlags b) {
    a = static_cast<FileFlags>(
        static_cast<std::underlying_type_t<FileFlags>>(a) |
        static_cast<std::underlying_type_t<FileFlags>>(b)
    );
    return a;
}

#endif //HEYSTACK_FILEFLAGS_H
