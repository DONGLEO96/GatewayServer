#pragma once
#include"LoadBalance.h"
#include<mutex>
#include"../Reactor/net/taskQueue.h"
using namespace std;
class RouteLoadBalance
{
public:
	RouteLoadBalance(int id, Json::Value config);
	~RouteLoadBalance();
	
	void ResetAllLoadBalance();

	void SetDnsQueue(ThreadQueue<DnsRequestMsg>* queue);
	void SetReportQueue(ThreadQueue<ReportStatusReq>* queue);

	void ReportHost(ReportRequest req);

	int GetRouteInfo(int modid, int cmdid, DnsResponseMsg& rep);
	int GetHosts(int modid, int cmdid, HostRespose& rep);
	int UpdateHost(int modid, int cmdid, DnsResponseMsg& rep);
private:
	unordered_map<unsigned long long, LoadBalance*> _lbmap;
	mutex _mutex;
	int _id;
	ThreadQueue<DnsRequestMsg>* dns_queue;
	ThreadQueue<ReportStatusReq>* report_queue;
	

	Json::Value _config;
	unsigned int updatetime;
};

