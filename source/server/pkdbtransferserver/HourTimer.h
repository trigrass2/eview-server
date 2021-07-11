/**************************************************************
 *  Filename:    DataBlock.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ���ݿ���Ϣʵ����.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _HOUR_TIMER_H_
#define _HOUR_TIMER_H_

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

#define TIMELEN_YYYYMMDD_HH					13  // yyyy-MM-dd hh	2017-06-28 10
#define TIMELEN_YYYYMMDD					10  // yyyy-MM-dd		2017-06-28
#define TIMELEN_YYYYMM						7  // yyyy-MM			2017-06
#define TIMELEN_YYYY						4  // yyyy				2017

#define DB_TABLENAME_DIFF_HOUR				"t_db_diff_hour"
#define DB_TABLENAME_DIFF_DAY				"t_db_diff_day"
#define DB_TABLENAME_DIFF_WEEK				"t_db_diff_week"
#define DB_TABLENAME_DIFF_QUARTER			"t_db_diff_quarter"
#define DB_TABLENAME_DIFF_MONTH				"t_db_diff_month"
#define DB_TABLENAME_DIFF_YEAR				"t_db_diff_year"

class CTimerTask;
class CDBTransferRule;
class CHourTimer: public ACE_Event_Handler
{
public:
	CHourTimer(CTimerTask *pTimerTask);
	virtual ~CHourTimer();

	void StartTimer();
	void StopTimer();
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data

protected:
 	int InitPKData();
	PKDATA *	m_pkTagDatas;
	int			m_nTagDataNum;

protected:
	map<string, std::pair<string, double> > m_mapTagName2BeginRawHourValue;		// tag����->{YYYYDDMMHH,dbValue}
	map<string, std::pair<string, double> > m_mapTagName2BeginRawDayValue;		// tag����->{YYYYDDMM,dbValue}
	map<string, std::pair<string, double> > m_mapTagName2BeginRawMonthValue;	// tag����->{YYYYDD,dbValue}
	map<string, std::pair<string, double> > m_mapTagName2BeginRawYearValue;		// tag����->{YYYY,dbValue}
	int LoadBeginRawValueFromDB2Map(string strTableName, map<string, std::pair<string, double> > & mapTagName2RawValue); // �����ݿ��ȡСʱ���ݣ��ۼ�ֵ��
	map<string, std::pair<string, double> > * GetMapOfTimeSpan(string strTableName);

public:
	int GetBeginRawTagValue(double &dbBeginRawValue, bool &bExistInDB, string strTagName, string strBaseHourYYYYMMMMDDHH, string strTableName);
	int GetTimeFormatString(string &strFormatTimeString, string strRawTimeString, string strTableName);
	int GetStatTimeString(string &strStatTimeString, ACE_Time_Value tvCurrentTime, string strTableName);
	int BuildInsertOrUpateSQLAndExecute(string strTagName, string strDataValue, int nDataQuality, ACE_Time_Value tvCurrentTime, string strTableName);

public:
	CTimerTask*	m_pTimerTask;
	CDBTransferRule *m_pRule; // ��Ӧ��ת������
	bool m_bFirstInsert; // ��һ����Ҫ�����¼
};

#endif  // _HOUR_TIMER_H_
