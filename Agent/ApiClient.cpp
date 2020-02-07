#include "ApiClient.h"
#include<arpa/inet.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include"../util/Message.h"
#include"../Reactor/net/Message.h"
#include"../json/json.h"
ApiClient::ApiClient():_seq(0)
{
	printf("Agent Client Start...\n");
	sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));

	serveraddr.sin_family = AF_INET;
	inet_aton("127.0.0.1", &serveraddr.sin_addr);
	for (int i = 0; i < 3; ++i)
	{
		_sockfd[i] = socket(AF_INET, SOCK_CLOEXEC | SOCK_DGRAM, IPPROTO_UDP);
		if (_sockfd[i] == -1)
		{
			fprintf(stderr, "ApiClient socket error\n");
			exit(1);
		}
		serveraddr.sin_port = htons(static_cast<unsigned short>(i + 8888));
		int ret = connect(_sockfd[i], (sockaddr*)&serveraddr, sizeof(serveraddr));
		if (ret == -1)
		{
			fprintf(stderr, "connect server address error\n");
			exit(1);
		}
	}

}


ApiClient::~ApiClient()
{
	for (int i = 0; i < 3; ++i)
	{
		close(_sockfd[i]);
	}
}

int ApiClient::GetHost(int modid, int cmdid, string & ip, int& port)
{
	unsigned int seq = _seq++;
	HostRequest req;
	req.cmdid = cmdid;
	req.modid = modid;
	req.seq = seq;

	Json::Value jsonreq;
	jsonreq["cmdid"] = req.cmdid;
	jsonreq["modid"] = req.modid;
	jsonreq["seq"] = req.seq;

	string reqstring = jsonreq.toStyledString();

	msg_head head;
	head.msgid = HostRequestID;
	head.msglen = static_cast<int>(reqstring.size());

	//string msg;
	//msg.append((char*)&head, MessageHeadLength);
	//msg.append(data, msglen);
	string msg;
	msg.append((char*)&head, MessageHeadLength);
	msg.append(reqstring.data(), head.msglen);

	int index = (modid + cmdid) % 3;

	int ret = send(_sockfd[index], msg.data(), msg.size(), 0);
	if (ret == -1)
	{
		fprintf(stderr, "send msg to server error\n");
		return CallResult::SystemError;
	}

	char buffer[80 * 1024];
	HostRespose rep;
	do
	{
		int ret = recv(_sockfd[index], buffer, sizeof(buffer), 0);
		if (ret == -1)
		{
			fprintf(stderr, "recv message from server error\n");
			return CallResult::SystemError;
		}

		memcpy(&head, buffer, MessageHeadLength);
		if (head.msgid != HostResponseID)
		{
			fprintf(stderr, "Message ID error\n");
			return CallResult::SystemError;
		}

		Json::Value jsonvalue;
		Json::Reader jr;
		
		bool flag = jr.parse(buffer + MessageHeadLength, buffer + MessageHeadLength + head.msglen, jsonvalue);
		if (!flag)
		{
			fprintf(stderr, "json string parse error\n");
			return CallResult::SystemError;
		}

		rep.callresult = jsonvalue["callresult"].asInt();
		rep.cmdid = jsonvalue["cmdid"].asInt();
		rep.modid = jsonvalue["modid"].asInt();
		rep.seq = jsonvalue["seq"].asInt();
		//rep.host.continue_err = jsonvalue["host"]["continue_err"].asUInt();
		//rep.host.continue_succ = jsonvalue["host"]["continue_succ"].asUInt();
		rep.host.ip = jsonvalue["host"]["ip"].asInt();
		rep.host.port = jsonvalue["host"]["port"].asInt();
		//rep.host.rerr = jsonvalue["host"]["rerr"].asUInt();
		//rep.host.rsucc = jsonvalue["host"]["rsucc"].asUInt();
		//rep.host.verr = jsonvalue["host"]["verr"].asUInt();
		//rep.host.vsucc = jsonvalue["host"]["vsucc"].asUInt();


	
	} while (rep.seq < seq);

	if (rep.seq != seq || rep.modid != modid || rep.cmdid != cmdid)
	{
		fprintf(stderr, "message error\n");
		return CallResult::SystemError;
	}

	if (rep.callresult == CallResult::Sueecss)
	{
		HostInfo host = rep.host;
		port = host.port;
		in_addr inaddr;
		inaddr.s_addr = host.ip;
		ip = inet_ntoa(inaddr);
	}
	return rep.callresult;
}

int ApiClient::GetRouteInfo(int modid, int cmdid, vector<pair<string, int>>& hosts)
{
	DnsRequestMsg req;
	req.modid = modid;
	req.cmdid = cmdid;

	Json::Value jsonreq;
	jsonreq["modid"] = req.modid;
	jsonreq["cmdid"] = req.cmdid;
	
	string jsonmsg = jsonreq.toStyledString();
	
	msg_head head;
	head.msgid = ApiRouteRequestID;
	head.msglen = static_cast<int>(jsonmsg.size());

	string msg;
	msg.append((char*)&head, MessageHeadLength);
	msg.append(jsonmsg.data(), head.msglen);

	int index = (modid + cmdid) % 3;

	int ret = send(_sockfd[index], msg.data(), msg.size(), 0);
	if (ret == -1)
	{
		fprintf(stderr, "send msg to server error\n");
		return CallResult::SystemError;
	}

	char buffer[80 * 1024];

	ret = recv(_sockfd[index], buffer, sizeof(buffer), 0);
	if (ret == -1)
	{
		fprintf(stderr, "recv message from server error\n");
		return CallResult::SystemError;
	}

	memcpy(&head, buffer, MessageHeadLength);
	if (head.msgid != ApiRouteResposeID)
	{
		fprintf(stderr, "Message ID error\n");
		return CallResult::SystemError;
	}

	Json::Value jsonrep;
	Json::Reader jr;
	bool flag = jr.parse(buffer + MessageHeadLength, buffer + MessageHeadLength + head.msglen, jsonrep);
	if (!flag)
	{
		fprintf(stderr, "json string parse error\n");
		return CallResult::SystemError;
	}

	DnsResponseMsg rep;
	rep.modid = jsonrep["modid"].asInt();
	rep.cmdid = jsonrep["cmdid"].asInt();
	rep.hostnum = jsonrep["hostnum"].asInt();
	for (int i = 0; i < rep.hostnum; ++i)
	{
		int ip = jsonrep["ips"][i].asInt();
		int port = jsonrep["ports"][i].asInt();
		rep.ips.push_back(ip);
		rep.ports.push_back(port);
	}
	for (int i = 0; i < rep.hostnum; ++i)
	{
		int ip = rep.ips[i];
		int port = rep.ports[i];
		in_addr inaddr;
		inaddr.s_addr = ip;
		string ipstring = inet_ntoa(inaddr);
		hosts.push_back(make_pair(ipstring, port));
	}
	return CallResult::Sueecss;
}

void ApiClient::report(int modid, int cmdid, string & ip, int & port, int resultcode)
{
	ReportRequest req;
	req.modid = modid;
	req.cmdid = cmdid;
	req.result = resultcode;

	in_addr inaddr;
	inet_aton(ip.data(), &inaddr);

	req.host.ip = inaddr.s_addr;
	req.host.port = port;

	//-----´ò°ü³Éjson×Ö·û´®-----
	Json::Value jsonvalue;
	jsonvalue["modid"] = req.modid;
	jsonvalue["cmdid"] = req.cmdid;
	jsonvalue["result"] = req.result;

	Json::Value jsonhost;
	jsonhost["ip"] = req.host.ip;
	jsonhost["port"] = req.host.port;

	jsonvalue["host"] = jsonhost;

	string jsonmsg = jsonvalue.toStyledString();


	msg_head head;
	head.msgid = ReportRequestID;
	head.msglen = static_cast<int>(jsonmsg.size());

	string msg;
	msg.append((char*)&head, MessageHeadLength);
	msg.append(jsonmsg.data(), head.msglen);

	int index = (modid + cmdid) % 3;

	int ret = send(_sockfd[index], msg.data(), msg.size(), 0);
	if (ret == -1)
	{
		fprintf(stderr, "send reportrequest error\n");
	}
	

}
