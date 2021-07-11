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

class CPKEviewDbApi:public CPKDbApi
{
public:
	CPKEviewDbApi();
	virtual ~CPKEviewDbApi();

public:
	int Init();
	int Exit();

protected:
	int InitLocalSQLite(char *szCodingSet);

public:
	long m_lConnectionId;
};

