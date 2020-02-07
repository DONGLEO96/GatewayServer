#pragma once
#include<sys/epoll.h>
#include"IOEvent.h"
#include<unordered_map>
#include<unordered_set>
#include<vector>
#include<functional>
using namespace std;

class EventLoop;

typedef function<void(EventLoop*)> Task;

class EventLoop
{
public:
	EventLoop();
	~EventLoop();
	void loop();
	void Clientloop();
	void addEvent(int fd, IOCallback cb, int mask);

	void delEvent(int fd);

	void delEvent(int fd, int mask);

	inline int getEvent(int fd)
	{
		auto it = ioEvents.find(fd);
		if (it != ioEvents.end())
		{
			return it->second.mask;
		}
		else
		{
			return -1;
		}
	}

	unordered_set<int> getListenFds();

	void addTask(Task task);
	void execReadyTask();

private:
	int epfd;
	unordered_map<int, IOEvent> ioEvents;
	//监控fd和对应句柄
	unordered_set<int> listenFds;
	//监听的所有fd
	vector<epoll_event> activityEvents;

	vector<Task> tasks;;
	//bool quit;

};

