#pragma once

#include "DB_worker.h"
#include <iostream>
#include <Windows.h>
#include <algorithm>

DB_worker::DB_worker(std::string con_data) {
	DBc = std::make_unique< pqxx::connection>(pqxx::connection(con_data));

};

void DB_worker::setTablesAndPrepares() {
	try{ 
		std::cout << "CREATING TABLES..." << std::endl;
		pqxx::transaction tr(*DBc);

		//Docs table 
		tr.exec(
			"CREATE TABLE IF NOT EXISTS Docs("
			"id SERIAL PRIMARY KEY,"
			"url VARCHAR(300) UNIQUE"
			");"
		);
		//Words table
		tr.exec(
			"CREATE TABLE IF NOT EXISTS Words("
			"id SERIAL PRIMARY KEY,"
			"word VARCHAR(60) UNIQUE"
			");"
		);
		//WordRepeatsInDocs table
		tr.exec(
			"CREATE TABLE IF NOT EXISTS WordRepeatsInDocs("
			"url_id INTEGER REFERENCES Docs(id),"
			"word_id INTEGER REFERENCES Words(id),"
			"number INTEGER NOT NULL"
			");"
		);
		tr.commit();
		std::cout << "TABLES CREATED!" << std::endl;

		
	}
	catch (pqxx::sql_error e){
		std::cout << e.what() << std::endl;
	}
}

void DB_worker::addUrlToTable(std::string url) {

	pqxx::work w(*DBc);
	try {
		DBc->prepare("insert_wordrepeatsindocs", "INSERT INTO WordRepeatsInDocs (word_id, url_id, number) VALUES ($1, $2, $3)");
		DBc->prepare("find_wordrepeatsindocs", "SELECT number FROM WordRepeatsInDocs WHERE (word_id = $1 AND url_id = $2)");
		DBc->prepare("update_wordrepeatsindocs", "UPDATE WordRepeatsInDocs SET number = number + 1 WHERE word_id = $1 AND url_id = $2");

		pqxx::result docResult = w.exec("SELECT id FROM Docs WHERE url = '" + url + "'");

		
		// Если документа нет, добавляем его
		if (docResult.empty()) {	
			w.exec("INSERT INTO Docs (url) VALUES ('" + url + "')");
			urlID = w.exec("SELECT id FROM Docs WHERE url = '"+url+"'")[0][0].as<int>();
		}
		else {
			urlID = docResult[0][0].as<int>();
		}
		w.commit();
	}
	catch (pqxx::sql_error e) {
		std::cout << e.what() << std::endl;
	}
}

void DB_worker::addWordToTable(std::string word) {
	try {
		pqxx::work w(*DBc);
		pqxx::result wordResult = w.exec("SELECT id FROM Words WHERE word = '" + word + "'");

		int wordID;

		//если слова нет, добавляем его
		if (wordResult.empty()) {
			w.exec("INSERT INTO Words (word) VALUES ('" + word + "')");
			wordID = w.exec("SELECT id FROM Words WHERE word = '" + word+"'")[0][0].as<int>();

			//и сразу добавляем запись в WordRepeatsInDocs
			w.exec_prepared("insert_wordrepeatsindocs", wordID, urlID, 1);
		}
		else {
			wordID = wordResult[0][0].as<int>();
			pqxx::result repeatsResult = w.exec_prepared("find_wordrepeatsindocs", wordID, urlID);
			//добавляем связку слова с документом с количеством, если не нашли
			if (repeatsResult.empty()) {
				w.exec_prepared("insert_wordrepeatsindocs", wordID, urlID, 1);
			}
			//если нашли, то плюсуем
			else {
				w.exec_prepared("update_wordrepeatsindocs", wordID, urlID);
			}
		}
		w.commit();
	}
	catch (pqxx::sql_error e){
		std::cout << e.what() << std::endl;
	}
}

//до 10 ссылок
std::vector<std::string> DB_worker::SortAndGetUrl(std::map<int, int>& map_result) {
	std::vector<std::string> sorted;
	std::vector<std::pair<int, int>> buffer;

	for (auto row : map_result) {
		buffer.push_back(row);

	}

	int pos = 0;
	for (auto row : buffer) {
	
		for(int i = pos; i < buffer.size(); ++i){
			if (row.second < buffer[i].second) {
				auto tmp = row;
				row = buffer[i];
				buffer[i] = tmp;
			}
		}
		++pos;

		pqxx::work tx(*DBc);
		pqxx::result res = tx.exec("SELECT url FROM Docs WHERE (id = " + std::to_string(row.first) + ") ");
		sorted.push_back(res[0][0].as<std::string>());
		tx.commit();

		if (pos == 9) {
			break;
		}
		
	}
	return sorted;
}

std::vector<std::string> DB_worker::GetSearchResult(std::vector<std::string>& value) {
	pqxx::work tx(*DBc);
	std::map <int, int> map_result;


	for (auto word : value) {
		pqxx::result res = tx.exec(
			"SELECT url_id, number FROM WordRepeatsInDocs WHERE word_id = ("
			"SELECT id FROM Words WHERE word = '" + word + "')"
			" ORDER BY number DESC");
		if (res.empty()) {
			std::cout << "word '" << word << "' is not found!" << std::endl;
		}
		else {

			for (auto row : res) {

				if (map_result.find(row["url_id"].as<int>()) != map_result.end()) {
					map_result[row["url_id"].as<int>()] += row["number"].as<int>();
				}
				else {
					map_result.insert(std::pair<int, int>(row["url_id"].as<int>(), row["number"].as<int>()));
				}
			}

			std::cout << std::endl;
		}

	}
	tx.commit();
	std::vector<std::string> sorted_result = SortAndGetUrl(map_result);

	return sorted_result;
}