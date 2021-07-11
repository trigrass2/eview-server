/**************************************************************
 *  Filename:    ExecutionTak.h
 *  Copyright:   Shanghai PeakInfo Co., Ltd.
 *
 *  Description: .
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
**************************************************************/
 
#ifndef _RECORD_TIMER_TASK_
#define _RECORD_TIMER_TASK_

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

class CRecordTimer;
class CRecordTimerTask : public ACE_Task<ACE_MT_SYNCH>  
{
	friend class ACE_Singleton<CRecordTimerTask, ACE_Thread_Mutex>;

public:
	CRecordTimerTask();
	virtual ~CRecordTimerTask();

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
	map<int, CRecordTimer *> m_mapInterval2TagGroupTimer;

public:
	// 从配置文件加载和重新加载所有的配置theTaskGroupList;
	int GenerateTimersByTagInfo();
	int StartTimers();
	int StopTimers();

	//初始化Mongodb;
	int InitMongdb();
	int InitRedis();

};
typedef ACE_Singleton<CRecordTimerTask, ACE_Thread_Mutex> CRecordTimerTaskSingleton;
#define RECORDTIMER_TASK CRecordTimerTaskSingleton::instance()

#endif // _RECORD_TIMER_TASK_

