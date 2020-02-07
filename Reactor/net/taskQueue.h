#pragma once
#include<queue>
#include<thread>
#include<sys/eventfd.h>
#include<stdio.h>
#include<unistd.h>
#include"EventLoop.h"
#include<mutex>

template<class T>
class ThreadQueue
{
public:
	ThreadQueue() :_loop(NULL), _queue(), _queuemutex()
	{
		_eventfd = eventfd(0, EFD_NONBLOCK);
		if (_eventfd == -1)
		{
			fprintf(stderr, "eventfd error\n");
			exit(1);
		}
	}
	~ThreadQueue()
	{
		close(_eventfd);
	}

	void addTask(const T& task)
	{
		long long n = 1;
		std::lock_guard<std::mutex> lock(_queuemutex);
		_queue.push_back(task);
		ssize_t ret = write(_eventfd, &n, sizeof(long long));
		if (ret == -1)
		{
			fprintf(stderr, "write eventfd error\n");
		}
	}
	void getTask(std::vector<T>& new_queue)
	{
		long long n = 1;
		std::lock_guard<std::mutex> lock(_queuemutex);
		ssize_t ret = read(_eventfd, &n, sizeof(long long));
		if (ret == -1)
		{
			fprintf(stderr, "read eventfd error\n");
		}
		std::swap(new_queue, _queue);
	}

	void setLoop(EventLoop* loop)
	{
		_loop = loop;
	}
	void setCallback(IOCallback cb)
	{
		if (_loop)
		{
			_loop->addEvent(_eventfd, cb, EPOLLIN);
		}
	}
	int getEventfd()
	{
		return _eventfd;
	}
	EventLoop* getLoop()
	{
		return _loop;
	}

private:
	int _eventfd;
	EventLoop* _loop;
	std::vector<T> _queue;
	std::mutex _queuemutex;
};