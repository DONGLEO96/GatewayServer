#include<vector>
using namespace std;
enum MessageID
{
	RequestID = 1000,
	ResponseID ,
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