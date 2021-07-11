/**************************************************************
 *  Filename:    DataBlock.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ���ݿ���Ϣʵ����.
 *
 * ȷʵ��ָ��CDrvObjectBase�ڴ�
**************************************************************/

#include "CycleReader.h" 
#include "MainTask.h"
#include "common/pklog.h"
#include "SystemConfig.h"

extern CPKLog PKLog;
//extern PFN_ReadAllTagsInfo	g_pfnReadAllTagsInfo;
//extern PFN_ReadAllTagsData	g_pfnReadAllTagsData;
//extern PFN_ReleaseAllTagsInfo	g_pfnReleaseAllTagsInfo;


/**
 *  ���캯��.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCycleReader::CCycleReader(CMainTask *pTask)
{
	 m_pMainTask = pTask;
	 m_sockHandler.m_pCycleReader = this;
	 m_sockHandler.m_pMainTask = m_pMainTask;
	 m_nPollRate = 0;
	 m_nTransId = 0;
	 memset(m_szGatewayId, 0, sizeof(m_szGatewayId));
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCycleReader::~CCycleReader()
{	
 
}

void CCycleReader::StartTimer()
{
	//// ��ȡ���е�tag����Ϣ
	//if(g_pfnReadAllTagsInfo)
	//	g_pfnReadAllTagsInfo(&m_vecTagGroup);
	//else
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ReadAllTagsInfo is null!");
	//}

	// ��ʼ����������
	if(m_sockHandler.TryConnectToServer() == 0)
	{
		RegisterToServer();
	}
	// ע�����е���
	//RegisterAllGroups();
	

	if(m_nPollRate > 0)
	{
		ACE_Time_Value	tvPollRate;		// ɨ�����ڣ���λms
		tvPollRate.msec(m_nPollRate);
		ACE_Time_Value tvPollPhase;
		tvPollPhase.msec(m_nPollPhase);
		tvPollPhase += ACE_Time_Value::zero;
		m_pMainTask->m_pReactor->schedule_timer((ACE_Event_Handler *)(CCycleReader *)this, NULL, tvPollPhase, tvPollRate);
	}	
}

void CCycleReader::StopTimer()
{
	m_pMainTask->m_pReactor->cancel_timer((ACE_Event_Handler *)(CCycleReader *)this);
	// ����������ͷ�tag����Ϣ�����ܳ����ͷ����handle_timeout���ᱻ���õ��������˾Ͳ��ͷ���
	//UnRegisterAllGroups();
}

// ÿ�ζ�ʱ������ʱ��ȥ��ȡһ������Ҫת���ĵ㣬Ȼ�����socket���ͽӿڷ��ͳ�ȥ
int CCycleReader::handle_timeout( const ACE_Time_Value &current_time, const void *act /*= 0*/ )
{
	//// ������tag�������ȶ�ȡ��tag�ڴ���
	//if(g_pfnReadAllTagsData)
	//	g_pfnReadAllTagsData(&m_vecTagGroup);

	time_t tmNow;
	time(&tmNow);

	// ��ʱ���ӷ���
	if(m_sockHandler.TryConnectToServer() == 0)
	{
		RegisterToServer();
	}

	//// ��֯��ת�������ܷ���
	//vector<TAGGROUP *>::iterator itGroup = m_vecTagGroup.begin();
	//for(; itGroup != m_vecTagGroup.end(); itGroup ++)
	//{
	//	TAGGROUP *pGroup = (TAGGROUP *)*itGroup;

	//	// �ж��Ƿ������3��û��Ӧ����Ҫ����ע��
	//	long lInterval = tmNow - pGroup->tmLastResponse;
	//	if(labs(lInterval) > 60 * 3) // ����3����û�дӷ�����յ�Ӧ������Ϊ��Ҫ����
	//	{
	//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Group does not receive response from server > 3 minutes! Reconnect!");
	//		pGroup->uServerGrpHandle = -1;
	//	}

	//	//ע�����ת��һ���������
	//	if(pGroup->uServerGrpHandle < 0) // δע�������Ҫ����
	//	{
	//		RegisterAGroup((TAGGROUP *) *itGroup);
	//	}
	//	else
	//	{
	//		ForwardAGroup(pGroup);
	//	}
	//	
	//}
	return 0;
}

long CCycleReader::RegisterToServer()
{
	// ���ͱ�ʶ��Դ�İ� �� ����
	char szSendBuf[MAX_SENDBUF_LEN];
	memset(szSendBuf, 0x00, sizeof(szSendBuf));

	char *pSendBuf = szSendBuf;

	// ��ʼ 1B
	*pSendBuf = PACK_HEADER_FLAG;
	pSendBuf++;


	unsigned int nCodeLen = SYSTEM_CONFIG->m_strProjectCode.length();

	if (nCodeLen > PROJECT_CODE_LENGTH)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��Ŀ���ų��� %d ���ȣ�ֻ����8λ����", nCodeLen);
		return -1;
	}

	unsigned int nNameLen = SYSTEM_CONFIG->m_strGatewayName.length();
	if (nNameLen > GATEWAY_ID_LEN)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "���ش��ų��� %d ���ȣ�ֻ����8λ����", nNameLen);
		return -1;
	}
	// ���� 2B
	unsigned short nSendLen = PROJECT_CODE_LENGTH + GATEWAY_ID_LEN + 2;
	memcpy(pSendBuf, & nSendLen, sizeof(unsigned short));
	pSendBuf ++;
	pSendBuf ++;

	// ����ID
	*pSendBuf = 01;
	pSendBuf ++;

	// ����ID
	*pSendBuf = COMMAND_NOTIFY_SOURCE;
	pSendBuf ++;

	// ��Դ
	*pSendBuf = CONN_FROM_GATEWAY;
	pSendBuf ++;

	// ����������
	memcpy(pSendBuf, SYSTEM_CONFIG->m_strProjectCode.c_str(), nCodeLen);
	memcpy(m_szProjectCode, SYSTEM_CONFIG->m_strProjectCode.c_str(), nCodeLen);
	pSendBuf += PROJECT_CODE_LENGTH; 

	memcpy(pSendBuf, SYSTEM_CONFIG->m_strGatewayName.c_str(), nNameLen);
	memcpy(m_szGatewayId, SYSTEM_CONFIG->m_strGatewayName.c_str(), nNameLen);
	pSendBuf += GATEWAY_ID_LEN; 

	// �������
	*pSendBuf = PACK_FOOTER_FLAG;
	pSendBuf ++;

	long lRet = m_sockHandler.SendDataToServer((char *)szSendBuf, pSendBuf - szSendBuf);

	if (lRet > 0 )
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"ע������[%s]", SYSTEM_CONFIG->m_strGatewayName.c_str());
		return 0;
	}
	else
	{
		return -1;
	}
}
