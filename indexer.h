#pragma once

#include "INIReader.h"

#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include<fstream>
#include <vector>
#include <mutex>
#include <filesystem>

#include <pqxx/pqxx>

class Indexer {
public:
	Indexer();

	void CorrectString(std::string& str);

	void HandFile(std::ifstream& file, const std::string& document);

	void HandCatalog();

	void InsertDataToSQL(std::vector<std::string>& vec, const std::string& document);

	~Indexer() = default;

private:
	std::string m_connection;
	std::mutex m_mtx;
};