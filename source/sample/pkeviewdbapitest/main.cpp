#include <stdlib.h>
#include <stdio.h>
#include "pkdata/pkdata.h"
#include <string>
#include <vector>
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"

using namespace std;

#ifdef WIN32
#include <windows.h>
#define strcasecmp stricmp
#else
#include <unistd.h>
#include "math.h"
#endif

void MySleep(int nSleepMS)
{
	#ifdef WIN32
	Sleep(nSleepMS);
	#else
	sleep(ceil(nSleepMS / 1000.0f));
	#endif
}

vector<string> m_vecHistoryEvent;
vector<string> m_vecAlarmSQL;
int m_nTotalEvtCount = 0;
int m_nTotalEvtFailCount = 0;
time_t m_tmLastExecute = 0;
time_t m_tmLastLogEvt = 0;
int m_nLastLogEvtCount = 0;
CPKEviewDbApi PKEviewDbApi;
CPKLog PKLog;
int WriteEvents2DB()
{


	long queryid = 2;
	int nRet = 0;

	time_t tmNow;
	::time(&tmNow);
	MySleep(50);
	//if (labs(tmNow - m_tmLastExecute) < 1) // 每1秒最多执行一次！
	//	return 0;

	m_tmLastExecute = tmNow;

	if (m_vecHistoryEvent.size() <= 0 && m_vecAlarmSQL.size() <= 0) // 没有日志要记录！
		return 0;

	int nCurLogEvtCount = m_vecHistoryEvent.size() + m_vecAlarmSQL.size(); // 日志的条数

	//遍历;
	for (int iHistEvt = 0; iHistEvt < m_vecHistoryEvent.size(); iHistEvt++)
	{
		string &strSQL = m_vecHistoryEvent[iHistEvt];
		m_nTotalEvtCount++;
		int nRet = PKEviewDbApi.SQLExecute(strSQL.c_str());
		if (nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "记录历史事件执行SQL失败,错误码:%d, SQL:%s", nRet, strSQL.c_str());
			m_nTotalEvtFailCount++;
			continue;
		}
	}
	//m_vecHistoryEvent.clear();

	PKEviewDbApi.SQLTransact();
	//遍历事件;
	for (int iHistAlarm = 0; iHistAlarm < m_vecAlarmSQL.size(); iHistAlarm++)
	{
		string &strSQL1 = m_vecAlarmSQL[iHistAlarm]; // 新报警是SQL语句。老报警是Update，如果失败则继续执行一个Insert
		m_nTotalEvtCount++;
		int nRet = PKEviewDbApi.SQLExecute(strSQL1.c_str());
		if (nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "记录或更新历史报警执行SQL失败,错误码:%d, SQL:%s", nRet, strSQL1.c_str());
			m_nTotalEvtFailCount++;
			//if (strSQL1.substr(0, 6).compare("insert") == 0) // 以插入开头
			//	continue;
		}
	}
	//m_vecAlarmSQL.clear();

	if (labs(tmNow - m_tmLastLogEvt) > 30)
	{
		int nLogEventNum = m_nTotalEvtCount - m_nLastLogEvtCount;
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "最近30秒,接收到 %d 条事件!重启启动至今共接收到事件:%d条,失败 %d条",
			nCurLogEvtCount, m_nTotalEvtCount, m_nTotalEvtFailCount);
		m_nLastLogEvtCount = m_nTotalEvtCount;
		m_tmLastLogEvt = tmNow;
	}
	PKEviewDbApi.SQLCommit();
	return nRet;
}

int main(int argc, char **argv)
{
	{
		CPKEviewDbApi PKEviewDbApi2;
		int nRet = PKEviewDbApi2.Init();
		if (nRet == 0)
			printf("连接数据库成功2!\n");
		else
			printf("连接数据库失败2:%d!\n", nRet);
	}
	{
		CPKEviewDbApi PKEviewDbApi3;
		int nRet = PKEviewDbApi3.Init();
		if (nRet == 0)
			printf("连接数据库成功3!\n");
		else
			printf("连接数据库失败3:%d!\n", nRet);
	}
	{
		CPKEviewDbApi PKEviewDbApi4;
		int nRet = PKEviewDbApi4.Init();
		if (nRet == 0)
			printf("连接数据库成功4!\n");
		else
			printf("连接数据库失败4:%d!\n", nRet);
	}

	printf("Usage:\n");
	printf("pkeviewdbapitest:read file config/db.conf and test SQLExecute!\n");
	//---------[ check the arg count ]----------------------------------------

   PKLog.SetLogFileName(PKComm::GetProcessName());

   //初始化数据库的登录;
   int nRet = PKEviewDbApi.Init();
   if (nRet != 0)
   {
	   PKLog.LogMessage(PK_LOGLEVEL_ERROR, "EventProcessTask fail to init database. please check db.conf");
	   PKEviewDbApi.Exit();
	   return -1;
   }

   PKLog.LogMessage(PK_LOGLEVEL_INFO, "EventProcessTask connect to db successful");

   char szSQL[2048] = { 0 };
   sprintf(szSQL, "create table test_history_event(maincat varchar(32),content varchar(32),operator varchar(32),producetime varchar(32),subsysname varchar(32));");
   PKEviewDbApi.SQLExecute(szSQL);
   m_vecHistoryEvent.push_back("insert into test_history_event(maincat,content,operator,producetime,subsysname) values('maincat1','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecHistoryEvent.push_back("insert into test_history_event(maincat,content,operator,producetime,subsysname) values('maincat2','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecHistoryEvent.push_back("insert into test_history_event(maincat,content,operator,producetime,subsysname) values('maincat3','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecHistoryEvent.push_back("insert into test_history_event(maincat,content,operator,producetime,subsysname) values('maincat4','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecHistoryEvent.push_back("insert into test_history_event(maincat2,content,operator,producetime,subsysname) values('maincat5','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecHistoryEvent.push_back("insert into test_history_event(maincat,content,operator,producetime,subsysname) values('maincat5','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecHistoryEvent.push_back("delete from test_history_event where maincat='maincat3';");

   sprintf(szSQL, "create table test_alarm_history(maincat varchar(32),content varchar(32),operator varchar(32),producetime varchar(32),subsysname varchar(32));");
   PKEviewDbApi.SQLExecute(szSQL);
   m_vecAlarmSQL.push_back("insert into test_alarm_history(maincat,content,operator,producetime,subsysname) values('maincat1','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecAlarmSQL.push_back("insert into test_alarm_history(maincat,content,operator,producetime,subsysname) values('maincat2','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecAlarmSQL.push_back("insert into test_alarm_history(maincat,content,operator,producetime,subsysname) values('maincat3','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecAlarmSQL.push_back("insert into test_alarm_history(maincat,content,operator,producetime,subsysname) values('maincat4','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecAlarmSQL.push_back("insert into test_alarm_history(maincat2,content,operator,producetime,subsysname) values('maincat5','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecAlarmSQL.push_back("insert into test_alarm_history(maincat,content,operator,producetime,subsysname) values('maincat5','content1','operator1','2018-04-26 00:00:00','subsys1');");
   m_vecAlarmSQL.push_back("delete from test_alarm_history where maincat='maincat3';");

   while (true)
	WriteEvents2DB();

   PKEviewDbApi.Exit();
}
