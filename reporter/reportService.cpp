#include"../Reactor/net/TcpServer.h"
#include"../util/ConfigFile.h"
#include"../json/json.h"
#include"../util/Message.h"
#include"StoreReport.h"
#include<vector>
#include"../Reactor/net/taskQueue.h"

vector<ThreadQueue<ReportStatusReq>*>* reportQueue = NULL;


void getReportMessage(const char* data, int len, int msgid, net_conn* conn,int reportthreadnum)
{
	Json::Value value;
	Json::Reader jr;
	jr.parse(data, data + len, value);
	ReportStatusReq req;
	req.modid = value["modid"].asInt();
	req.cmdid = value["cmdid"].asInt();
	req.caller = value["caller"].asInt();
	req.ts = value["ts"].asUInt();
	req.resnum = value["resnum"].asInt();
	for (int i = 0; i < req.resnum; ++i)
	{
		HostCallResult hostres;
		hostres.err = value["results"][i]["err"].asUInt();
		hostres.ip = value["results"][i]["ip"].asInt();
		hostres.overload = value["results"][i]["overload"].asBool();
		hostres.port = value["results"][i]["port"].asInt();
		hostres.succ = value["results"][i]["succ"].asInt();
		
		req.results.push_back(hostres);
	}

	static int index = 0;
	(*reportQueue)[index]->addTask(req);//使用专门的任务线程池向数据库进行读写，防止IO线程阻塞
	index += 1;
	index = index % reportthreadnum;
	
	//StoreReport st;
	//st.store(req);

}

void WriteToDataBase(EventLoop* loop, ThreadQueue<ReportStatusReq>* queue, StoreReport& st)
{
	vector<ReportStatusReq> reqqueue;
	queue->getTask(reqqueue);
	for (int i = 0; i < static_cast<int>(reqqueue.size()); ++i)
	{
		ReportStatusReq req = reqqueue[i];
		st.store(req);
	}
}

void storemain(ThreadQueue<ReportStatusReq>* queue)
{
	
	EventLoop loop;

	StoreReport st;
	queue->setLoop(&loop);
	queue->setCallback(std::bind(WriteToDataBase, &loop, queue, st));

	loop.loop();
}

void CreateReportThread(int reportthreadnum)
{
	reportQueue = new vector<ThreadQueue<ReportStatusReq>*>(reportthreadnum, NULL);
	for (int i = 0; i < reportthreadnum; ++i)
	{
		ThreadQueue<ReportStatusReq>* queue = new ThreadQueue<ReportStatusReq>();
		(*reportQueue)[i] = queue;
		thread t(std::bind(storemain, queue));
		t.detach();
	}
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
	string ip = config["report"]["ip"].asString();
	short port = static_cast<short>(config["report"]["port"].asInt());
	int maxconn = config["report"]["maxconn"].asInt();
	int threadnum = config["report"]["threadnum"].asInt();
	int reportthreadnum = config["report"]["threadnum"].asInt();
	EventLoop loop;
	TcpServer t(&loop, ip.data(), port, maxconn, threadnum);

	t.addMsgCallback(ReportID, std::bind(getReportMessage, std::placeholders::_1, std::placeholders::_2
		, std::placeholders::_3, std::placeholders::_4, reportthreadnum));
	
	CreateReportThread(reportthreadnum);//任务线程池，想mysql写入数据，不使用IO线程进行写入，防止阻塞服务器
	
	loop.loop();

}