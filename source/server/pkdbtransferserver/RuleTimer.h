/**************************************************************
 *  Filename:    DataBlock.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 数据块信息实体类.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _RULE_TIMER_H_
#define _RULE_TIMER_H_

#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include "ace/Acceptor.h"
#include <ace/SOCK_Acceptor.h>
#include <ace/Thread_Mutex.h>
#include <ace/Guard_T.h>
#include <list>
#include <set>
#include <string>
#include <vector>

#include "pkdata/pkdata.h"
#include "DbTransferDefine.h"
using namespace std;

class CTimerTask;
class CDBTransferRule;
class CRuleTimer: public ACE_Event_Handler
{
public:
	CRuleTimer(CTimerTask *pTimerTask);
	virtual ~CRuleTimer();

	void StartTimer();
	void StopTimer();
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data
	int BuildValueWithFormat(char *szDbFieldValueExpr, int nFieldValueBuffLen, int nDataType, string strValue, int nQuality, bool bInsertComma);

public:
	CTimerTask*	m_pTimerTask;
	CDBTransferRule *m_pRule; // 对应的转储规则

	int m_nPeriodSec;
	bool m_bFirstInsert; // 第一次需要插入记录

public:
	int GetUpdateTagsSQL(char *szSQL, int nSQLBuffLen, vector<CColValue> &vecColAndValue, vector<CColInfo *> &vecColInfo, const char *szTagTime, int nQuality, char *szCurDateTime);
	int GetInsertTagsSQL(char *szSQL, int nSQLBuffLen, vector<CColValue> &vecColAndValue, vector<CColInfo *> &vecColInfo, const char *szTagTime, int nQuality, char *szCurDateTime);

	int GetUpdateObjectSQL(char *szSQL, int nSQLBuffLen, const char *szObjectName, vector<CColValue> &vecColAndValue, vector<CColInfo *> &vecColInfo, const char *szTagTime, int nQuality, char *szCurDateTime);
	int GetInsertObjectSQL(char *szSQL, int nSQLBuffLen, const char *szObjectName, vector<CColValue> &vecColAndValue, vector<CColInfo *> &vecColInfo, const char *szTagTime, int nQuality, char *szCurDateTime);
};

#endif  // _RULE_TIMER_H_
