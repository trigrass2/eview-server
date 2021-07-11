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

#define  FD_SETSIZE			1024		// the same with ACE�����������winsock2.h�Ὣ��ֵ����Ϊ64����ôACE��new ACE_Select_...���쳣������
#ifdef _WIN32
#include <winsock2.h>
#endif // _WIN32
#include "mongo/client/dbclient.h"

#include <ace/Task.h>
#include "ace/Reactor.h"
#include "ace/Select_Reactor.h"

#include "tinyxml/tinyxml.h"
#include "json/json.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "pkdriver/pkdrvcmn.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "global.h"
#include <map>
#include <vector>

using namespace std;

class CSaveTimer;
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
	//virtual int		handle_output (ACE_HANDLE fd = ACE_INVALID_HANDLE);
	int				OnStart();
	int				OnStop();
	virtual int		svc();
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data

public:
	ACE_Reactor*	m_pReactor;
	mongo::DBClientConnection m_mongoConn;
	string m_strMongoConnString;

	vector<CTagInfo *> m_vecTags;
	CMemDBClientSync m_redisRW;
	map<int, CSaveTimer *> m_mapInterval2TagGroupTimer;
public:
	// �������ļ����غ����¼������е�����theTaskGroupList;
	int LoadConfig();
	int GenerateTimersByTagInfo();
	int ReadLastUnProcessedTempData(int &nQueryCount, int &nRecordCount);
	int StartProcessTimers();

	//��ʼ��Mongodb;
	int InitMongdb();
	int InitRedis();

	CTagRawRecord * QueryMaxTimeRecord_From_TempTable(string & strTagName);
	int QueryRecords_From_TempTable(string strTagName, string strBeginTime, string strEndTime, vector<CTagRawRecord *> &vecRecords);
	int StopTimers();
	int ReadHisDatabaseUrl();

public:
	map<string, bool>	m_mapTableName2Index;	// �Ƿ��mongo�ı��Ѿ�����������
	void EnsureTableIndex(string strTableName);
	int CreateTableIndex(string &strTableName);
};
typedef ACE_Singleton<CMainTask, ACE_Thread_Mutex> CMainTaskSingleton;
#define MAIN_TASK CMainTaskSingleton::instance()

#endif // _MAIN_TASK_

