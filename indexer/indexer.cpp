#include "indexer.h"

Indexer::Indexer() {
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
	std::istringstream iss(str);
	std::string word;
	while (iss >> word) {
		vec.push_back(word);
	}

	std::lock_guard<std::mutex> lock(m_db_mtx);
	InsertDataToSQL(vec, document);
}

void Indexer::HandCatalog() {
	INIReader reader("file.ini");
	std::string path = reader.Get("files", "dir", "unknown");
	std::string extensions = reader.Get("files", "extensions", "unknown");

	std::vector<std::filesystem::path> vec_files;
	std::error_code ec;
	for (const auto& entry : std::filesystem::recursive_directory_iterator(path,
		std::filesystem::directory_options::skip_permission_denied, ec)) {
		if (ec) { ec.clear(); continue; }
		if (entry.is_regular_file(ec)) {
			std::string ext = entry.path().extension().string();
			if (!ext.empty()) {
				ext = ext.substr(1);
				if (extensions.find(ext) != std::string::npos) {
					vec_files.push_back(entry.path());
				}

			}

		}
	}

	int cores = std::thread::hardware_concurrency();
	std::vector<std::jthread> vec_threads;
	size_t next_index = 0; 
	size_t total_files = vec_files.size();
	for (int i = 0; i < cores; ++i) {
		vec_threads.emplace_back([&]() {
			while (true) {
				size_t current_index;
				{
					std::lock_guard<std::mutex> lock(m_queue_mtx);
					if (next_index >= total_files) return;
					current_index = next_index++;
				}

				std::filesystem::path path = vec_files[current_index];
				std::ifstream file(path);
				HandFile(file, path.filename().string());
			}

			});

	}

}