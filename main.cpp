#include <iostream>

#include "Debug/DebugTimer.h"
#include "Index/SystemIndex.h"
#include "Scanner/FileScanner.h"

int main() {
    SystemIndex index;
    FileScanner scanner{index};

    {
        DebugTimer timer{"Filesystem indexing"};
        scanner.start("/Users/elliot");
    }


    std::cout
        << "Indexed "
        << index.getTotalRecords()
        << " records.\n";

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
            results = index.search(query);
        }

        std::cout
            << results.size()
            << " results\n";

        for (RecordId id : results) {
            std::cout << index.getPath(id) << '\n';
        }
    }
}

