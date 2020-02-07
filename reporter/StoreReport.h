#pragma once
#include"../util/Message.h"
#include<mysql/mysql.h>
class StoreReport
{
public:
	StoreReport();
	~StoreReport();

	void store(ReportStatusReq req);
private:
	MYSQL dbconn;
};

