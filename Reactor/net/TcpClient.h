#pragma once
#include"../IOBuff/IObuf.h"
#include"EventLoop.h"
#include"Message.h"
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<functional>
#include"net_conn.h"
class TcpClient;

class TcpClient:net_conn
{
public:
	TcpClient(EventLoop* loop, const char* ip, short port, const char *name);
	int send_message(const char * data, int msglen, int msgid);
	int getFd();
	void ConnectServer();
	~TcpClient();
	void addMsgCallback(int msgid, MessageCallback cb);
	//void setMseeageCallback(MessageCallback cb)
	//{
	//	_MessageCallback = cb;
	//}
	void setCompleteCallback(netCallback cb);
	void setCloseCallback(netCallback cb);

private:

	int _sockfd;
	
	socklen_t _addrlen;
	
	EventLoop* _loop;

	const char* _name;
	
	MessageCallback _MessageCallback;
	
	bool connected;
	
	sockaddr_in _serverAddr;
	
	IOBuff _ibuf;
	
	IOBuff _obuf;

	MsgRouter router;

	netCallback _connCompleteCallback;
	netCallback _connCloseCallback;

	int handleRead();
	int handleWrite();
	void handleClose();
	void connection_delay();
};

