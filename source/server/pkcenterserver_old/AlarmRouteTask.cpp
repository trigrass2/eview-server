/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  确认和删除报警也放在控制里面进行执行。这里也是二道贩子，将其转发给数据处理就完事
**************************************************************/

#include "common/OS.h"
#include "ace/High_Res_Timer.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "math.h"
#include "common/CodingConv.h"
#include "AlarmRouteTask.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "common/pktagdef.h"
#include "pklog/pklog.h"
#include "pkDataServer.h"
#include "CtrlRouteTask.h"

extern CDataServer* g_pServer;

#define	DEFAULT_SLEEP_TIME		100	

#define ALARM_HH_NAME					"alm_hh_name"
#define ALARM_HH_ENABLE					"alm_hh_enable"
#define ALARM_HH_THRESH					"alm_hh_thresh"
#define ALARM_HH_LEVEL					"alm_hh_level"
#define ALARM_HH_TYPE					"alm_hh_type"
#define ALARM_HH_ALARMING				"alm_hh_alarming"
#define ALARM_HH_PRODUCETIME			"alm_hh_producetime"
#define ALARM_HH_CONFIRMED				"alm_hh_confirmed"
#define ALARM_HH_CONFIRMER				"alm_hh_confirmer"
#define ALARM_HH_CONFIRMTIME			"alm_hh_confirmtime"
#define ALARM_HH_RECOVER				"alm_hh_recover"
#define ALARM_HH_RECOVERTIME			"alm_hh_recovertime"
#define ALARM_HH_REPETITIONS			"alm_hh_repetitions"
#define ALARM_HH_VALONPRODUCE			"alm_hh_valonproduce"
#define ALARM_HH_VALBEFOREPRODUCE		"alm_hh_valbeforepoduce"
#define ALARM_HH_VALAFTERPRODUCE		"alm_hh_valafterproduce"

#define ALARM_H_NAME					"alm_h_name"
#define ALARM_H_ENABLE					"alm_h_enable"
#define ALARM_H_THRESH					"alm_h_thresh"
#define ALARM_H_LEVEL					"alm_h_level"
#define ALARM_H_TYPE					"alm_h_type"
#define ALARM_H_ALARMING				"alm_h_alarming"
#define ALARM_H_PRODUCETIME				"alm_h_producetime"
#define ALARM_H_CONFIRMED				"alm_h_confirmed"
#define ALARM_H_CONFIRMER				"alm_h_confirmer"
#define ALARM_H_CONFIRMTIME				"alm_h_confirmtime"
#define ALARM_H_RECOVER					"alm_h_recover"
#define ALARM_H_RECOVERTIME				"alm_h_recovertime"
#define ALARM_H_REPETITIONS				"alm_h_repetitions"
#define ALARM_H_VALONPRODUCE			"alm_h_valonproduce"
#define ALARM_H_VALBEFOREPRODUCE		"alm_h_valbeforepoduce"
#define ALARM_H_VALAFTERPRODUCE			"alm_h_valafterproduce"

#define ALARM_L_NAME					"alm_l_name"
#define ALARM_L_ENABLE					"alm_l_enable"
#define ALARM_L_THRESH					"alm_l_thresh"
#define ALARM_L_LEVEL					"alm_l_level"
#define ALARM_L_TYPE					"alm_l_type"
#define ALARM_L_ALARMING				"alm_l_alarming"
#define ALARM_L_PRODUCETIME				"alm_l_producetime"
#define ALARM_L_CONFIRMED				"alm_l_confirmed"
#define ALARM_L_CONFIRMER				"alm_l_confirmer"
#define ALARM_L_CONFIRMTIME				"alm_l_confirmtime"
#define ALARM_L_RECOVER					"alm_l_recover"
#define ALARM_L_RECOVERTIME				"alm_l_recovertime"
#define ALARM_L_REPETITIONS				"alm_l_repetitions"
#define ALARM_L_VALONPRODUCE			"alm_l_valonproduce"
#define ALARM_L_VALBEFOREPRODUCE		"alm_l_valbeforepoduce"
#define ALARM_L_VALAFTERPRODUCE			"alm_l_valafterproduce"

#define ALARM_LL_NAME					"alm_ll_name"
#define ALARM_LL_ENABLE					"alm_ll_enable"
#define ALARM_LL_THRESH					"alm_ll_thresh"
#define ALARM_LL_LEVEL					"alm_ll_level"
#define ALARM_LL_TYPE					"alm_ll_type"
#define ALARM_LL_ALARMING				"alm_ll_alarming"
#define ALARM_LL_PRODUCETIME			"alm_ll_producetime"
#define ALARM_LL_CONFIRMED				"alm_ll_confirmed"
#define ALARM_LL_CONFIRMER				"alm_ll_confirmer"
#define ALARM_LL_CONFIRMTIME			"alm_ll_confirmtime"
#define ALARM_LL_RECOVER				"alm_ll_recover"
#define ALARM_LL_RECOVERTIME			"alm_ll_recovertime"
#define ALARM_LL_REPETITIONS			"alm_ll_repetitions"
#define ALARM_LL_VALONPRODUCE			"alm_ll_valonproduce"
#define ALARM_LL_VALBEFOREPRODUCE		"alm_ll_valbeforepoduce"
#define ALARM_LL_VALAFTERPRODUCE		"alm_ll_valafterproduce"

#define ALARM_FX1_NAME					"alm_fx1_name"
#define ALARM_FX1_ENABLE				"alm_fx1_enable"
#define ALARM_FX1_THRESH				"alm_fx1_thresh"
#define ALARM_FX1_LEVEL					"alm_fx1_level"
#define ALARM_FX1_TYPE					"alm_fx1_type"
#define ALARM_FX1_ALARMING				"alm_fx1_alarming"
#define ALARM_FX1_PRODUCETIME			"alm_fx1_producetime"
#define ALARM_FX1_CONFIRMED				"alm_fx1_confirmed"
#define ALARM_FX1_CONFIRMER				"alm_fx1_confirmer"
#define ALARM_FX1_CONFIRMTIME			"alm_fx1_confirmtime"
#define ALARM_FX1_RECOVER				"alm_fx1_recover"
#define ALARM_FX1_RECOVERTIME			"alm_fx1_recovertime"
#define ALARM_FX1_REPETITIONS			"alm_fx1_repetitions"
#define ALARM_FX1_VALONPRODUCE			"alm_fx1_valonproduce"
#define ALARM_FX1_VALBEFOREPRODUCE		"alm_fx1_valbeforepoduce"
#define ALARM_FX1_VALAFTERPRODUCE		"alm_fx1_valafterproduce"

#define ALARM_FX2_NAME					"alm_fx2_name"
#define ALARM_FX2_ENABLE				"alm_fx2_enable"
#define ALARM_FX2_THRESH				"alm_fx2_thresh"
#define ALARM_FX2_LEVEL					"alm_fx2_level"
#define ALARM_FX2_TYPE					"alm_fx2_type"
#define ALARM_FX2_ALARMING				"alm_fx2_alarming"
#define ALARM_FX2_PRODUCETIME			"alm_fx2_producetime"
#define ALARM_FX2_CONFIRMED				"alm_fx2_confirmed"
#define ALARM_FX2_CONFIRMER				"alm_fx2_confirmer"
#define ALARM_FX2_CONFIRMTIME			"alm_fx2_confirmtime"
#define ALARM_FX2_RECOVER				"alm_fx2_recover"
#define ALARM_FX2_RECOVERTIME			"alm_fx2_recovertime"
#define ALARM_FX2_REPETITIONS			"alm_fx2_repetitions"
#define ALARM_FX2_VALONPRODUCE			"alm_fx2_valonproduce"
#define ALARM_FX2_VALBEFOREPRODUCE		"alm_fx2_valbeforepoduce"
#define ALARM_FX2_VALAFTERPRODUCE		"alm_fx2_valafterproduce"			

/**
 *  构造函数.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CAlarmRouteTask::CAlarmRouteTask()
{
	m_bStop = false;
	m_bStopped = false;
}

/**
 *  析构函数.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CAlarmRouteTask::~CAlarmRouteTask()
{
	
}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CAlarmRouteTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();
	int nRet = 0;
	nRet = OnStart();

	char szTime[ICV_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	while(!m_bStop)
	{
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);		

		string strControlsChannel = CHANNELNAME_TEMP_ALARMING;
		nRet = m_redisSub.SubChannel(strControlsChannel);

		// 更新驱动的状态
		if(nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Control process task,Subscribe channle:%s failed,ret:%d", strControlsChannel.c_str(), nRet);
			g_pServer->UpdateServerStatus(-2,"subscribe control channel failed");
		}

		while(!m_bStop)
		{
			string strChannel = "";
			string strMessage;
			if(m_redisSub.RecvChannelMsg(strChannel, strMessage,100) != 0) // 未收到消息
			{
				ACE_OS::sleep(tvSleep);
				continue;
			}

			PKLog.LogMessage(PK_LOGLEVEL_INFO, "AlarmRouteTask recv an controls msg! %s",strMessage.c_str());
			Json::Reader reader;
			Json::Value root;
			if (!reader.parse(strMessage, root, false))
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "AlarmRouteTask recv an msg(%s), invalid!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"control msg format invalid");
				continue;
			}

			if (!root.isObject() && !root.isArray())
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "AlarmRouteTask recv an msg(%s), not an object or array!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"control msg format invalid");
				continue;
			}

			RouteOrSetAlarm(strMessage,root);
		}
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"AlarmRouteTask 正常退出！");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CAlarmRouteTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	nStatus = m_redisSub.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "AlarmRouteTask, Initialize m_redisSub failed, ret:%d", nStatus);
	}

	nStatus = m_redisPublish.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "AlarmRouteTask, Initialize m_redisPublish failed, ret:%d", nStatus);
	}

	nStatus = m_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);	
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "AlarmRouteTask, Initialize m_redisRW failed, ret:%d", nStatus);
	}

	nStatus = m_redisRW1.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD, 1);
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "AlarmRouteTask, Initialize m_redisRW1 failed, ret:%d", nStatus);
	}
	return nStatus;
}

// 本任务停止时
void CAlarmRouteTask::OnStop()
{
	m_redisSub.Finalize();
	m_redisRW.Finalize();
	m_redisRW1.Finalize();
	m_redisPublish.Finalize();
}

int CAlarmRouteTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CAlarmRouteTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}

// Json中数据为：[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]  {\"name\":\"tag1\",\"value\":\"100\"}
int CAlarmRouteTask::RouteOrSetAlarm(string &strCtrlCmd, Json::Value &root)
{
	int nRet = PK_SUCCESS;
	vector<Json::Value *> vectorCmds;
	if (root.isObject())
	{
		vectorCmds.push_back(&root);
	}
	else if(root.isArray())
	{
		for(unsigned int i = 0; i < root.size(); i ++)
			vectorCmds.push_back(&root[i]);
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error，应为json array or object：{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}",strCtrlCmd.c_str());
		g_pServer->UpdateServerStatus(-12,"control format invalid");
		return -12;
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "收到的报警数据格式正确，下面开始处理");
	for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++)
	{
		Json::Value &jsonCmd = **itCmd;
		// 根据地址得到设备信息和其余地址串。以冒号:隔开
		Json::Value type = jsonCmd[ALARMJSONFIELD_TYPE];
		string strtype = type.asString();
		
		string strSpace = "";
		BOOL bConfirmAlarm = false;
		// 如果是更新报警点状态
		if (strtype == ALARM_ACTION_UPDATE_TAGALARM)
		{
			Json::Value tagname = jsonCmd[ALARMJSONFIELD_TAGNAME];
			string strTagName = tagname.asString();	

			/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
			+ 此处要判断一下tag点是否存在，若果不存在打log然后跳过
			+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + */

			Json::Value tagAlarmingStatus = jsonCmd[ALARMJSONFIELD_ISALARMING];
			Json::Value tagConfirmStatus = jsonCmd[ALARMJSONFIELD_ISCONFIRMED];
			string strAlarmingStatus = tagAlarmingStatus.asString();
			string strConfirmStatus = tagConfirmStatus.asString();		
			BOOL bIsAlarming = ::atoi(strAlarmingStatus.c_str());
			BOOL bConfirmed = ::atoi(strConfirmStatus.c_str());
			
			/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			- Json::Value jsonAlmTagVal;
			- jsonAlmTagVal["name"] = jsonCmd["name"];
			- jsonAlmTagVal["v"] = jsonCmd["level"];
			- string strAlmTagVal = jsonAlmTagVal.toStyledString();
			- m_redisRW.set(strTagName.c_str(),strAlmTagVal.c_str());
			- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
			Json::Value jsonValue;
			jsonValue[JSONFIELD_NAME] = jsonCmd[JSONFIELD_NAME];
			//string strUTF = CCodingConv::GB2UTF(jsonCmd[JSONFIELD_CNAME].asString());
			jsonValue[JSONFIELD_CNAME] = jsonCmd[JSONFIELD_CNAME].asString();
			
			jsonValue[ALARMJSONFIELD_JUDGEMETHOD] = jsonCmd[ALARMJSONFIELD_JUDGEMETHOD];
			jsonValue[ALARMJSONFIELD_ISALARMING] = jsonCmd[ALARMJSONFIELD_ISALARMING];
			jsonValue[ALARMJSONFIELD_LEVEL] = jsonCmd[ALARMJSONFIELD_LEVEL];

			//strUTF = CCodingConv::GB2UTF(jsonCmd[ALARMJSONFIELD_ALARMTYPE].asString());
			jsonValue[ALARMJSONFIELD_ALARMTYPE] = jsonCmd[ALARMJSONFIELD_ALARMTYPE].asString();

			//strUTF = CCodingConv::GB2UTF(jsonCmd[JSONFIELD_SUBSYSNAME].asString());
			jsonValue[JSONFIELD_SUBSYSNAME] = jsonCmd[JSONFIELD_SUBSYSNAME].asString();

			jsonValue[ALARMJSONFIELD_ISCONFIRMED] = jsonCmd[ALARMJSONFIELD_ISCONFIRMED];
			jsonValue[ALARMJSONFIELD_VALBEFOREPRODUCE] = jsonCmd[ALARMJSONFIELD_VALBEFOREPRODUCE];
			jsonValue[ALARMJSONFIELD_VALONPRODUCE] = jsonCmd[ALARMJSONFIELD_VALONPRODUCE];
			jsonValue[ALARMJSONFIELD_VALAFTERPRODUCE] = jsonCmd[ALARMJSONFIELD_VALAFTERPRODUCE];
			jsonValue[ALARMJSONFIELD_PRODUCETIME] = jsonCmd[ALARMJSONFIELD_PRODUCETIME];
			jsonValue[ALARMJSONFIELD_REPETITIONS] = jsonCmd[ALARMJSONFIELD_REPETITIONS];

			if (bConfirmed && bIsAlarming) // 已确认，未恢复重写报警值和报警前值，恢复值为0
			{
				// 更新tag点报警属性
				UpdateTagConfirmedStatus(jsonCmd, strTagName);

				// 实时报警推送
				jsonValue[ALARM_ACTIONTYPE] = ALARM_ACTIONTYPE_CONFIRM;
				jsonValue[ALARMJSONFIELD_CONFIRMER] = jsonCmd[ALARMJSONFIELD_CONFIRMER];
				jsonValue[ALARMJSONFIELD_VALAFTERPRODUCE] = strSpace;
				jsonValue[ALARMJSONFIELD_RECOVERTIME] = strSpace;
				jsonValue[ALARMJSONFIELD_CONFIRMTIME] = jsonCmd[ALARMJSONFIELD_CONFIRMTIME];

				string strAlarmInfo = jsonValue.toStyledString();
				Json::Value jsonAlarmTagName = jsonCmd[JSONFIELD_NAME];
				m_redisRW1.set(jsonAlarmTagName.asString().c_str(), strAlarmInfo.c_str());
				bConfirmAlarm = TRUE;

				Json::Value jsonConfirmer = jsonCmd[ALARMJSONFIELD_CONFIRMER];
				string strConfirmer = jsonConfirmer.asString();
				Json::Value jsonAlarmName = jsonCmd[JSONFIELD_NAME];
				string strAlarmName = jsonAlarmName.asString();
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "%s确认报警：%s",strConfirmer.c_str(), strAlarmName.c_str());
			}
			else if (!bConfirmed && bIsAlarming) // 未确认，未恢复重写报警值和报警前值，恢复值为0
			{
				// 更新tag点报警属性
				UpdateTagProduceStatus(jsonCmd,strTagName.c_str());

				// 实时报警推送
				jsonValue[ALARM_ACTIONTYPE] = ALARM_ACTIONTYPE_PRODUCE;
				jsonValue[ALARMJSONFIELD_CONFIRMER] = strSpace;
				jsonValue[ALARMJSONFIELD_VALAFTERPRODUCE] = strSpace;
				jsonValue[ALARMJSONFIELD_RECOVERTIME] = strSpace;
				jsonValue[ALARMJSONFIELD_CONFIRMTIME] = strSpace;

				string strAlarmInfo = jsonValue.toStyledString();
				Json::Value jsonAlarmTagName = jsonCmd[JSONFIELD_NAME];
				m_redisRW1.set(jsonAlarmTagName.asString().c_str(), strAlarmInfo.c_str());

				Json::Value jsonAlarmName = jsonCmd[JSONFIELD_NAME];
				string strAlarmName = jsonAlarmName.asString();
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "产生报警：%s",strAlarmName.c_str());
			}
			else if (!bConfirmed && !bIsAlarming)  // 未确认，已恢复，写恢复时的值，报警值和报警当前值为0
			{
				// 更新tag点报警属性
				UpdateTagRecoverStatus(jsonCmd,strTagName.c_str());

				// 实时报警推送
				jsonValue[ALARM_ACTIONTYPE] = ALARM_ACTIONTYPE_RECOVER;
				jsonValue[ALARMJSONFIELD_CONFIRMER] = strSpace;
				jsonValue[ALARMJSONFIELD_VALAFTERPRODUCE] = jsonCmd[ALARMJSONFIELD_VALAFTERPRODUCE];
				jsonValue[ALARMJSONFIELD_RECOVERTIME] = jsonCmd[ALARMJSONFIELD_RECOVERTIME];
				jsonValue[ALARMJSONFIELD_CONFIRMTIME] = strSpace;

				string strAlarmInfo = jsonValue.toStyledString();
				Json::Value jsonAlarmTagName = jsonCmd[JSONFIELD_NAME];
				m_redisRW1.set(jsonAlarmTagName.asString().c_str(), strAlarmInfo.c_str());

				Json::Value jsonAlarmName = jsonCmd[JSONFIELD_NAME];
				string strAlarmName = jsonAlarmName.asString();
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "%s：报警恢复",strAlarmName.c_str());
			}
			else if(bConfirmed && !bIsAlarming) // 已确认，已恢复，写恢复时的值，报警值和报警当前值为0
			{
				// 更新tag点报警属性
				UpdateTagDeleteStatus(jsonCmd,strTagName.c_str());

				// 实时报警推送
				jsonValue[ALARM_ACTIONTYPE] = ALARM_ACTIONTYPE_DELETE;
				jsonValue[ALARMJSONFIELD_CONFIRMER] = jsonCmd[ALARMJSONFIELD_CONFIRMER];
				jsonValue[ALARMJSONFIELD_VALAFTERPRODUCE] = jsonCmd[ALARMJSONFIELD_VALAFTERPRODUCE];
				jsonValue[ALARMJSONFIELD_RECOVERTIME] = jsonCmd[ALARMJSONFIELD_RECOVERTIME];
				jsonValue[ALARMJSONFIELD_CONFIRMTIME] = jsonCmd[ALARMJSONFIELD_CONFIRMTIME];

				Json::Value jsonAlarmTagName = jsonCmd[JSONFIELD_NAME];
				m_redisRW1.del(jsonAlarmTagName.asString().c_str());

				Json::Value jsonConfirmer = jsonCmd[ALARMJSONFIELD_CONFIRMER];
				string strConfirmer = jsonConfirmer.asString();
				Json::Value jsonAlarmName = jsonCmd[JSONFIELD_NAME];
				string strAlarmName = jsonAlarmName.asString();
				Json::Value jsonConfirmTime = jsonCmd[JSONFIELD_ALM_CONFIRMTIME];
				string strConfirmTime = jsonConfirmTime.asString();
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "报警【%s】已确认已恢复,从报警列表移除，确认者：%s，确认时间：%s",strConfirmer.c_str(), strAlarmName.c_str(),strConfirmTime.c_str());
			}

			// 推送报警动作给java程序、联动、历史报警
			string strValue = jsonValue.toStyledString();
			m_redisPublish.PublishMsg(CHANNELNAME_ALARMING, strValue);
			PKLog.LogMessage(PK_LOGLEVEL_INFO, "推送报警状态信息到server-alarming通道，信息内容：%s",strValue.c_str());

			// 推送报警日志
			jsonValue["infotype"] = "alarm";
			string strLog = jsonValue.toStyledString();
			m_redisPublish.PublishMsg(CHANNELNAME_LOG,strLog);
			PKLog.LogMessage(PK_LOGLEVEL_INFO, "推送报警状态信息到log-channel通道，信息内容：%s",strValue.c_str());
// 			if (bConfirmAlarm)
// 			{
// 				jsonValue["infotype"] = "confirm-alarm";
// 				string strLog = jsonValue.toStyledString();
// 				m_redisPublish.PublishMsg(CHANNELNAME_LOG,strLog);
// 			}

			// 只有产生报警的时候转发联动
			//if (bNeedLink)
			//{
				// 转发到linkserver
				//string strValue;
				////Json::Value jsonAlm2LinkSvr;
				//jsonAlm2LinkSvr[ALARMJSONFIELD_ID] = jsonCmd[ALARMJSONFIELD_ID];
				//strValue = jsonAlm2LinkSvr.toStyledString();
			//	string strValue = jsonValue.toStyledString();
			//	m_redisPublish.PublishMsg(CHANNELNAME_LINK_TRIGGERTRIGGERED, strValue);
			//	PKLog.LogMessage(PK_LOGLEVEL_INFO,"AlarmRouteTask 转发一条报警信息到联动 triggertriggered，%s",strValue.c_str());
			//}
		}//报警点信息处理结束
		// 如果是更新对象状态，object处理开始
		else
		{
			// 推送报警日志
			Json::Value jsonValue;
			jsonValue[OBJECTJSONFIELD_ISNEW] = jsonCmd[OBJECTJSONFIELD_ISNEW];
			jsonValue[JSONFIELD_NAME] = jsonCmd[JSONFIELD_NAME];
			jsonValue[JSONFIELD_MAXALARM_LEVEL] = jsonCmd[JSONFIELD_MAXALARM_LEVEL];
			jsonValue[JSONFIELD_MINALARM_LEVEL] = jsonCmd[JSONFIELD_MINALARM_LEVEL];
			jsonValue[JSONFIELD_ALM_COUNT] = jsonCmd[JSONFIELD_ALM_COUNT];
			jsonValue[JSONFIELD_ALM_UNACKCOUNT] = jsonCmd[JSONFIELD_ALM_UNACKCOUNT];
			jsonValue[JSONFIELD_ALMINGCOUNT] = jsonCmd[JSONFIELD_ALMINGCOUNT];
			//jsonValue[JSONFIELD_ALM_FIRSTALARMTIME] = jsonCmd[JSONFIELD_ALM_FIRSTALARMTIME];
			//jsonValue[JSONFIELD_ALM_RECENTALARMTIME] = jsonCmd[JSONFIELD_ALM_RECENTALARMTIME];
			jsonValue[JSONFIELD_ALM_CONFIRMTIME] = jsonCmd[JSONFIELD_ALM_CONFIRMTIME];
			jsonValue[JSONFIELD_ALM_RECOVERTIME] = jsonCmd[JSONFIELD_ALM_RECOVERTIME];
			jsonValue[ALARMJSONFIELD_ISALARMING] = jsonCmd[ALARMJSONFIELD_ISALARMING];
			jsonValue[ALARMJSONFIELD_ISCONFIRMED] = jsonCmd[ALARMJSONFIELD_ISCONFIRMED];
			jsonValue[ALARMJSONFIELD_ISRECOVER] = jsonCmd[ALARMJSONFIELD_ISRECOVER];

			jsonValue["infotype"] = "alarmobject";
			string strLog = jsonValue.toStyledString();
			//strLog = CCodingConv::GB2UTF(strLog);
			m_redisPublish.PublishMsg(CHANNELNAME_LOG,strLog);

			// 更新内存数据库对象的报警属性
			Json::Value jsonObjectName = jsonCmd[JSONFIELD_NAME];
			Json::Value jsonMaxAlarm = jsonCmd[JSONFIELD_MAXALARM_LEVEL];
			Json::Value jsonMinAllarm = jsonCmd[JSONFIELD_MINALARM_LEVEL];
			Json::Value jsonAlarmCount = jsonCmd[JSONFIELD_ALM_COUNT];
			Json::Value jsonUnactcount = jsonCmd[JSONFIELD_ALM_UNACKCOUNT];
			Json::Value jsonUnrecovercount = jsonCmd[JSONFIELD_ALMINGCOUNT];
			//Json::Value jsonFirstAlarmtime = jsonCmd[JSONFIELD_ALM_FIRSTALARMTIME];
			//Json::Value jsonRecentAlarmtime = jsonCmd[JSONFIELD_ALM_RECENTALARMTIME];
			//Json::Value jsonConfirmTime = jsonCmd[JSONFIELD_ALM_CONFIRMTIME];
			//Json::Value jsonRecoverTime = jsonCmd[JSONFIELD_ALM_RECOVERTIME];
			//Json::Value jsonIsalarming = jsonCmd[ALARMJSONFIELD_ISALARMING];
			//Json::Value jsonIsconfirmed = jsonCmd[ALARMJSONFIELD_ISCONFIRMED];
			//Json::Value jsonIsrecover = jsonCmd[ALARMJSONFIELD_ISRECOVER]; 

			m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_MAXALARM_LEVEL, jsonMaxAlarm.asString().c_str());
			m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_MINALARM_LEVEL, jsonMinAllarm.asString().c_str());
			m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_ALM_COUNT, jsonAlarmCount.asString().c_str());
			m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_ALM_UNACKCOUNT, jsonUnactcount.asString().c_str());
			m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_ALMINGCOUNT, jsonUnrecovercount.asString().c_str());
			//m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_ALM_FIRSTALARMTIME, jsonFirstAlarmtime.asString().c_str());
			//m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_ALM_RECENTALARMTIME, jsonRecentAlarmtime.asString().c_str());
			//m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_ALM_CONFIRMTIME, jsonConfirmTime.asString().c_str());
			//m_redisRW.hset(jsonObjectName.asString().c_str(), JSONFIELD_ALM_RECOVERTIME, jsonRecoverTime.asString().c_str());
			//m_redisRW.hset(jsonObjectName.asString().c_str(), ALARMJSONFIELD_ISALARMING, jsonIsalarming.asString().c_str());
			//m_redisRW.hset(jsonObjectName.asString().c_str(), ALARMJSONFIELD_ISCONFIRMED, jsonIsconfirmed.asString().c_str());
			//m_redisRW.hset(jsonObjectName.asString().c_str(), ALARMJSONFIELD_ISRECOVER, jsonIsrecover.asString().c_str());
		}//object处理结束
	}// 一个json对象结束
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "处理完成");
	return nRet;
}

string CAlarmRouteTask::GetCurrentAlarmActionExecute(string name)
{
	string strValue = "";
	m_redisRW.get(name.c_str(),strValue);
	Json::Reader reader;
	Json::Value root;
	if (!reader.parse(strValue, root, false))
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "AlarmRouteTask recv an msg(%s), invalid!",strValue.c_str());
		g_pServer->UpdateServerStatus(-2,"control msg format invalid");
		return strValue;
	}

	Json::Value valExecute = root["execite"];
	string strExecute = valExecute.asString();
	return strExecute;
}

/**
* 当收到的报警动作为确认报警时，更新tag点的报警状态
*
* @param    -[in]   Json::Value &jsonCmd : [包含所有报警状态信息的json对象]
* @param    -[in]   string strTagName : [要更新的tag点名]
*
* @version  11/21/2016	liucaixia   Initial Version
**/
void CAlarmRouteTask::UpdateTagConfirmedStatus(Json::Value &jsonCmd,string strTagName)
{
	Json::Value jsonAlarmcount = jsonCmd[JSONFIELD_ALM_COUNT];
	Json::Value jsonAlarmingcount = jsonCmd[JSONFIELD_ALMINGCOUNT];
	Json::Value jsonAlmunackcount = jsonCmd[JSONFIELD_ALM_UNACKCOUNT];
	Json::Value jsonAlarmlevel = jsonCmd[JSONFIELD_MAXALARM_LEVEL];
	Json::Value jsonTagenablealarm = jsonCmd[TAGJSONFIELD_ENABLEALARM];
	//Json::Value jsonRecoverTime = jsonCmd[JSONFIELD_ALM_RECOVERTIME];
	//Json::Value jsonTagConfirmTime = jsonCmd[JSONFIELD_ALM_RECENTALARMTIME];

	Json::Value jsonJudgemethod = jsonCmd[ALARMJSONFIELD_JUDGEMETHOD];
	Json::Value jsonIsconfirmed = jsonCmd[ALARMJSONFIELD_ISCONFIRMED];
	Json::Value jsonConfirmer = jsonCmd[ALARMJSONFIELD_CONFIRMER];
	Json::Value jsonConfirmtime = jsonCmd[ALARMJSONFIELD_CONFIRMTIME];

	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_COUNT, jsonAlarmcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALMINGCOUNT, jsonAlarmingcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_UNACKCOUNT, jsonAlmunackcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_MAXALARM_LEVEL, jsonAlarmlevel.asString().c_str());///////////
	m_redisRW.hset(strTagName.c_str(), TAGJSONFIELD_ENABLEALARM, jsonTagenablealarm.asString().c_str());
	//m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_RECOVERTIME, jsonRecoverTime.asString().c_str());
	//m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_FIRSTALARMTIME, "-1");/////
	//m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_RECENTALARMTIME, "-1");//////

	if (jsonJudgemethod.asString() == ALARM_HH)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_CONFIRMED, jsonIsconfirmed.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_HH_CONFIRMER, jsonConfirmer.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_HH_CONFIRMTIME, jsonConfirmtime.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_H)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_H_CONFIRMED, jsonIsconfirmed.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_H_CONFIRMER, jsonConfirmer.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_H_CONFIRMTIME, jsonConfirmtime.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_L)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_L_CONFIRMED, jsonIsconfirmed.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_L_CONFIRMER, jsonConfirmer.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_L_CONFIRMTIME, jsonConfirmtime.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_LL)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_CONFIRMED, jsonIsconfirmed.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_LL_CONFIRMER, jsonConfirmer.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_LL_CONFIRMTIME, jsonConfirmtime.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_FX1)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_CONFIRMED, jsonIsconfirmed.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX1_CONFIRMER, jsonConfirmer.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX1_CONFIRMTIME, jsonConfirmtime.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_FX2)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_CONFIRMED, jsonIsconfirmed.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX2_CONFIRMER, jsonConfirmer.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX2_CONFIRMTIME, jsonConfirmtime.asString().c_str());
	}else
	{
		/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
		+ 不属于任何一种类型，打log
		+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + */
	}
}

/**
* 当收到的报警动作为报警恢复时，更新tag点的报警状态
*
* @param    -[in]   Json::Value &jsonCmd : [包含所有报警状态信息的json对象]
* @param    -[in]   string strTagName : [要更新的tag点名]
*
* @version  11/21/2016	liucaixia   Initial Version
**/
void CAlarmRouteTask::UpdateTagRecoverStatus(Json::Value &jsonCmd,string strTagName)
{
	Json::Value jsonAlarmcount = jsonCmd[JSONFIELD_ALM_COUNT];
	Json::Value jsonAlarmingcount = jsonCmd[JSONFIELD_ALMINGCOUNT];
	Json::Value jsonAlmunackcount = jsonCmd[JSONFIELD_ALM_UNACKCOUNT];
	Json::Value jsonAlarmlevel = jsonCmd[JSONFIELD_MAXALARM_LEVEL];
	Json::Value jsonTagRecoverTime = jsonCmd[JSONFIELD_ALM_RECOVERTIME];

	Json::Value jsonJudgemethod = jsonCmd[ALARMJSONFIELD_JUDGEMETHOD];
	Json::Value jsonIsalarming = jsonCmd[ALARMJSONFIELD_ISALARMING];
	Json::Value jsonRecovertime = jsonCmd[ALARMJSONFIELD_RECOVERTIME];
	Json::Value jsonValAfterproduce = jsonCmd[ALARMJSONFIELD_VALAFTERPRODUCE];

	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_COUNT, jsonAlarmcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALMINGCOUNT, jsonAlarmingcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_UNACKCOUNT, jsonAlmunackcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_MAXALARM_LEVEL, jsonAlarmlevel.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_RECOVERTIME, jsonTagRecoverTime.asString().c_str());

	if (jsonJudgemethod.asString() == ALARM_HH)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_ALARMING, jsonIsalarming.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_RECOVERTIME, jsonRecovertime.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_VALAFTERPRODUCE, jsonValAfterproduce.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_H)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_H_ALARMING, jsonIsalarming.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_H_RECOVERTIME, jsonRecovertime.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_H_VALAFTERPRODUCE, jsonValAfterproduce.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_L)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_L_ALARMING, jsonIsalarming.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_L_RECOVERTIME, jsonRecovertime.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_L_VALAFTERPRODUCE, jsonValAfterproduce.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_LL)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_ALARMING, jsonIsalarming.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_RECOVERTIME, jsonRecovertime.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_VALAFTERPRODUCE, jsonValAfterproduce.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_FX1)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_ALARMING, jsonIsalarming.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_RECOVERTIME, jsonRecovertime.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_VALAFTERPRODUCE, jsonValAfterproduce.asString().c_str());
	}else if (jsonJudgemethod.asString() == ALARM_FX2)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_ALARMING, jsonIsalarming.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_RECOVERTIME, jsonRecovertime.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_VALAFTERPRODUCE, jsonValAfterproduce.asString().c_str());
	}else
	{
		/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
		+ 不属于任何一种类型，打log
		+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + */
	}
}

void CAlarmRouteTask::UpdateTagProduceStatus(Json::Value &jsonCmd,string strTagName)
{
	Json::Value jsonAlarmcount = jsonCmd[JSONFIELD_ALM_COUNT];
	Json::Value jsonAlarmingcount = jsonCmd[JSONFIELD_ALMINGCOUNT];
	Json::Value jsonAlmunackcount = jsonCmd[JSONFIELD_ALM_UNACKCOUNT];
	Json::Value jsonAlarmlevel = jsonCmd[JSONFIELD_MAXALARM_LEVEL];
	Json::Value jsonTagenablealarm = jsonCmd[TAGJSONFIELD_ENABLEALARM];
	//Json::Value jsonFirstAlarmTime = jsonCmd[JSONFIELD_ALM_FIRSTALARMTIME];
	//Json::Value jsonRecentAlarmTime = jsonCmd[JSONFIELD_ALM_RECENTALARMTIME];

	Json::Value jsonJudgemethod = jsonCmd[ALARMJSONFIELD_JUDGEMETHOD];
	Json::Value jsonCname = jsonCmd[JSONFIELD_CNAME];
	Json::Value jsonEnablealarm = jsonCmd[ALARMJSONFIELD_ENABLEALARM];
	Json::Value jsonLevel = jsonCmd[ALARMJSONFIELD_LEVEL];
	Json::Value jsonAlarmtype = jsonCmd[ALARMJSONFIELD_ALARMTYPE];
	Json::Value jsonProducetime = jsonCmd[ALARMJSONFIELD_PRODUCETIME];
	Json::Value jsonThresh = jsonCmd[ALARMJSONFIELD_THRESH];
	string strThresh = jsonThresh.asString();
	Json::Value jsonValBeforeProduce = jsonCmd[ALARMJSONFIELD_VALBEFOREPRODUCE];
	string strValBrforeProduce = jsonValBeforeProduce.asString();
	Json::Value jsonValOnProduce = jsonCmd[ALARMJSONFIELD_VALONPRODUCE];
	Json::Value jsonIsalarming = jsonCmd[ALARMJSONFIELD_ISALARMING];

	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_COUNT, jsonAlarmcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALMINGCOUNT, jsonAlarmingcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_UNACKCOUNT, jsonAlmunackcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_MAXALARM_LEVEL, jsonAlarmlevel.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), TAGJSONFIELD_ENABLEALARM, jsonTagenablealarm.asString().c_str());
	//m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_FIRSTALARMTIME, jsonFirstAlarmTime.asString().c_str());
	//m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_RECENTALARMTIME, jsonRecentAlarmTime.asString().c_str());

	if (jsonJudgemethod.asString() == ALARM_HH)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_NAME, jsonCname.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_ENABLE, jsonEnablealarm.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_THRESH, jsonThresh.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_LEVEL, jsonLevel.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_TYPE, jsonAlarmtype.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_HH_ALARMING, jsonIsalarming.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_HH_PRODUCETIME, jsonProducetime.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_HH_VALBEFOREPRODUCE, jsonValBeforeProduce.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_HH_VALONPRODUCE, jsonValOnProduce.asString().c_str());
	}
	else if (jsonJudgemethod.asString() == ALARM_H)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_H_NAME, jsonCname.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_H_ENABLE, jsonEnablealarm.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_H_THRESH, jsonThresh.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_H_LEVEL, jsonLevel.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_H_TYPE, jsonAlarmtype.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_H_ALARMING, jsonIsalarming.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_H_PRODUCETIME, jsonProducetime.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_H_VALBEFOREPRODUCE, jsonValBeforeProduce.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_H_VALONPRODUCE, jsonValOnProduce.asString().c_str());
	}
	else if (jsonJudgemethod.asString() == ALARM_L)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_L_NAME, jsonCname.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_L_ENABLE, jsonEnablealarm.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_L_THRESH, jsonThresh.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_L_LEVEL, jsonLevel.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_L_TYPE, jsonAlarmtype.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_L_ALARMING, jsonIsalarming.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_L_PRODUCETIME, jsonProducetime.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_L_VALBEFOREPRODUCE, jsonValBeforeProduce.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_L_VALONPRODUCE, jsonValOnProduce.asString().c_str());
	}
	else if (jsonJudgemethod.asString() == ALARM_LL)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_NAME, jsonCname.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_ENABLE, jsonEnablealarm.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_THRESH, jsonThresh.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_LEVEL, jsonLevel.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_TYPE, jsonAlarmtype.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_LL_ALARMING, jsonIsalarming.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_LL_PRODUCETIME, jsonProducetime.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_LL_VALBEFOREPRODUCE, jsonValBeforeProduce.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_LL_VALONPRODUCE, jsonValOnProduce.asString().c_str());
	}
	else if (jsonJudgemethod.asString() == ALARM_FX1)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_NAME, jsonCname.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_ENABLE, jsonEnablealarm.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_THRESH, jsonThresh.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_LEVEL, jsonLevel.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_TYPE, jsonAlarmtype.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX1_ALARMING, jsonIsalarming.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX1_PRODUCETIME, jsonProducetime.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX1_VALBEFOREPRODUCE, jsonValBeforeProduce.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX1_VALONPRODUCE, jsonValOnProduce.asString().c_str());
	}
	else if (jsonJudgemethod.asString() == ALARM_FX2)
	{
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_NAME, jsonCname.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_ENABLE, jsonEnablealarm.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_THRESH, jsonThresh.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_LEVEL, jsonLevel.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_TYPE, jsonAlarmtype.asString().c_str());
		m_redisRW.hset(strTagName.c_str(), ALARM_FX2_ALARMING, jsonIsalarming.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX2_PRODUCETIME, jsonProducetime.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX2_VALBEFOREPRODUCE, jsonValBeforeProduce.asString().c_str());
		//m_redisRW.hset(strTagName.c_str(), ALARM_FX2_VALONPRODUCE, jsonValOnProduce.asString().c_str());
	}else
	{
		/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
		+ 不属于任何一种类型，打log
		+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + */
	}
}

void CAlarmRouteTask::UpdateTagDeleteStatus(Json::Value &jsonCmd,string strTagName)
{
	Json::Value jsonAlarmcount = jsonCmd[JSONFIELD_ALM_COUNT];
	Json::Value jsonAlarmingcount = jsonCmd[JSONFIELD_ALMINGCOUNT];
	Json::Value jsonAlmunackcount = jsonCmd[JSONFIELD_ALM_UNACKCOUNT];
	Json::Value jsonAlarmlevel = jsonCmd[JSONFIELD_MAXALARM_LEVEL];
	Json::Value jsonRecoverTime = jsonCmd[JSONFIELD_ALM_RECOVERTIME];

	Json::Value jsonJudgemethod = jsonCmd[ALARMJSONFIELD_JUDGEMETHOD];
	string strSpace = "--";

	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_COUNT, jsonAlarmcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALMINGCOUNT, jsonAlarmingcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_ALM_UNACKCOUNT, jsonAlmunackcount.asString().c_str());
	m_redisRW.hset(strTagName.c_str(), JSONFIELD_MAXALARM_LEVEL, jsonAlarmlevel.asString().c_str());

	if (jsonJudgemethod.asString() == ALARM_HH)
	{
		m_redisRW.hdel(strTagName.c_str(), ALARM_HH_NAME);
		m_redisRW.hdel(strTagName.c_str(), ALARM_HH_ENABLE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_HH_ALARMING);
		m_redisRW.hdel(strTagName.c_str(), ALARM_HH_LEVEL);
		m_redisRW.hdel(strTagName.c_str(), ALARM_HH_THRESH);
		m_redisRW.hdel(strTagName.c_str(), ALARM_HH_TYPE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_HH_REPETITIONS);
		m_redisRW.hdel(strTagName.c_str(), ALARM_HH_CONFIRMED);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_HH_CONFIRMER);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_HH_CONFIRMTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_HH_RECOVERTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_HH_PRODUCETIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_HH_VALAFTERPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_HH_VALBEFOREPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_HH_VALONPRODUCE);
	}
	else if (jsonJudgemethod.asString() == ALARM_H)
	{
		m_redisRW.hdel(strTagName.c_str(), ALARM_H_NAME);
		m_redisRW.hdel(strTagName.c_str(), ALARM_H_ENABLE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_H_ALARMING);
		m_redisRW.hdel(strTagName.c_str(), ALARM_H_LEVEL);
		m_redisRW.hdel(strTagName.c_str(), ALARM_H_THRESH);
		m_redisRW.hdel(strTagName.c_str(), ALARM_H_TYPE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_H_REPETITIONS);
		m_redisRW.hdel(strTagName.c_str(), ALARM_H_CONFIRMED);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_H_CONFIRMER);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_H_CONFIRMTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_H_RECOVERTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_H_PRODUCETIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_H_VALAFTERPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_H_VALBEFOREPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_H_VALONPRODUCE);
	}
	else if (jsonJudgemethod.asString() == ALARM_L)
	{
		m_redisRW.hdel(strTagName.c_str(), ALARM_L_NAME);
		m_redisRW.hdel(strTagName.c_str(), ALARM_L_ENABLE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_L_ALARMING);
		m_redisRW.hdel(strTagName.c_str(), ALARM_L_LEVEL);
		m_redisRW.hdel(strTagName.c_str(), ALARM_L_THRESH);
		m_redisRW.hdel(strTagName.c_str(), ALARM_L_TYPE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_L_REPETITIONS);
		m_redisRW.hdel(strTagName.c_str(), ALARM_L_CONFIRMED);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_L_CONFIRMER);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_L_CONFIRMTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_L_RECOVERTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_L_PRODUCETIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_L_VALAFTERPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_L_VALBEFOREPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_L_VALONPRODUCE);
	}
	else if (jsonJudgemethod.asString() == ALARM_LL)
	{
		m_redisRW.hdel(strTagName.c_str(), ALARM_LL_NAME);
		m_redisRW.hdel(strTagName.c_str(), ALARM_LL_ENABLE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_LL_ALARMING);
		m_redisRW.hdel(strTagName.c_str(), ALARM_LL_LEVEL);
		m_redisRW.hdel(strTagName.c_str(), ALARM_LL_THRESH);
		m_redisRW.hdel(strTagName.c_str(), ALARM_LL_TYPE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_LL_REPETITIONS);
		m_redisRW.hdel(strTagName.c_str(), ALARM_LL_CONFIRMED);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_LL_CONFIRMER);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_LL_CONFIRMTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_LL_RECOVERTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_LL_PRODUCETIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_LL_VALAFTERPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_LL_VALBEFOREPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_LL_VALONPRODUCE);
	}
	else if (jsonJudgemethod.asString() == ALARM_FX1)
	{
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_NAME);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_ENABLE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_ALARMING);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_LEVEL);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_THRESH);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_TYPE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_REPETITIONS);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_CONFIRMED);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_CONFIRMER);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_CONFIRMTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_RECOVERTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_PRODUCETIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_VALAFTERPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_VALBEFOREPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX1_VALONPRODUCE);
	}
	else if (jsonJudgemethod.asString() == ALARM_FX2)
	{
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_NAME);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_ENABLE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_ALARMING);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_LEVEL);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_THRESH);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_TYPE);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_REPETITIONS);
		m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_CONFIRMED);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_CONFIRMER);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_CONFIRMTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_RECOVERTIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_PRODUCETIME);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_VALAFTERPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_VALBEFOREPRODUCE);
		//m_redisRW.hdel(strTagName.c_str(), ALARM_FX2_VALONPRODUCE);
	}
	else
	{
		/* + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
		+ 不属于任何一种类型，打log
		+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + */
	}
}