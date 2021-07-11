/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:    shijunpu
 *  @version    05/28/2008  shijunpu  Initial Version
 *  @version	9/17/2012  shijunpu  ���ݿ������޸�Ϊ�ַ������ͣ������޸��������ݿ�ʱ�Ľӿ�
 *  @version	3/1/2013   shijunpu  �����ɵ����ɿ���߼������豸��û���������ݿ�ʱ��ֱ���ɵ��������ݿ� .
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "math.h"
#include "MemDBTask.h"
#include "pkcomm/pkcomm.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"

extern CPKLog g_logger;

#define	DEFAULT_SLEEP_TIME		100	

extern PFN_OnControl	g_pfnOnControl;	// pkservermgr�����̵ĺ���

#include <fstream>  
#include <streambuf>  
#include <iostream>  

bool	g_bMemDBSubConnected = false;
void OnPM_MemDBDisconnect(void *pCallbackParam)
{
	g_bMemDBSubConnected = false;
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "proc2memdb m_memDBSubAsync Disconnected!");
}

void OnPM_ChannelMsg(const char *szChannelName, const char *szMessage, void *pCallbackParam)
{
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Recv a ControlMsg, channel:%s, message:%s", szChannelName, szMessage);
	if (strcmp(szChannelName, CHANNELNAME_PM_CONTROL) != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "ServerMgr plugin proc2memdb OnPM_ChannelMsg, recv a msg but not %s msg.channel:%s, message:%s", CHANNELNAME_PM_CONTROL, szChannelName, szMessage);
		return;
	}

	if (g_pfnOnControl)
		g_pfnOnControl((char *)szMessage);
}

// �õ�PKMemDB�Ķ˿ں�PKMemDB������
int GetPKMemDBPort(int &nListenPort, string &strPassword)
{
	nListenPort = PKMEMDB_LISTEN_PORT;
	strPassword = PKMEMDB_ACCESS_PASSWORD;

	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmemdb.conf";
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // �����ڸ��ļ�������Ϊ��sqlite��eview.db
	{
		// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return -1;
	}

	// �����ļ���С  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// ����1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());
	textFile.close();
	vector<string> vecLine = PKStringHelper::StriSplit(strConfigData, "\n");
	for (unsigned int iLine = 0; iLine < vecLine.size(); iLine++)
	{
		string strLine = vecLine[iLine];
		vector<string> vecContent = PKStringHelper::StriSplit(strLine, " ");
		if (vecContent.size() < 2)
			continue;
		string strKey = vecContent[0];
		string strValue = vecContent[1];
		if (strKey.compare("port") == 0)
			nListenPort = ::atoi(strValue.c_str());
		if (strKey.compare("requirepass") == 0)
			strPassword = strValue.c_str();
	}
	return 0;
}

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMemDBTask::CMemDBTask()
{
	m_bStop = false;
	m_bStopped = false;
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMemDBTask::~CMemDBTask()
{
	
}

int CMemDBTask::StartTask()
{
	return activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
}

void CMemDBTask::StopTask()
{
	m_bStop = true;
	int nWaitResult = wait();
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CMemDBTask::svc()
{	
	ACE_High_Res_Timer::global_scale_factor();

	int nRet = 0;
	nRet = OnStart();

	char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

	// ��������ն����������е�����PM��Ϣ�����⻺����˳���Ϣ���³����޷�����
	//while (true){
	//	string strChannel;
	//	string strMessage;
	//	nRet = m_memDBSubSync.receivemessage(strChannel, strMessage, 0); // 500 ms
	//	// ����������״̬
	//	if (nRet != 0)
	//		break;
	//}

	while(!m_bStop)
	{
		//ACE_Time_Value tvTimes = ACE_OS::gettimeofday() + ACE_Time_Value(0, 100 * 1000);
		string strChannel;
		string strMessage;

		bool bConnected = m_memDBSubAsync.isconnect();
		if (!bConnected)
		{
			bool bConnected = m_memDBSubAsync.connect();
			if (bConnected)
			{
				g_bMemDBSubConnected = true;
				g_logger.LogMessage(PK_LOGLEVEL_INFO, "proc2memdb m_memDBSubAsync connected!");
				nRet = m_memDBSubAsync.subscribe(CHANNELNAME_PM_CONTROL, OnPM_ChannelMsg, this);
				if (nRet != 0)
				{
					g_logger.LogMessage(PK_LOGLEVEL_ERROR, "proc2memdb task subscribe channel:%s failed, ret:%d", CHANNELNAME_PM_CONTROL, nRet);
				}
				else
					g_logger.LogMessage(PK_LOGLEVEL_INFO, "proc2memdb m_memDBSubAsync subscribe channel:%s success!", CHANNELNAME_PM_CONTROL);

			}
		}
		PKComm::Sleep(500); 

		//nRet = m_memDBSubSync.receivemessage(strChannel, strMessage, 500); // 500 ms
		//// ����������״̬
		//if (nRet != 0)
		//{
		//	PKComm::Sleep(500); // ��ʱ����û�г�ʱʱ�䣡����
		//	continue;
		//}

		//if (strChannel.compare(CHANNELNAME_PM_CONTROL) != 0)
		//{
		//	g_logger.LogMessage(PK_LOGLEVEL_INFO, "ServerMgr plugin proc2memdb recv a msg but not %s msg.channel:%s, message:%s", CHANNELNAME_PM_CONTROL, strChannel.c_str(), strMessage.c_str());
		//	continue;
		//}

		//if (g_pfnOnControl)
		//	g_pfnOnControl((char *)strMessage.c_str());

		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "ServerMgr plugin proc2memdb MemDBTask recv an controls msg:%s", strMessage.c_str());
	}//while(!m_bStop)

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE,"ServerMgr plugin proc2memdb MemDBTask exit normally��");
	OnStop();

	m_bStopped = true;
	return PK_SUCCESS;	
}

int CMemDBTask::DelProcessStatusTags()
{
	string strKeys = m_memDBClientAsync.keys("keys pm.*");
	vector<string> vecKeys = PKStringHelper::StriSplit(strKeys, "&");

	for(vector<string>::iterator itKey = vecKeys.begin(); itKey != vecKeys.end(); itKey ++)
	{
		string strKey = *itKey;
		m_memDBClientAsync.del(strKey.c_str());
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "del pm.%s", strKey.c_str());
	}
	m_memDBClientAsync.commit();
	vecKeys.clear();

	return 0;
}

int CMemDBTask::OnStart()
{
	int nRet = PK_SUCCESS;
	//ACE_OS::sleep(1);	// �ȴ��ڴ����ݿ���������ȣ�

	int nPKMemDBListenPort = PKMEMDB_LISTEN_PORT;
	string strPKMemDBPassword = PKMEMDB_ACCESS_PASSWORD;
	GetPKMemDBPort(nPKMemDBListenPort, strPKMemDBPassword);

	nRet = m_memDBClientAsync.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), 0);
	if (nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize RW failed, ret:%d", nRet);
	}

	DelProcessStatusTags(); // ɾ������pm:��tag��

	//nRet = m_memDBSubSync.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str());
	//if (nRet != PK_SUCCESS)
	//{
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize Sub failed, ret:%d", nRet);
	//}
	//nRet = m_memDBSubSync.subscribe(CHANNELNAME_PM_CONTROL);// , OnChannel_PMControlMsg, this);
	//if (nRet != 0)
	//{
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "proc2memdb task subscribe channel:%s failed, ret:%d", CHANNELNAME_PM_CONTROL, nRet);
	//}

	nRet = m_memDBSubAsync.initialize(PK_LOCALHOST_IP, nPKMemDBListenPort, strPKMemDBPassword.c_str(), OnPM_MemDBDisconnect, NULL);
	if (nRet != PK_SUCCESS)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "procmgr Main Task, Initialize Sub failed, ret:%d", nRet);
	}

	nRet = m_memDBSubAsync.subscribe(CHANNELNAME_PM_CONTROL, OnPM_ChannelMsg, this);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "proc2memdb task subscribe channel:%s failed, ret:%d", CHANNELNAME_PM_CONTROL, nRet);
	}
	m_memDBSubAsync.commit();
	return nRet;
}

// ������ֹͣʱ
void CMemDBTask::OnStop()
{
	DelProcessStatusTags(); // ɾ������pm:��tag��
	//m_memDBSubSync.finalize();
	m_memDBClientAsync.finalize();
	m_memDBSubAsync.finalize();
}
