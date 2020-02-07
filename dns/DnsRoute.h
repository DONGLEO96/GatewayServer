#pragma once
#include<mutex>
#include<mysql/mysql.h>
#include<unordered_map>
#include<unordered_set>
#include<vector>
using namespace std;


class DnsRoute
{
public:
	static void init();
	static DnsRoute* getInstance();

	unordered_set<unsigned long long> getHostSet(int modid, int cmdid);
	void ConnectDb();
	void BuildMaps();
	int LoadVersion();
	int LoadData();
	void Updata();
	vector<unsigned long long> LoadChange();
	void removeChange(bool removeall = false);

private:
	DnsRoute();
	DnsRoute(const DnsRoute& rhs) = delete;
	const DnsRoute& operator=(const DnsRoute&) = delete;
	
	static DnsRoute* _instance;
	
	long _version;
	MYSQL dbconn;
	char sql[1024];
	
	unordered_map<unsigned long long, unordered_set<unsigned long long>> datamap;
	unordered_map<unsigned long long, unordered_set<unsigned long long>> tempmap;
	mutex _mutex;
};