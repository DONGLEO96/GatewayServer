#include "StoreReport.h"
#include<stdio.h>
#include<string.h>

StoreReport::StoreReport()
{
	mysql_library_init(0, NULL, NULL);
	//多线程初始化

	mysql_init(&dbconn);
	
	mysql_options(&dbconn, MYSQL_OPT_CONNECT_TIMEOUT, "30");
	char reconnect = 1;
	mysql_options(&dbconn, MYSQL_OPT_RECONNECT, &reconnect);


	if (!mysql_real_connect(&dbconn, "0.0.0.0", "root", "111111", "dns", 3306, NULL, 0))
	{
		fprintf(stderr, "Faied to connect mysql\n");
		exit(1);
	}

}


StoreReport::~StoreReport()
{
}

void StoreReport::store(ReportStatusReq req)
{
	for (int i = 0; i < static_cast<int>(req.results.size()); ++i)
	{
		HostCallResult& res = req.results[i];
		int overlload = res.overload;
		char sql[1024];
		snprintf(sql, 1024, "INSERT INTO ServerCallStatus (modid,cmdid,ip,port,caller,succ_cnt,err_cnt,ts,overload) VALUES (%d,%d,%u,%u,%u,%u,%u,%u,%d) ON DUPLICATE KEY UPDATE succ_cnt=%u,err_cnt=%u,ts=%u,overload=%d",
			req.modid, req.cmdid, res.ip, res.port, req.caller, res.succ, res.err, req.ts, overlload, res.succ, res.err, req.ts, overlload);
		
		printf("INSERT INTO ServerCallStatus (modid,cmdid,ip,port,caller,succ_cnt,err_cnt,ts,overload) VALUES (%d,%d,%u,%u,%u,%u,%u,%u,%d) ON DUPLICATE UPDATE succ_cnt=%u,err_cnt=%u,ts=%u,overload=%d\n",
			req.modid, req.cmdid, res.ip, res.port, req.caller, res.succ, res.err, req.ts, overlload, res.succ, res.err, req.ts, overlload);
		mysql_ping(&dbconn);

		if (mysql_real_query(&dbconn, sql, strlen(sql)) != 0)
		{
			fprintf(stderr,"Fail to Insert Into ServerCallStatus %s\n", mysql_error(&dbconn));
		}
	
	}
}
