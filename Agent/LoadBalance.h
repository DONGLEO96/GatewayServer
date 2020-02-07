#pragma once
#include<unordered_map>
#include"HostDetailedInfo.h"
#include"../util/Message.h"
#include<list>
#include"../json/json.h"
#include"../Reactor/net/taskQueue.h"
using namespace std;
class LoadBalance
{
public:
	LoadBalance(int modid, int cmdid, ThreadQueue<DnsRequestMsg>* queue, ThreadQueue<ReportStatusReq>* report_queue,Json::Value config);
	~LoadBalance();

	bool Empty();
	int GetOneHost(HostRespose& rep);
	vector<HostDetailedInfo*> GetAllHost();
	
	int pull();
	void update(DnsResponseMsg& rep);

	void report(int ip, int port, int retcode);
	void commit();

	enum Status
	{
		Pulling,
		New,
	};
	Status status;

private:
	int _modid;
	int _cmdid;
	int _access_cnt;

public:
	unsigned int _lastupdatets;

private:

	int _probenum;
	int _initSucc;
	int _initerr;
	unsigned int _continuesucc;
	unsigned int _continueerr;
	double succrate;
	double errrate;
	double _windowerrrate;
	int _idletimeout;
	int _overloadtimeout;



	
	unordered_map<unsigned long long, HostDetailedInfo*> _hostmap;
	list<HostDetailedInfo*> _idlelist;
	list<HostDetailedInfo*> overloadlist;
	ThreadQueue<DnsRequestMsg>* dns_queue;
	ThreadQueue<ReportStatusReq>* report_queue;
	Json::Value _config;
};

