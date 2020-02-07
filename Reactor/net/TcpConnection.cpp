#include "TcpConnection.h"
#include<sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include<memory.h>
#include"Message.h"
#include<unistd.h>
#include<sstream>
#include"TcpServer.h"
TcpConnection::TcpConnection(int connfd, EventLoop * loop):_connfd(connfd),_loop(loop)
{
	int op = 1;
	setsockopt(_connfd, IPPROTO_TCP, TCP_NODELAY, &op, sizeof(op));
	
	if (TcpServer::_connCompleteCallback)
	{
		TcpServer::_connCompleteCallback(this);
	}
	
	_loop->addEvent(_connfd, std::bind(&TcpConnection::handleRead, this), EPOLLIN);
	//_loop->addEvent(_connfd, std::bind(&TcpConnection::handleReadForTest, this), EPOLLIN);
}

TcpConnection::~TcpConnection()
{
	printf("conncection desturctor\n");

	close(_connfd);
}

void TcpConnection::handleRead()
{
	int ret = _ibuf.readData(_connfd);
	if (ret == -1)
	{
		fprintf(stderr, "read data from socket error\n");
		this->handleClose();
		return;
	}
	else if (ret == 0)
	{
		//完善后换成日志
		printf("connection closed\n");
		handleClose();
		return;
	}
	//parse message
	msg_head head;
	while (_ibuf.length() >= MessageHeadLength)
	{
		memcpy(&head, _ibuf.data(), MessageHeadLength);
		if (head.msglen > MessageMaxLength || head.msglen < 0)
		{
			fprintf(stderr, "data format error,need close ,msglen=%d\n", head.msglen);
			handleClose();
			break;
		}
		if (_ibuf.length() <static_cast<size_t>(MessageHeadLength + head.msglen))
		{
			//还没有接到一个完整的包
			break;
		}

		_ibuf.pop(MessageHeadLength);

		string s(_ibuf.data(), _ibuf.data() + head.msglen);
		printf("read data:%s\n", s.data());

		//完成业务
		//send_message(_ibuf.data(), head.msglen, head.msgid);

		
		TcpServer::router.call(head.msgid, head.msglen, _ibuf.data(), this);


		_ibuf.pop(head.msglen);
	}
	_ibuf.adjust();
}


void TcpConnection::handleReadForTest()
{
	int ret = _ibuf.readData(_connfd);
	if (ret == -1)
	{
		fprintf(stderr, "read data from socket error\n");
		this->handleClose();
		return;
	}
	else if (ret == 0)
	{
		//完善后换成日志
		printf("connection closed\n");
		handleClose();
		return;
	}

	send_message(_ibuf.data(), static_cast<int>(_ibuf.length()), 1);
	handleClose();
	//_ibuf.adjust();
}


void TcpConnection::handleWrite()
{
	while (_obuf.length())
	{
		int ret = _obuf.writeFd(_connfd);
		if (ret == -1)
		{
			fprintf(stderr, "write2Fd error ,close conn\n");
			handleClose();
			return;
		}
		if (ret == 0)
		{
			break;
		}
	}

	if (_obuf.length() == 0)
	{
		_loop->delEvent(_connfd, EPOLLOUT);
	}
	return;
}

void TcpConnection::handleClose()
{
	if (TcpServer::_connCloseCallback)
	{
		TcpServer::_connCloseCallback(this);
	}


	TcpServer::removeConn(_connfd);

	_loop->delEvent(_connfd);
	_ibuf.clear();
	_obuf.clear();


}

int TcpConnection::send_message(const char * data, int msglen, int msgid)
{
	
	//package
	msg_head head;
	head.msgid = msgid;
	head.msglen = msglen;

	string msg;
	msg.append((char*)&head, MessageHeadLength);
	msg.append(data, msglen);


	
	printf("server send_message:%s,msglen=%d,msgid=%d\n", data, msglen, msgid);
	
	
	ssize_t ret = 0;
	//如果未关注写事件且输出缓冲区没有数据，直接向套接字写
	if (!(_loop->getEvent(_connfd) & EPOLLOUT) && (_obuf.length() == 0))
	{
		ret = write(_connfd, msg.data(), msg.length());
		if (ret < 0)
		{
			fprintf(stderr, "write data to socket %d error\n, errno=%d", _connfd, errno);
		}
		if (static_cast<size_t>(ret) == msg.length())
		{
			//成功发送所有数据
			return 0;
		}
	}
	if (static_cast<size_t>(ret) < msg.length())
	{
		//消息没发完，丢到缓冲区去
		if (_obuf.sendData(msg.data() + ret, static_cast<int>(msg.size() - ret)))
		{
			return -1;
		}
		_loop->addEvent(_connfd, std::bind(&TcpConnection::handleWrite, this), EPOLLOUT);
	}



	return 0;
}

int TcpConnection::getFd()
{
	return _connfd;
}
//void TcpConnection::settie(shared_ptr<TcpConnection> conn)
//{
//	_tie = conn;
//}

