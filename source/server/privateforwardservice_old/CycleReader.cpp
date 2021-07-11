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
#include "GWProtoDef.h"

extern CPKLog PKLog;
extern PFN_Initialize		g_pfnInitialize;
extern PFN_ReadAllTagsData	g_pfnReadAllTagsData;
extern PFN_UnInitialize		g_pfnUnInitialize;


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
	// ��ȡ���е�tag����Ϣ
	if(g_pfnInitialize)
		g_pfnInitialize(&m_vecTagGroup);
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ReadAllTagsInfo is null!");
	}

	// ��ʼ����������
	// ��ʱ����ע�ᵽ����������ע���л᳢������
	long lRetConn = this->m_sockHandler.TryConnectToServer();
	if(lRetConn != 0)
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterToServer failed, retcode:%d, will continue...", lRetConn);
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "RegisterToServer success");

	// ע�����е���
	long lRetRegister = RegisterAllGroups();
	if(lRetConn != 0)
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterAllGroups failed(%d ��ʧ��), retcode:%d, will continue...",  -1 * lRetRegister);
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "RegisterAllGroups success");

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
	UnRegisterAllGroups();
}

// ÿ�ζ�ʱ������ʱ��ȥ��ȡһ������Ҫת���ĵ㣬Ȼ�����socket���ͽӿڷ��ͳ�ȥ
int CCycleReader::handle_timeout( const ACE_Time_Value &current_time, const void *act /*= 0*/ )
{
	// ��ʱ���ӷ���
	if(m_sockHandler.TryConnectToServer() == 0)
	{
		if (RegisterToServer() != 0)
		{
			return -1;
		}
		
	}

	// ������tag�������ȶ�ȡ��tag�ڴ���
	if(g_pfnReadAllTagsData)
		g_pfnReadAllTagsData(&m_vecTagGroup);

	time_t tmNow;
	time(&tmNow);

	// ��֯��ת�������ܷ���
	vector<TAGGROUP *>::iterator itGroup = m_vecTagGroup.begin();
	for(; itGroup != m_vecTagGroup.end(); itGroup ++)
	{
		TAGGROUP *pGroup = (TAGGROUP *)*itGroup;

		// �ж��Ƿ������3��û��Ӧ����Ҫ����ע��
		long lInterval = tmNow - pGroup->tmLastResponse;
		if(labs(lInterval) > m_nPollRate /1000 * 3) // ����3����û�дӷ�����յ�Ӧ��������ע������
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Group does not receive response from server > 3 minutes! Set ServerGrpHandle==0!");
			pGroup->uServerGrpHandle = 0;
		}

		//ע�����ת��һ���������
		if(pGroup->uServerGrpHandle <= 0) // δע�������Ҫ����
		{
			long lInterval = tmNow - pGroup->tmLastRequest;
			if(labs(lInterval) > m_nPollRate /1000 * 3) // ��ʱ���3��û�з��ͳɹ�������
			{
				RegisterAGroup((TAGGROUP *) *itGroup);
			}
		}
		else
		{
			ForwardAGroup(pGroup);
		}
		
	}
	return 0;
}

// ע��һ��tag��
// ��ʽ������ID+��ID+ʱ��+�������+����ID+tag����1����+tag����1+tag��������+tag2���Ƴ���+.....
int CCycleReader::ForwardAGroup(TAGGROUP *pTagGroup)
{
	char szRegisterBuf[MAX_SENDBUF_LEN] = {0}; // һ��������󳤶�Ϊ1024
	char *pBuf = szRegisterBuf;
	// ����ID
	*pBuf ++ = COMMAND_ID_WRTITE_GROUP;
	// ����˷��ص����ID
	memcpy(pBuf, &pTagGroup->uServerGrpHandle, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);

	// ʱ���
	time_t nowTime;
	time(&nowTime);
	memcpy(pBuf, &nowTime, sizeof(time_t));
	pBuf += sizeof(time_t);

	// �������2B+����ID 2B+tag����1B+ֵ
	// �������
	unsigned short nDeviceNum = 1;
	memcpy(pBuf, &nDeviceNum, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);

	for (int i = 0 ; i < nDeviceNum; i++)
	{
		// ����ID
		unsigned short nDeviceId = 1;
		memcpy(pBuf, &nDeviceId, sizeof(unsigned short));
		pBuf += sizeof(unsigned short);
		vector<CVTAG *>::iterator itTag = pTagGroup->vecTag.begin();
		for(; itTag != pTagGroup->vecTag.end(); itTag ++)
		{
			CVTAG *pTag = (CVTAG *)*itTag;
			// tag����
			*pBuf ++ = (unsigned char )pTag->uQuality;
			// tagֵ����
			memcpy(pBuf, pTag->rawData.pszData, pTag->rawData.nDataSize);
			pBuf += pTag->rawData.nDataSize;
		}
	}

	//// tag��ĸ���
	//unsigned short nTagNum = pTagGroup->vecTag.size();
	//memcpy(pBuf, &nTagNum, sizeof(unsigned short));
	//pBuf += sizeof(unsigned short);

	//// tag1������1Byte��+tag1ֵ�������������Ͷ�����.....
	//vector<CVTAG *>::iterator itTag = pTagGroup->vecTag.begin();
	//for(; itTag != pTagGroup->vecTag.end(); itTag ++)
	//{
	//	CVTAG *pTag = (CVTAG *)*itTag;
	//	// tag����
	//	*pBuf ++ = (unsigned char )pTag->uQuality;
	//	// tagֵ����
	//	memcpy(pBuf, pTag->rawData.pszData, pTag->rawData.nDataSize);
	//	pBuf += pTag->rawData.nDataSize;
	//}

	long lRet = SendToServer(szRegisterBuf, pBuf - szRegisterBuf);
	return 0;
}

int CCycleReader::GetGroupsNum()
{
	return m_vecTagGroup.size();
}

// ע�����е�tag�����
int CCycleReader::RegisterAllGroups()
{
	long lRetFail = 0;
	vector<TAGGROUP *>::iterator itGroup = m_vecTagGroup.begin();  
	for(; itGroup != m_vecTagGroup.end(); itGroup ++)
	{
		long lRet = RegisterAGroup((TAGGROUP *) *itGroup);
		// ע�����е���
		if(lRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterAGroup() failed, retcode:%d, will continue...", lRet);
			lRetFail --;
		}
		else
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "RegisterAGroup() success");
	}
	return lRetFail;
}

// ע��һ��tag��
int CCycleReader::RegisterAGroup(TAGGROUP *pTagGroup)
{
	char szRegisterBuf[MAX_SENDBUF_LEN] = {0}; // һ��������󳤶�Ϊ1024
	char *pBuf = szRegisterBuf;
	// ����ID
	*pBuf ++ = COMMAND_ID_REGISTER_GROUP;
	//// ���ص�ID
	//memcpy(pBuf, m_szGatewayId, GATEWAY_ID_LEN);
	//pBuf += GATEWAY_ID_LEN;

	// �������ID
	memcpy(pBuf, &pTagGroup->uLocalGrpId, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);

	// �豸����ĸ���
	unsigned short nDeviceNum = 1;
	memcpy(pBuf, &nDeviceNum, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);

	//����������ÿ�������еĲ�����tag�㣩
	// hjhj �������������Լ�metername ����Ҫ���������ļ��䶯��
	for (int i = 0 ; i < nDeviceNum; i++)
	{
		// �����ID ���ɱ���ά��
		unsigned short nDeviceId = 1;
		memcpy(pBuf, &nDeviceId, sizeof(unsigned short));
		pBuf += sizeof(unsigned short);

		string metername = "Meter1";
		// �豸��������Ƴ���
		unsigned short nDeviceNameLen = metername.length();
		memcpy(pBuf, &nDeviceNameLen, sizeof(unsigned short));
		pBuf += sizeof(unsigned short);

		// ���������
		memcpy(pBuf, metername.c_str(), nDeviceNameLen);
		pBuf += nDeviceNameLen;
		// tag��ĸ���
		unsigned short nTagNum = pTagGroup->vecTag.size();
		memcpy(pBuf, &nTagNum, sizeof(unsigned short));
		pBuf += sizeof(unsigned short);

		// tag����1����+tag����1+tag��������+tag2���Ƴ���+.....
		vector<CVTAG *>::iterator itTag = pTagGroup->vecTag.begin();
		for(; itTag != pTagGroup->vecTag.end(); itTag ++)
		{
			CVTAG *pTag = (CVTAG *)*itTag;
			// tag����
			unsigned short nTagNameLen = pTag->strTagName.length();
			memcpy(pBuf, &nTagNameLen, sizeof(unsigned short));
			pBuf += sizeof(unsigned short);
			// tag��������
			memcpy(pBuf, pTag->strTagName.c_str(), nTagNameLen);
			pBuf += nTagNameLen;
			// tag��������
			*pBuf ++ = pTag->nDataType;
		}

		pTagGroup->lRequestWriteCount++;
		time(&pTagGroup->tmLastRequest);
	}

	long lRet = SendToServer(szRegisterBuf, pBuf - szRegisterBuf);
	return lRet;
}

// ע�����е�tag�����
int CCycleReader::UnRegisterAllGroups()
{
	char szRegisterBuf[MAX_SENDBUF_LEN] = {0}; // һ��������󳤶�Ϊ1024
	char *pBuf = szRegisterBuf;
	// ����ID
	*pBuf ++ = COMMAND_ID_UNREGISTER_ALL;
	// ���ص�ID
	memcpy(pBuf, m_szGatewayId, GATEWAY_ID_LEN);
	pBuf += GATEWAY_ID_LEN;
	// ����˷��ص����ID
	unsigned short lServerGrpHandle = 0; // 0��ʾע�����е���
	memcpy(pBuf, &lServerGrpHandle, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);
	
	long lRet = SendToServer(szRegisterBuf, pBuf - szRegisterBuf);
	return lRet;
}

// ע��һ��tag��
int CCycleReader::UnRegisterAGroup(TAGGROUP *pTagGroup)
{
	return 0;
}

// ���͸���������������Ͽ�����
long CCycleReader::SendToServer(char * szPackBodyBuf, unsigned short nPackLen )
{
	// ��֯������
	unsigned char szPackBuf[MAX_SENDBUF_LEN] = {0}; // һ��������󳤶�Ϊ1024
	unsigned char *pBuf = szPackBuf;
	*pBuf ++ = 0xAB;

	nPackLen += 1; // ����ţ������Ҫ+1
	memcpy(pBuf, &nPackLen, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);
	// �����ID
	*pBuf++ = ++m_nTransId;

	memcpy(pBuf, szPackBodyBuf, nPackLen);
	pBuf += nPackLen;

	*pBuf ++ = 0xED;
	// ��������
	long lRet = m_sockHandler.SendDataToServer((char *)szPackBuf, pBuf - szPackBuf);
	return lRet;
}

long CCycleReader::OnRegisterGroupResponse(unsigned short uLocalGrpId, unsigned short uServerGrpId)
{
	bool bFound = false;
	vector<TAGGROUP *>::iterator itGrp = m_vecTagGroup.begin();
	for(;itGrp != m_vecTagGroup.end(); itGrp ++)
	{
		TAGGROUP *pGrp = (TAGGROUP *)*itGrp;
		if(pGrp->uLocalGrpId == uLocalGrpId)
		{
			pGrp->uServerGrpHandle = uServerGrpId;
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "��ID: %d ע�᷵�ط���˺���: %d", uLocalGrpId, uServerGrpId);
			bFound = true;
			// ���·���˵���Ӧʱ��
			time(&pGrp->tmLastResponse);
			break;
		}
	}


	if(!bFound)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "��ID: %d �����ע�᷵�غ���: %dδ�ҵ��ͻ���ID", uLocalGrpId, uServerGrpId);
		return -1;
	}
	return 0;
}

long CCycleReader::OnUnRegisterAllGroupResponse(unsigned short uServerGrpId)
{
	bool bFound = false;
	vector<TAGGROUP *>::iterator itGrp = m_vecTagGroup.begin();
	for(;itGrp != m_vecTagGroup.end(); itGrp ++)
	{
		TAGGROUP *pGrp = (TAGGROUP *)*itGrp;
		pGrp->uServerGrpHandle = 0; 
		pGrp->tmLastRequest = pGrp->tmLastResponse = 0;
		pGrp->lRequestWriteCount = pGrp->lSucessWriteCount = 0;
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "�����ע����������÷���");
 
	return 0;
}

long CCycleReader::OnWriteGroupResponse(unsigned short uServerGrpId, unsigned char nStatus )
{
	bool bFound = false;
	vector<TAGGROUP *>::iterator itGrp = m_vecTagGroup.begin();
	for(;itGrp != m_vecTagGroup.end(); itGrp ++)
	{
		TAGGROUP *pGrp = (TAGGROUP *)*itGrp;
		if(	pGrp->uServerGrpHandle == uServerGrpId)
		{
			pGrp->lSucessWriteCount ++;
			time(&pGrp->tmLastResponse);

			if(nStatus == 0)
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "���յ������(�������:%d)����ȷ�������ݵ�Ӧ��", pGrp->uServerGrpHandle);
			else
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "���յ������(�������:%d)����������ݵ�Ӧ��, ����״̬:%d, ������ע�����", pGrp->uServerGrpHandle, nStatus);
				RegisterAGroup(pGrp);
			}

			return 0;
		}
	}


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
	*pSendBuf = PROTOCOL_GW_TO_SERVER_REGISTERGW;
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
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"����ע������[%s]����ɹ�", SYSTEM_CONFIG->m_strGatewayName.c_str());
		return 0;
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"����ע������[%s]����ʧ��", SYSTEM_CONFIG->m_strGatewayName.c_str());
		return -1;
	}
}

long CCycleReader::OnRegisterToServerResponse(unsigned char cRetCode, time_t uServerGrpId)
{
	return 0;
}

