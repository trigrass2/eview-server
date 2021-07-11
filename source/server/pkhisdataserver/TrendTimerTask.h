/**************************************************************
 *  Filename:    ExecutionTak.h
 *  Copyright:   Shanghai PeakInfo Co., Ltd.
 *
 *  Description: .
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
**************************************************************/
 
#ifndef _TREND_TIMER_TASK_
#define _TREND_TIMER_TASK_

#define  FD_SETSIZE			1024		// the same with ACE。如果不定义winsock2.h会将该值定义为64，那么ACE在new ACE_Select_...会异常！！！

#ifdef _WIN32
#include <winsock2.h>
#endif // _WIN32
#include "mongo/client/dbclient.h"

#include <ace/Task.h>
#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"

#include "json/json.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "pkdriver/pkdrvcmn.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "global.h"
#include <map>
#include <vector>

using namespace std;

class CTrendTimer;
class CTrendTimerTask : public ACE_Task<ACE_MT_SYNCH>  
{
	friend class ACE_Singleton<CTrendTimerTask, ACE_Thread_Mutex>;

public:
	CTrendTimerTask();
	virtual ~CTrendTimerTask();

public:
	bool			m_bStop;
	long			StartTask();
	long			StopTask();
	
protected:
	int				OnStart();
	int				OnStop();
	virtual int		svc();

public:
	ACE_Reactor*	m_pReactor;
	mongo::DBClientConnection m_mongoConn;

	CMemDBClientSync m_redisRW;
	map<int, CTrendTimer *> m_mapInterval2TagGroupTimer;

public:
	// 从配置文件加载和重新加载所有的配置theTaskGroupList;
	int GenerateTimersByTagInfo();
	//int ReadLastUnProcessedTrendData(int &nQueryCount, int &nRecordCount);
	
	int StartTimers();
	int StopTimers();

	//初始化Mongodb;
	int InitMongdb();
	int InitRedis();

	CTagRawRecord * QueryMaxTimeRecord_From_NewestTrendTable(string & strTagName);
	int QueryRecords_From_Table_TrendNewest(string strTagName, string strBeginTime, string strEndTime, vector<CTagRawRecord *> &vecRecords);
};
typedef ACE_Singleton<CTrendTimerTask, ACE_Thread_Mutex> CTrendTimerTaskSingleton;
#define TRENDTIMER_TASK CTrendTimerTaskSingleton::instance()

#endif // _TREND_TIMER_TASK_

