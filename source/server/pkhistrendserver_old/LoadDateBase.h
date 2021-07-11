/**************************************************************
 *  Filename:    SQLAPIWrapper.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: SQLAPIWrapper扩展类的头文件.
 *
 *  @author:     zhangqiang
 *  @version     10/26/2012  zhangqiang  Initial Version
**************************************************************/
#pragma once
#include "pkdbapi/pkdbapi.h"
using namespace std;

class CTrafficDbApi:public CPKDbApi
{
public:
	CTrafficDbApi();
	~CTrafficDbApi();

public:
	int Init();
	int Exit();
	int InitLocalSQLite();

public:
	long m_lConnectionId;
	char szConnectStr[256];
};

//CPKDbApi SQL_Exe;
extern CTrafficDbApi PKTrafficDbApi;
