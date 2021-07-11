/**************************************************************
*  Filename:    CtrlProcessTask.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 控制命令处理类;
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
#include "PKEventServer.h"
#include "pkcomm/pkcomm.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "pklog/pklog.h"
#include "vector"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pkdbapi/pkdbapi.h"
#include "MainTask.h"

extern CEventServer* g_pEventServer;

extern CPKLog PKLog;
vector<CTagInfo *> m_vecTagInfo;
vector<CAlarmAndTag *> m_vecCAlarmAndTag;
vector<CTriggerAndAlarm *> m_vecCTriggerAndAlarm;
vector<CTriggerAndAction *> m_vecCTriggerAndAction;
vector<CActionType *> m_vecCActionType;
vector<CSubSysInfo *> m_vecCSubSysInfo;
vector<CLogTypeInfo *> m_vecCLogTypeInfo;
map<int, string> g_mapAlarmID2Cname;

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME		100	
#define NULLASSTRING(x) x==NULL?"":x
#define SQLBUFFERSIZE			1024


CMainTask::CMainTask()
{
	m_bStop = false;
	m_nLastLogEvtCount = 0;
	m_nTotalEvtCount = 0;
	m_nTotalEvtFailCount = 0;	// 已经记录的日志条数
	time(&m_tmLastLogEvt);
}

CMainTask::~CMainTask()
{
	m_bStop = true;
}

/**
*  初始函数，启动线程和相关服务;
*
*  @return   0/1.
*
*  @version  10/14/2016	xingxing   Initial Version
*/

int CMainTask::svc()
{
	ACE_High_Res_Timer::global_scale_factor();

	//获取初始化状态
	int nStatus = RMAPI_Init();
	if (nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "初始化RMAPI失败!");
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
		// 更新驱动的状态
		if (nRet != 0)
		{
			WriteEvents2DB(); // 尝试插入数据库
			continue;
		}
		if (strChannel.compare(CHANNELNAME_EVENT) != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKEventServer recv an event msg.channel:%s, message%s", strChannel.c_str(), strMessage.c_str());
			WriteEvents2DB(); // 尝试插入数据库
			continue;
		}

		PKLog.LogMessage(PK_LOGLEVEL_DEBUG, "PKEventServer recv an msg:%s", strMessage.c_str());

		Json::Reader reader;
		Json::Value root;
		//读取字符串，转换成Json格式;
		if (!reader.parse(strMessage.c_str(), root, false))
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "PKEventServer recv an msg, but is not json format, invalid! %s", strMessage.c_str());
			WriteEvents2DB(); // 尝试插入数据库
			continue;
		}

		PKLog.LogMessage(PK_LOGLEVEL_DEBUG, "PKEventServer recv an event:%s", strMessage.c_str());
		if (!root.isObject() && !root.isArray())
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg . type invalid:%d,not an object or array.message:%s!", root.type(), strMessage.c_str());
			WriteEvents2DB(); // 尝试插入数据库
			continue;
		}

		this->ParseEventsMessage((char *)strMessage.c_str(), root);
		WriteEvents2DB(); // 尝试插入数据库
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "MainTask 正常退出！");
	OnStop();
	return PK_SUCCESS;
}


int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	int nRet = 0;

	CPKEviewDbApi PKEviewDbApi;
	//初始化数据库的登录;
	nRet = PKEviewDbApi.Init();
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "EventProcessTask fail to init database. please check db.conf");
		PKEviewDbApi.Exit();
		return -1;
	}

	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, "EventProcessTask connect to db successful");

	//nStatus = m_memDBSubSync.initialize(PK_LOCALHOST_IP, PKMEMDB_LISTEN_PORT, PKMEMDB_ACCESS_PASSWORD, NULL, NULL);
	nStatus = m_memDBSubSync.initialize(PK_LOCALHOST_IP, PKMEMDB_LISTEN_PORT, PKMEMDB_ACCESS_PASSWORD);
	if (nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Control Process Task, Initialize RW failed, ret:%d", nStatus);
	}
	m_memDBSubSync.subscribe(CHANNELNAME_EVENT);
	//m_memDBSubSync.subscribe(CHANNELNAME_EVENT, OnChannel_EventMsg, this);

	AutoUpdateVersion();

	PKEviewDbApi.Exit();
	return nRet;
}

// 本任务停止时;
int CMainTask::OnStop()
{
	if (m_vecFailSQL.size() > 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "程序退出, 但还有失败的SQL: %d条尚未执行", m_vecFailSQL.size());
		m_vecFailSQL.clear();
	}

	m_memDBSubSync.finalize();
	return 0;
}

int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();
}

int CMainTask::ParseEventsMessage(char *szEventsInfo, Json::Value &root)
{
	long queryid = 2;
	vector<Json::Value *> vectorCmds;
	int nRet = 0;

	//判断是否为Json对象;
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
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error，应为json array or object：{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}", szEventsInfo);
		g_pEventServer->UpdateServerStatus(-12, "control format invalid");
		return -12;
	}

	int nCurLogEvtCount = m_nTotalEvtCount+ vectorCmds.size(); // 日志的条数

	char szSQL[SQLBUFFERSIZE] = { 0 };
	//遍历;
	for (vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd++)
	{
		Json::Value &jsonCmd = **itCmd;
		// string strTmp = jsonCmd.toStyledString(); 
		// 执行的sql语句;
		Json::Value jsonEventType = jsonCmd[EVENT_TYPE_MSGID];
		string strEvtType = jsonEventType.asString();
		if (PKStringHelper::StriCmp(strEvtType.c_str(), EVENT_TYPE_CONTROL) == 0) // 事件类型：控制命令
		{
			// 传过来的还有：name，value两个字段，这里没有用到
			Json::Value jsonConent = jsonCmd["content"]; // 预定义变量没有
			Json::Value jSubsys = jsonCmd["subsys"];
			Json::Value jUserName = jsonCmd["username"];
			Json::Value jOperTime = jsonCmd["rectime"]; // 操作时间
			string strMaitCat = "控制";
			string strSubsys = jSubsys.asString();
			string strContent = jsonConent.asString();
			string strOperator = jUserName.asString();
			string strProducetime = jOperTime.asString();

			// t_history_event表中，还有字段：subcat、location没有使用！
			sprintf(szSQL, "insert into t_history_event(maincat,content,operator,producetime,subsysname) \
						   	values(\"%s\",\"%s\",\"%s\",\"%s\",\"%s\");",
							strMaitCat.c_str(), strContent.c_str(), strOperator.c_str(), strProducetime.c_str(), strSubsys.c_str());
			
			m_vecHistoryEvent.push_back(szSQL);
		}
		else if (PKStringHelper::StriCmp(strEvtType.c_str(), EVENT_TYPE_LINK) == 0) // 事件类型：控制命令
		{
			// 传过来的还有：name，value两个字段，这里没有用到
			Json::Value jsonConent = jsonCmd["content"]; // 预定义变量没有
			Json::Value jSubsys = jsonCmd["subsys"];
			Json::Value jUserName = jsonCmd["username"];
			Json::Value jOperTime = jsonCmd["rectime"]; // 操作时间
			string strMaitCat = "联动";
			string strContent = jsonConent.asString();
			string strSubsys = jSubsys.asString();
			string strOperator = jUserName.asString();
			string strProducetime = jOperTime.asString();

			// t_history_event表中，还有字段：subcat、location没有使用！
			sprintf(szSQL, "insert into t_history_event(maincat,content,operator,producetime,subsysname) \
						   	values(\"%s\",\"%s\",\"%s\",\"%s\",\"%s\");",
							strMaitCat.c_str(), strContent.c_str(), strOperator.c_str(), strProducetime.c_str(), strSubsys.c_str());
			m_vecHistoryEvent.push_back(szSQL);
		}
		// 报警信息;
		else if (PKStringHelper::StriCmp(strEvtType.c_str(), EVENT_TYPE_ALARM) == 0)
		{
			int nRet = 0;

			Json::Value jObjectName = jsonCmd[ALARM_OBJECT_NAME];
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

			//转换字符串类型;
			string jtagname = jTagName.asString();
			string strObjName = jObjectName.asString();
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


			// 报警事件的主键为：tagname+judgemethod+producetime。当报警发生时会在该表中记录一条报警事件。
			// 当主键相同的报警状态更新（确认或恢复）时，会根据该主键更新该记录。
			if (strComfiremdTime.compare("") == 0 && strRecoverTime.compare("") == 0 && nRepCount <= 1) // 新的记录，需要插入新的记录了！
			{
				sprintf(szSQL, "insert into t_alarm_history(tagname,objectname,propname,judgemethod,producetime,alarmtype,isalarming,isconfirmed,\
					subsysname,level,confirmer,confirmtime,recoverytime,valonproduce,valonrecover,valonconfirm,threshold,repetitions,\
					description, param1,param2,param3,param4) \
					values('%s','%s','%s','%s','%s','%s','%s','%s',\
					'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',\
					'%s','%s','%s','%s','%s');",
					jtagname.c_str(), strObjName.c_str(),strPropName.c_str(), strJudgemethod.c_str(), strProduceTime.c_str(), strAlarmType.c_str(), strAlarmStatus.c_str(), strIsComfirmed.c_str(), 
					strSubsysName.c_str(), strLevel.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strRecoverTime.c_str(), strValOnProduce.c_str(), strValOnRecover.c_str(), strValOnConfirm.c_str(), strThresholdOnProduce.c_str(), strRepeattime.c_str(),
					strAlarmDesc.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str());
			}
			else
			{
				// 下面执行更新报警状态操作
				sprintf(szSQL, "update t_alarm_history set recoverytime='%s',valonrecover='%s',isalarming='%s', \
					isconfirmed='%s',confirmer='%s',confirmtime='%s',valonconfirm='%s',param2='%s',param2='%s',param3='%s',param4='%s' \
					where tagname='%s' and judgemethod='%s' and producetime='%s' ",
					strRecoverTime.c_str(), strValOnRecover.c_str(), strAlarmStatus.c_str(),
					strIsComfirmed.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strValOnConfirm.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str(),
					jtagname.c_str(), strJudgemethod.c_str(), strProduceTime.c_str());
			}
			m_vecAlarmSQL.push_back(szSQL);

			// 实时报警表的插入
			if (strComfiremdTime.compare("") == 0 && strRecoverTime.compare("") == 0 && nRepCount <= 1) // 新的记录，需要插入新的记录了！
			{
				sprintf(szSQL, "insert into t_alarm_real(tagname,objectname,propname,judgemethod,producetime,alarmtype,isalarming,isconfirmed,\
							   					subsysname,level,confirmer,confirmtime,recoverytime,valonproduce,valonrecover,valonconfirm,threshold,repetitions,\
												description,param1,param2,param3,param4) \
								values('%s','%s','%s','%s','%s','%s','%s','%s',\
								'%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',\
								'%s','%s','%s','%s','%s');",
								jtagname.c_str(), strObjName.c_str(), strPropName.c_str(), strJudgemethod.c_str(), strProduceTime.c_str(),strAlarmType.c_str(), strAlarmStatus.c_str(), strIsComfirmed.c_str(),
								strSubsysName.c_str(), strLevel.c_str(), strComfirmer.c_str(), strComfiremdTime.c_str(), strRecoverTime.c_str(), strValOnProduce.c_str(), strValOnRecover.c_str(), strValOnConfirm.c_str(), strThresholdOnProduce.c_str(), strRepeattime.c_str(),
								strAlarmDesc.c_str(), strParam1.c_str(), strParam2.c_str(), strParam3.c_str(), strParam4.c_str());
			}
			else if (strComfiremdTime.compare("") != 0 && strRecoverTime.compare("") != 0) // 已确认已恢复的报警，需要删除之！
			{
				sprintf(szSQL, "delete from t_alarm_real where tagname='%s' and judgemethod='%s' and producetime='%s'", jtagname.c_str(), strJudgemethod.c_str(), strProduceTime.c_str());
			}
			else
			{
				// 下面执行更新实时表的报警状态
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

int CMainTask::WriteEvents2DB()
{
	long queryid = 2;
	int nRet = 0;

	time_t tmNow;
	::time(&tmNow);
	if (labs(tmNow - m_tmLastExecute) < 1) // 每1秒最多执行一次！这样周期就不超过1秒
		return 0;

	m_tmLastExecute = tmNow;

	if (m_vecHistoryEvent.size() <= 0 && m_vecAlarmSQL.size() <= 0) // 没有日志要记录！
		return 0;

	CPKEviewDbApi PKEviewDbApi;
	PKEviewDbApi.Init();
	PKEviewDbApi.SQLTransact(); // 开始事务

	if (m_vecFailSQL.size() > 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "尝试执行失败的SQL: %d条...", m_vecFailSQL.size());
	}

	int nSuccessNum = 0;
	int nFailNum = 0;
	//遍历;
	for (vector<SQLStatus>::iterator itSQL = m_vecFailSQL.begin(); itSQL != m_vecFailSQL.end(); itSQL ++)
	{
		SQLStatus &sqlStatus = *itSQL;
		vector<SQLStatus>::iterator itCur = itSQL;

		int nRet = PKEviewDbApi.SQLExecute(sqlStatus.strSQL.c_str());
		if (nRet != 0)
		{
			long lSec = labs(tmNow - sqlStatus.tmFirstExecute);
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "执行失败的SQL:仍然失败,执行时间:%d秒,执行次数:%d,错误码:%d, SQL:%s", 
				lSec, sqlStatus.nExecuteCount, nRet, sqlStatus.strSQL.c_str());

			if (lSec) // 一小时还没有执行完毕，则结束
			{
				m_vecFailSQL.erase(itCur);
				itSQL--;
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "执行失败的SQL:仍然失败,执行%d 秒, 超过:3600秒,执行次数:%d, 错误码:%d, SQL:%s", 
					lSec, sqlStatus.nExecuteCount, nRet, sqlStatus.strSQL.c_str());
				continue;
			}

			if (m_vecFailSQL.size() > 10000) // 超过10000条，则删除前面的
			{
				m_vecFailSQL.erase(itCur);
				itSQL--;
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "执行失败的SQL:仍然失败,执行%d 秒, 失败共:%d条,超过:10000条,错误码:%d, SQL:%s",
					lSec, sqlStatus.nExecuteCount, nRet, sqlStatus.strSQL.c_str());
				continue;
			}

			// 否则
			sqlStatus.nExecuteCount++;
			nFailNum++;
		}
		else
		{
			m_vecFailSQL.erase(itCur);
			itSQL--;
			nSuccessNum++;
		}
	}

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
			nFailNum++;
			m_vecFailSQL.push_back(SQLStatus(strSQL)); // 失败SQL语句中
			continue;
		}
		else
		{
			nSuccessNum++;
		}
	}
	m_vecHistoryEvent.clear();

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
			nFailNum++;
			m_vecFailSQL.push_back(SQLStatus(strSQL1)); // 失败SQL语句中
			//if (strSQL1.substr(0, 6).compare("insert") == 0) // 以插入开头
			//	continue;
		}
		else
		{
			nSuccessNum++;
		}
	}
	m_vecAlarmSQL.clear();

	if (labs(tmNow - m_tmLastLogEvt) > 30)
	{
		int nLogEventNum = m_nTotalEvtCount - m_nLastLogEvtCount;
		PKLog.LogMessage(PK_LOGLEVEL_INFO, "最近30秒,接收到 %d 条事件!重启启动至今共接收到事件:%d条,失败 %d条",
			nCurLogEvtCount, m_nTotalEvtCount, m_nTotalEvtFailCount);
		m_nLastLogEvtCount = m_nTotalEvtCount;
		m_tmLastLogEvt = tmNow;
	}

	PKLog.LogMessage(PK_LOGLEVEL_INFO, "本次执行SQL语句,成功:%d, 失败:%d条, 重启启动至今共接收到事件:%d条,失败 %d条",
		nSuccessNum, nFailNum, m_nTotalEvtCount, m_nTotalEvtFailCount);

	PKEviewDbApi.SQLCommit();  // 提交事务
	PKEviewDbApi.Exit();
	return nRet;
}

// 此时一定存在了该模块的版本记录
int CMainTask::AutoUpdateVersion()
{
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "本模块不支持数据库的自动升级, 如有改变请手工改变数据库");
	return 0;

	//CPKEviewDbApi PKEviewDbApi;
	//PKEviewDbApi.Init();
	//int nCurVersion = -1;
	//int nRet = GetModuleVersion("pkeventserver", &nCurVersion);
	//if (nRet)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "自动检查数据库版本,以便更新模块失败,模块:pkeventserver,返回码：%d",  nRet);
	//	PKEviewDbApi.Exit();
	//	return nRet;
	//}
	//PKLog.LogMessage(PK_LOGLEVEL_INFO, "pkeventserver 当前版本为: %d", nCurVersion);
	//if (nCurVersion < 20180104)
	//{
	//	char szSql[2048];
	//	sprintf(szSql, "CREATE TABLE t_alarm_real (tagname varchar(64) NOT NULL,tagcname varchar(64),judgemethod varchar(8) NOT NULL,\
	//		alarmname varchar(64) NOT NULL,alarmcname varchar(64),devicename varchar(64),alarmtype varchar(32) NOT NULL,isalarming integer NOT NULL,isconfirmed integer NOT NULL,\
	//		producetime varchar(24) NOT NULL,level integer NOT NULL,confirmer varchar(32),confirmtime varchar(24),recoverytime varchar(24),subsysname varchar(32),\
	//		valonproduce varchar(32) DEFAULT NULL,valonrecover varchar(32) DEFAULT NULL,valonconfirm varchar(32) DEFAULT NULL,repetitions integer,\
	//		PRIMARY KEY(tagname, judgemethod)) ");
	//	int nDBTypeId = PKEviewDbApi.GetDatabaseTypeId(); // 否则中文乱码
	//	if (nDBTypeId == PK_DATABASE_TYPEID_MYSQL)
	//		strcat(szSql, " DEFAULT CHARSET=utf8");

	//	nRet = PKEviewDbApi.SQLExecute(szSql);
	//	if (nRet != 0)
	//	{
	//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "更新模块：pkeventserver 版本为：20180104,创建表t_real_alarm失败:%s", szSql);
	//		PKEviewDbApi.Exit();
	//		return nRet;
	//	}
	//	sprintf(szSql, "CREATE TABLE t_alarm_history(tagname varchar(64) NOT NULL,tagcname varchar(64),judgemethod varchar(8) NOT NULL,\
	//				   					   			alarmname varchar(64) NOT NULL,alarmcname varchar(64),devicename varchar(64),alarmtype varchar(32) NOT NULL,isalarming integer NOT NULL,isconfirmed integer NOT NULL,\
	//																							producetime varchar(24) NOT NULL,level integer NOT NULL,confirmer varchar(32),confirmtime varchar(24),recoverytime varchar(24),subsysname varchar(32),\
	//																																					valonproduce varchar(32) DEFAULT NULL,valonrecover varchar(32) DEFAULT NULL,valonconfirm varchar(32) DEFAULT NULL,repetitions integer,\
	//																																																						PRIMARY KEY(tagname, judgemethod,producetime)) ");
	//	nDBTypeId = PKEviewDbApi.GetDatabaseTypeId(); // 否则中文乱码
	//	if (nDBTypeId == PK_DATABASE_TYPEID_MYSQL)
	//		strcat(szSql, " DEFAULT CHARSET=utf8"); 
	//	nRet = PKEviewDbApi.SQLExecute(szSql);
	//	if (nRet != 0)
	//	{
	//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "更新模块：pkeventserver 版本为：20180104,创建表t_alarm_history失败:%s", szSql);
	//		//return nRet;
	//	}

	//	int nRet = UpdateModuleVersion("pkeventserver", 20180104);
	//	if (nRet)
	//	{
	//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "自动更新模块失败,模块:pkeventserver ,新版本:20180104,返回码：%d", nRet);
	//		PKEviewDbApi.Exit();
	//		return nRet;
	//	}
	//	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "更新模块：pkeventserver 版本为：20180104 成功:增加了t_alarm_real和t_alarm_history" , szSql);
	//}

	//if (nCurVersion < 20180128)
	//{ // 在20180104和20180128间，有版本了
	//	char szSql[256], szSql2[256];
	//	sprintf(szSql, "alter table t_alarm_real add column suppresstime varchar(24),add column process varchar(256);");
	//	sprintf(szSql2, "alter table t_alarm_history add column suppresstime varchar(24),add column process varchar(256);");
	//	int nDBTypeId = PKEviewDbApi.GetDatabaseTypeId(); // 否则中文乱码

	//	nRet = PKEviewDbApi.SQLExecute(szSql);
	//	int nRet2 = PKEviewDbApi.SQLExecute(szSql2);
	//	if (nRet != 0 || nRet2 != 0)
	//	{
	//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "更新模块：pkeventserver 版本为：20180128,表t_alarm_history和t_alarm_history增加列失败:%s", szSql);
	//		//return nRet;
	//	}

	//	int nRet = UpdateModuleVersion("pkeventserver", 20180128);
	//	if (nRet)
	//	{
	//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "自动更新模块失败,模块:pkeventserver ,新版本:20180128,返回码：%d", nRet);
	//		PKEviewDbApi.Exit();
	//		return nRet;
	//	}
	//	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "更新模块：pkeventserver 版本为：20180128 成功:t_alarm_real和t_alarm_history增加了列", szSql);
	//}
	//PKEviewDbApi.Exit();
}

// 此时一定存在了该模块的版本记录
int CMainTask::UpdateModuleVersion(char *szModuleName, int nNewVersion)
{
	vector< vector<string> > vecRow;
	vector<string> vecFieldName;
	vector<int> vecFieldType;
	char szSql[2048] = { 0 };
	CPKEviewDbApi PKEviewDbApi;
	PKEviewDbApi.Init();
	sprintf(szSql, "update t_version set version=%d where module='%s'", nNewVersion, szModuleName);
	int nRet = PKEviewDbApi.SQLExecute(szSql);
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "更新模块：%s版本为：%d失败:%s", szModuleName, nNewVersion, szSql);
		PKEviewDbApi.Exit();
		return -1;
	}
	PKEviewDbApi.Exit();
	return 0;
}

int CMainTask::GetModuleVersion(char *szModuleName, int *pnVersion)
{
	*pnVersion = -1;
	vector< vector<string> > vecRow;
	vector<string> vecFieldName;
	vector<int> vecFieldType;
	CPKEviewDbApi PKEviewDbApi;
	PKEviewDbApi.Init();

	char szSql[2048] = { 0 };
	sprintf(szSql, "select version from t_version where module='%s'", szModuleName);
	int nRet = PKEviewDbApi.SQLExecute(szSql, vecRow, vecFieldName, vecFieldType);
	if (nRet != 0)
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "Query Database failed:%s!(查询数据库失败), table t_version doesn't exist, create!", szSql);
		
		// 生成表t_version
		sprintf(szSql, "create table t_version(module varchar(32), version integer,primary key(module)) DEFAULT CHARSET=utf8");
		nRet = PKEviewDbApi.SQLExecute(szSql);

		// 插入表中该模块初始版本，初始版本为0
		sprintf(szSql, "insert into t_version(module,version) values('%s',0)", szModuleName);
		nRet = PKEviewDbApi.SQLExecute(szSql);
		if (nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "执行生成t_version表和插入数据失败!%s", szSql);
			PKEviewDbApi.Exit();
			return -2;
		}
		*pnVersion = 0;
		PKEviewDbApi.Exit();
		return 0;
	}

	// 如果没有该模块的版本记录则插入
	if (vecRow.size() == 0)
	{
		sprintf(szSql, "insert into t_version(module,version) values('%s',0)", szModuleName);
		nRet = PKEviewDbApi.SQLExecute(szSql);
		if (nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "执行模块:%s 插入t_version表缺省版本失败!%s",szModuleName, szSql);
			PKEviewDbApi.Exit();
			return -2;
		}
		*pnVersion = 0;
		PKEviewDbApi.Exit();
		return 0;
	}

	*pnVersion = ::atoi(vecRow[0][0].c_str());
	PKEviewDbApi.Exit();
	return 0;
}