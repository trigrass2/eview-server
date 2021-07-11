/**************************************************************
 *  Filename:    ExecutionTak.h
 *  Copyright:   Shanghai PeakInfo Co., Ltd.
 *
 *  Description: .
 *
 *  @author:     shijunpu
 *  @version     05/22/2008  shijunpu   Initial Version
**************************************************************/
 
#ifndef _MAIN_TASK_
#define _MAIN_TASK_

#define  FD_SETSIZE			1024		// the same with ACE。如果不定义winsock2.h会将该值定义为64，那么ACE在new ACE_Select_...会异常！！！

#include <ace/Task.h>
#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"
#include "ace/Thread_Mutex.h"

#include "mongo/client/dbclient.h"

#include <map>
#include <vector>

#include "json/json.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "pkdbapi/pkdbapi.h"

#include "global.h"

#define VERSION_PKHISDB_OBJECTINTERVAL	1	// 具有对象间隔
using namespace std;
using namespace mongo;

class CMainTask : public ACE_Task<ACE_MT_SYNCH>  
{
	friend class ACE_Singleton<CMainTask, ACE_Thread_Mutex>;

public:
	CMainTask();
	virtual ~CMainTask();

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
	ACE_Thread_Mutex	m_mutexMongo;
	string m_strMongoConnString;
	mongo::DBClientConnection m_mongoConn;

	vector<CTagInfo *> m_vecTags;
	time_t	m_tmLastMainData;		// 上次维护系统（删除多余数据）的时间。超过一分钟进行一次维护

	int m_nTrendIntervalSec;	// 秒为单位
	int m_nRecordIntervalSec;	// 秒为单位
	int m_nRecordSaveDay;		// 天为单位
	int m_nVersion;				// 版本
public:
	int ReadHisDatabaseUrl();

public:
	//map<string, bool>	m_mapTableName2Index;	// 是否该mongo的表已经建立了索引
	//void EnsureTableIndex(mongo::DBClientConnection & mongoConn, string strTableName);
	int CreateTableIndex(mongo::DBClientConnection & mongoConn, string &strTableName);
	int EnsureIndexOfExistTables(mongo::DBClientConnection & mongoConn);
	int MaintainAllTables();
	int InitMongdb();

public:
	// 从配置文件加载和重新加载所有的配置theTaskGroupList;
	bool m_bHaveClassAndObject;
	int LoadConfig();
	int LoadObjectPropIntervalConfig(CPKDbApi &PKDbApi, map<string, string> &mapTagName2Interval);
};
typedef ACE_Singleton<CMainTask, ACE_Thread_Mutex> CMainTaskSingleton;
#define MAIN_TASK CMainTaskSingleton::instance()

#endif // _MAIN_TASK_

