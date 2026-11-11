#include "indexer.h"

Indexer::Indexer() {
	INIReader reader("file.ini");
	
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

	m_connection = "host=" + host + " port=" + port + " name=" + name + " user=" + user + " password=" + password;

	try {
		pqxx::connection conn(m_connection);
		if (!conn.is_open()) return;
		
		pqxx::work tx(conn);
		tx.exec(R"( 
        CREATE TABLE IF NOT EXISTS documents ( 
           id SERIAL PRIMARY KEY,
           name TEXT NOT NULL
           )
        )");
		tx.exec(R"( 
        CREATE TABLE IF NOT EXISTS words ( 
           id SERIAL PRIMARY KEY,
           data VARCHAR(100) NOT NULL UNIQUE
           )
        )");
		tx.exec(R"( 
        CREATE TABLE IF NOT EXISTS document_words ( 
           document_id INTEGER NOT NULL REFERENCES documents(id) ON DELETE CASCADE,
           word_id INTEGER NOT NULL REFERENCES words(id) ON DELETE CASCADE,
           count INTEGER NOT NULL DEFAULT 1 CHECK (count > 0),
           PRIMARY KEY (document_id, word_id)
           )
        )");
		tx.exec("CREATE INDEX IF NOT EXISTS idx_dw_word ON document_words(word_id)");
		
		tx.commit();
	} catch (pqxx::sql_error& e) {
		std::cerr << e.what() << std::endl;
		return;
	}

}

void Indexer::CorrectString(std::string& str) {
	str.erase(std::remove_if(str.begin(), str.end(), [](char c) {
		return std::isdigit((unsigned char)c);}), str.end());

	std::replace_if(str.begin(), str.end(),
		[](char c) { return !std::isalpha((unsigned char)c) && c != ' '; }, ' ');

	str.erase(std::unique(str.begin(), str.end(),
		[](char a, char b) { return a == ' ' && b == ' '; }), str.end());

	str.erase(str.begin(), std::find_if(str.begin(), str.end(),
		[](char c) { return c != ' '; }));
	str.erase(std::find_if(str.rbegin(), str.rend(),
		[](char c) { return c != ' '; }).base(), str.end());

	std::transform(str.begin(), str.end(), str.begin(),
		[](unsigned char c) { return std::tolower(c); });
}

void Indexer::InsertDataToSQL(std::vector<std::string>& vec, const std::string& document) {
	try {
		pqxx::connection conn(m_connection);
		if (!conn.is_open()) return;

		pqxx::work tx(conn);

		pqxx::result doc_res = tx.exec_params(
			"INSERT INTO documents (name) VALUES ($1) RETURNING id",
			document
		);

		int document_id = doc_res[0][0].as<int>();

		std::unordered_map<std::string, int> freq;
		for (const auto& w : vec) ++freq[w];

		for (const auto& [word, count] : freq) {
			pqxx::result w_res = tx.exec_params(
				"INSERT INTO words (data) VALUES ($1) "
				"ON CONFLICT (data) DO NOTHING "
				"RETURNING id",
				word
			);

			int word_id;
			if (w_res.empty()) {
				pqxx::result sel = tx.exec_params(
					"SELECT id FROM words WHERE data = $1", word);
				word_id = sel[0][0].as<int>();
			} else {
				word_id = w_res[0][0].as<int>();
			}

			tx.exec_params(
				"INSERT INTO document_words (document_id, word_id, count) "
				"VALUES ($1, $2, $3) "
				"ON CONFLICT (document_id, word_id) "
				"DO UPDATE SET count = document_words.count + EXCLUDED.count",
				document_id, word_id, count
			);

		}

		tx.commit();
	} catch (pqxx::sql_error& e) {
		std::cerr << e.what() << std::endl;
		return;
	}

}

void Indexer::HandFile(std::ifstream& file, const std::string& document) {
	try {
		if (!file.is_open()) {
			throw std::runtime_error("File doesn't open");
		}
	} catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return;
	}

	std::string str;
	file >> str;
	file.close();

	if (str.size() == 0) return;

	CorrectString(str);

	std::vector<std::string> vec;
	auto it1 = str.begin();
	auto it2 = std::next(it1, 1);
	while (it1 != str.end() && it2 != str.end()) {
		if (*it2 != ' ') std::advance(it2, 1);
		int length = std::distance(it1, it2);
		std::string word = str.substr(*it1, length);
		vec.push_back(word);
		it2++;
		it1 = it2;
	}

	std::lock_guard<std::mutex>{ m_mtx };
	InsertDataToSQL(vec, document);
}

void Indexer::HandCatalog() {
	INIReader reader("file.ini");
	std::string path = reader.Get("database", "dir", "unknown");
	std::string extensions = reader.Get("database", "extensions", "unknown");

	std::vector<std::filesystem::path> vec_files;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
		if (entry.is_regular_file() && entry.path().extension() == extensions) {
			vec_files.push_back(entry.path());
		}

	}

	int cores = std::thread::hardware_concurrency();
	std::vector<std::jthread> vec_threads;
	auto it_f = vec_files.begin();
	for (const auto& it_t : vec_threads) {
		vec_threads.emplace_back([&]() {
			while (true) {
				std::filesystem::path path;

				{
					std::lock_guard{ m_mtx };
					if (it_f == vec_files.end()) return;
					path = *it_f;
					++it_f;
				}

				std::ifstream file(path);
				HandFile(file, path.filename().string());
			}
			
			});

	}

}