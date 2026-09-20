#include <iostream>

#include "Debug/DebugTimer.h"
#include "Index/SystemIndex.h"
#include "Scanner/FileScanner.h"
#include "Serialization/IndexSerializer.h"

int main() {

    constexpr std::string_view ROOT_PATH{"/Users/elliot"};
    constexpr std::string_view INDEX_PATH{"/Users/elliot/.heystack.idx"};


    SystemIndex index;
    bool loaded_from_disk{false};

    // Try cache first
    {
        DebugTimer timer{"Index Load"};

        loaded_from_disk = IndexSerializer::loadFromDisk(
            index, INDEX_PATH);
    }

    if (!loaded_from_disk) {
        std::cout << "No usable index cache. "
        << "Scanning filesystem...\n";

        FileScanner scanner{index};

        {
            DebugTimer timer{"Filesystem indexing"};
            scanner.start(ROOT_PATH.data());
        }

        {
            DebugTimer timer{"Index Serialization"};

            if (!IndexSerializer::saveToDisk(
                index, INDEX_PATH)) {
                std::cerr << "Failed to save index cache.\n";
            }
        }
    }

    std::cout
        << "Indexed "
        << index.getTotalRecords()
        << " records.\n";

    std::cout
        << "FileRecord size: "
        << sizeof(FileRecord)
        << " bytes\n";

    std::string query;

    while (true) {

        std::cout << "> " << std::flush;

        if (!std::getline(std::cin, query)) {
            break;
        }


        if (query == "q" || query == "quit") break;

        std::vector<RecordId> results;

        {
            DebugTimer timer{"Search"};
            results = index.search(query, 1000);
        }

        std::cout
            << results.size()
            << " results\n";

        for (const RecordId id : results) {
            std::cout << index.getPath(id) << '\n';
        }
    }
}

