#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "spider/link.h"

#include <iterator>
#include <regex>

Link MakeLinkFromString(std::string& url);
std::string GetStringFullUrlFromLink(const Link& link);
std::vector<std::string> SplitToWords(std::string& value);
std::vector<Link> CollectUrlsFromHtmlDoc(Link link, std::string& html);

struct Config_data{
	std::string host;
	std::string port;
	std::string dbname;
	std::string user;
	std::string password;
	int depth;
	Link url;
	int server_port;

	std::string getConnectionData();

	void GetConfigFromFile(std::string path);

};

