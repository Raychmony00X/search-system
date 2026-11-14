#include "search_service.h"

SearchService::SearchService() {
	std::vector<std::string> words;
	std::string word;
	std::cout << "Input 1-4 words or break to stop:" << std::endl;
	while (true) {
		std::cin >> word;
		if (words.size() == 4 || word == "break") break;
		words.emplace_back(std::move(word));
	}

	try {
		pqxx::connection conn(m_connection);
		if (!conn.is_open()) return;

		pqxx::work tx(conn);

		pqxx::result result = tx.exec_params(
			"SELECT d.id, d.name, SUM(dw.count) AS total_hits "
			"FROM documents d "
			"JOIN document_words dw ON dw.document_id = d.id "
			"JOIN words w            ON w.id = dw.word_id "
			"WHERE w.data = ANY($1::text[]) "
			"GROUP BY d.id, d.name "
			"HAVING COUNT(DISTINCT w.id) = ("
			"    SELECT COUNT(DISTINCT x) FROM unnest($1::text[]) AS x"
			") "
			"ORDER BY total_hits DESC, d.name "
			"LIMIT 10;",
			words
		);

		if (result.empty()) {
			std::cout << "Nothing was found" << std::endl;
			return;
		}

		for (const auto& files : result) {
			std::cout << files["name"].as<std::string>() << "\n";
		}

		std::cout << std::endl;

		tx.commit();
	} catch (pqxx::sql_error& e) {
		std::cerr << e.what() << std::endl;
		return;
	}

}