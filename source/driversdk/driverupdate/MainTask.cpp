/**************************************************************
 *  Filename:    RecvThread.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ������Ϣ�߳�.
 *
 *  @author:     liuqifeng
 *  @version     04/13/2009  liuqifeng  Initial Version
**************************************************************/
// RecvThread.cpp: implementation of the CRecvThread class.
//
//////////////////////////////////////////////////////////////////////

#include <ace/INET_Addr.h>
#include <ace/Log_Msg.h>
#include "ClientSockHandler.h"
#include "ace/Select_Reactor.h"
#include "ace/OS_NS_strings.h" 
#include "MainTask.h"
#include "SystemConfig.h"
#include "common/pklog.h"
#include "SystemConfig.h"
#include "ace/SOCK_Connector.h"

extern CPKLog	PKLog;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
//ACE_DLL g_dllforwardaccessor;
//PFN_ReadAllTagsInfo	g_pfnReadAllTagsInfo = NULL;
//PFN_ReadAllTagsData	g_pfnReadAllTagsData = NULL;
//PFN_ReleaseAllTagsInfo	g_pfnReleaseAllTagsInfo = NULL;
#define DLLNAME_FORWARDACCESSOR			"forwardaccessor"


CMainTask::CMainTask():m_cycleReader(this)
{ 
	ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
	m_pReactor = new ACE_Reactor(pSelectReactor, 1);
	reactor(m_pReactor);
}

CMainTask::~CMainTask()
{
	if (m_pReactor != NULL)
	{
		delete m_pReactor;
		m_pReactor = NULL;
	}
}

//�̷߳���
int CMainTask::svc()
{
	//long nErr = g_dllforwardaccessor.open(DLLNAME_FORWARDACCESSOR);
	//if (nErr == 0)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, " load Module %s �ɹ�", DLLNAME_FORWARDACCESSOR);
	//}
	//else
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, " load Module %s ʧ��", DLLNAME_FORWARDACCESSOR);

	//// �����ʼ������
	//g_pfnReadAllTagsInfo = (PFN_ReadAllTagsInfo)g_dllforwardaccessor.symbol("ReadAllTagsInfo"); 
	//if(g_pfnReadAllTagsInfo == NULL)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, " ��%s ��ȡ��������%sʧ��",g_dllforwardaccessor.dll_name_, "ReadAllTagsInfo");
	//}
	//g_pfnReadAllTagsData = (PFN_ReadAllTagsData)g_dllforwardaccessor.symbol("ReadAllTagsData"); 
	//if(g_pfnReadAllTagsData == NULL)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, " ��%s ��ȡ��������%sʧ��",g_dllforwardaccessor.dll_name_, "ReadAllTagsData");
	//}
	//g_pfnReleaseAllTagsInfo = (PFN_ReleaseAllTagsInfo)g_dllforwardaccessor.symbol("ReleaseAllTagsInfo"); 
	//if(g_pfnReleaseAllTagsInfo == NULL)
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, " ��%s ��ȡ��������%sʧ��",g_dllforwardaccessor.dll_name_, "ReleaseAllTagsInfo");
	//}

	this->reactor()->owner(ACE_OS::thr_self ());
	m_cycleReader.m_nPollRate = SYSTEM_CONFIG->m_nCycleMS;
	m_cycleReader.StartTimer();

	this->reactor()->reset_reactor_event_loop();
	this->reactor()->run_reactor_event_loop();

	m_cycleReader.StopTimer();
	return 0;
}

void CMainTask::Stop()
{ 
	// �����ر���Ӧ�������
	this->reactor()->end_reactor_event_loop();
	
	// �ȴ���Ӧ���رպ��˳�
	this->wait();
}


int CMainTask::Start()
{
	return this->activate();
}

