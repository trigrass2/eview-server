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
#include "math.h"
#include "RTDataRouteTask.h"
#include "common/pkcomm.h"
#include "common/pkGlobalHelper.h"
#include "redisproxy/redisproxy.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/pktagdef.h"
#include "pklog/pklog.h"
#include "common/CodingConv.h"
#include "pkDataServer.h"
#include "RTDataRouteTask.h"
#include "ace/High_Res_Timer.h"

extern CPKLog PKLog;
extern CDataServer* g_pServer;

#define	DEFAULT_SLEEP_TIME		100

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CRTDataRouteTask::CRTDataRouteTask()
{
	m_bStop = false;
	m_bStopped = false;

	m_nPackNumRecvRecent = 0;
	m_nPackNumRecvFromStart = 0;
	
	m_tvRecentRecvPack = m_tvFirstRecvPack = ACE_OS::gettimeofday();	// �ϴδ�ӡ
	m_nTimeoutPackNumFromStart = 0;
	m_nTimeoutPackNumRecent = 0;
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
CRTDataRouteTask::~CRTDataRouteTask()
{
	
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  lijingjing  Initial Version.
 */
int CRTDataRouteTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = 0;
	nRet = OnStart();

	char szTime[ICV_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
	m_tvFirstRecvPack = ACE_OS::gettimeofday();

	while(!m_bStop)
	{
		ACE_Time_Value tvSleep;
		tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms
		ACE_OS::sleep(tvSleep);		

		// ��Ϣ����  ����  [{"type":"control","name":"tag1","value":"100"},{"type":"control","name":"tag2","value":"100"}]  
		//           ȷ�ϱ���  [{"type":"confirm-alarms","value":"*"},{"type":"confirm-alarms","value":"*HH"},{"type":"confirm-alarms","value":"obj1.*"}]
		string strControlsChannel = CHANNELNAME_DATA;
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
			int nRetRecv =m_redisSub.RecvChannelMsg(strChannel, strMessage,100); // ������ʱ����ղ�����û���ĳɹ���
			if(nRetRecv != 0) // δ�յ���Ϣ
			{
				ACE_OS::sleep(tvSleep);
				continue;
			}

			PKLog.LogMessage(PK_LOGLEVEL_DEBUG, "RTDataRouteTask recv an msg from %s!", strChannel.c_str());
			Json::Reader reader;
			Json::Value jsonRoot;
			if (!reader.parse(strMessage, jsonRoot, false)) // ����json����Ҫ�ķ�CPU�϶࣬10000�����Ҫ13%CPU�����Բ��������ַ�ʽ
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RTDataRouteTask recv an msg(%s), invalid!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"rtdata msg format invalid");
				continue;
			}

			if (!jsonRoot.isObject())
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RTDataRouteTask recv an msg(%s), not an object or array!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"rtdata msg format invalid");
				continue;
			}

			if(jsonRoot["packSec"].isNull() || jsonRoot["packMSec"].isNull() || jsonRoot["tags"].isNull())
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RTDataRouteTask recv an msg(%s), object has no packSec/packMSec/tags!",strMessage.c_str());
				g_pServer->UpdateServerStatus(-2,"rtdata msg format invalid");
				continue;
			}

			// ȡʱ���
			unsigned int nPackTimeStampSec = jsonRoot["packSec"].asUInt();
			unsigned int nPackTimeStampMSec = jsonRoot["packMSec"].asUInt();
			Json::Value &jsonTags = jsonRoot["tags"];
			ACE_Time_Value tvPack;
			tvPack.sec(nPackTimeStampSec);
			tvPack.usec(nPackTimeStampMSec * 1000);
			m_nPackNumRecvFromStart ++;
			m_nPackNumRecvRecent ++;

			ACE_Time_Value tvNow = ACE_OS::gettimeofday(); // �յ�����ʱ��
			// ������û�г�ʱ
			ACE_Time_Value tvPackSpan = tvNow - tvPack;
			if(labs(tvPackSpan.sec()) > 2) // ���ͺͽ���ʱ�������2�룡
			{
				m_nTimeoutPackNumFromStart ++;
				m_nTimeoutPackNumRecent ++;
				// PKLog.LogMessage(PK_LOGLEVEL_ERROR, "--��������-- CRTDataRouteTask::svc(), ���յ�����ʱ��ͷ���ʱ������:%d ����>2��", tvSpan.sec());
			}

			ACE_Time_Value tvSpanRecentRecv = tvNow - m_tvRecentRecvPack;
			int nSecSpan = labs(tvSpanRecentRecv.sec());
			if(nSecSpan > 10) // ��ʱ����Ҫ��ӡ��Ϣ��
			{
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "���: %d ����� %d ��, ƽ�� %f��/��. �����������յ�:%d��", nSecSpan, m_nPackNumRecvRecent, m_nPackNumRecvRecent * 1.0/nSecSpan, m_nPackNumRecvFromStart);
				if(m_nTimeoutPackNumRecent > 0)
				{
					PKLog.LogMessage(PK_LOGLEVEL_ERROR, "������������: ���պͷ��ͼ������>2��,��� %d �빲: %d ��,��ϵͳ������: %d ��", nSecSpan, m_nTimeoutPackNumRecent, m_nTimeoutPackNumFromStart);
				}
				m_nPackNumRecvRecent = 0;
				m_nTimeoutPackNumRecent = 0;
				m_tvRecentRecvPack = tvNow;
			}

			RouteOrSetTagsData(strMessage, jsonTags); // 1���㲻����redisд������pkmemdbռ��CPU�ﵽ13%������Ӧ��ʱ��
		}
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"RTDataRouteTask �����˳���");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CRTDataRouteTask::OnStart()
{
	int nStatus = PK_SUCCESS;
	nStatus = m_redisSub.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "TRDataRouteTask, Initialize m_redisSub failed, ret:%d", nStatus);
	}

	nStatus = m_redisRW.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);	
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "TRDataRouteTask, Initialize m_redisRW failed, ret:%d", nStatus);
	}

	nStatus = m_redisPublish.Initialize(PK_LOCALHOST_IP, PDB_LISTEN_PORT, PDB_ACCESS_PASSWORD);	
	if(nStatus != PK_SUCCESS)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "TRDataRouteTask, Initialize m_redisPublish failed, ret:%d", nStatus);
	}
	return nStatus;
}

// ������ֹͣʱ
void CRTDataRouteTask::OnStop()
{
	m_redisSub.Finalize();
	m_redisRW.Finalize();
	m_redisPublish.Finalize();
}

int CRTDataRouteTask::Start()
{
	return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CRTDataRouteTask::Stop()
{
	m_bStop = true;
	int nWaitResult = wait();

	ACE_Time_Value tvSleep;
	tvSleep.msec(100);
	while(!m_bStopped)
		ACE_OS::sleep(tvSleep);
}

int CRTDataRouteTask::RouteOrSetTagsData(string &strCtrlCmd,Json::Value &jsonTags)
{
	if(m_redisRW.checkConnect() != 0)
		return -1;

	int nRet = PK_SUCCESS;
	if((!jsonTags.isArray()) && (!jsonTags.isObject()))
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "jsonTags(%s),format error��ӦΪjson array �� object",strCtrlCmd.c_str());
		g_pServer->UpdateServerStatus(-12,"control format invalid");
		return -12;
	}
	
	vector<string> vecCommands;
	int nCmdNum = 0;
	for(int iTag = 0; iTag < jsonTags.size(); iTag ++)
	{
		Json::Value &jsonTag = jsonTags[iTag];
		
		string strTagDetail = jsonTag.toStyledString();
		Json::Value tagKey = jsonTag[JSONFIELD_NAME];
		string strKey = tagKey.asString();
		Json::Value tagEnableAlarm = jsonTag[TAGJSONFIELD_ENABLEALARM];
		Json::Value tagValue = jsonTag[JSONFIELD_VALUE];
		string strValue = tagValue.asString();
		Json::Value tagTime = jsonTag[JSONFIELD_TIME];
		Json::Value tagQuality = jsonTag[JSONFIELD_QUALITY];
		Json::Value tagMsg = jsonTag[JSONFIELD_MSG];     
		string strMsg = tagMsg.asString();
		// ���µ��ڴ����ݿ�
		
		m_redisRW.pipe_append("hmset %s enablealarm %s value %s time %s quality %s msg %s", 
			strKey.c_str(), tagEnableAlarm.asString().c_str(),tagValue.asString().c_str(), tagTime.asString().c_str(), tagQuality.asString().c_str(), strMsg.c_str());
		nCmdNum ++;
		/*
		vector<string> vecKey,vecField,vecValue;
		vecKey.push_back(strKey.c_str());
		vecField.push_back(TAGJSONFIELD_ENABLEALARM);
		vecValue.push_back(tagEnableAlarm.asString());

		vecKey.push_back(strKey.c_str());
		vecField.push_back("value");
		vecValue.push_back(tagValue.asString());

		vecKey.push_back(strKey.c_str());
		vecField.push_back("time");
		vecValue.push_back(tagTime.asString());

		vecKey.push_back(strKey.c_str());
		vecField.push_back("quality");
		vecValue.push_back(tagQuality.asString());

		vecKey.push_back(strKey.c_str());
		vecField.push_back("msg");
		vecValue.push_back(strMsg);*/
		/*
		char szCmd[1024];
		sprintf(szCmd, "hset \"%s\" %s \"%s\"", strKey.c_str(), TAGJSONFIELD_ENABLEALARM, tagEnableAlarm.asString().c_str());
		vecCommands.push_back(szCmd);
		sprintf(szCmd, "hset \"%s\" value \"%s\"", strKey.c_str(), tagValue.asString().c_str());
		vecCommands.push_back(szCmd);
		sprintf(szCmd, "hset \"%s\" time \"%s\"", strKey.c_str(), tagTime.asString().c_str());
		vecCommands.push_back(szCmd);
		sprintf(szCmd, "hset \"%s\" quality \"%s\"",strKey.c_str(), tagQuality.asString().c_str());
		vecCommands.push_back(szCmd);
		sprintf(szCmd, "hset \"%s\" msg \"%s\"", strKey.c_str(), strMsg.c_str());
		vecCommands.push_back(szCmd);

		nRet = m_redisRW.redisCommon(vecCommands);*/
		//nRet = m_redisRW.hmset_pipe(vecKey, vecField, vecValue);
		//nRet = m_redisRW.hset(strKey.c_str(), TAGJSONFIELD_ENABLEALARM, tagEnableAlarm.asString().c_str());
		//nRet = m_redisRW.hset(strKey.c_str(), "value", tagValue.asString().c_str());
		//nRet = m_redisRW.hset(strKey.c_str(), "time", tagTime.asString().c_str());
		//nRet = m_redisRW.hset(strKey.c_str(), "quality", tagQuality.asString().c_str());
		//nRet = m_redisRW.hset(strKey.c_str(), "msg", strMsg.c_str());

		// ���������ʷ���ݼ�¼�������ݷ��͵���ʷ���ݷ�����д���
		//Json::Value jsonEnableHistory =  jsonTag[TAGJSONFIELD_ENABLEHISTORY];
		//Json::Value jsonHitory;
		//jsonHitory[JSONFIELD_NAME] = jsonTag[JSONFIELD_NAME];
		//jsonHitory[JSONFIELD_VALUE] = jsonTag[JSONFIELD_VALUE];
		//jsonHitory[JSONFIELD_TIME] = jsonTag[JSONFIELD_TIME];
		//jsonHitory[JSONFIELD_QUALITY] = jsonTag[JSONFIELD_QUALITY];

		// string strDetail = jsonHitory.toStyledString();
		// if (PKStringHelper::StriCmp(jsonEnableHistory.asCString(),"true") == 0)
			//m_redisPublish.PublishMsg(CHANNELNAME_HISDATA, strDetail);
	}

	vector<string> vecResult;
	if(nCmdNum > 0)
		m_redisRW.pipe_getReply(nCmdNum, vecResult);

	//if(nRet != 0)
	{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "reidsִ��pipeline��%d��, ������:%d", nRet);
	}


	return nRet;
}
