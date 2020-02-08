#include"../ApiClient.h"
#include"../../util/Message.h"
using namespace std;
int main(int argc, char* argv[])
{
	int modid = 1;
	int cmdid = 1;
	if (argc == 3)
	{
		modid = atoi(argv[1]);
		cmdid = atoi(argv[2]);
	}

	ApiClient client;

	string ip;
	int port;

	int ret = client.GetHost(modid, cmdid, ip, port);
	if (ret == 3)
	{
		printf("server is not found\n");
	}
	if (ret == 0)
	{
		printf("[%d:%d]:host is %s:%d\n", modid, cmdid, ip.data(), port);
		client.report(modid, cmdid, ip, port, 0);
	}

	vector<pair<string, int>> res;
	ret = client.GetRouteInfo(modid, cmdid, res);
	if (ret == SystemError)
	{
		printf("Get Route Info SystemError\n");
		return 0;
	}
	if (ret == CallResult::Sueecss)
	{
		if (res.size() == 0)
		{
			printf("GetRoute Empty\n");
			return 0;
		}
		else
		{
			printf("==========================\n");
			printf("Get Route Info:\n");
			printf("modid=%d\n", modid);
			printf("cmdid=%d\n", cmdid);
			for (int i = 0; i < static_cast<int>(res.size()); ++i)
			{
				printf("%s:%d", res[i].first.data(), res[i].second);
				printf("\n");
			}
		}
	}


	return 0;
}
