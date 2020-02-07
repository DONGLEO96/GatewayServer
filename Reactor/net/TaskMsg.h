#pragma once
#include<functional>
#include"EventLoop.h"
enum TASK_TYPE
{
	NEW_CONN = 1,
	NEW_TASK

};

struct TaskMsg
{
	TASK_TYPE type;

	
	int connfd;
	std::function<void(EventLoop*)> task;
};

