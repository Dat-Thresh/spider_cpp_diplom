#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "http_utils.h"
#include <functional>


#include "../DB/DB_worker.h"
#include "../config_and_utils.h"
#include <Windows.h>
#include <iterator>
#include <regex>


std::mutex mtx;
std::condition_variable cv;
std::queue<std::function<void()>> tasks;
bool exitThreadPool = false;


Config_data ini_data;
bool createdTables = false;

void ParseStringToDB(const Link& link, std::string& words);

void threadPoolWorker() {
	std::unique_lock<std::mutex> lock(mtx);
	while (!exitThreadPool || !tasks.empty()) {
		if (tasks.empty()) {
			cv.wait(lock);
		}
		else {
			auto task = tasks.front();
			tasks.pop();
			lock.unlock();
			task();
			lock.lock();
		}
	}
}
void parseLink(const Link& link, int depth)
{
	try {

		std::this_thread::sleep_for(std::chrono::milliseconds(500));

		std::cout << "PARSING THIS Link >> >: " << GetStringFullUrlFromLink(link) << std::endl;
		std::string html = getHtmlContent(link);

		if (html.size() == 0)
		{
			std::cout << "Failed to get HTML Content" << std::endl;
			return;
		}

		// TODO: Parse HTML code here on your own

		//std::cout << "html content:" << std::endl;
		//std::cout << html << std::endl;

		std::string buff_html = html;
		std::string clearedString = clearHtmlContent(buff_html);
		//std::cout << "html content CLEARED:" << std::endl;

		SetConsoleCP(CP_UTF8);
		SetConsoleOutputCP(CP_UTF8);
		setlocale(LC_ALL, "en-US");
		//std::cout << clearedString << std::endl;

		ParseStringToDB(link, clearedString);

		// TODO: Collect more links from HTML code and add them to the parser like that:
		 
		std::regex s_sublink(R"(<a\s+href\s*=\s*\"(.*?)\")", std::regex_constants::ECMAScript | std::regex_constants::icase);
		std::smatch url_match;

		std::vector<Link> s_links;

		// Итерация и поиск всех совпадений
		auto begin = std::sregex_iterator(html.begin(), html.end(), s_sublink);

		auto end = std::sregex_iterator();

		for (std::sregex_iterator i = begin; i != end; ++i) {
			std::smatch match = *i;
			std::string url = match[1].str();
			//не добавляем ссылки, по которым не получится пройти "напрямую" и запарсить 
			//ИСПРАВЛЯЕМ
			//если не находим, впереди добавляем текущую ссылку

			if (url.substr(0, 4) != "http") {
				//std::cout << std::endl << "BEFORE: ";
				//std::cout << url << std::endl;
				//std::cout << url.substr(0, 4) << std::endl;
				url = GetStringFullUrlFromLink(link) + url;
				//std::cout << std::endl << "AFTER: " << url;
				continue;
			}
			//std::cout << url << std::endl;
			s_links.push_back(MakeLinkFromString(url));
		}

		// Вывод всех найденных ссылок
		//for (const auto& link : s_links) {
		//	std::cout << link << std::endl;
		//}
		
		//Для реализации HTTP - клиента рекомендуется подключить и использовать библиотеку Boost Beast.
		//Порт, на котором будет запущен HTTP-сервер, должен взять из ini-файла конфигурации

		//Все настройки программ следует хранить в ini - файле.Этот файл должен парситься при запуске вашей программы.

	
		//  в вектор записываем основную ссылку текущей глубины и к ней пути -- сублинки -- найденные ссылки внутри этой ссылки?

				//глубина рекурсии depth ограничена
				//собираем ссылки в очередь на скачивание 
				//реализуем пул потоков для выполнения скачивания и индексации


		//std::vector<Link> links = {
		//	{ProtocolType::HTTPS, "en.wikipedia.org", "/wiki/Wikipedia"},
		//	{ProtocolType::HTTPS, "wikimediafoundation.org", "/"},
		//};

		if (depth > 0) {
			std::lock_guard<std::mutex> lock(mtx);

			size_t count = s_links.size();
			size_t index = 0;
			for (auto& subLink : s_links)
			{
				tasks.push([subLink, depth]() { parseLink(subLink, depth - 1); });
			}
			cv.notify_one();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

}



int main()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	setlocale(LC_ALL, "en-US");

	try {
		//парсим ини файл
		ini_data.GetConfigFromFile("C:/Netology/Diplom project/config.ini");

		int numThreads = std::thread::hardware_concurrency();

		std::vector<std::thread> threadPool;

		for (int i = 0; i < numThreads; ++i) {
			threadPool.emplace_back(threadPoolWorker);
		}

		Link link = ini_data.url;
		{
			std::lock_guard<std::mutex> lock(mtx);
			tasks.push([link]() { parseLink(link, ini_data.depth); });
			cv.notify_one();
		}


		std::this_thread::sleep_for(std::chrono::seconds(2));


		{
			std::lock_guard<std::mutex> lock(mtx);
			exitThreadPool = true;
			cv.notify_all();
		}

		for (auto& t : threadPool) {
			t.join();
		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
	return 0;
}


void ParseStringToDB(const Link& link, std::string& words) {
	
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "en-US");

	DB_worker DB_worker(ini_data.getConnectionData());
	//pqxx::connection DBc(
	//	con_data.getConnectionData()
	//);
	if (!createdTables) {
		DB_worker.setTablesAndPrepares();
		createdTables = true;
	}
	
	/*DBc.prepare("insert_wordrepeatsindocs", "INSERT INTO WordRepeatsInDocs (url_id, word_id, number) VALUES ($1, $2, $3)");
	DBc.prepare("find_wordrepeatsindocs", "SELECT number FROM WordRepeatsInDocs WHERE (word_id = $1 AND url_id = $2)");
	DBc.prepare("update_wordrepeatsindocs", "UPDATE WordRepeatsInDocs SET number = number + 1 WHERE word_id = $1 AND url_id = $2");*/


	DB_worker.addUrlToTable(GetStringFullUrlFromLink(link));

	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);

	setlocale(LC_ALL, "en-US");

	std::istringstream stream(words);
	std::string word;
	//int ind = 1;
	while (stream >> word){
		//std::cout << "# " << ind << " word added: " << word << std::endl;
		DB_worker.addWordToTable(word);
		//++ind;
	}

}




