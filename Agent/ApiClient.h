#pragma once
#include<string>
#include<vector>
using namespace std;
class ApiClient
{
public:
	ApiClient();
	~ApiClient();
	int GetHost(int modid, int cmdid, string& ip, int& port);
	int GetRouteInfo(int modid, int cmdid, vector<pair<string, int>>& hosts);
	void report(int modid, int cmdid, string& ip, int& port, int resultcode);
private:
	int _sockfd[3];
	unsigned int _seq;
};

