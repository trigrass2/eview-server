/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 控制命令处理类.
 *
 *  @author:    lijingjing
 *  @version    05/28/2008  lijingjing  Initial Version
 *  @version	9/17/2012  baoyuansong  数据块类型修改为字符串类型，并且修改增加数据块时的接口
 *  @version	3/1/2013   baoyuansong  增加由点生成块的逻辑，当设备下没有配置数据块时，直接由点生成数据块 .
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "ace/High_Res_Timer.h"
#include "math.h"

#include "MainTask.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "common/CodingConv.h"
#include "pkDataServer.h"
#include "CtrlRouteTask.h"
#include "RTDataRouteTask.h"
#include "AlarmRouteTask.h"

extern CDataServer* g_pServer;

#define  CHECK_NULL_RETURN_ERR(X)  { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }
#define	DEFAULT_SLEEP_TIME		100	
#define NULLASSTRING(x) x==NULL?"":x
/**
 *  构造函数.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CMainTask::CMainTask()
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
CMainTask::~CMainTask()
{

}

/**
 *  虚函数，继承自ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CMainTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = OnStart();
	while(!m_bStop)
	{
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);

		string strControlsChannel = CHANNELNAME_INIT;
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

			PKLog.LogMessage(PK_LOGLEVEL_INFO, "MainTask recv an Init msg!%s",strMessage.c_str());
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

			ParseAndSetInitData2MenDB(strMessage,root);
		}
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"PbServer 正常退出！");
	OnStop();

	m_bStopped = true;
	return nRet;	
}

int CMainTask::OnStart()
{
	int nStatus = PK_SUCCESS;
// 	nStatus = RMAPI_Init();
// 	if (nStatus != PK_SUCCESS)
// 	{
// 		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"初始化RMAPI失败!");
// 	}

	nStatus = m_redisSub.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "MainTask, Initialize m_redisSub failed, ret:%d", nStatus);
	}

	nStatus = m_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "MainTask, Initialize m_redisRW failed, ret:%d", nStatus);
	}

	CTRLROUTE_TASK->Start();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "CTRLROUTE_TASK start");
	RTDATAROUTE_TASK->Start();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "RTDATAROUTE_TASK start");
	ALARMROUTE_TASK->Start();
	PKLog.LogMessage(PK_LOGLEVEL_INFO, "ALARMROUTE_TASK start");
	return PK_SUCCESS;
}

// 本任务停止时
void CMainTask::OnStop()
{
	m_redisSub.Finalize();
	m_redisRW.Finalize();
	// 停止其他线程
	CTRLROUTE_TASK->Stop();
	RTDATAROUTE_TASK->Stop();
	ALARMROUTE_TASK->Stop();
}

int CMainTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}

int CMainTask::UpdateTagNum(Json::Value jsonCmd)
{
	Json::Value jsonKey = jsonCmd[JSONFIELD_NAME];
	Json::Value jsonValue = jsonCmd[JSONFIELD_VALUE];
	Json::Value jsonTime = jsonCmd[JSONFIELD_TIME];
	Json::Value jsonQuality = jsonCmd[JSONFIELD_QUALITY];

	m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_VALUE, jsonValue.asString().c_str());
	m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_TIME, jsonTime.asString().c_str());
	m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_QUALITY, jsonQuality.asString().c_str());
	return 0;
}

int CMainTask::UpdateTag(Json::Value jsonCmd)
{
	Json::Value jsonKey = jsonCmd[JSONFIELD_NAME];
	Json::Value jsonCname = jsonCmd[JSONFIELD_CNAME];
	Json::Value jsonDesc = jsonCmd[JSONFIELD_DESC];
	Json::Value jsonDataType = jsonCmd[TAGJSONFIELD_DATATYPE];
	Json::Value jsonAddress = jsonCmd[TAGJSONFIELD_ADDRESS];
	Json::Value jsonEnableHistory = jsonCmd[TAGJSONFIELD_ENABLEHISTORY];
	Json::Value jsonEnableControl = jsonCmd[TAGJSONFIELD_ENABLCONTROL];
	string strEnHistory = jsonEnableHistory.asString();
	string strEnControl = jsonEnableControl.asString();
	Json::Value jsonDriver = jsonCmd[JSONFIELD_DRIVER];
	Json::Value jsonDevice = jsonCmd[JSONFIELD_DEVICE];
	Json::Value jsonSubsys = jsonCmd[JSONFIELD_SUBSYSNAME];
	Json::Value jsonValue = jsonCmd[JSONFIELD_VALUE];
	Json::Value jsonQuality = jsonCmd[JSONFIELD_QUALITY];

	char szUtf8Value[4096] = {0};
	//CCodingConv::GB2312_2_UTF8(szUtf8Value, sizeof(szUtf8Value), jsonCname.asString().c_str());
	int nRet = m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_CNAME, jsonCname.asString().c_str());
	//CCodingConv::GB2312_2_UTF8(szUtf8Value, sizeof(szUtf8Value), jsonDesc.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_DESC, jsonDesc.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), TAGJSONFIELD_DATATYPE, jsonDataType.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), TAGJSONFIELD_ADDRESS, jsonAddress.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), TAGJSONFIELD_ENABLEHISTORY, strEnHistory.c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), TAGJSONFIELD_ENABLCONTROL, strEnControl.c_str());
	//CCodingConv::GB2312_2_UTF8(szUtf8Value, sizeof(szUtf8Value), jsonDriver.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_DRIVER, jsonDriver.asString().c_str());
	//CCodingConv::GB2312_2_UTF8(szUtf8Value, sizeof(szUtf8Value), jsonDevice.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_DEVICE, jsonDevice.asString().c_str());
	//CCodingConv::GB2312_2_UTF8(szUtf8Value, sizeof(szUtf8Value), jsonSubsys.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_SUBSYSNAME, jsonSubsys.asString().c_str());
	//CCodingConv::GB2312_2_UTF8(szUtf8Value, sizeof(szUtf8Value), jsonValue.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_VALUE, jsonValue.asString().c_str());
	nRet = m_redisRW.hset(jsonKey.asString().c_str(), JSONFIELD_QUALITY, jsonQuality.asString().c_str());
	return 0;
}

int CMainTask::UpdateObject(Json::Value jsonCmd)
{
	Json::Value jsonKey = jsonCmd[JSONFIELD_NAME];
	Json::Value jsonTagCount = jsonCmd[OBJECTJSONFIELD_TAGCOUNT];
	string strTagcount = jsonTagCount.asString();

	m_redisRW.hset(jsonKey.asString().c_str(), OBJECTJSONFIELD_TAGCOUNT, jsonTagCount.asString().c_str());
	return 0;
}

int CMainTask::ParseAndSetInitData2MenDB(string &strMessage,Json::Value &root)
{
	int nRet = PK_SUCCESS;
	string strJsonRoot = root.toStyledString();
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
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error，应为json array or object：{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}",strMessage.c_str());
		g_pServer->UpdateServerStatus(-12,"control format invalid");
		return -12;
	}

	for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++)
	{
		Json::Value &jsonCmd = **itCmd;
		Json::Value &jsonTags = jsonCmd["tags"];
		for (Json::Value::iterator iter = jsonTags.begin(); iter != jsonTags.end(); iter ++)
		{
			Json::Value &jsonTag = *iter;
			Json::Value jsonMsgType = jsonTag["msgtype"];
			string strMsgtype = jsonMsgType.asString();

			if (PKStringHelper::StriCmp(jsonMsgType.asCString(),INITJSONFIELD_MSGTYPE_INITTAG) == 0)
			{
				UpdateTag(jsonTag);
			}else if (PKStringHelper::StriCmp(jsonMsgType.asCString(),INITJSONFIELD_MSGTYPE_INITOBJECT) == 0)
			{
				UpdateObject(jsonTag);
			}else if (PKStringHelper::StriCmp(jsonMsgType.asCString(),INITJSONFIELD_MSGTYPE_INITTAGNUM) == 0)
			{
				UpdateTagNum(jsonTag);
			}else
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "MainTask 接收到来自pklserver的错误的初始化消息类型%s",jsonMsgType.asCString());
				continue;
			}
		}
	}
}