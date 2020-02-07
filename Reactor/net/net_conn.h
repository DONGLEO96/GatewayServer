#pragma once
#include<functional>


class net_conn
{
public:
	virtual int send_message(const char* data, int datalen, int msgid) = 0;
	virtual int getFd() = 0;

	void* context;

};
typedef std::function<void(net_conn*)> netCallback;
