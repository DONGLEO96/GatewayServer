#pragma once
#include<unordered_map>
#include<unordered_set>
#include"DnsRoute.h"
#include<mutex>
#include<vector>
#include"../Reactor/net/EventLoop.h"
#include"../Reactor/net/TcpServer.h"
using namespace std;


class SubscribeList
{
public:
	static void init();
	static SubscribeList* getInstance();

	void setTcpServer(TcpServer* server);

	void subscribe(unsigned long long mod,int fd);
	void unsubscribe(unsigned long long mod, int fd);

	void publish(vector<unsigned long long>& changemods);
	void makePublisMap(unordered_set<int>& fds, unordered_map<int, unordered_set<unsigned long long>>& publishlist);
	void pushChangeTask(EventLoop* loop);
	~SubscribeList();
private:
	SubscribeList();
	SubscribeList(const SubscribeList&) = delete;
	const SubscribeList& operator=(const SubscribeList&) = delete;

	static SubscribeList* _instance;

	TcpServer* _server;
	unordered_map<unsigned long long, unordered_set<int>> subscribemap;//�����б�key:modid/cmdid,value:fds
	unordered_map<int, unordered_set<unsigned long long>> publishmap;//�����б����и��ĵ�modid/cmdid���͸����ĵĿͻ���

	mutex sublistlock;
	mutex publistlock;

};

