#pragma once
#include"taskQueue.h"
#include"TaskMsg.h"
#include<vector>
#include<functional>
#include"EventLoop.h"

using namespace std;
class ThreadPool
{
public:
	ThreadPool(int threadNum);
	~ThreadPool();
	ThreadQueue<TaskMsg>* getThread();

	void sendTask(Task func);

private:


	int _threadNum;
	vector<ThreadQueue<TaskMsg>*> _queues;
	vector<thread::id> _tids;
	int _currIndex;
};

