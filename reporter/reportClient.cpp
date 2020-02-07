#include"../Reactor/net/TcpClient.h"
#include"../Reactor/net/net_conn.h"
#include"../util/Message.h"
#include<stdio.h>
#include<string>
#include"../json/json.h"

using namespace std;
void reportStatus(net_conn* conn)
{
	ReportStatusReq req;
	req.modid = 10;
	req.cmdid = 1;
	req.caller = 10000;
	req.resnum = 3;
	req.ts = static_cast<unsigned int>(time(NULL));
	for (int i = 0; i < 3; ++i)
	{
		HostCallResult res;
		res.ip = i;
		res.port = i + 1000;
		res.succ = 100;
		res.err = 3;
		res.overload = true;
		req.results.push_back(res);
	}

	//-------´ò°ü³ÉJson--------
	Json::Value value;
	value["caller"] = req.caller;
	value["cmdid"] = req.cmdid;
	value["modid"] = req.modid;
	value["resnum"] = req.resnum;
	value["ts"] = req.ts;
	for (int i = 0; i < 3; ++i)
	{
		Json::Value hostres;
		hostres["err"] = req.results[i].err;
		hostres["succ"] = req.results[i].succ;
		hostres["ip"] = req.results[i].ip;
		hostres["port"] = req.results[i].port;
		hostres["overload"] = req.results[i].overload;
		
		value["results"].append(hostres);
	}
	
	string jsonmsg = value.toStyledString();

	conn->send_message(jsonmsg.data(), static_cast<int>(jsonmsg.size()), ReportID);
}
int main()
{
	EventLoop loop;
	TcpClient client(&loop, "0.0.0.0", 8000, "reportClient");

	client.ConnectServer();
	client.setCompleteCallback(reportStatus);

	loop.Clientloop();
}