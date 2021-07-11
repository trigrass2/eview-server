/**************************************************************
*  Filename:    CtrlProcessTask.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �����������;
*
*  @author:    xingxing;
*  @version    1.0.0
**************************************************************/
#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"

#include "math.h"
#ifdef _WIN32
#include "windows.h"  
#endif
#include "../pknodeserver/RedisFieldDefine.h"
#include "Alarm2DBTask.h"
#include "pkcomm/pkcomm.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "pklog/pklog.h"
#include "vector"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pkdbapi/pkdbapi.h"
#include "MainTask.h"

extern CPKLog g_logger;
int GetPKMemDBPort(int &nListenPort, string &strPassword);

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME		100	
#define NULLASSTRING(x) x==NULL?"":x

#define	DEFAULT_SLEEP_TIME		100	
#define SQLBUFFERSIZE			1024


CAlarm2DBTask::CAlarm2DBTask()
{
	m_bStop = false;
	m_nLastLogEvtCount = 0;
	m_nTotalEvtCount = 0;
	m_nTotalEvtFailCount = 0;	// �Ѿ���¼����־����
	time(&m_tmLastLogEvt);
}

CAlarm2DBTask::~CAlarm2DBTask()
{
	m_bStop = true;
}

/**
*  ��ʼ�����������̺߳���ط���;
*
*  @return   0/1.
*
*  @version  10/14/2016	xingxing   Initial Version
*/

int CAlarm2DBTask::svc()
{
	ACE_High_Res_Timer::global_scale_factor();

	//��ȡ��ʼ��״̬
	int nStatus = RMAPI_Init();
	if (nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��ʼ��RMAPIʧ��!");
	}

	int nRet = 0;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	while (!m_bStop)
	{
		ACE_Time_Value tmWait;
		tmWait.msec(50);
		ACE_OS::sleep(tmWait);

		// channelname:simensdrv,value:[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
		string strChannel;
		string strMessage;
		nRet = m_memDBSubSync.receivemessage(strChannel, strMessage, 500); // 500 ms
		// ����������״̬
		if (nRet != 0)
		{
			WriteEvents2DB(); // ���Բ������ݿ�
			continue;
		}

		vector<string> vecMessage;
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "��ʱ�յ���Ϣ %s", strMessage.c_str());
		// ��������ֱ�����������е���Ϣ
		while (nRet == 0)
		{
			vecMessage.push_back(strMessage);
			nRet = m_memDBSubSync.receivemessage(strChannel, strMessage, 1); // 0 ms
			//g_logger.LogMessage(PK_LOGLEVEL_INFO, "��ʱ�յ���Ϣ %s", strMessage.c_str());
		}

		ParseEventMessages(vecMessage);
		WriteEvents2DB(); // ����ִ�в������ݿ���߸��µĲ���
	}

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "nodeserver Alarm2DBTask exit normally!");
	OnStop();
	return PK_SUCCESS;
}

int CAlarm2DBTask::ParseEventMessages(vector<string> &vecMessage)
{
	for (int i = 0; i < vecMessage.size(); i++)
	{
		string &strMessage = vecMessage[i];
		
		Json::Reader reader;
		Json::Value root;
		//��ȡ�ַ�����ת����Json��ʽ;
		if (!reader.parse(strMessage.c_str(), root, false))
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKEventServer recv an msg, but is not json format, invalid! %s", strMessage.c_str());
			continue;
		}

		if (!root.isObject() && !root.isArray())
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg . type invalid:%d,not an object or array.message:%s!", root.type(), strMessage.c_str());
			continue;
		}

		this->ParseEventsMessage((char *)strMessage.c_str(), root);
	}
	return 0;
}

int CAlarm2DBTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nRet = 0;

	time_t tmNow;
	::time(&tmNow);
	m_tmLastExecute = tmNow;

	m_nWaitCommitCount = 0;

	CPKEviewDbApi PKEviewDbApi;
	//��ʼ�����ݿ�ĵ�¼;
	nRet = PKEviewDbApi.Init();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "EventProcessTask fail to init database. please check db.conf");
		PKEviewDbApi.Exit();
		return -1;
	}

	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "EventProcessTask connect to db successful");
	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	nStatus = m_memDBSubSync.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str());
	if (nStatus != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Control Process Task, Initialize RW failed, ret:%d", nStatus);
	}
	m_memDBSubSync.subscribe(CHANNELNAME_EVENT);
	//m_memDBSubSync.subscribe(CHANNELNAME_EVENT, OnChannel_EventMsg, this);

	PKEviewDbApi.Exit();
	return nRet;
}

// ������ֹͣʱ;
int CAlarm2DBTask::OnStop()
{
	if (m_vecFailSQL.size() > 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�����˳�, ������ʧ�ܵ�SQL: %d����δִ��", m_vecFailSQL.size());
		m_vecFailSQL.clear();
	}

	m_memDBSubSync.finalize();
	return 0;
}

int CAlarm2DBTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
}

void CAlarm2DBTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}

int CAlarm2DBTask::ParseEventsMessage(char *szEventsInfo, Json::Value &root)
{
	long queryid = 2;
	vector<Json::Value *> vectorCmds;
	int nRet = 0;

	//�ж��Ƿ�ΪJson����;
	if (root.isObject())
	{
		vectorCmds.push_back(&root);
	}
	else if (root.isArray())
	{
		for (unsigned int i = 0; i < root.size(); i++)
			vectorCmds.push_back(&root[i]);
	}
	else
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error��ӦΪjson array or object��{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}", szEventsInfo);
		return -12;
	}

	int nCurLogEvtCount = m_nTotalEvtCount+ vectorCmds.size(); // ��־������

	char szSQL[SQLBUFFERSIZE] = { 0 };
	//����;
	for (vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd++)
	{
		Json::Value &jsonCmd = **itCmd;
		// ִ�е�sql���;
		Json::Value jsonEventType = jsonCmd[EVENT_TYPE_MSGID];
		string strEvtType = jsonEventType.asString();
		if (PKStringHelper::StriCmp(strEvtType.c_str(), EVENT_TYPE_CONTROL) == 0) // �¼����ͣ���������
		{
			// �������Ļ��У�name��value�����ֶΣ�����û���õ�
			Json::Value jsonConent = jsonCmd["content"]; // Ԥ�������û��
			Json::Value jSubsys = jsonCmd["subsys"];
			Json::Value jUserName = jsonCmd["username"];
			Json::Value jOperTime = jsonCmd["rectime"]; // ����ʱ��
			string strMaitCat = "����";
			string strSubsys = jSubsys.asString();
			string strContent = jsonConent.asString();
			string strOperator = jUserName.asString();
			string strProducetime = jOperTime.asString();

			// t_history_event���У������ֶΣ�subcat��locationû��ʹ�ã�
			sprintf(szSQL, "insert into t_history_event(maincat,content,operator,producetime,subsysname) \
						   	values(\"%s\",\"%s\",\"%s\",\"%s\",\"%s\");",
							strMaitCat.c_str(), strContent.c_str(), strOperator.c_str(), strProducetime.c_str(), strSubsys.c_str());
			
			m_vecHistoryEvent.push_back(szSQL);
		}
		else if (PKStringHelper::StriCmp(strEvtType.c_str(), EVENT_TYPE_LINK) == 0) // �¼����ͣ���������
		{
			// �������Ļ��У�name��value�����ֶΣ�����û���õ�
			Json::Value jsonConent = jsonCmd["content"]; // Ԥ�������û��
			Json::Value jSubsys = jsonCmd["subsys"];
			Json::Value jUserName = jsonCmd["username"];
			Json::Value jOperTime = jsonCmd["rectime"]; // ����ʱ��
			string strMaitCat = "����";
			string strContent = jsonConent.asString();
			string strSubsys = jSubsys.asString();
			string strOperator = jUserName.asString();
			string strProducetime = jOperTime.asString();

			// t_history_event���У������ֶΣ�subcat��locationû��ʹ�ã�
			sprintf(szSQL, "insert into t_history_event(maincat,content,operator,producetime,subsysname) \
						   	values(\"%s\",\"%s\",\"%s\",\"%s\",\"%s\");",
							strMaitCat.c_str(), strContent.c_str(), strOperator.c_str(), strProducetime.c_str(), strSubsys.c_str());
			m_vecHistoryEvent.push_back(szSQL);
		}
		// ������Ϣ;
		else if (PKStringHelper::StriCmp(strEvtType.c_str(), EVENT_TYPE_ALARM) == 0)
		{
			int nRet = 0;

			Json::Value jObjectName = jsonCmd[ALARM_OBJECT_NAME];
			Json::Value jObjectDesc = jsonCmd[ALARM_OBJECT_DESCRIPTION];
			Json::Value jPropName = jsonCmd[ALARM_PROP_NAME];
			Json::Value jTagName = jsonCmd[ALARM_TAG_NAME];
			Json::Value jJudgeMethod = jsonCmd[ALARM_JUDGEMUTHOD];
			Json::Value jAlarmStatus = jsonCmd[ALARM_ALARMING];
			Json::Value jProduceTime = jsonCmd[ALARM_PRODUCETIME];
			Json::Value jAlarmType = jsonCmd[ALARM_TYPE];
			Json::Value jAlarmDesc = jsonCmd[ALARM_DESC];

			Json::Value jIsComfirmed = jsonCmd[ALARM_CONFIRMED];
			Json::Value jSubsysName = jsonCmd[ALARM_SYSTEMNAME];
			Json::Value jLevel = jsonCmd[ALARM_LEVEL];
			Json::Value jComfirmer = jsonCmd[ALARM_CONFIRMER];
			Json::Value jComfiremdTime = jsonCmd[ALARM_CONFIRMTIME];
			Json::Value jRecoverTime = jsonCmd[ALARM_RECOVERTIME];

			Json::Value jValOnConfirm = jsonCmd[ALARM_VALONCONFIRM];
			Json::Value jValOnProduce = jsonCmd[ALARM_VALONPRODUCE];
			Json::Value jValOnRecover = jsonCmd[ALARM_VALONRECOVER];
			Json::Value jTresholdOnProduce = jsonCmd[ALARM_THRESH];
			Json::Value jRepeatTime = jsonCmd[ALARM_REPETITIONS];
			Json::Value jParam1 = jsonCmd[ALARM_PARAM1];
			Json::Value jParam2 = jsonCmd[ALARM_PARAM2];
			Json::Value jParam3 = jsonCmd[ALARM_PARAM3];
			Json::Value jParam4 = jsonCmd[ALARM_PARAM4];

			//ת���ַ�������;
			string jtagname = jTagName.asString();
			string strObjName = jObjectName.asString();
			string strObjDesc = jObjectDesc.asString();
			string strPropName = jPropName.asString();
			string strJudgemethod = jJudgeMethod.asString();

			string strAlarmStatus = jAlarmStatus.asString();
			string strProduceTime = jProduceTime.asString();
			string strAlarmType = jAlarmType.asString();
			string strAlarmDesc = jAlarmDesc.asString();
			
			string strIsComfirmed = jIsComfirmed.asString();
			string strSubsysName = jSubsysName.asString();
			string strLevel = jLevel.asString();
			string strComfirmer = jComfirmer.asString();
			string strComfiremdTime = jComfiremdTime.asString();
			string strRecoverTime = jRecoverTime.asString();

			string strValOnConfirm = jValOnConfirm.asString();
			string strValOnProduce = jValOnProduce.asString();
			string strValOnRecover = jValOnRecover.asString();
			string strThresholdOnProduce = jTresholdOnProduce.asString();

			string strRepeattime = jRepeatTime.asString();
			int nRepCount = ::atoi(strRepeattime.c_str());
			string strParam1 = jParam1.asString();
			string strParam2 = jParam2.asString();
			string strParam3 = jParam3.asString();
			string strParam4 = jParam4.asString();

			// �����¼�������Ϊ��tagname+judgemethod+producetime������������ʱ���ڸñ��м�¼һ�������¼���
			// ��������ͬ�ı���״̬���£�ȷ�ϻ�ָ���ʱ������ݸ��������¸ü�¼��
			if (strComfiremdTime.compare("") == 0 && strRecoverTime.compare("") == 0) // �µļ�¼����Ҫ�����µļ�¼�ˣ�
			{
				if (MAIN_TASK->m_nVersion < VERSION_NODESERVER_ALARM_HAVE_OBJDESCRIPTION)
				{
					sprintf(szSQL, "insert into t_alarm_history(tagname,objectname,propname,judgemethod,producetime,alarmtype,isalarming,isconfirmed,\
								   					subsysname,priority,confirmer,confirmtime,recoverytime,valonproduce,valonrecover,valonconfirm,threshold,repetitions,\
													description, param1,param2,param3,param4) \
													values('%s','%s','%s','%s','%s','%s','%s','%s',\
													'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',\
													'%s','%s','%s','%s','%s');",
													jtagname.c_str(), strObjName.c_str(), strPropName.c_str(), strJudgemethod.c_str(), strProduceTime.c_str(), strAlarmType.c_str(), strAlarmStatus.c_str(), strIsComfirmed.c_str(),
													strSubsysName.c_str(), strLevel.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strRecoverTime.c_str(), strValOnProduce.c_str(), strValOnRecover.c_str(), strValOnConfirm.c_str(), strThresholdOnProduce.c_str(), strRepeattime.c_str(),
													strAlarmDesc.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str());
				}
				else
				{
					sprintf(szSQL, "insert into t_alarm_history(tagname,objectname,objectdescription,propname,judgemethod,producetime,alarmtype,isalarming,isconfirmed,\
								   					subsysname,priority,confirmer,confirmtime,recoverytime,valonproduce,valonrecover,valonconfirm,threshold,repetitions,\
													description, param1,param2,param3,param4) \
													values('%s','%s','%s','%s','%s','%s','%s','%s',\
													'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',\
													'%s','%s','%s','%s','%s');",
													jtagname.c_str(), strObjName.c_str(), strObjDesc.c_str(), strPropName.c_str(), strJudgemethod.c_str(), strProduceTime.c_str(), strAlarmType.c_str(), strAlarmStatus.c_str(), strIsComfirmed.c_str(),
													strSubsysName.c_str(), strLevel.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strRecoverTime.c_str(), strValOnProduce.c_str(), strValOnRecover.c_str(), strValOnConfirm.c_str(), strThresholdOnProduce.c_str(), strRepeattime.c_str(),
													strAlarmDesc.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str());
				}
			}
			else
			{
				// ����ִ�и��±���״̬����
				sprintf(szSQL, "update t_alarm_history set recoverytime='%s',valonrecover='%s',isalarming='%s', \
					isconfirmed='%s',confirmer='%s',confirmtime='%s',valonconfirm='%s',param2='%s',param2='%s',param3='%s',param4='%s' \
					where tagname='%s' and judgemethod='%s' and producetime='%s' ",
					strRecoverTime.c_str(), strValOnRecover.c_str(), strAlarmStatus.c_str(),
					strIsComfirmed.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strValOnConfirm.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str(),
					jtagname.c_str(), strJudgemethod.c_str(), strProduceTime.c_str());
			}
			m_vecAlarmSQL.push_back(szSQL);

			// ʵʱ������Ĳ���
			if (strComfiremdTime.compare("") == 0 && strRecoverTime.compare("") == 0) // �µļ�¼����Ҫ�����µļ�¼�ˣ�
			{
				if (MAIN_TASK->m_nVersion < VERSION_NODESERVER_ALARM_HAVE_OBJDESCRIPTION)
				{
					sprintf(szSQL, "insert into t_alarm_real(tagname,objectname,propname,judgemethod,producetime,alarmtype,isalarming,isconfirmed,\
							subsysname,priority,confirmer,confirmtime,recoverytime,valonproduce,valonrecover,valonconfirm,threshold,repetitions,\
							description,param1,param2,param3,param4) \
							values('%s','%s','%s','%s','%s','%s','%s','%s',\
							'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',\
							'%s','%s','%s','%s','%s');",
							jtagname.c_str(), strObjName.c_str(), strPropName.c_str(), strJudgemethod.c_str(), strProduceTime.c_str(), strAlarmType.c_str(), strAlarmStatus.c_str(), strIsComfirmed.c_str(),
							strSubsysName.c_str(), strLevel.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strRecoverTime.c_str(), strValOnProduce.c_str(), strValOnRecover.c_str(), strValOnConfirm.c_str(), strThresholdOnProduce.c_str(), strRepeattime.c_str(),
							strAlarmDesc.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str());
				}
				else
				{
					sprintf(szSQL, "insert into t_alarm_real(tagname,objectname,objectdescription,propname,judgemethod,producetime,alarmtype,isalarming,isconfirmed,\
						subsysname,priority,confirmer,confirmtime,recoverytime,valonproduce,valonrecover,valonconfirm,threshold,repetitions,\
						description,param1,param2,param3,param4) \
						values('%s','%s','%s','%s','%s','%s','%s','%s',\
						'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',\
						'%s','%s','%s','%s','%s');",
						jtagname.c_str(), strObjName.c_str(), strObjDesc.c_str(), strPropName.c_str(), strJudgemethod.c_str(), strProduceTime.c_str(), strAlarmType.c_str(), strAlarmStatus.c_str(), strIsComfirmed.c_str(),
						strSubsysName.c_str(), strLevel.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strRecoverTime.c_str(), strValOnProduce.c_str(), strValOnRecover.c_str(), strValOnConfirm.c_str(), strThresholdOnProduce.c_str(), strRepeattime.c_str(),
						strAlarmDesc.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str());
				}

			}
			else if (strComfiremdTime.compare("") != 0 && strRecoverTime.compare("") != 0) // ��ȷ���ѻָ��ı�������Ҫɾ��֮��
			{
				sprintf(szSQL, "delete from t_alarm_real where tagname='%s' and judgemethod='%s' and producetime='%s'", jtagname.c_str(), strJudgemethod.c_str(), strProduceTime.c_str());
			}
			else
			{
				// ����ִ�и���ʵʱ��ı���״̬
				sprintf(szSQL, "update t_alarm_real set recoverytime='%s',valonrecover='%s',isalarming='%s', \
							   	isconfirmed='%s',confirmer='%s',confirmtime='%s',valonconfirm='%s', param1='%s',param2='%s', param3='%s',param4='%s' \
								where tagname='%s' and judgemethod='%s' and producetime='%s' ",
																							strRecoverTime.c_str(), strValOnRecover.c_str(), strAlarmStatus.c_str(),
																							strIsComfirmed.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strValOnConfirm.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str(),
																							jtagname.c_str(), strJudgemethod.c_str(), strProduceTime.c_str());
			}
			m_vecAlarmSQL.push_back(szSQL);
		}	// else if(PKStringHelper::StriCmp(strEvtType.c_str(),"alarm") == 0) 
	}

	vectorCmds.clear();

	return nRet;
}

int CAlarm2DBTask::WriteEvents2DB()
{
	long queryid = 2;
	int nRet = 0;

	time_t tmNow;
	::time(&tmNow);
	if (labs(tmNow - m_tmLastExecute) < 1) // ÿ1�����ִ��һ�Σ��������ھͲ�����1��
		return 0;

	m_tmLastExecute = tmNow;

	int nSQLToExeute = m_vecFailSQL.size() + m_vecHistoryEvent.size() + m_vecAlarmSQL.size();
	if (nSQLToExeute <= 0) // û����־Ҫ��¼��
		return 0;

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "--------WriteEvents2DB, �� %d�����Ҫִ��, ����ʧ��:%d, �¼�:%d, ����:%d", nSQLToExeute, 
		m_vecFailSQL.size(), m_vecHistoryEvent.size(), m_vecAlarmSQL.size());

	CPKEviewDbApi PKEviewDbApi;
	PKEviewDbApi.Init();
	PKEviewDbApi.SQLTransact(); // ��ʼ����

	int nSuccessNum = 0;
	int nFailNum = 0;
	//����;
	for (vector<SQLStatus>::iterator itSQL = m_vecFailSQL.begin(); itSQL != m_vecFailSQL.end(); itSQL ++)
	{
		SQLStatus &sqlStatus = *itSQL;
		vector<SQLStatus>::iterator itCur = itSQL;
		string strError;
		int nRet = PKEviewDbApi.SQLExecute(sqlStatus.strSQL.c_str(), &strError);
		if (nRet != 0)
		{
			long lSec = labs(tmNow - sqlStatus.tmFirstExecute);
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ִ��ʧ�ܵ�SQL:��Ȼʧ��,ִ��ʱ��:%d��,ִ�д���:%d,������:%d, SQL:%s, error:%s", 
				lSec, sqlStatus.nExecuteCount, nRet, sqlStatus.strSQL.c_str(), strError.c_str());

			if (lSec) // һСʱ��û��ִ����ϣ������
			{
				m_vecFailSQL.erase(itCur);
				itSQL--;
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ִ��ʧ�ܵ�SQL:��Ȼʧ��,ִ��%d ��, ����:3600��,ִ�д���:%d, ������:%d, SQL:%s", 
					lSec, sqlStatus.nExecuteCount, nRet, sqlStatus.strSQL.c_str());
				continue;
			}

			if (m_vecFailSQL.size() > 10000) // ����10000������ɾ��ǰ���
			{
				m_vecFailSQL.erase(itCur);
				itSQL--;
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ִ��ʧ�ܵ�SQL:��Ȼʧ��,ִ��%d ��, ʧ�ܹ�:%d��,����:10000��,������:%d, SQL:%s",
					lSec, sqlStatus.nExecuteCount, nRet, sqlStatus.strSQL.c_str());
				continue;
			}

			// ����
			sqlStatus.nExecuteCount++;
			nFailNum++;
		}
		else
		{
			m_vecFailSQL.erase(itCur);
			itSQL--;
			nSuccessNum++;
			m_nWaitCommitCount++;
		}
	}

	//����;
	for (int iHistEvt = 0; iHistEvt < m_vecHistoryEvent.size(); iHistEvt++)
	{
		string &strSQL = m_vecHistoryEvent[iHistEvt];
		string strError;
		int nRet = PKEviewDbApi.SQLExecute(strSQL.c_str(), &strError);
		if (nRet != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��¼��ʷ�¼�ִ��SQLʧ��,������:%d, SQL:%s, error:%s", nRet, strSQL.c_str(), strError.c_str());
			m_nTotalEvtFailCount++;
			nFailNum++;
			m_vecFailSQL.push_back(SQLStatus(strSQL)); // ʧ��SQL�����
			continue;
		}
		else
		{
			nSuccessNum++;
			m_nWaitCommitCount++;
		}
	}
	m_vecHistoryEvent.clear();

	//��������
	for (int iHistAlarm = 0; iHistAlarm < m_vecAlarmSQL.size(); iHistAlarm++)
	{
		string &strSQL1 = m_vecAlarmSQL[iHistAlarm]; // �±�����SQL��䡣�ϱ�����Update�����ʧ�������ִ��һ��Insert
		string strError;
		int nRet = PKEviewDbApi.SQLExecute(strSQL1.c_str(), &strError);
		if (nRet != 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��¼�������ʷ����ִ��SQLʧ��,������:%d, SQL:%s, error:%s", nRet, strSQL1.c_str(), strError.c_str());
			m_nTotalEvtFailCount++;
			nFailNum++;
			m_vecFailSQL.push_back(SQLStatus(strSQL1)); // ʧ��SQL�����
		}
		else
		{
			nSuccessNum++;
			m_nWaitCommitCount++;
		}
	}
	m_vecAlarmSQL.clear();

	int nCurLogEvtAndAlarmCount = m_vecHistoryEvent.size() + m_vecAlarmSQL.size(); // �¼��ͱ�����־������
	m_nTotalEvtCount += nCurLogEvtAndAlarmCount;

	time_t tmExecuteOver;
	::time(&tmExecuteOver);
	int nTimeSpanExecute = tmExecuteOver - tmNow; // ÿ1�����ִ��һ�Σ��������ھͲ�����1��
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "-----******WriteEvents2DB,����ִ��SQL���,�ɹ�:%d, ʧ��:%d��, �����������񹲽��յ��¼�:%d��,ʧ�� %d��,ִ��ʱ�䣺%d ��",
		nSuccessNum, nFailNum, m_nTotalEvtCount, m_nTotalEvtFailCount, nTimeSpanExecute);

	if (labs(tmNow - m_tmLastLogEvt) > 30)
	{
		int nLogEventNum = m_nTotalEvtCount - m_nLastLogEvtCount;
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "���30��,���յ� %d ���¼�!�����������񹲽��յ��¼�:%d��,ʧ�� %d��",
			nCurLogEvtAndAlarmCount, m_nTotalEvtCount, m_nTotalEvtFailCount);
		m_nLastLogEvtCount = m_nTotalEvtCount;
		m_tmLastLogEvt = tmNow;
	}

	PKEviewDbApi.SQLCommit();  // �ύ����
	PKEviewDbApi.Exit();
		
	return nRet;
}
