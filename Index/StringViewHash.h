//
// Created by elliot on 9/20/26.
//

#ifndef HEYSTACK_STRINGVIEWHASH_H
#define HEYSTACK_STRINGVIEWHASH_H

#include <string>

/**
 * Transparent Hasher for the System Index's String Map
 * to avoid std::string allocations for each hash lookup.
 */
struct StringViewHash {
    using is_transparent = void;

    size_t operator()(const std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }

    size_t operator()(const std::string& str) const noexcept {
        return (*this)(std::string_view{str});
    }
};

#endif //HEYSTACK_STRINGVIEWHASH_H
