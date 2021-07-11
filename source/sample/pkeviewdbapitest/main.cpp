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
	//if (labs(tmNow - m_tmLastExecute) < 1) // ÿ1�����ִ��һ�Σ�
	//	return 0;

	m_tmLastExecute = tmNow;

	if (m_vecHistoryEvent.size() <= 0 && m_vecAlarmSQL.size() <= 0) // û����־Ҫ��¼��
		return 0;

	int nCurLogEvtCount = m_vecHistoryEvent.size() + m_vecAlarmSQL.size(); // ��־������

	//����;
	for (int iHistEvt = 0; iHistEvt < m_vecHistoryEvent.size(); iHistEvt++)
	{
		string &strSQL = m_vecHistoryEvent[iHistEvt];
		m_nTotalEvtCount++;
		int nRet = PKEviewDbApi.SQLExecute(strSQL.c_str());
		if (nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��¼��ʷ�¼�ִ��SQLʧ��,������:%d, SQL:%s", nRet, strSQL.c_str());
			m_nTotalEvtFailCount++;
			continue;
		}
	}
	//m_vecHistoryEvent.clear();

	PKEviewDbApi.SQLTransact();
	//�����¼�;
	for (int iHistAlarm = 0; iHistAlarm < m_vecAlarmSQL.size(); iHistAlarm++)
	{
		string &strSQL1 = m_vecAlarmSQL[iHistAlarm]; // �±�����SQL��䡣�ϱ�����Update�����ʧ�������ִ��һ��Insert
		m_nTotalEvtCount++;
		int nRet = PKEviewDbApi.SQLExecute(strSQL1.c_str());
		if (nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��¼�������ʷ����ִ��SQLʧ��,������:%d, SQL:%s", nRet, strSQL1.c_str());
			m_nTotalEvtFailCount++;
			//if (strSQL1.substr(0, 6).compare("insert") == 0) // �Բ��뿪ͷ
			//	continue;
		}
	}
	//m_vecAlarmSQL.clear();

	if (labs(tmNow - m_tmLastLogEvt) > 30)
	{
		int nLogEventNum = m_nTotalEvtCount - m_nLastLogEvtCount;
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "���30��,���յ� %d ���¼�!�����������񹲽��յ��¼�:%d��,ʧ�� %d��",
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
			printf("�������ݿ�ɹ�2!\n");
		else
			printf("�������ݿ�ʧ��2:%d!\n", nRet);
	}
	{
		CPKEviewDbApi PKEviewDbApi3;
		int nRet = PKEviewDbApi3.Init();
		if (nRet == 0)
			printf("�������ݿ�ɹ�3!\n");
		else
			printf("�������ݿ�ʧ��3:%d!\n", nRet);
	}
	{
		CPKEviewDbApi PKEviewDbApi4;
		int nRet = PKEviewDbApi4.Init();
		if (nRet == 0)
			printf("�������ݿ�ɹ�4!\n");
		else
			printf("�������ݿ�ʧ��4:%d!\n", nRet);
	}

	printf("Usage:\n");
	printf("pkeviewdbapitest:read file config/db.conf and test SQLExecute!\n");
	//---------[ check the arg count ]----------------------------------------

   PKLog.SetLogFileName(PKComm::GetProcessName());

   //��ʼ�����ݿ�ĵ�¼;
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
