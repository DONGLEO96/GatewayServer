#include"../net/EventLoop.h"
#include"../IOBuff/BuffPool.h"
#include<thread>
#include"../net/TcpServer.h"
#include"../../util/ConfigFile.h"
#include<string.h>
using namespace std;


TcpServer* t;

void printTask(EventLoop* loop)
{
	printf("========Active Task Func========\n");
	unordered_set<int> fds = loop->getListenFds();
	
	for (auto it = fds.begin(); it != fds.end(); ++it)
	{
		int fd = *it;
		shared_ptr<TcpConnection> tcpconn = TcpServer::_conns[fd];

		if (tcpconn.get() != NULL)
		{
			int msgid = 101;
			const char* msg = "Hello I am a Task!\n";
			tcpconn->send_message(msg, static_cast<int>(strlen(msg)), msgid);
		}
	}
}

void callback1(const char* data, int len, int msgid, net_conn* conn)
{
	printf("call echo\n");
	string msg(data, len);
	conn->send_message(msg.data(), len, msgid);
}
void callback2(const char* data, int len, int msgid, net_conn*)
{
	printf("call print\n");
	printf("recv client: [%s]\n", data);
	printf("msgid: [%d]\n", msgid);
	printf("len: [%d]\n", len);
}
void connCompleteCallback(net_conn* conn)
{
	int msgid = 101;
	string data = "welcome!you online...";
	conn->send_message(data.data(), static_cast<int>(data.size()), msgid);
	
	t->sendTaskToPool(printTask);
}
void connCloseCallback(net_conn* conn)
{
	printf("client Offline\n");
}

int main(int argc,char* argv[])
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
	int maxconn = config["reactor"]["maxconn"].asInt();
	int threadnum = config["reactor"]["threadnum"].asInt();
	EventLoop loop;
	t=new TcpServer(&loop, ip.data(), port, maxconn, threadnum);
	t->addMsgCallback(1, callback1);
	t->addMsgCallback(2, callback2);
	t->setCloseCallback(connCloseCallback);
	t->setCompleteCallback(connCompleteCallback);

	loop.loop();
	return 0;
}