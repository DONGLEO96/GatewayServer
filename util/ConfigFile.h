#pragma once
#include<string>
#include"../json/json.h"
using namespace std;
class ConfigFile
{
public:
	ConfigFile();
	~ConfigFile();
	void setPath(const string& path);
	bool load();
	Json::Value getConfig();
private:
	

	string ConfigPath;
	Json::Value Jsonvalue;;
	

};

