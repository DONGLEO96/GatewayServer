#include<string>
#include"../../util/ConfigFile.h"
#include"../net/UdpServer.h"

void callback1(const char* data, int len, int msgid, net_conn* conn)
{
	printf("call echo\n");
	string msg(data, len);
	conn->send_message(msg.data(), len, msgid);
}

int main(int argc, char* argv[])
{
	ConfigFile configfile;
	if (argc > 1)
	{
		string path(argv[1]);
		configfile.setPath(path);
	}
	configfile.load();
	Json::Value config = configfile.getConfig();
	string ip = config["reactor"]["ip"].asString();
	short port = static_cast<short>(config["reactor"]["port"].asInt());
	
	EventLoop loop;
	UdpServer server(&loop, ip.data(), port);

	server.addMsgCallback(1, callback1);
	loop.loop();
}