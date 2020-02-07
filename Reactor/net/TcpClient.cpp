#include "TcpClient.h"
#include<memory.h>
#include<unistd.h>
#include<assert.h>
#include<sys/ioctl.h>
TcpClient::TcpClient(EventLoop * loop, const char * ip, short port, const char * name):
_sockfd(-1),_loop(loop),_name(name), _MessageCallback(NULL),
connected(false),_ibuf(4*1024*1024),
_obuf(4 * 1024 * 1024),router()
{
	memset(&_serverAddr, 0, sizeof(_serverAddr));
	_serverAddr.sin_family = AF_INET;
	inet_aton(ip, &_serverAddr.sin_addr);
	_serverAddr.sin_port = htons(port);

	_addrlen = sizeof(_serverAddr);
	//ConnectServer();
}
TcpClient::~TcpClient()
{
	if (_sockfd != -1)
	{
		_loop->delEvent(_sockfd);
		close(_sockfd);
	}
}
void TcpClient::addMsgCallback(int msgid, MessageCallback cb)
{
	router.registerMsgCallback(msgid, cb);
}
void TcpClient::setCompleteCallback(netCallback cb)
{
	_connCompleteCallback = cb;
}
void TcpClient::setCloseCallback(netCallback cb)
{
	_connCloseCallback = cb;
}
int TcpClient::send_message(const char * data, int msglen, int msgid)
{
	if (connected == false)
	{
		fprintf(stderr, "no connected,can not send message\n");
		return -1;
	}

	bool needAddEvent = (_obuf.length == 0) ? true : false;
	if (static_cast<size_t>(msglen + MessageHeadLength) > this->_obuf.capacity - _obuf.length)
	{
		fprintf(stderr, "No more space to write socket\n");
		return -1;
	}

	msg_head head;
	head.msgid = msgid;
	head.msglen = msglen;

	memcpy(_obuf.data + _obuf.length, &head, MessageHeadLength);
	_obuf.length += MessageHeadLength;

	memcpy(_obuf.data + _obuf.length, data, msglen);
	_obuf.length += msglen;

	if (needAddEvent)
	{
		_loop->addEvent(_sockfd, std::bind(&TcpClient::handleWrite, this), EPOLLOUT);
	}
	return 0;
}

int TcpClient::getFd()
{
	return _sockfd;
}

void TcpClient::ConnectServer()
{
	if (_sockfd != -1)
	{
		close(_sockfd);
	}
	_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP);
	if (_sockfd == -1)
	{
		fprintf(stderr, "create sock fd error\n");
		exit(1);
	}
	int ret = connect(_sockfd, (const sockaddr*)&_serverAddr, _addrlen);
	if (ret == 0)
	{
		connected = true;

		if (_connCompleteCallback)
		{
			_connCompleteCallback(this);
		}


		_loop->addEvent(_sockfd, std::bind(&TcpClient::handleRead, this), EPOLLIN);
		if (this->_obuf.length != 0)
		{
			_loop->addEvent(_sockfd, std::bind(&TcpClient::handleWrite, this), EPOLLOUT);
		}
		printf("connect %s:%d success\n", inet_ntoa(_serverAddr.sin_addr), ntohs(_serverAddr.sin_port));
	}
	else
	{
		if (errno == EINPROGRESS)
		{
			fprintf(stderr, "connect EINPROGRESS\n");
			_loop->addEvent(_sockfd, std::bind(&TcpClient::connection_delay, this), EPOLLOUT);
		}
		else
		{
			fprintf(stderr, "connection error\n");
			exit(1);
		}
	}
}

int TcpClient::handleRead()
{
	assert(connected == true);

	int readAbleBytes = 0;
	if (ioctl(_sockfd, FIONREAD, &readAbleBytes) == -1)
	{
		fprintf(stderr, "ioctl error\n");
		return -1;
	}
	assert(static_cast<size_t>(readAbleBytes) <= _ibuf.capacity - _ibuf.length);

	ssize_t ret = 0;

	do 
	{
		ret = read(_sockfd, _ibuf.data + _ibuf.length, readAbleBytes);
	} while (ret == -1 && errno == EINTR);

	if (ret == 0)
	{
		fprintf(stderr,"connnection closed by server\n");
		
		handleClose();

		//todo 使用重连

		return -1;
	}
	else if (ret == -1)
	{
		fprintf(stderr, "client read error\n");
		handleClose();
		//todo 重连
		
		return -1;

	}

	assert(ret == readAbleBytes);
	_ibuf.length += ret;

	msg_head head;
	int msgid, length;
	while (_ibuf.length >= MessageHeadLength)
	{
		memcpy(&head, _ibuf.data + _ibuf.head, MessageHeadLength);
		msgid = head.msgid;
		length = head.msglen;
		

		if (static_cast<size_t>(length + MessageHeadLength) > _ibuf.length)
		{
			//消息不完全
			break;
		}

		_ibuf.pop(MessageHeadLength);

		/*if (_MessageCallback)
		{
			_MessageCallback(_ibuf.data + _ibuf.head, length, msgid, this);
		}*/
		
		router.call(msgid, length, _ibuf.data + _ibuf.head, this);

		_ibuf.pop(length);
	}

	_ibuf.adjust();

	return 0;


}

int TcpClient::handleWrite()
{
	if (_obuf.head != 0)
		_obuf.adjust();
	assert(_obuf.length);
	ssize_t ret;
	while (_obuf.length)
	{
		do
		{ 
			ret = write(_sockfd, _obuf.data, _obuf.length);

		} while (ret == -1 && errno == EINTR);

		if (ret > 0)
		{
			_obuf.pop(static_cast<int>(ret));
			_obuf.adjust();
		}
		else if (ret == -1 && errno != EAGAIN)
		{
			fprintf(stderr, "tcpclient write errno\n");
			handleClose();
		}
		else
		{
			break;//EAGAIN了，下次再写
		}
	}

	if (_obuf.length == 0)
	{
		printf("write all data,delete EPOLLPOUT\n");
		_loop->delEvent(_sockfd, EPOLLOUT);
	}

	return 0;
}

void TcpClient::handleClose()
{
	if (_sockfd != -1)
	{
		printf("close sockfd\n");
		_loop->delEvent(_sockfd);
		close(_sockfd);
	}
	connected = false;

	if (_connCloseCallback)
	{
		_connCloseCallback(this);
		
	}
	_sockfd = -1;

	ConnectServer();
	
}

void TcpClient::connection_delay()
{
	_loop->delEvent(_sockfd);//删除观测链接是否成功的事件
	int result = 0;
	socklen_t result_len = sizeof(result);
	getsockopt(_sockfd, SOL_SOCKET, SO_ERROR, &result, &result_len);
	if (result == 0)
	{
		connected = true;
		printf("connect %s:%d success \n", inet_ntoa(_serverAddr.sin_addr), ntohs(_serverAddr.sin_port));

		if (_connCompleteCallback)
		{
			_connCompleteCallback(this);
		}

		//const char* msg = "hello world\0";
		//int msgid = 1;
		//
		//send_message(msg, static_cast<int>(strlen(msg)), msgid);
		
		//const char* msg2 = "hello server\0";
		//int msgid2 = 2;

		//send_message(msg2, static_cast<int>(strlen(msg2)), msgid2);
		
		
		
		_loop->addEvent(_sockfd, std::bind(&TcpClient::handleRead, this), EPOLLIN);
		if (_obuf.length != 0)
		{
			_loop->addEvent(_sockfd, std::bind(&TcpClient::handleWrite, this), EPOLLOUT);

		}
	}
	else
	{
		fprintf(stderr, "connect %s:%d failed \n", inet_ntoa(_serverAddr.sin_addr), ntohs(_serverAddr.sin_port));
	}
	
}

