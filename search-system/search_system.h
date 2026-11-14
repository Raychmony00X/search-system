#pragma once

#include "INIReader.h"

#include <iostream>

#include <pqxx/pqxx>

class SearchSystem {
public:
	SearchSystem();

	virtual ~SearchSystem() = default;

protected:
	std::string m_connection;
};