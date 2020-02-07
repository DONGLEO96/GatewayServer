#pragma once
#include<unordered_map>
#include"IObuf.h"
#include<mutex>
using namespace std;

//buff内存池
static const int size_4k = 4096;//5000,20M
static const int size_16k = 16384;//1000,16M
static const int size_64k = 65536;//500,32M
static const int size_256k = 262144;//200,50M
static const int size_1M = 1048576;//50,50
static const int size_4M = 4194304;//20,80
static const int size_8M = 8388608;//10,80

static const int MAXLIMIT = 1 * 1024 * 1024;//单位 kb
static once_flag _once;

class BuffPool
{
public:
	BuffPool(const BuffPool& rhs) = delete;
	BuffPool& operator=(const BuffPool&) = delete;
	~BuffPool();

	IOBuff* alloc_buf(size_t size);//分配buf
	//IOBuff* alloc_buf() { return alloc_buf(); }
	void revert(IOBuff* buf);//归还buf

	inline static BuffPool* getInstance()
	{
		call_once(_once, init);
		return _instance;
	}

	void useInfo()
	{
		printf("-------memory pool------------\n");
		printf("4K :  %d\n",bufCount[size_4k]);
		printf("16K :  %d\n", bufCount[size_16k]);
		printf("64K :  %d\n", bufCount[size_64k]);
		printf("256K :  %d\n", bufCount[size_256k]);
		printf("1M :  %d\n", bufCount[size_1M]);
		printf("4M :  %d\n", bufCount[size_4M]);
		printf("8M :  %d\n", bufCount[size_8M]);
		printf("-------memory pool------------\n");
	}

private:
	BuffPool();
	unordered_map<int, IOBuff*> _bufpool;

	unordered_map<int, int> bufCount;//用于记录已经被使用的块，check memory leak;

	long long _totalmem;
	static BuffPool* _instance;
	mutex _mutex;//需要使用静态变量吗？

	static void init()
	{
		_instance = new BuffPool();
	}
};

