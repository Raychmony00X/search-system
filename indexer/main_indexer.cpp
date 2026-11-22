#include "search_system.h"
#include "indexer.h"

int main() {
    SearchSystem search_system;
    Indexer indexer;

    std::cout << "Starting indexing..." << std::endl;
    indexer.HandCatalog();
    std::cout << "Indexing finished." << std::endl;

    return 0;
}