#pragma once
#include<vector>
using namespace std;
//用到的数据格式和消息ID
enum MessageID
{
	DnsRequestID = 1000,
	DnsResponseID ,
	ReportID,
	HostRequestID,
	HostResponseID,
	ReportRequestID,
	ApiRouteRequestID,
	ApiRouteResposeID
};
enum CallResult
{
	Sueecss=0,
	Overload,
	SystemError,
	NotFound,
};
struct  HostInfo
{
	int ip;
	int port;
};

struct DnsRequestMsg
{
	int modid;
	int cmdid;
};

struct DnsResponseMsg
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

struct HostRequest
{
	unsigned int seq;
	int modid;
	int cmdid;
};

struct ReportRequest
{
	int modid;
	int cmdid;
	HostInfo host;
	int result;
};
//struct HostInfo
//{
//	int ip;
//	int port;
//	unsigned int vsucc;
//	unsigned int verr;
//	unsigned int rsucc;
//	unsigned int rerr;
//	unsigned int continue_succ;
//	unsigned int continue_err;
//};

struct HostRespose
{
	unsigned int seq;
	int modid;
	int cmdid;
	int callresult;
	HostInfo host;
};