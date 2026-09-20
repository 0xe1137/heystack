//
// Created by elliot on 9/20/26.
//

#ifndef HEYSTACK_FILESCANNER_H
#define HEYSTACK_FILESCANNER_H

#include "../Index/SystemIndex.h"
#include "../File/FileFlags.h"

class FileScanner {
private:
    /**
     * Reference to the central system index where all discovered files are stored.
     */
    SystemIndex& index;

    void scanDirectory(const std::string& path, RecordId parent_id);

public:
    explicit FileScanner(SystemIndex& index_ref);

    /**
     * Initializes file scan from a given root path.
     * @param root_path  Root path of indexable file system.
     */
    void start(const std::string& root_path);
};


#endif //HEYSTACK_FILESCANNER_H
