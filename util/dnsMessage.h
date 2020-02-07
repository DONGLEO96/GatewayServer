#pragma once
#include<vector>
using namespace std;
enum MessageID
{
	RequestID = 1000,
	ResponseID ,
	ReportID,
};

struct  HostInfo
{
	int ip;
	int port;
};

struct RequestMsg
{
	int modid;
	int cmdid;
};

struct ResponseMsg
{
	int modid;
	int cmdid;
	int hostnum;
	vector<int> ips;
	vector<int> ports;
};

struct HostCallResult 
{
	int ip;
	int port;
	unsigned int succ;
	unsigned int err;
	bool overload;
};

struct ReportStatusReq
{
	int modid;
	int cmdid;
	int caller;
	int resnum;
	vector<HostCallResult> results;
	unsigned int ts;
};