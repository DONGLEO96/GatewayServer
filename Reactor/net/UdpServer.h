#pragma once
#include<netinet/in.h>
#include"EventLoop.h"
#include"net_conn.h"
#include"Message.h"
class UdpServer:public net_conn
{
public:
	UdpServer(EventLoop* loop, const char* ip, unsigned short port);
	~UdpServer();

	virtual int send_message(const char* data, int datalen, int msgid);
	virtual int getFd();

	void handleSIGPIPE();

	void addMsgCallback(int msgid, MessageCallback cb);

	void do_read();
private:
	int _sockfd;

	char _readbuffer[MessageMaxLength];
	char _writebuffer[MessageMaxLength];
	EventLoop * _loop;

	sockaddr_in _clientAddr;
	socklen_t _clientAddrLen;

	MsgRouter _router;


};

