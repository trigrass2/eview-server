/**************************************************************
 *  Filename:    SACommandEx.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: SACommand扩展类的头文件.
 *
 *  @author:     zhangqiang
 *  @version     10/26/2012  zhangqiang  Initial Version
**************************************************************/

#pragma once

#include "sqlapi/SQLAPI.h"
#include <map>
#include <list>
using namespace std;

class SACommandEx : public SACommand
{
public:
	SACommandEx()
	{
		m_resultSetLength = -1;
		m_isTagMode = true;
	}
	long GetResultSetLength();
	void SetResultSetLength(long resultSetLength);
	bool m_isTagMode;

private:
	long m_resultSetLength;
};
