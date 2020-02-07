#include<string>
#include"../../util/ConfigFile.h"
#include"../net/UdpClient.h"

void callback1(const char* data, int len, int msgid, net_conn* conn)
{
	string msg(data, len);
	printf("recv server:[%s]\n", msg.data());
	printf("msgid:[%d]\n", msgid);
	printf("len:[%d]\n", len);
}

int main(int argc, char* argv[])
{
	EventLoop loop;
	UdpClient client(&loop, "0.0.0.0", 8000);

	client.addMsgCallback(1, callback1);

	int msgid = 1;
	string msg = "Hello World";
	client.send_message(msg.data(),static_cast<int>( msg.size()), msgid);

	loop.Clientloop();
}