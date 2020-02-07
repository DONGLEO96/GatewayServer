#include"../Reactor/net/taskQueue.h"
#include"../Reactor/net/UdpServer.h"
#include"../util/Message.h"
#include"../util/ConfigFile.h"
#include<iostream>
#include<thread>
#include"../json/json.h"
#include"../Reactor/net/TcpClient.h"
#include"RouteLoadBalance.h"
using namespace std;
ThreadQueue<ReportStatusReq>* report_queue = NULL;
ThreadQueue<DnsRequestMsg>* dns_queue = NULL;
vector<RouteLoadBalance*>* routelb = NULL;

ConfigFile configfile;



void ReportRequestCallback(const char* data, int len, int msgid, net_conn* conn,RouteLoadBalance* rlb)
{
	ReportRequest req;
	Json::Value jsonvalue;
	Json::Reader jr;

	jr.parse(data, data + len, jsonvalue);

	req.modid = jsonvalue["modid"].asInt();
	req.cmdid = jsonvalue["cmdid"].asInt();
	req.result = jsonvalue["result"].asInt();
	req.host.ip = jsonvalue["host"]["ip"].asInt();
	req.host.port = jsonvalue["host"]["port"].asInt();

	rlb->ReportHost(req);
	

}
void DnsResposesCallback(const char* data, int len, int msgid, net_conn* conn)
{
	DnsResponseMsg rep;
	Json::Value jsonvalue;
	Json::Reader jr;
	jr.parse(data, data + len, jsonvalue);

	rep.cmdid = jsonvalue["cmdid"].asInt();
	rep.modid = jsonvalue["modid"].asInt();
	rep.hostnum = jsonvalue["hostnum"].asInt();
	for (int i = 0; i < rep.hostnum; ++i)
	{
		int ip = jsonvalue["ips"][i].asInt();
		int port = jsonvalue["ports"][i].asInt();
		rep.ips.push_back(ip);
		rep.ports.push_back(port);
	}

	int index = (rep.modid + rep.cmdid) % 3;
	auto rlb = (*routelb)[index];
	rlb->UpdateHost(rep.modid, rep.cmdid, rep);
}
void GetHostCallBack(const char* data, int len, int msgid, net_conn* conn, RouteLoadBalance* routelb)
{
	HostRequest req;
	Json::Value jsonvalue;
	Json::Reader jr;

	jr.parse(data, data + len, jsonvalue);

	req.cmdid = jsonvalue["cmdid"].asInt();
	req.modid = jsonvalue["modid"].asInt();
	req.seq = jsonvalue["seq"].asUInt();

	HostRespose rep;
	rep.seq = req.seq;
	rep.cmdid = req.cmdid;
	rep.modid = req.modid;

	routelb->GetHosts(req.modid, req.cmdid, rep);

	//-----repתjson--------
	Json::Value jsonmsg;
	jsonmsg["modid"] = rep.modid;
	jsonmsg["cmdid"] = rep.cmdid;
	jsonmsg["callresult"] = rep.callresult;
	jsonmsg["seq"] = rep.seq;

	Json::Value host;
	host["ip"] = rep.host.ip;
	host["port"] = rep.host.port;

	jsonmsg["host"] = host;
	string jsonstring = jsonmsg.toStyledString();
	conn->send_message(jsonstring.data(), static_cast<int>(jsonstring.size()), HostResponseID);

}
void GetRouteInfoCallback(const char* data, int len, int msgid, net_conn* conn, RouteLoadBalance* routelb)
{
	Json::Value routereq;
	Json::Reader jr;
	jr.parse(data, data + len, routereq);
	DnsRequestMsg req;
	req.cmdid = routereq["cmdid"].asInt();
	req.modid = routereq["modid"].asInt();
	
	DnsResponseMsg rep;
	rep.modid = req.modid;
	rep.cmdid = req.cmdid;
	
	routelb->GetRouteInfo(rep.modid, rep.cmdid, rep);
	rep.hostnum = static_cast<int>(rep.ips.size());

	//------打包成json------
	Json::Value jsonvalue;
	jsonvalue["cmdid"] = rep.cmdid;
	jsonvalue["modid"] = rep.modid;
	jsonvalue["hostnum"] = rep.hostnum;
	
	for (int i = 0; i < rep.hostnum; ++i)
	{
		int ip = rep.ips[i];
		int port = rep.ports[i];
		jsonvalue["ips"].append(Json::Value(ip));
		jsonvalue["ports"].append(Json::Value(port));
	}

	string jsonmsg = jsonvalue.toStyledString();
	conn->send_message(jsonmsg.data(), static_cast<int>(jsonmsg.size()), ApiRouteResposeID);

}

void InitLoadBalance()
{
	routelb = new vector<RouteLoadBalance*>(3, NULL);
	for (int i = 0; i < 3; ++i)
	{
		(*routelb)[i] = new RouteLoadBalance(i + 1, configfile.getConfig());
		(*routelb)[i]->SetDnsQueue(dns_queue);
		(*routelb)[i]->SetReportQueue(report_queue);
		if ((*routelb)[i] == NULL)
		{
			fprintf(stderr, "new RouteLB error\n");
			exit(1);
		}
	}
}

void ReportRequsetToServer(TcpClient* client)
{
	vector<ReportStatusReq> tasks;
	report_queue->getTask(tasks);

	for (int i = 0; i < static_cast<int>(tasks.size()); ++i)
	{
		auto req = tasks[i];
		Json::Value value;
		value["caller"] = req.caller;
		value["cmdid"] = req.cmdid;
		value["modid"] = req.modid;
		value["resnum"] = req.resnum;
		value["ts"] = req.ts;
		for (int i = 0; i < req.resnum; ++i)
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
		client->send_message(jsonmsg.data(), static_cast<int>(jsonmsg.size()), ReportID);
	}
}
void ReportClientMain()
{
	EventLoop loop;
	Json::Value config = configfile.getConfig();
	string ip = config["report"]["ip"].asString();
	short port = static_cast<short>(config["report"]["port"].asInt());

	printf("report client thread start...\n");
	printf("send report to server %s:%d\n", ip.data(), port);
	

	TcpClient client(&loop, ip.data(), port, "reportClient");

	report_queue->setLoop(&loop);
	report_queue->setCallback(std::bind(ReportRequsetToServer, &client));

	client.ConnectServer();
	loop.Clientloop();
}

void DnsRequestToServer(TcpClient* client)
{
	vector<DnsRequestMsg> tasks;
	dns_queue->getTask(tasks);
	for (int i = 0; i < static_cast<int>(tasks.size()); ++i)
	{
		DnsRequestMsg task = tasks[i];
		Json::Value value;
		value["modid"] = task.modid;
		value["cmdid"] = task.cmdid;
		string msg = value.toStyledString();
		client->send_message(msg.data(), static_cast<int>(msg.size()), DnsRequestID);
	}
}

void DnsConnectionComplete(net_conn*)
{
	for (int i = 0; i < 3; ++i)
	{
		(*routelb)[i]->ResetAllLoadBalance();
	}

}

void DnsClientMain()
{
	Json::Value config = configfile.getConfig();
	string ip = config["dns"]["ip"].asString();
	short port = static_cast<short>(config["dns"]["port"].asInt());

	printf("dns client thread start...\n");
	printf("send dns request to server %s:%d\n", ip.data(), port);

	EventLoop loop;
	TcpClient client(&loop, ip.data(), port, "dnsClient");
	dns_queue->setLoop(&loop);
	dns_queue->setCallback(std::bind(DnsRequestToServer, &client));

	client.addMsgCallback(DnsResponseID, DnsResposesCallback);
	client.setCompleteCallback(DnsConnectionComplete);
	client.ConnectServer();

	loop.Clientloop();

}






void AgentMain(const char* ip,unsigned short port)
{
	EventLoop loop;

	UdpServer server(&loop, ip, port);
	
	server.addMsgCallback(HostRequestID, std::bind(GetHostCallBack, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, (*routelb)[port - 8888]));
	server.addMsgCallback(ReportRequestID, std::bind(ReportRequestCallback, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, (*routelb)[port - 8888]));
	server.addMsgCallback(ApiRouteRequestID, std::bind(GetRouteInfoCallback, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, (*routelb)[port - 8888]));


	//printf("Agent UDP Server %d started...\n", port);
	loop.Clientloop();
}

void StartUdpServer()
{
	for (int i = 0; i < 3; ++i)
	{
		thread t(std::bind(AgentMain, "127.0.0.1", 8888+i));
		t.detach();
	}
}
void StartDnsClient()
{
	thread dnsclient(DnsClientMain);
	dnsclient.detach();
}
void StartReportClient()
{
	thread reportclient(ReportClientMain);
	reportclient.detach();
}




int main(int argc,char* argv[])
{
	
	if (argc > 1)
	{
		string path(argv[1]);
		configfile.setPath(path);
	}
	configfile.load();
	
	dns_queue = new ThreadQueue<DnsRequestMsg>();
	report_queue = new ThreadQueue<ReportStatusReq>();

	InitLoadBalance();

	StartUdpServer();

	
	StartReportClient();

	
	StartDnsClient();

	cout << "Agent started" << endl;
	while (1)
	{
		sleep(10);
	}
	return 0;
}
