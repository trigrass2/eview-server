#pragma once		 
#include "ace/Time_Value.h"   
#include "ace/Log_Msg.h"   
#include "ace/Synch.h"   
#include "ace/Reactor.h"   
#include "ace/Event_Handler.h"  
#include "ace/Select_Reactor.h"
#include <ace/Task.h>

#include <map>
#include <iostream>
#include <string>
#include "json/json.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "mongo/client/dbclient.h"

#include "global.h"

using namespace std;
using namespace mongo;

class CTrendTimerTask;
class CTrendTimer : public  ACE_Event_Handler
{
public:
	CTrendTimer();
	~CTrendTimer();	 

public:
	int ProcessTagTempData(vector<BSONObj> & vecBsonObj, CTagInfo *pTag);

	int ProcessTags(ACE_Time_Value tvTimer);
	int ProcessTagForTrend(vector<BSONObj> & vecBsonObjNewest, vector<BSONObj> & vecBsonObj10Minute, CTagInfo *pTag, string &strNewRecTime, string &strNewRecValue, int nNewRecQuality);

public:
	ACE_Reactor* m_pReactor;
	map<int, CTrendTimer *> m_mapInterval2TagGroupTimer;
	CTrendTimerTask * m_pTimerTask;
public:
	int m_nIntervalMS;
	bool m_bFirstTimerCome; // 是否是第一次定时器到达。如果是第一次，那么不能保存数据，因为不是整分整秒
	vector<CTagInfo*>	m_vecTags;
	vector<string>		m_vecTagName; // 要请求内存数据库数据用

public:
	virtual   int handle_timeout(const  ACE_Time_Value &current_time, const   void  *act  /* = 0 */);
	int  Start();
	int  Stop();

public: 
	int Query_Trend_Table_Common(string & strTagName, string strYYYYMMDD10M, vector<CTagRawRecord *> &vecRecOf10M);

	//int Write_1m_Rec_TO_Table_Trend(vector<BSONObj> &vecBsonObj10Minute, string &strTagName, string strYYMMDDHHMM, vector<CTagRawRecord *> &vecRecOf1Minute);
	//int Write_10m_Rec_TO_Table_Trend(vector<BSONObj> &vecBsonObj10Minute, string & strTagName, string strTableName, string strYYMMDDHH10M, vector<CTagRawRecord *> &vec10Minute);

	int RealData_TO_Table_TrendNewest(vector<BSONObj> &vecBsonObj1Newest, string &strTagname, string &strValue, string &strTimeStamp, int &nQuality);
	
	void SetTimerTask(CTrendTimerTask *pTimerTask);
	//void DeleteTempRecordMap(map<string, CTagRawRecord *> &mapRec);

	//bool MergeRawDataRecs(string strTagName, vector<CTagRawRecord *> &vecBaseRec, vector<CTagRawRecord *> &vecRecToMerge, vector<CTagRawRecord *> &vecNewRec);
	//void DeleteTempRecordVector(vector<CTagRawRecord *> &vecRec);
};
