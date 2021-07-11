/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  ȷ�Ϻ�ɾ������Ҳ���ڿ����������ִ�С�����Ҳ�Ƕ������ӣ�����ת�������ݴ��������
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "ace/High_Res_Timer.h"
#include "math.h"

#include "CtrlRouteTask.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "pkDataServer.h"
#include "CtrlRouteTask.h"
#include "common/CodingConv.h"

extern CDataServer* g_pServer;

#define	DEFAULT_SLEEP_TIME		100	

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CCtrlRouteTask::CCtrlRouteTask()
{
	m_bStop = false;
	m_bStopped = false;
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CCtrlRouteTask::~CCtrlRouteTask()
{
	
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CCtrlRouteTask::svc()
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

		// channelname:simensdrv,value:[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]
		string strControlsChannel = CHANNELNAME_CONTROL;
		nRet = m_redisSub.SubChannel(strControlsChannel);

		// ����������״̬
		if(nRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Control process task,Subscribe channle:%s failed,ret:%d", strControlsChannel.c_str(), nRet);
			g_pServer->UpdateServerStatus(-2,"subscribe control channel failed");
		}

		while(!m_bStop)
		{
			string strChannel = "";
			string strMessage;
			if(m_redisSub.RecvChannelMsg(strChannel, strMessage,100) != 0) // δ�յ���Ϣ
			{
				ACE_OS::sleep(tvSleep);
				continue;
			}

			PKLog.LogMessage(PK_LOGLEVEL_INFO, "CtrlProcTask recv an controls msg:%s!", strMessage.c_str());
			Json::Reader reader;
			Json::Value root;
			if (!reader.parse(strMessage, root, false))
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg(%s), invalid!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"control msg format invalid");
				continue;
			}

			if (!root.isObject() && !root.isArray())
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask recv an msg(%s), not an object or array!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"control msg format invalid");
				continue;
			}

			// ����tag���ƣ��ҵ���Ӧ��������Ӧ�Ķ���ͨ��������������������
			RouteControl2Node(strMessage, root);
		}
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"CtrlProcTask �����˳���");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CCtrlRouteTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	nStatus = m_redisSub.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask, Initialize m_redisSub failed, ret:%d", nStatus);
	}

	nStatus = m_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);	
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask, Initialize m_redisRW failed, ret:%d", nStatus);
	}

	nStatus = m_redisPublish.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);	
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "CtrlProcTask, Initialize m_redisPublish failed, ret:%d", nStatus);
	}
	return nStatus;
}

// ������ֹͣʱ
void CCtrlRouteTask::OnStop()
{
	m_redisSub.Finalize();
	m_redisRW.Finalize();
	m_redisPublish.Finalize();
}

int CCtrlRouteTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CCtrlRouteTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}

// Json������Ϊ��[{"name":tag1,"value":"100"},{"name":tag2,"value":"100"}]  {\"name\":\"tag1\",type:,\"value\":\"100\"}
int CCtrlRouteTask::RouteControl2Node(string &strCtrlCmd, Json::Value &root)
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
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error��ӦΪjson array or object��{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}",strCtrlCmd.c_str());
		g_pServer->UpdateServerStatus(-12,"control format invalid");
		return -12;
	}

	for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++)
	{
		Json::Value &jsonCmd = **itCmd;

		Json::Value encType = jsonCmd["enc"]; // ��������
		Json::Value tagValue = jsonCmd["value"];
		string strValue;
		if(!tagValue.isObject())
			 strValue= tagValue.asString();
		else
			strValue=tagValue.toStyledString();
		string strEncType = encType.asString();
		//if(strEncType.compare("UTF8") == 0) // java���û���unicode���룬��Ҫת����
		//{
		//	char szValue[4096] = {0};
		//	CCodingConv::UTF8_2_GB2312(szValue, sizeof(szValue), strValue.c_str(), strValue.length());
		//	strValue = szValue;
		//}

		// �������������Ӧ��pklserver����
		Json::Value	jsonCtrl;
		jsonCtrl["name"] = jsonCmd["name"];
		jsonCtrl["value"] = jsonCmd["value"];
		jsonCtrl["type"] = jsonCmd["type"];
		jsonCtrl["username"] = jsonCmd["username"];
		string strNewCtrl = jsonCtrl.toStyledString();

		Json::Value tagGw = jsonCmd["gw"];
		string strGw = tagGw.asString();

		string strChannel = "control-";
		if (strGw == "")
		{
			strChannel += "local";
		}else
		{
			strChannel += strGw;
		}
		m_redisPublish.PublishMsg(strChannel,strNewCtrl.c_str());
	}

	return nRet;
}
