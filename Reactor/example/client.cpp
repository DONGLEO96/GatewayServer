#include"../net/TcpClient.h"
#include"../net/net_conn.h"
#include<stdio.h>
#include<string>
using namespace std;
void busi(const char* data, size_t len, int msgid, net_conn* conn)
{
	string msg(data, len);
	printf("recv server:[%s]\n", msg.data());
	printf("msgid:[%d]\n", msgid);
	printf("len:[%d]\n", static_cast<int>(len));
}

void connCompleteCallback(net_conn* conn)
{
	int msgid = 1;
	string msg("Hello server");
	conn->send_message(msg.data(), static_cast<int>(msg.size()), msgid);
}

void connCloseCallback(net_conn* conn)
{
	printf("Connection Closed\n");
}

int main()
{
	EventLoop loop;
	TcpClient client(&loop, "0.0.0.0", 8000, "client");
	//client.setMseeageCallback(busi);
	client.addMsgCallback(1, busi);
	client.addMsgCallback(101, busi);
	client.setCompleteCallback(connCompleteCallback);
	client.setCloseCallback(connCloseCallback);
	loop.Clientloop();
	return 0;
}
