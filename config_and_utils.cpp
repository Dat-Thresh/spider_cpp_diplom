#include "config_and_utils.h"


std::string Config_data::getConnectionData() {

	return host + " " +
		port + " " +
		dbname + " " +
		user + " " +
		password;
}

void Config_data::GetConfigFromFile(std::string path) {
	//открываем файл
	std::ifstream config_reader(path);
	if (!config_reader.is_open()) {
		std::cout << "Failed to open config.ini" << std::endl;
	}

	//read config
	std::string buffer;
	std::getline(config_reader, buffer);
	if (buffer != "[db_connection]") {
		throw (std::exception("Wrong ini-file"));
	}
	std::getline(config_reader, host);
	std::getline(config_reader, port);
	std::getline(config_reader, dbname);
	std::getline(config_reader, user);
	std::getline(config_reader, password);
	std::getline(config_reader, buffer);
	std::getline(config_reader, buffer);

	if (buffer != "[spider_settings]") {
		throw (std::exception("Wrong ini-file"));
	}
	std::getline(config_reader, buffer);
	url = MakeLinkFromString(buffer);

	std::getline(config_reader, buffer);
	buffer = buffer.substr(buffer.find("=") + 1, buffer.length());
	depth = std::stoi(buffer);


	std::getline(config_reader, buffer);
	std::getline(config_reader, buffer);
	if (buffer != "[http_server]") {
		throw (std::exception("Wrong ini-file"));
	}
	std::getline(config_reader, buffer);
	buffer = buffer.substr(buffer.find("=") + 1, buffer.length());
	server_port = std::stoi(buffer);

	config_reader.close();
}

Link MakeLinkFromString(std::string& url) {
	ProtocolType prot;
	if (url.find_first_of(':') == 4) {
		prot = ProtocolType::HTTP;
	}
	else {
		prot = ProtocolType::HTTPS;
	}
	std::string buffer(url.substr(url.find("//") + 2, url.length()));
	std::string hostname;
	int querystart = 0;
	for (int i = 0; i < buffer.length(); ++i) {
		if (buffer[i] == '/') {
			querystart = i;
			break;
		}
		hostname += buffer[i];
	}
	std::string query = buffer.substr(hostname.length(), buffer.length());

	return Link{ prot, hostname, query };
}

std::string GetStringFullUrlFromLink(const Link& link) {
	std::string protoc = (ProtocolType::HTTP == link.protocol) ? "http://" : "https://";
	return protoc + link.hostName + link.query;
}

std::vector<std::string> SplitToWords(std::string& value) {
	std::string buffer;
	std::vector<std::string> values;
	int word_count = 0;
	for (int i = 0; i < value.length(); ++i) {
		if (value[i] == '+') {			
			values.push_back(buffer);
			++word_count;
			if (word_count == 4) {
				std::cout << "will count only 4 words!" << std::endl;
				break;
			}
			buffer.clear();
			continue;
		}
		if (i == value.length() - 1) {
			buffer += value[i];
			values.push_back(buffer);
			break;
		}
		buffer += value[i];
	}

	return values;
}
