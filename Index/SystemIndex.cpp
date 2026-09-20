//
// Created by elliot on 9/20/26.
//

#include "SystemIndex.h"

#include <algorithm>
#include <ranges>
#include <cstring>

RecordId SystemIndex::addRecordUnlocked(
    const uint32_t name_offset,
    const uint64_t path_hash,
    const RecordId parent,
    const uint32_t size,
    const FileFlags flags)
{
    const auto id = static_cast<RecordId>(next_record_idx);

    if (id >= records.size()) {
        records.resize(records.size() * 2);
    }

    records[id] = {
        .name_offset = name_offset,
        .parent_id = parent,
        .size_bytes = size,
        .flags = flags,
        .path_hash = path_hash
    };

    path_hash_to_id.emplace(records[id].path_hash, id);

    ++next_record_idx;

    return id;
}

uint32_t SystemIndex::internStringUnlocked(std::string_view name) {

    if (const auto it = string_map.find(name); it != string_map.end()) {
        return it->second;
    }

    const auto offset = static_cast<uint32_t>(string_arena.size());
    string_arena.insert(string_arena.end(), name.begin(), name.end());
    string_arena.push_back('\0');

    string_map.emplace(std::string(name), offset);

    return offset;
}

std::string SystemIndex::getPathUnlocked(const RecordId id) const {
    if (id == ROOT_ID || id >= next_record_idx) {
        return "/";
    }

    std::vector<RecordId> path_chain;
    RecordId current = id;

    while (current != ROOT_ID && current != INVALID_ID) {
        path_chain.push_back(current);
        current = records[current].parent_id;
    }

    std::string full_path;
    full_path.reserve(path_chain.size() * 16);

    for (const RecordId node_id : std::ranges::views::reverse(path_chain)) {
        if (std::string_view name = &string_arena[records[node_id].name_offset]; full_path.empty() && !name.empty() && name.front() == '/') {
            full_path += name;
        } else {
            if (!full_path.empty() && full_path.back() != '/') {
                full_path += '/';
            }
            full_path += name;
        }
    }

    return full_path;
}

std::vector<RecordId> SystemIndex::search(const std::string_view query, size_t limit) const {
    const std::string query_str{query};
    const char* query_c_str = query_str.c_str();
    const size_t query_len = query_str.length();

    std::vector<ScoredMatch> candidates;

    /**
     * CRITICAL SECTION START
     */
    {
        std::shared_lock lock(index_mutex);
        const size_t total = next_record_idx;

        for (size_t i = 1; i < total; ++i) {
            if (static_cast<bool>(records[i].flags & FileFlags::Deleted)) continue;

            const char* name = &string_arena[records[i].name_offset];

            if (const char* match_ptr = strcasestr(name, query_c_str); match_ptr != nullptr) {
                const size_t name_len = strlen(name);
                int score = 0;

                if (name_len == query_len) score = 1000; // exact match
                else if (match_ptr == name) score = 500; // prefix match
                else score = 100; // substring match

                score -= static_cast<int>(name_len);
                candidates.push_back({static_cast<RecordId>(i), score});
            }
        }
    } // END CRITICAL SECTION

    const size_t num_results = std::min(limit, candidates.size());

    if (num_results > 0) {
        std::partial_sort(
            candidates.begin(),
            candidates.begin() + num_results,
            candidates.end(),
            [](const ScoredMatch& a, const ScoredMatch& b) {
                return a.score > b.score;
            });
    }

    std::vector<RecordId> results;
    results.reserve(num_results);

    for (size_t i = 0; i < num_results; ++i) {
        results.push_back(candidates[i].id);
    }

    return results;
}

RecordId SystemIndex::findRecordByPathUnlocked(const std::string_view path) const {
    const uint64_t hash = hashPath(path);

    const auto [begin, end] = path_hash_to_id.equal_range(hash);

    for (auto it = begin; it != end; ++it) {
        const RecordId id = it->second;

        if (static_cast<bool>(
            records[id].flags & FileFlags::Deleted)) {
            continue;
        }

        // Collision Guard
        if (getPathUnlocked(id) == path) {
            return id;
        }
    }

    return INVALID_ID;
}

/** Public API methods */
RecordId SystemIndex::addRecord(
    const std::string_view name,
    const std::string_view full_path,
    const RecordId parent,
    const uint32_t size,
    const FileFlags flags)
{
    std::unique_lock lock(index_mutex);
    const uint32_t name_offset = internStringUnlocked(name);

    const uint64_t path_hash = hashPath(full_path);

    return addRecordUnlocked(
        name_offset,
        path_hash,
        parent,
        size,
        flags);
}

uint32_t SystemIndex::internString(const std::string_view name) {
    std::unique_lock lock(index_mutex);
    return internStringUnlocked(name);
}

std::string SystemIndex::getPath(const RecordId id) const {
    std::shared_lock lock(index_mutex);
    return getPathUnlocked(id);
}

RecordId SystemIndex::findRecordByPath(const std::string_view path) const {
    std::shared_lock lock(index_mutex);
    return findRecordByPathUnlocked(path);
}

uint64_t SystemIndex::hashPath(std::string_view path) noexcept {
    return std::hash<std::string_view>{}(path);
}
