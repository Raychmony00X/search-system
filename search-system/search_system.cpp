#include "search_system.h"

SearchSystem::SearchSystem() {
	INIReader reader("D:\\C++\\search_system\\file.ini");

	try {
		if (reader.ParseError() < 0) {
			throw std::runtime_error("File wasn't read");
		}

	} catch (const std::exception& ex) {
		std::cerr << ex.what() << std::endl;
	}

	std::string port = reader.Get("database", "port", "unknown");
	std::string host = reader.Get("database", "host", "unknown");
	std::string name = reader.Get("database", "name", "unknown");
	std::string user = reader.Get("database", "user", "unknown");
	std::string password = reader.Get("database", "password", "unknown");

	m_connection = "host=" + host + " port=" + port + " dbname=" + name + " user=" + user + " password=" + password;
}