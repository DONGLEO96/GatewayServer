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
	int begin = time(NULL);
	int succ = 0;
	int err = 0;
	for (int i = 0; i < 50001; ++i)
	{
		int ret = client.GetHost(modid, cmdid, ip, port);
		if (ret == 0)
		{
			succ += 1;
		}
		else
		{
			err += 1;
		}
	}
	int end = time(NULL);
	int qps = succ / (end - begin);
	double succrate = (double)(succ) / 50000;
	printf("qps=%d,success rate=%d", qps, succrate);

	return 0;
}
