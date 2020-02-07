#include "ThreadPool.h"
#include"TcpConnection.h"
#include"TcpServer.h"


void dealTaskMessage(EventLoop* loop, ThreadQueue<TaskMsg>* taskqueue)
{
	vector<TaskMsg> tasks;
	taskqueue->getTask(tasks);
	for (size_t i = 0; i < tasks.size(); ++i)
	{
		if (tasks[i].type == NEW_CONN)
		{
			TcpConnection* conn = new TcpConnection(tasks[i].connfd, loop);
			if (conn == NULL)
			{
				fprintf(stderr, "int thread new tcpconn error\n");
				exit(1);
			}
			TcpServer::addConn(tasks[i].connfd, conn);
			printf("[thread]:get new conntion success\n");
		}
		else if (tasks[i].type == NEW_TASK)
		{
			loop->addTask(tasks[i].task);
		}
		else
		{
			fprintf(stderr, "unkonw task\n");
		}
	}
}
void thread_main(ThreadQueue<TaskMsg>* threadqueue)
{
	EventLoop* loop = new EventLoop();
	if (loop == NULL)
	{
		fprintf(stderr, "new eventloop error\n");
		exit(1);
	}
	threadqueue->setLoop(loop);
	threadqueue->setCallback(std::bind(dealTaskMessage, loop, threadqueue));

	loop->loop();

	return;


}

ThreadPool::ThreadPool(int threadNum):_threadNum(threadNum),_queues(_threadNum),
_tids(_threadNum),_currIndex(0)
{
	if (_threadNum <= 0)
	{
		fprintf(stderr, "_threadNum<0\n");
		exit(1);
	}

	for(int i = 0; i < _threadNum; ++i)
	{
		printf("create %d  thread\n", i);
		_queues[i] = new ThreadQueue<TaskMsg>();
		thread t(std::bind(thread_main, _queues[i]));
		_tids[i] = t.get_id();
		//thread t()
		t.detach();
	}
}

ThreadPool::~ThreadPool()
{
}

ThreadQueue<TaskMsg>* ThreadPool::getThread()
{
	_currIndex += 1;
	if (_currIndex == _threadNum)
	{
		_currIndex = 0;
	}
	return _queues[_currIndex];
	vector<int> a;
	
}

void ThreadPool::sendTask(Task func)
{
	TaskMsg task;

	for (int i = 0; i < _threadNum; ++i)
	{
		task.type = NEW_TASK;
		task.task = func;

		ThreadQueue<TaskMsg>* queue = _queues[i];
		
		queue->addTask(task);
	}
}




