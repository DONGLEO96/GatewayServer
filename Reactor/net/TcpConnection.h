#pragma once
#include"EventLoop.h"
#include"../IOBuff/InputBuff.h"
#include"../IOBuff/OutputBuff.h"
#include"net_conn.h"
#include<memory>
class TcpConnection:net_conn
{
public:
	TcpConnection(int connfd, EventLoop* loop);
	~TcpConnection();

	void handleRead();
	void handleWrite();
	void handleClose();
	int send_message(const char* data, int msglen, int msgid);
	int getFd();

private:

	void handleReadForTest();
	int _connfd;
	EventLoop* _loop;
	OutputBuff _obuf;
	InputBuff _ibuf;
	
	
	//IOCallback _closeCallback;
};

