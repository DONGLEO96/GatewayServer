#include"../Reactor/net/TcpServer.h"
#include"../util/ConfigFile.h"
#include"mysql/mysql.h"
#include"../json/json.h"
#include"../util/Message.h"
#include<sys/timerfd.h>
#include"WriteToDataBase.h"
#include<set>
#include<mutex>
#include<arpa/inet.h>
#include<assert.h>
using namespace std;
int timerfd = 0;
vector<ThreadQueue<WriteMessage>*>* writeQueue = NULL;
static int index = 0;
mutex _mutex;
//map<unsigned long long, set<unsigned long long>> hostmap;
map<unsigned long long, pair<int,TcpConnection*>> hostmap;//ip_port,<count,tcpconn>
void RegisterCallback(const char* data, int len, int msgid, net_conn* conn,int writethreadnum)
{
	Json::Value value;
	Json::Reader jr;
	jr.parse(data, data + len, value);
	RegisterRequest req;
	req.modid = value["modid"].asInt();
	req.cmdid = value["cmdid"].asInt();
	req.ip = value["ip"].asUInt();
	req.port = value["port"].asInt();

	WriteMessage msg;
	msg.modid = req.modid;
	msg.cmdid = req.cmdid;
	msg.ip = req.ip;
	msg.port = req.port;
	msg.op = 0;

	unsigned long long key = (static_cast<unsigned long long>(msg.modid) << 32) + msg.cmdid;
	unsigned long long host = (static_cast<unsigned long long>(msg.ip) << 32) + msg.port;
	pair<unsigned long long, unsigned long long>* p = new pair<unsigned long long, unsigned long long>();
	p->first = key;
	p->second = host;
	conn->context = (void*)p;


	lock_guard<mutex> lock(mutex);
	//hostmap[key].insert(host);
	auto pairvalue = make_pair(0, dynamic_cast<TcpConnection*>(conn));
	hostmap[host] = pairvalue;
	
	(*writeQueue)[index]->addTask(msg);
	index += 1;
	index = index % writethreadnum;
}
void HeartbeatCallback(const char* data, int len, int msgid, net_conn* conn)
{
	Json::Value value;
	Json::Reader jr;
	jr.parse(data, data + len, value);
	HeartBeat msg;
	msg.ip = value["ip"].asUInt();
	msg.port = value["port"].asInt();

	unsigned long long host = (static_cast<unsigned long long>(msg.ip) << 32) + msg.port;
	lock_guard<mutex> lock(_mutex);
	if(hostmap.find(host)!=hostmap.end())
		hostmap[host].first = 0;
}

void CloseCallback(net_conn* conn,int threadnum)
{
	if (conn->context)
	{
		pair<unsigned long long, unsigned long long> p = *(pair<unsigned long long, unsigned long long>*)(conn->context);
		delete (pair<unsigned long long, unsigned long long>*)(conn->context);
		WriteMessage msg;
		unsigned long long key = p.first;
		unsigned long long host = p.second;
		msg.modid = (unsigned int)(key >> 32);
		msg.cmdid = (unsigned int)key;
		msg.ip = (unsigned int)(host >> 32);
		msg.port = (unsigned int)host;
		msg.op = 1;
		

		lock_guard<mutex> lock(_mutex);
		//hostmap[key].erase(host);
		hostmap.erase(host);
		(*writeQueue)[index]->addTask(msg);
		index += 1;
		index = index % threadnum;
	}
}
void TimeTick()
{
	long long a;
	ssize_t ret=read(timerfd, &a, sizeof(a));
	assert(ret!=0);
	printf("%u:HeartBeatCheck\n", static_cast<unsigned int>(time(NULL)));
	for (auto it = hostmap.begin(); it != hostmap.end(); ++it)
	{
		it->second.first += 1;
		if (it->second.first >= 10)
		{
			auto conn = it->second.second;//心跳超时
			unsigned long long host = it->first;
			int ip = static_cast<int>(host >> 32);
			int port = static_cast<int>(port);
			in_addr inaddr;
			inaddr.s_addr = ip;
			string ipstring = inet_ntoa(inaddr);
			printf("[%s:%d] heartbeat faild\nRemove this Host\n", ipstring.data(), port);
			conn->handleClose();//关闭连接
			
		}
	}

}
void WriteToDB(EventLoop* loop, ThreadQueue<WriteMessage>* queue, WriteToDataBase& wr)
{
	vector<WriteMessage> tasks;
	queue->getTask(tasks);
	for (int i = 0; i < static_cast<int>(tasks.size()); ++i)
	{
		auto msg = tasks[i];
		wr.write(msg);
	}
}
void registermain(ThreadQueue<WriteMessage>* queue)
{
	EventLoop loop;

	WriteToDataBase wr;
	queue->setLoop(&loop);
	queue->setCallback(std::bind(WriteToDB, &loop, queue, wr));

	loop.loop();
}
void CreateWriteThread(int writethreadnum)
{
	writeQueue = new vector<ThreadQueue<WriteMessage>*>(writethreadnum, NULL);
	for (int i = 0; i < writethreadnum; ++i)
	{
		ThreadQueue<WriteMessage>* queue = new ThreadQueue<WriteMessage>();
		(*writeQueue)[i] = queue;
		thread t(std::bind(registermain, queue));
		t.detach();
	}
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
	string ip = config["register"]["ip"].asString();
	short port = static_cast<short>(config["register"]["port"].asInt());
	int maxconn = config["register"]["maxconn"].asInt();
	int threadnum = config["register"]["threadnum"].asInt();
	int writethreadnum = config["register"]["writethread"].asInt();
	EventLoop loop;
	TcpServer t(&loop, ip.data(), port, maxconn, threadnum);
	t.addMsgCallback(RegisterRequestID, std::bind(RegisterCallback, placeholders::_1, placeholders::_2, placeholders::_3, placeholders::_4, writethreadnum));
	t.addMsgCallback(HeartBeatID, HeartbeatCallback);
	t.setCloseCallback(std::bind(CloseCallback, placeholders::_1, writethreadnum));
	
	//init timerfd
	timerfd = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK | TFD_CLOEXEC);
	itimerspec newvalue;
	timespec now;
	clock_gettime(CLOCK_REALTIME, &now);
	newvalue.it_value.tv_sec = 1;
	newvalue.it_value.tv_nsec = now.tv_nsec;
	newvalue.it_interval.tv_sec = 1;
	newvalue.it_interval.tv_nsec = 0;
	timerfd_settime(timerfd, 0, &newvalue, NULL);
	
	loop.addEvent(timerfd, TimeTick, EPOLLIN);

	CreateWriteThread(writethreadnum);
	loop.loop();
	
}
