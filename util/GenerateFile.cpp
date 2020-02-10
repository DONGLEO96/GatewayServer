#include"../json/json.h"
#include<stdio.h>
#include<iostream>
using namespace std;
int main()
{
	Json::Value jv;
	Json::StyledWriter jw;
	jv["ip"] = Json::Value("127.0.0.1");
	jv["port"] = Json::Value(8000);
	jv["maxconn"] = Json::Value(1024);
	jv["threadnum"] = Json::Value(3);

	Json::Value report;
	report["threadnum"] = 3;
	report["ip"] = "127.0.0.1";
	report["port"] = 9001;
	report["maxconn"] = 1024;
	report["writethread"] = 3;

	Json::Value dns;
	dns["threadnum"] = 3;
	dns["ip"] = "127.0.0.1";
	dns["port"] = 9000;
	dns["maxconn"] = 1024;

	Json::Value mysql;
	mysql["ip"] = "127.0.0.1";
	mysql["port"] = 3306;
	mysql["user"] = "root";
	mysql["pwd"] = "111111";
	mysql["dbname"] = "dns";
	
	Json::Value loadbalance;
	loadbalance["probenum"] = 10;
	loadbalance["initsucc"] = 100;
	loadbalance["initerr"] = 5;
	loadbalance["errrate"] = 0.1;
	loadbalance["succrate"] = 0.95;
	loadbalance["continueerr"] = 15;
	loadbalance["continuesucc"] = 15;
	loadbalance["localip"] = "127.0.0.1";
	loadbalance["idletimeout"] = 15;
	loadbalance["overloadtimeout"] = 15;
	loadbalance["windowerrrate"] = 0.7;
	
	Json::Value Register;
	Register["threadnum"] = 3;
	Register["ip"] = "127.0.0.1";
	Register["port"] = 9002;
	Register["maxconn"] = 1024;
	Register["writethread"] = 3;

	Json::Value config;
	config["reactor"] = jv;
	config["report"] = report;
	config["mysql"] = mysql;
	config["dns"] = dns;
	config["loadbalance"] = loadbalance;
	config["register"] = Register;
	string value = jw.write(config);
	
	


	string path(getenv("PWD"));
	path.append("/config");
	//cout << path << endl;
	FILE* configfile = fopen(path.data(), "ab+");
	fwrite(value.data(), sizeof(char), value.size(), configfile);
	return 0;
}