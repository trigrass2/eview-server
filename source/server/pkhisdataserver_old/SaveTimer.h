#pragma once		 
#include "ace/Time_Value.h"   
#include "ace/Log_Msg.h"   
#include "ace/Synch.h"   
#include "ace/Reactor.h"   
#include "ace/Event_Handler.h"  
#include "global.h"
#include <iostream>
#include <ace/Task.h>
#include "tinyxml/tinyxml.h"
#include "json/json.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "ace/Select_Reactor.h"
#include <map>


using namespace mongo;
using namespace std;

class CMainTask;
class CSaveTimer : public  ACE_Event_Handler
{
public:
	CSaveTimer();
	~CSaveTimer();	 

public:
	int ProcessTagTempData(CTagInfo *pTag);
	int Query_60m_Records_From_MinuteTable(string & strTagname, string strYYMMDDHH, vector<CTagStatRecord *> &vecRecOf60M);

	int ProcessTags();
	int ProcessTag(CTagInfo *pTag, string &strNewRecTime, string &strNewRecValue, int nNewRecQuality);

public:
	ACE_Reactor* m_pReactor;
	map<int, CSaveTimer *> m_mapInterval2TagGroupTimer;

public:
	int m_nCyclePeriodMS;
	vector<CTagInfo*>	m_vecTags;
	CMainTask *m_pMainTask;

	CMemDBClientSync *m_pMemDBClient;
	mongo::DBClientConnection *m_pMongoConn;
	time_t	m_tmLastMainData;		// 上次维护系统（删除多余数据）的时间。超过一分钟进行一次维护

public:
	virtual   int handle_timeout(const  ACE_Time_Value &current_time, const   void  *act  /* = 0 */);
	int  Start();
	int  Stop();

public:
	int MaintainAllTables();

	CTagRawRecord *GetMaxTimeRecord_From_TempTable(string &strTagName);
	int Query_Raw_Record(string & strTagName, string strYYYYMMDD10M, vector<CTagRawRecord *> &vecRecOf10M);
	int Query_Stat_CompoundRecords_By_NameAndTime(string & strTagName, string strTableName, string strTime, vector<CTagStatRecord *> &vecRec);
	int Query_24h_Records_From_HourTable(string & strTagName, string strYYYYMMDD, vector<CTagStatRecord *> &vecRecOf24h);
	int Query_30d_Records_From_DayTable(string & strTagName, string strYYYYMM, vector<CTagStatRecord *> &vecRecOf30d);
	int Query_12Month_Records_From_MonthTable(string & strTagName, string strYYYY, vector<CTagStatRecord *> &vecOldRecOf12M);
	//CTagStatRecord * Query_12Month_Records_From_MonthTable(string & strTagName, string strYYYY);
	//CTagStatRecord * Query_Stat_SimpleRecord_By_NameAndTime(string & strTagName, string strTableName, string strTime);

	int Write_1m_Rec_TO_RawTable(string &strTagName, string strYYMMDDHHMM, vector<CTagRawRecord *> &vecRecOf1Minute);
	int Write_10m_Rec_TO_RawTable(string & strTagName, string strTableName, string strYYMMDDHH10M, vector<CTagRawRecord *> &vec10Minute);
	int Write_60m_records_TO_MinuteTable(string & strTagName, string strTableName, string strYYMMDDMM_HH, vector<CTagStatRecord *> &vec60Rec);
	int Write_Stat_records_TO_Table(string & strTagName, string strTableName, string strRowTime, int nTimeLenInVS, vector<CTagStatRecord *> &vecStatRec);
	int Write_24h_records_TO_HourTable(string & strTagName, string strTableName, string strYYMMDD, vector<CTagStatRecord *> &vec24hRec);
	int Write_30d_records_TO_DayTable(string & strTagName, string strTableName, string strYYYYMM, vector<CTagStatRecord *> &vecNewRecOf30d);
	//int Write_1M_records_TO_MonthTable(string & strTagName, string strTableName, string strYYYYMM, CTagStatRecord * pMonthRec);
	//int Write_Stat_OneRecord_TO_Table(string & strTagName, string strTableName, string strRowTime, CTagStatRecord * pRec);

	int RealData_TO_TempTable(string &strTagname, CTagRawRecord *pTagRecord);
	int RealData_TO_MinuteTable(string &strTagname, string strYYMMDDHH, vector<CTagRawRecord *> &vec60MinuteRec);
	
	void SetMainTask(CMainTask *pMainTask);
	void DeleteTempRecordMap(map<string, CTagRawRecord *> &mapRec);
	void DeleteStatRecordMap(map<string, CTagStatRecord *> &mapRec);

	int MinuteData_TO_HourTable(string strTagName, string strYYYYMMDDHH, vector<CTagStatRecord *> &vec60mRec);
	int HourData_TO_DayTable(string strTagName, string strYYYYMMDD, vector<CTagStatRecord *> &vec24hRec);
	int DayData_TO_MonthTable(string strTagName, string strYYYYMM, vector<CTagStatRecord *> &vec30dRec);

	CTagStatRecord * Calc1MinuteStatRecFromRealDatas(string strTagName, vector<CTagRawRecord *> &vecRawRecOf1Minute);
	CTagStatRecord * CalcStatRowFromStatDatas(string strTagName, int nResultRecTimeLen, vector<CTagStatRecord *> &vecTempData);
	void DeleteStatRecordVector(vector<CTagStatRecord *> &vecRec);
	bool MergeStatDataRecs(string strTagName, int nRecTimeLen, vector<CTagStatRecord *> &vecBaseRec, vector<CTagStatRecord *> &vecRecToMerge, vector<CTagStatRecord *> &vecNewRec);
	bool MergeStatDataRecs(string strTagName, int nRecTimeLen, vector<CTagStatRecord *> &vecBaseRec, CTagStatRecord * pRecToMerge, vector<CTagStatRecord *> &vecNewRec);
	bool MergeRawDataRecs(string strTagName, vector<CTagRawRecord *> &vecBaseRec, vector<CTagRawRecord *> &vecRecToMerge, vector<CTagRawRecord *> &vecNewRec);
	void DeleteTempRecordVector(vector<CTagRawRecord *> &vecRec);
	string SubString(string &src, int nLen);
	int Write_12M_records_TO_MonthTable(string & strTagName, string strTableName, string strYYYY, vector<CTagStatRecord *> &vecNewRecOf12M);
};
