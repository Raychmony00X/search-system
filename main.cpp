#include "search_system.h"
#include "indexer.h"
#include "search_service.h"

int main() {
	SearchSystem search_system;
	
	Indexer indexer;
	indexer.HandCatalog();

	SearchService search_service;

	return 0;
}