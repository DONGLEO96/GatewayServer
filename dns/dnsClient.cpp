#include"../Reactor/net/TcpClient.h"
#include"../Reactor/net/net_conn.h"
#include"../util/Message.h"
#include<stdio.h>
#include<string>
#include"../json/json.h"
using namespace std;

void onConnection(net_conn* conn)
{
	Json::Value value;
	value["modid"] = 1;
	value["cmdid"] = 1;
	string msg = value.toStyledString();
	conn->send_message(msg.data(), static_cast<int>(msg.size()), DnsRequestID);
}
void dealGetRoute(const char* data, int len, int msgid, net_conn* conn)
{
	Json::Value value;
	Json::Reader jr;
	jr.parse(data, data + len, value);
	printf("modid=%d\n", value["modid"].asInt());
	printf("cmdid=%d\n", value["cmdid"].asInt());
	printf("hostnum=%d\n", value["hostnum"].asInt());
	int size = value["hostnum"].asInt();
	for(int i=0;i<size;++i)
	{
		printf("ip=%d\n", value["ips"][i].asInt());
		printf("port=%d\n", value["ports"][i].asInt());
	}
	printf("fininsh dns\n");
}	
int main()
{
	EventLoop loop;
	TcpClient client(&loop, "0.0.0.0", 8000, "client");
	//client.setMseeageCallback(busi);
	client.setCompleteCallback(onConnection);
	client.addMsgCallback(DnsResponseID, dealGetRoute);
	client.ConnectServer();
	loop.Clientloop();
	return 0;
}