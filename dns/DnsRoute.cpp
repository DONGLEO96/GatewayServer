#include"DnsRoute.h"
#include<string.h>
#include<unistd.h>
#include"Subscribe.h"

DnsRoute* DnsRoute::_instance = NULL;
once_flag _once;

DnsRoute::DnsRoute():_version(0),datamap(),tempmap(),_mutex()
{
	ConnectDb();
	BuildMaps();
}
void DnsRoute::init()
{
	DnsRoute::_instance = new DnsRoute();
}

DnsRoute * DnsRoute::getInstance()
{
	call_once(_once, init);
	return _instance;
}

unordered_set<unsigned long long> DnsRoute::getHostSet(int modid, int cmdid)
{
	unordered_set<unsigned long long> hosts;
	unsigned long long key = ((unsigned long long)modid << 32) + cmdid;
	lock_guard<mutex> lock(this->_mutex);
	auto it = datamap.find(key);
	if (it != datamap.end())
	{
		hosts = it->second;
	}
	return hosts;
}

void DnsRoute::ConnectDb()
{
	mysql_init(&this->dbconn);
	
	if (!mysql_real_connect(&dbconn, "0.0.0.0", "root","111111","dns",3306,NULL,0))
	{
		fprintf(stderr, "Faied to connect mysql\n");
		exit(1);
	}
}

void DnsRoute::BuildMaps()
{

	int ret = 0;
	snprintf(sql, 1024, "SELECT * FROM RouteData;");
	ret = mysql_real_query(&dbconn, sql, strlen(sql));
	if (ret != 0)
	{
		fprintf(stderr, "failed to find anydata,error %s\n", mysql_error(&dbconn));
		exit(1);
	}

	MYSQL_RES* result = mysql_store_result(&dbconn);
	unsigned long long linenum = mysql_num_rows(result);

	MYSQL_ROW row;
	for (unsigned long long i = 0; i < linenum; ++i)
	{
		row = mysql_fetch_row(result);
		int modId = atoi(row[1]);
		int cmdId = atoi(row[2]);
		unsigned ip = atoi(row[3]);
		int port = atoi(row[4]);

		unsigned long long key = (static_cast<unsigned long long>(modId) << 32) + cmdId;
		unsigned long long value = (static_cast<unsigned long long>(ip) << 32) + port;
		
		printf("modID = %d, cmdID = %d, ip = %u, port = %d\n", modId, cmdId, ip, port);

		datamap[key].insert(value);
	}

	mysql_free_result(result);
}

int DnsRoute::LoadVersion()
{
	snprintf(sql, 1024, "SELECT version from RouteVersion WHERE id=1;");

	int ret = mysql_real_query(&dbconn, sql, strlen(sql));

	if (ret)
	{
		fprintf(stderr, "load version error:%s\n", mysql_error(&dbconn));

		return -1;
	}
	MYSQL_RES * result = mysql_store_result(&dbconn);
	if (!result)
	{
		fprintf(stderr, "mysql store result:%s\n", mysql_error(&dbconn));
		return -1;
	}
	unsigned long long linenum = mysql_num_rows(result);
	if (linenum == 0)
	{
		fprintf(stderr, "No version in dns DB:%s\n", mysql_error(&dbconn));
		return -1;
	}
	MYSQL_ROW row = mysql_fetch_row(result);

	long newversion = atol(row[0]);

	if (newversion == _version)//°æ±¾Î´¸üÐÂ
	{
		mysql_free_result(result);
		return 0;
	}
	_version = newversion;

	printf("now route version is %ld\n", _version);

	mysql_free_result(result);

	return 1;
}

int DnsRoute::LoadData()
{
	tempmap.clear();
	snprintf(sql, 1024, "SELECT * FROM RouteData;");
	int ret = mysql_real_query(&dbconn,sql, strlen(sql));

	if (ret)
	{
		fprintf(stderr, "load version error:%s\n", mysql_error(&dbconn));
		return -1;
	}

	MYSQL_RES *res = mysql_store_result(&dbconn);
	if (!res)
	{
		fprintf(stderr, "mysql store result:%s\n", mysql_error(&dbconn));

		return -1;
	}
	
	unsigned long long linenum = mysql_num_rows(res);
	MYSQL_ROW row;
	for (unsigned long long i = 0; i < linenum; ++i)
	{
		row = mysql_fetch_row(res);
		int modid = atoi(row[1]);
		int cmdid = atoi(row[2]);
		int ip = atoi(row[3]);
		int port = atoi(row[4]);

		unsigned long long key = (static_cast<unsigned long long>(modid) << 32) + cmdid;
		unsigned long long value = (static_cast<unsigned long long>(ip) << 32) + port;

		tempmap[key].insert(value);
	}
	printf("load data to temp\n");

	mysql_free_result(res);

	return 0;
}

void DnsRoute::Updata()
{
	lock_guard<mutex> lock(_mutex);
	datamap.swap(tempmap);
	tempmap.clear();
}

vector<unsigned long long> DnsRoute::LoadChange()
{
	snprintf(sql, 1024, "SELECT modid,cmdid FROM RouteChange Where version <= %ld", _version);
	int ret = mysql_real_query(&dbconn, sql, strlen(sql));
	vector<unsigned long long> res;

	if (ret)
	{
		fprintf(stderr, "load change error:%s\n", mysql_error(&dbconn));
		return res;
	}
	MYSQL_RES* result = mysql_store_result(&dbconn);
	if (!result)
	{
		fprintf(stderr, "mysql store error:%s\n", mysql_error(&dbconn));
		return res;
	}
	unsigned long long linenum = mysql_num_rows(result);
	if (linenum == 0)
	{
		fprintf(stderr, "No version in table RouteChange %s\n", mysql_error(&dbconn));
		return res;
	}

	MYSQL_ROW row;
	for (unsigned long long i = 0; i < linenum; ++i)
	{
		row = mysql_fetch_row(result);
		int modid = atoi(row[0]);
		int cmdid = atoi(row[1]);
		unsigned long long key = (static_cast<unsigned long long>(modid) << 32 )+ cmdid;

		res.push_back(key);
	}

	mysql_free_result(result);
	return res;

}

void DnsRoute::removeChange(bool removeall)
{
	if (removeall)
	{
		snprintf(sql, 1024, "DELETE FROM RouteChange;");
	}
	else
	{
		snprintf(sql, 1024, "DELETE FROM RouteChange WHERE version <= %ld;", _version);
	}

	int ret = mysql_real_query(&dbconn, sql, strlen(sql));

	if (ret!=0)
	{
		fprintf(stderr, "delete RouteChange: %s\n", mysql_error(&dbconn));
		return;
	}
	return;
}

