//
// Created by elliot on 9/20/26.
//

#include "FileScanner.h"
#include <string_view>
#include <dirent.h>

/*
 * TODO: Integrate a more systematic ignoring policy obviously.
 */
static bool shouldIgnore(const std::string& path) {
    // macOS bullshit
    if (path.find("Library/Mobile Documents") != std::string::npos) return true;
    if (path.find("Library/CloudStorage") != std::string::npos) return true;
    if (path.find(".Trash") != std::string::npos) return true;
    if (path.find(".Spotlight-V100") != std::string::npos) return true;

    // developer caches (clutters index)
    if (path.find("/.git") != std::string::npos) return true;
    if (path.find("/node_modules") != std::string::npos)  return true;
    if (path.find("/.rvm") != std::string::npos) return true;
    if (path.find("/.vscode/extensions") != std::string::npos) return true;
    if (path.find("/.rustup") != std::string::npos) return true;
    if (path.find("/.cargo/registry") != std::string::npos) return true;

    return false;
}

FileScanner::FileScanner(SystemIndex &index_ref)
    : index{index_ref} {}

void FileScanner::start(const std::string &root_path) {
    const RecordId scan_root_id = index.addRecord(
        root_path,
        root_path,
        ROOT_ID,
        0,
        FileFlags::Dir
    );

    if (scan_root_id != INVALID_ID) {
        scanDirectory(root_path, scan_root_id);
    }
}

void FileScanner::scanDirectory(const std::string &path, RecordId parent_id) {
    if (shouldIgnore(path)) return;

    DIR *dir = ::opendir(path.c_str());
    if (!dir) return;

    struct dirent* entry;

    while ((entry = readdir(dir)) != nullptr) {
        std::string_view name(entry->d_name);

        if (name == "." || name == "..") continue;
        auto flags = FileFlags::None;

        if (!name.empty() && name[0] == '.') {
            flags |= FileFlags::Hidden;
        }

        std::string full_path;
        full_path.reserve(path.size() + 1 + name.size());

        full_path += path;
        full_path += '/';
        full_path += name;

        if (entry->d_type == DT_DIR) {
            flags |= FileFlags::Dir;

            const RecordId id = index.addRecord(
                name,
                full_path,
                parent_id,
                0,
                flags
            );

            scanDirectory(full_path, id);
        } else if (entry->d_type == DT_REG) {
            flags |= FileFlags::File;

            index.addRecord(
                name,
                full_path,
                parent_id,
                0,
                flags
            );

        } else if (entry->d_type == DT_LNK) {
            flags |= FileFlags::Symlink;

            index.addRecord(
                name,
                full_path,
                parent_id,
                0,
                flags
            );
        }
    }

    ::closedir(dir);
}
