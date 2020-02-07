#pragma once
#include<unordered_map>
#include<functional>

struct msg_head
{
	int msgid;
	int msglen;
};

const int MessageHeadLength = sizeof(msg_head);
const int MessageMaxLength = 65535 - 8;

class net_conn;
typedef std::function<void(const char*, int, int, net_conn*)> MessageCallback;
class MsgRouter
{
public:
	MsgRouter():_router()
	{
	}
	int registerMsgCallback(int msgid, MessageCallback cb)
	{
		if (_router.find(msgid) != _router.end())
		{
			return -1;
		}

		_router[msgid] = cb;
		
		return 0;
	}
	void call(int msgid, int msglen, const char* data, net_conn* client)
	{
		if (_router.find(msgid) == _router.end())
		{
			fprintf(stderr, "msgid %d is not register\n", msgid);
			return;
		}
		MessageCallback callback = _router[msgid];
		callback(data, msglen, msgid, client);
		printf("call message callback,msgid=%d\n============\n", msgid);
	}
	
private:
	std::unordered_map<int, MessageCallback> _router;
	std::unordered_map<int, void*> _args;
};