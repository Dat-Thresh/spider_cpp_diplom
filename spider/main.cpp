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
		std::vector<Link> s_links = CollectUrlsFromHtmlDoc(link, html);
		 

		// Вывод всех найденных ссылок
		//for (const auto& link : s_links) {
		//	std::cout << link << std::endl;
		//}
		

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

	if (!createdTables) {
		DB_worker.setTablesAndPrepares();
		createdTables = true;
	}
	


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




