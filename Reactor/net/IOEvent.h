#pragma once
#include<functional>
using namespace std;
typedef std::function<void()> IOCallback;

struct IOEvent
{
	int mask;
	IOCallback _readCallback;
	IOCallback _writeCallback;
};