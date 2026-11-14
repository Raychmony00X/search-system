#pragma once

#include "search_system.h"

#include <string>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <vector>
#include <mutex>
#include <filesystem>

class Indexer : public SearchSystem {
public:
	Indexer();

	void CorrectString(std::string& str);

	void HandFile(std::ifstream& file, const std::string& document);

	void HandCatalog();

	void InsertDataToSQL(std::vector<std::string>& vec, const std::string& document);

private:
	std::mutex m_queue_mtx;
	std::mutex m_db_mtx;    
};