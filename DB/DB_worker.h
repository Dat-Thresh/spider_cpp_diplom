#pragma once

#include <pqxx/pqxx>
//#include "DB_connection_data.h"

// создаем класс для работы с БД
// подключение к бд
// создаем три стола в конструкторе
// методы для заполнения таблиц


class DB_worker {
private:
	int urlID = 1;
	std::unique_ptr<pqxx::connection> DBc;
	DB_worker() = delete;

	std::vector<std::string> SortAndGetUrl(std::map<int, int>& map_result);
public:
	DB_worker(std::string con_data);
	void setTablesAndPrepares();
	void addUrlToTable(std::string url);
	void addWordToTable(std::string word);
	std::vector<std::string> GetSearchResult(std::vector<std::string>& value);
};