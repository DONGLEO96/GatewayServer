#include"../Reactor/net/TcpServer.h"
#include"../util/ConfigFile.h"
#include"mysql/mysql.h"
#include"DnsRoute.h"
#include"../json/json.h"
#include"../util/Message.h"
#include"Subscribe.h"
using namespace std;
TcpServer* server;
void CheckRouteChanges()
{
	int wait_time = 10;
	long last_load_time = time(NULL);

	DnsRoute::getInstance()->removeChange(true);

	while (true)
	{
		sleep(1);

		long currenttime = time(NULL);
		int ret = DnsRoute::getInstance()->LoadVersion();
		if (ret == 1)//�汾�Ѿ�����
		{
			if (DnsRoute::getInstance()->LoadData() == 0)//����װ��·����Ϣ
			{
				DnsRoute::getInstance()->Updata();
				last_load_time = currenttime;
			}

			vector<unsigned long long> changes = DnsRoute::getInstance()->LoadChange();//��ȡ·����Ϣ�ı����modid/cmdid

			SubscribeList::getInstance()->publish(changes);

			DnsRoute::getInstance()->removeChange();
		}
		else
		{
			if (currenttime - last_load_time >= wait_time)//û���޸ģ�ÿ��һ��ʱ��Ҳ���¼�������
			{
				if (DnsRoute::getInstance()->LoadData() == 0)
				{
					DnsRoute::getInstance()->Updata();
					last_load_time = currenttime;
				}
			}
		}

	}
}

void getRouteMessage(const char* data, int len, int msgid, net_conn* conn)
{
	Json::Value value;
	Json::Reader jr;
	jr.parse(data, data + len, value);

	int modid = value["modid"].asInt();
	int cmdid = value["cmdid"].asInt();

	auto hosts = DnsRoute::getInstance()->getHostSet(modid, cmdid);

	DnsResponseMsg msg;
	msg.modid = modid;
	msg.cmdid = cmdid;
	msg.hostnum = static_cast<int>(hosts.size());

	unsigned long long mod = (static_cast<unsigned long long>(modid) << 32) + cmdid;
	unordered_set<unsigned long long>* sublist = (unordered_set<unsigned long long>*)conn->context;
	if (sublist->find(mod) == sublist->end())//���û�ж��Ĺ�
	{
		sublist->insert(mod);
		SubscribeList::getInstance()->subscribe(mod, conn->getFd());//��ӵ������б�
	}

	for (auto it = hosts.begin(); it != hosts.end(); ++it)
	{
		unsigned long long ipport = *it;
		int ip = (unsigned int)(ipport >> 32);
		int port = (unsigned int)ipport;
		msg.ips.push_back(ip);
		msg.ports.push_back(port);
	}

	value["modid"] = msg.modid;
	value["cmdid"] = msg.cmdid;
	value["hostnum"] = msg.hostnum;
	for (int i = 0; i < msg.hostnum; ++i)
	{
		value["ips"].append(Json::Value(msg.ips[i]));
		value["ports"].append(Json::Value(msg.ports[i]));
	}
	string jsonmsg = value.toStyledString();
	conn->send_message(jsonmsg.data(), static_cast<int>(jsonmsg.size()), DnsResponseID);
}
void createSubscribe(net_conn* conn)
{
	conn->context = new unordered_set<unsigned long long>();
	
}
void clearSubscribe(net_conn* conn)
{
	unordered_set<unsigned long long>* sublist = static_cast<unordered_set<unsigned long long>*>(conn->context);
	auto it = sublist->begin();
	for (; it != sublist->end(); ++it)
	{
		unsigned long long mod = *it;
		SubscribeList* sub = SubscribeList::getInstance();
		sub->unsubscribe(mod, conn->getFd());
	}
	delete sublist;
	conn->context = NULL;
}
int main(int argc,char* argv[])
{

	ConfigFile configfile;
	if (argc > 1)
	{
		string path(argv[1]);
		configfile.setPath(path);
	}
	configfile.load();
	Json::Value config = configfile.getConfig();
	string ip = config["dns"]["ip"].asString();
	short port = static_cast<short>(config["dns"]["port"].asInt());
	int maxconn = config["dns"]["maxconn"].asInt();
	int threadnum = config["dns"]["threadnum"].asInt();
	EventLoop loop;
	server = new TcpServer(&loop, ip.data(), port, maxconn, threadnum);


	//ע��·��ҵ��
	server->setCompleteCallback(createSubscribe);
	server->setCloseCallback(clearSubscribe);
	server->addMsgCallback(DnsRequestID, getRouteMessage);
	//t->setCompleteCallback();
	//ע��·��ҵ��

	//����mysql
	DnsRoute::getInstance();
	//����mysql
	SubscribeList::getInstance()->setTcpServer(server);
	

	thread BackThread(CheckRouteChanges);//��̨�̶߳��ڼ��·����Ϣ���ݿ��Ƿ��и���
	BackThread.detach();
	printf("dns service start... ...\n");
	loop.loop();
	return 0;
}