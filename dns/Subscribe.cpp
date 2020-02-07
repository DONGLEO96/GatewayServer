#include "Subscribe.h"
#include"../Reactor/net/EventLoop.h"
#include"../util/Message.h"
#include"DnsRoute.h"
#include"../json/json.h"

using namespace std;


SubscribeList* SubscribeList::_instance = NULL;
static once_flag once;

SubscribeList::SubscribeList()
{

}


void SubscribeList::init()
{
	_instance = new SubscribeList();
}

SubscribeList * SubscribeList::getInstance()
{
	call_once(once, init);
	return _instance;
}

void SubscribeList::setTcpServer(TcpServer * server)
{
	this->_server = server;
}

void SubscribeList::subscribe(unsigned long long mod, int fd)
{
	lock_guard<mutex> lock(sublistlock);
	subscribemap[mod].insert(fd);
}

void SubscribeList::unsubscribe(unsigned long long mod, int fd)
{
	lock_guard<mutex> lock(sublistlock);
	if (subscribemap.find(mod) != subscribemap.end())
	{
		subscribemap[mod].erase(fd);
		if (subscribemap[mod].empty())
		{
			subscribemap.erase(mod);
		}
	}
}
void SubscribeList::pushChangeTask(EventLoop* loop)
{
	auto onlinefds = loop->getListenFds();
	unordered_map<int, unordered_set<unsigned long long>> publist;
	//key:fd,value:modid/cmdid
	this->makePublisMap(onlinefds, publist);

	auto it = publist.begin();
	for (; it != publist.end(); ++it)
	{
		int fd = it->first;
		auto modit = it->second.begin();
		for (; modit != it->second.end(); ++modit)
		{
			int modid = static_cast<int>((*modit) >> 32);
			int cmdid = static_cast<int>(*modit);

			DnsResponseMsg msg;
			msg.modid = modid;
			msg.cmdid = cmdid;
			auto hosts = DnsRoute::getInstance()->getHostSet(modid, cmdid);
			msg.hostnum = static_cast<int>(hosts.size());

			for (auto it = hosts.begin(); it != hosts.end(); ++it)
			{
				unsigned long long ipport = *it;
				int ip = (unsigned int)(ipport >> 32);
				int port = (unsigned int)ipport;
				msg.ips.push_back(ip);
				msg.ports.push_back(port);
			}

			Json::Value value;

			value["modid"] = msg.modid;
			value["cmdid"] = msg.cmdid;
			value["hostnum"] = msg.hostnum;
			for (int i = 0; i < msg.hostnum; ++i)
			{
				value["ips"].append(Json::Value(msg.ips[i]));
				value["ports"].append(Json::Value(msg.ports[i]));
			}
			string jsonmsg = value.toStyledString();
			auto conn = TcpServer::getConn(fd);
			conn->send_message(jsonmsg.data(), static_cast<int>(jsonmsg.size()), DnsResponseID);
		}
	}

}
void SubscribeList::publish(vector<unsigned long long>& changemods)
{
	{
		lock_guard<mutex> lock1(sublistlock);
		lock_guard<mutex> lock2(publistlock);

		auto it = changemods.begin();

		for (it = changemods.begin(); it != changemods.end(); ++it)
		{
			unsigned long long mod = *it;
			if (subscribemap.find(mod) != subscribemap.end())//如果改变的modid/cmdid被订阅
			{
				unordered_set<int>::iterator fds_it;
				for (fds_it = subscribemap[mod].begin(); fds_it != subscribemap[mod].end(); ++fds_it)
				{
					int fd = *fds_it;
					publishmap[fd].insert(mod);//记录下所有订阅这个服务的fd及其改变的服务
				}
			}
		}
	}
	_server->sendTaskToPool(std::bind(&SubscribeList::pushChangeTask, this, std::placeholders::_1));//让IO线程推送出去

}

void SubscribeList::makePublisMap(unordered_set<int>& fds, unordered_map<int, unordered_set<unsigned long long>>& res)
{
	
	auto it = publishmap.begin();
	lock_guard<mutex> lock(publistlock);
	for (; it != publishmap.end();)
	{
		if (fds.find(it->first) != fds.end())
		{
			res[it->first] = publishmap[it->first];//将在线的fd的结果全部推送出去
			it=publishmap.erase(it);
		}
		else
		{
			++it;
		}
	}
}

SubscribeList::~SubscribeList()
{
}
