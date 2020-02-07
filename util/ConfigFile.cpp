#include "ConfigFile.h"



ConfigFile::ConfigFile():ConfigPath(getenv("PWD"))
{
	ConfigPath.append("/config");//default path;

}


ConfigFile::~ConfigFile()
{
}

void ConfigFile::setPath(const string & path)
{
	this->ConfigPath = path;
}

bool ConfigFile::load()
{
	FILE* configfile = fopen(ConfigPath.data(), "ab+");
	if (configfile == NULL)
		return false;
	char buff[1024];
	ssize_t ret=fread(buff, sizeof(char), 1024,configfile);
	if (ret == 0 || ret == -1)
		return false;
	string s(buff);
	Json::Reader jr;
	jr.parse(s, Jsonvalue);
	return true;
}

Json::Value ConfigFile::getConfig()
{
	return Jsonvalue;
}
