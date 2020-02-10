#include"../Reactor/net/TcpClient.h"
#include"../Reactor/net/net_conn.h"
#include<stdio.h>
#include<string>
#include"../util/Message.h"
#include"../json/json.h"
#include<unistd.h>
#include<sys/timerfd.h>
#include<assert.h>
using namespace std;
int timerfd = 0;
void SendHeartBeat(net_conn* client, unsigned int ip,int port)
{
	long long a;
	ssize_t ret=read(timerfd, &a, sizeof(a));
	assert(ret!=0);
	printf("%u:SendHeartBeat\n",static_cast<unsigned int>(time(NULL)));

	HeartBeat hb;
	hb.ip = ip;
	hb.port = port;
	Json::Value value;
	value["ip"] = hb.ip;
	value["port"] = hb.port;
	string msg = value.toStyledString();
	client->send_message(msg.data(), static_cast<int>(msg.size()), HeartBeatID);
}
void Register(net_conn* conn,int modid,int cmdid, unsigned int ip,int port)
{
	printf("Build TCP Connection\nSend Register Message");
	RegisterRequest req;
	req.modid = modid;
	req.cmdid = cmdid;
	req.ip = ip;
	req.port = port;
	
	Json::Value value;
	value["modid"] = req.modid;
	value["cmdid"] = req.cmdid;
	value["ip"] = req.ip;
	value["port"] = req.port;
	string msg = value.toStyledString();
	conn->send_message(msg.data(), static_cast<int>(msg.size()), RegisterRequestID);
}


int main()
{
	EventLoop loop;
	TcpClient client(&loop, "0.0.0.0", 9002, "client");
	//client.setMseeageCallback(busi);
	int modid = 1;
	int cmdid = 1;
	string ipstring = "192.168.65.129";
	int port = 5000;

	in_addr inaddr;
	inet_aton(ipstring.data(), &inaddr);

	unsigned int ip = htonl(inaddr.s_addr);

	client.setCompleteCallback(std::bind(Register, placeholders::_1, modid, cmdid, ip, port));
	timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
	itimerspec newvalue;
	timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	newvalue.it_value.tv_sec = 1;
	newvalue.it_value.tv_nsec = now.tv_nsec;
	newvalue.it_interval.tv_sec = 1;
	newvalue.it_interval.tv_nsec = 0;
	timerfd_settime(timerfd, 0, &newvalue, NULL);


	loop.addEvent(timerfd, std::bind(SendHeartBeat, dynamic_cast<net_conn*>(&client), ip, port), EPOLLIN);
	client.ConnectServer();
	loop.Clientloop();
	return 0;
}

