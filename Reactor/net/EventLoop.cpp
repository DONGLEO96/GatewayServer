#include "EventLoop.h"
#include<assert.h>
#include"../IOBuff/BuffPool.h"//check memory leak
#include"TcpServer.h"



EventLoop::EventLoop():ioEvents(),listenFds(),activityEvents(16)
{
	epfd = epoll_create(1);
	if (epfd == 1)
	{
		fprintf(stderr, "epoll_create error\n");
		exit(1);
	}
}


EventLoop::~EventLoop()
{
}

void EventLoop::loop()
{
	printf("start EventLoop\n");
	while (true)
	{
		int eventNum = epoll_wait(epfd, static_cast<epoll_event*>(&(*activityEvents.begin())), static_cast<int>(activityEvents.capacity()), -1);
		for (int i = 0; i < eventNum; ++i)
		{
			auto guard = TcpServer::getConn(activityEvents[i].data.fd);//保证connection在执行完成后才会析构
			//只有TCP服务端需要guard
			auto it = ioEvents.find(activityEvents[i].data.fd);
			assert(it != ioEvents.end());

			IOEvent* ev = &(it->second);

			if (activityEvents[i].events&EPOLLIN)
			{
				ev->_readCallback();
			}
			else if(activityEvents[i].events&EPOLLOUT)
			{
				ev->_writeCallback();
			}
			else if (activityEvents[i].events&(EPOLLHUP | EPOLLERR))
			{
				if (ev->_readCallback)
				{
					ev->_readCallback();
				}
				else if (ev->_writeCallback)
				{
					ev->_writeCallback();
				}
				else
				{
					delEvent(activityEvents[i].data.fd);
				}
			}
		}
		if (static_cast<size_t>(eventNum) == activityEvents.capacity())
		{
			activityEvents.reserve(eventNum * 2);
		}
		
		this->execReadyTask();
		//BuffPool::getInstance()->useInfo();
	}
}

void EventLoop::Clientloop()
{
	printf("start EventLoop\n");
	while (true)
	{
		int eventNum = epoll_wait(epfd, static_cast<epoll_event*>(&(*activityEvents.begin())), static_cast<int>(activityEvents.capacity()), -1);
		for (int i = 0; i < eventNum; ++i)
		{
			//auto guard = TcpServer::getConn(activityEvents[i].data.fd);//保证connection在执行完成后才会析构

			auto it = ioEvents.find(activityEvents[i].data.fd);
			assert(it != ioEvents.end());

			IOEvent* ev = &(it->second);

			if (activityEvents[i].events&EPOLLIN)
			{
				ev->_readCallback();
			}
			else if (activityEvents[i].events&EPOLLOUT)
			{
				ev->_writeCallback();
			}
			else if (activityEvents[i].events&(EPOLLHUP | EPOLLERR))
			{
				if (ev->_readCallback)
				{
					ev->_readCallback();
				}
				else if (ev->_writeCallback)
				{
					ev->_writeCallback();
				}
				else
				{
					delEvent(activityEvents[i].data.fd);
				}
			}
		}
		if (static_cast<size_t>(eventNum) == activityEvents.capacity())
		{
			activityEvents.reserve(eventNum * 2);
		}

		this->execReadyTask();
		//BuffPool::getInstance()->useInfo();
	}
}


void EventLoop::addEvent(int fd, IOCallback cb, int mask)
{
	int finalmask;
	int op;
	auto it = ioEvents.find(fd);
	if (it == ioEvents.end())
	{
		finalmask = mask;
		op = EPOLL_CTL_ADD;
	}
	else
	{
		finalmask = it->second.mask | mask;
		op = EPOLL_CTL_MOD;
	}

	if (mask&EPOLLIN)
	{
		ioEvents[fd]._readCallback = cb;
	}
	else if (mask&EPOLLOUT)
	{
		ioEvents[fd]._writeCallback = cb;
	}

	ioEvents[fd].mask = finalmask;

	epoll_event event;
	event.events = finalmask;
	event.data.fd = fd;
	if (epoll_ctl(epfd, op, fd, &event) == -1)
	{
		fprintf(stderr, "epoll ctl %d error\n", fd);
		return;
	}

	listenFds.insert(fd);
}

void EventLoop::delEvent(int fd)
{
	ioEvents.erase(fd);
	listenFds.erase(fd);
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}

void EventLoop::delEvent(int fd, int mask)
{
	auto it = ioEvents.find(fd);
	if (it == ioEvents.end())
	{
		return;
	}
	int& oldmask = it->second.mask;

	oldmask = oldmask & (~mask);

	if (oldmask == 0)
	{
		this->delEvent(fd);
	}
	else
	{
		epoll_event event;

		event.data.fd = fd;
		event.events = oldmask;

		epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
	}
}

unordered_set<int> EventLoop::getListenFds()
{
	return listenFds;
}

void EventLoop::addTask(Task task)
{
	tasks.push_back(task);
}

void EventLoop::execReadyTask()
{
	for (size_t i = 0; i < tasks.size(); ++i)
	{
		Task t = tasks[i];
		t(this);
	}
	tasks.clear();
}

