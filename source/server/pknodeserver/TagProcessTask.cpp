/**************************************************************
*  Filename:    TaskGroup.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �豸����Ϣʵ����.
*
*  @author:     shijunpu
*  @version     05/28/2008  shijunpu  Initial Version
**************************************************************/
#include "ace/High_Res_Timer.h"
#include "MainTask.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "TagProcessTask.h"
#include "shmqueue/ProcessQueue.h"
#include <stdlib.h>
#include <cmath>
extern	CPKLog			g_logger;
extern CProcessQueue * g_pQueData2NodeSrv;
int TagValFromString2Bin(const char *szTagName, int nTagDataType, const char *szStrData, char *szBinData, int nBinDataBufLen, int *pnResultBinDataBytes);

/**
*  ���캯��.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CTagProcessTask::CTagProcessTask()
{
    ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
    m_pReactor = new ACE_Reactor(pSelectReactor, true);
    reactor(m_pReactor);

	m_predefTagHandler.m_pTagprocessTask = this;
	m_simTagHandler.m_pTagProcessTask = this;
	m_memTagHandler.m_pTimerTask = this;

	m_nQueRecBufferLen = MAIN_TASK->m_nNodeServerQueSize / 2;
	if (m_nQueRecBufferLen < 1 * 1024 * 1024)
		m_nQueRecBufferLen = 1 * 1024 * 1024;
	m_pQueRecBuffer = new char[m_nQueRecBufferLen];

	m_nTagVTQBufferLen = MAIN_TASK->m_nNodeServerQueSize / 2;	// 1��TAG����ڴ��С
	if (m_nTagVTQBufferLen < 100 * 1024)
		m_nTagVTQBufferLen = 100 * 1024;
	m_pTagVTQBuffer = new char[m_nTagVTQBufferLen];	// 1��TAG����ڴ�ָ��

	m_pTagValueBuffer = new char[m_nTagVTQBufferLen];	// 1��TAG����ڴ�ָ��
	m_nTagValueBufferLen = m_nTagVTQBufferLen;	// 1��TAG����ڴ��С;
}

/**
*  ��������.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CTagProcessTask::~CTagProcessTask()
{
    // ��������豸ɾ��֮�󣬷����豸�����ݿ��е�ACE_Event_Handler�����reator��Cancel_timer������reactor��ɾ�����쳣
    if (m_pReactor)
    {
        delete m_pReactor;
        m_pReactor = NULL;
    }
}

/**
*  �����߳�
*
*
*  @version     07/09/2008  shijunpu  Initial Version.
*/
int CTagProcessTask::Start()
{
    m_bExit = false;
    int nRet = this->activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
    if (0 == nRet)
        return PK_SUCCESS;

    return nRet;
}

void CTagProcessTask::Stop()
{
    this->m_bExit = true;
	m_predefTagHandler.StopTimer();
	m_simTagHandler.StopTimer();

    this->reactor()->end_reactor_event_loop(); // ֹͣ��Ϣѭ��

    this->wait();
    g_logger.LogMessage(PK_LOGLEVEL_WARN, "TagProcessTask thread exited! ");
}

int CTagProcessTask::svc()
{
    this->reactor()->owner(ACE_OS::thr_self());

    OnStart();
    while (!m_bExit)
    {
        this->reactor()->reset_reactor_event_loop();
        this->reactor()->run_reactor_event_loop();
    }

    OnStop();

    return 0;
}

// Group�߳�����ʱ
void CTagProcessTask::OnStart()
{
	m_predefTagHandler.InitTags();
	m_simTagHandler.InitTags();
	m_memTagHandler.InitTags();
	
	m_predefTagHandler.StartTimer();
	m_simTagHandler.StartTimer();
	m_memTagHandler.StartTimer();
}

// Group�߳�ֹͣʱ
void CTagProcessTask::OnStop()
{
    m_pReactor->close();
}

// Ͷ��һ������������豸��������
int CTagProcessTask::PostWriteTagCmd(CDataTag *pTag, string strTagValue)
{
    int nCtrlMsgLen = sizeof(int) + sizeof(int) + sizeof(int) + strTagValue.length();
    ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

    int nCmdCode = 0;
    // ������������
    memcpy(pMsg->wr_ptr(), &nCmdCode, sizeof(int));
    pMsg->wr_ptr(sizeof(int));

	char szTmp[100] = { 0 };
	// TAG�ڴ��ַ
	sprintf(szTmp, "%d", pTag);
	int nPointer = ::atoi(szTmp); //reinterpret_cast<int>(pNewTaskGroup);
	memcpy(pMsg->wr_ptr(), (void *)&nPointer, sizeof(PKTAG *));
	pMsg->wr_ptr(sizeof(PKTAG *));

	// ����Ⱥ�����
	int nTagDataLen = strTagValue.length();
	memcpy(pMsg->wr_ptr(), &nTagDataLen, sizeof(int));
	pMsg->wr_ptr(sizeof(int));

	// TAGֵ
	if (nTagDataLen > 0)
	{
		memcpy(pMsg->wr_ptr(), strTagValue.c_str(), nTagDataLen);
		pMsg->wr_ptr(nTagDataLen);
	}

    putq(pMsg);
    this->reactor()->notify(this, ACE_Event_Handler::WRITE_MASK);
	return 0;
}

/**
*  $(Desp) .
*  $(Detail) .
*
*  @param		-[in]  ACE_HANDLE fd : [comment]
*  @return		int.
*
*  @version	11/20/2012  shijunpu  Initial Version.
*  @version	8/20/2012  shijunpu  �޸��ж϶������Ƿ��������ж��������󣬵����ж����Ϣʱ�������������һ����Ϣ.
*/
int CTagProcessTask::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/)
{
    ACE_Message_Block *pMsgBlk = NULL;
    ACE_Time_Value tvTimes = ACE_OS::gettimeofday();
    while (getq(pMsgBlk, &tvTimes) >= 0)
    {
        int nCmdCode = 0;
        memcpy(&nCmdCode, pMsgBlk->rd_ptr(), sizeof(int));
        pMsgBlk->rd_ptr(sizeof(int));
        if (nCmdCode == 0)
        {
			int nPointer = 0;
            // �豸��ַ
            memcpy(&nPointer, pMsgBlk->rd_ptr(), sizeof(CDataTag *));
            pMsgBlk->rd_ptr(sizeof(int));
			CDataTag *pTag = (CDataTag *)nPointer;

			char szTagData[2048] = { 0 };
			int nTagDataLen = 0;
			memcpy(&nTagDataLen, pMsgBlk->rd_ptr(), sizeof(int));
            pMsgBlk->rd_ptr(sizeof(int));
			if (nTagDataLen >= sizeof(szTagData) - 1)
				nTagDataLen = sizeof(szTagData) - 1;
			if (nTagDataLen > 0)
            {
				memcpy(szTagData, pMsgBlk->rd_ptr(), nTagDataLen);
				pMsgBlk->rd_ptr(nTagDataLen);
				szTagData[nTagDataLen] = '\0';
            }
			if (pTag->nTagType == VARIABLE_TYPE_MEMVAR)
				m_memTagHandler.WriteTag(pTag, szTagData, nTagDataLen);
			else if (pTag->nTagType == VARIABLE_TYPE_SIMULATE)
				m_simTagHandler.WriteTag(pTag, szTagData, nTagDataLen);
			else if (pTag->nTagType == VARIABLE_TYPE_PREDEFINE)
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Ԥ������� %s ��֧�ֿ���ָ��", pTag->strName.c_str());
			}
			else
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���� %s, ����:%d, δ֪,��֧�ֿ���ָ��", pTag->strName.c_str(), pTag->nTagType);
			}
        }
        
        pMsgBlk->release();
    }

    return 0;
}

// CDataTag�д�ŵĶ�Ӧ�����ַ�����ʾ������
int CTagProcessTask::SendTagsData(vector<CDataTag *> &vecTags)
{
	if (vecTags.size() <= 0)
		return 0;

	CProcessQueue *pQue = g_pQueData2NodeSrv;
	if (!pQue)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "SendTagsData, ��ȡ�������ʧ��, Ҫ���͸���:%d", vecTags.size());
		return -1;
	}

	// ȡ��ʱ����Ϣ
	unsigned int nCurSec = 0;
	unsigned int nCurMSec22 = 0;
	nCurSec = PKTimeHelper::GetHighResTime(&nCurMSec22);
	unsigned short nCurMSec = nCurMSec;
	int nRet = 0;
	int nFailTagCount = 0;
	int nSuccessTagCount = 0;
	//m_nQueRecBufferLen; //ÿ��Que��һ����¼�Ĵ�С
	char *szQueRecBuffer = m_pQueRecBuffer;
	char *pQueBufP = m_pQueRecBuffer;

	char *pQueBufEnd = szQueRecBuffer + m_nQueRecBufferLen; // ָ��β��

	// ����ʱ���
	memcpy(pQueBufP, &nCurSec, sizeof(unsigned int));
	pQueBufP += sizeof(unsigned int);
	memcpy(pQueBufP, &nCurMSec, sizeof(unsigned short));
	pQueBufP += sizeof(unsigned short);

	// ��������
	int nActionType = ACTIONTYPE_DRV2SERVER_SET_TAGS_VTQ;
	memcpy(pQueBufP, &nActionType, sizeof(nActionType));
	pQueBufP += sizeof(int); // ����acitontype

	//memcpy(pQueBufP, &nActionType, sizeof(nActionType));
	pQueBufP += sizeof(int); // ����tag����
	int nTagNumThisPack = 0;
	for (int iTag = 0; iTag < vecTags.size(); iTag++)
	{
		CDataTag *pTag = vecTags[iTag];
		char *pszTagValue = NULL;
		int nTagValueLen = 0;
		if (pTag->nTagType == VARIABLE_TYPE_SIMULATE)
		{
			CSimTag *pSimTag = (CSimTag *)pTag;
			pszTagValue = (char *)pSimTag->m_strSimValue.c_str();
		}
		else if (pTag->nTagType == VARIABLE_TYPE_MEMVAR)
		{
			CMemTag *pMemTag = (CMemTag *)pTag;
			pszTagValue = (char *)pMemTag->m_strMemValue.c_str();
		}
		else if (pTag->nTagType == VARIABLE_TYPE_PREDEFINE)
		{
			CPredefTag *pPredefTag = (CPredefTag *)pTag;
			pszTagValue = (char *)pPredefTag->m_strPredefValue.c_str();
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, ("send tagdata,tag:%s tagtype:%d invalid"), pTag->strName.c_str(), pTag->nTagType);
			continue;
		}

		char *szBinData = m_pTagValueBuffer;	// 1��TAG����ڴ�ָ��
		TagValFromString2Bin(pTag->strName.c_str(), pTag->nDataType, pszTagValue, szBinData, m_nTagValueBufferLen, &nTagValueLen);

		char *szOneTagBuf = m_pTagVTQBuffer; //ÿ��Que��һ����¼�Ĵ�С
		int nOneTagBufLen = GetTagDataBuffer(pTag, szOneTagBuf, m_nTagVTQBufferLen, nCurSec, nCurMSec, szBinData, nTagValueLen);
		if (nOneTagBufLen + pQueBufP > pQueBufEnd) // һ�ηŲ����ˣ���Ҫ�ȷ�����
		{
			memcpy(szQueRecBuffer + sizeof(unsigned int) + sizeof(unsigned short) + sizeof(int), &nTagNumThisPack, sizeof(int)); // ����tag����
			int nRet = pQue->EnQueue(szQueRecBuffer, pQueBufP - szQueRecBuffer);
			if (nRet == PK_SUCCESS)
			{
				g_logger.LogMessage(PK_LOGLEVEL_DEBUG, ("send tagdata success, tagcount:%d, len:%d"), nTagNumThisPack, pQueBufP - szQueRecBuffer);
			}
			else
			{
				g_logger.LogMessage(PK_LOGLEVEL_ERROR, ("send tagdata to pbserver failed,len:%d, ret:%d"), pQueBufP - szQueRecBuffer, nRet);
				nFailTagCount += nTagNumThisPack;
			}
			// ���¿�ʼ
			nTagNumThisPack = 0;
			pQueBufP = szQueRecBuffer;
			pQueBufP += sizeof(unsigned int); // ����ʱ�����
			pQueBufP += sizeof(unsigned short); // ����ʱ�������
			pQueBufP += sizeof(int); // ����acitontype
			pQueBufP += sizeof(int); // ����tag����

			// ��ǰ���һ������δ���ͣ���Ҫ�ȷŽ�ȥ
			nTagNumThisPack++;
			memcpy(pQueBufP, szOneTagBuf, nOneTagBufLen);
			pQueBufP += nOneTagBufLen;
		}
		else // ��������û���꣬�ɷ��ڰ���һ����
		{
			nTagNumThisPack++;
			memcpy(pQueBufP, szOneTagBuf, nOneTagBufLen);
			pQueBufP += nOneTagBufLen;
		}
	}

	// �������һ����������еĻ�
	if (pQueBufP - szQueRecBuffer > sizeof(int) * 2)
	{
		memcpy(szQueRecBuffer + sizeof(unsigned int) + sizeof(unsigned short) + sizeof(int), &nTagNumThisPack, sizeof(int)); // ����tag����,ǰ����ʱ�����ActionType
		int nRet = pQue->EnQueue(szQueRecBuffer, pQueBufP - szQueRecBuffer);
		if (nRet == PK_SUCCESS)
		{
			nSuccessTagCount += nTagNumThisPack;
			g_logger.LogMessage(PK_LOGLEVEL_DEBUG, ("send tagdata to pbserver success, tagcount:%d, len:%d"), nTagNumThisPack, pQueBufP - szQueRecBuffer);
		}
		else
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, ("send tagdata to pbserver failed, len:%d, ret:%d"), pQueBufP - szQueRecBuffer, nRet);
			nFailTagCount += nTagNumThisPack;
		}
	}

	if (nFailTagCount > 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��������, �ɹ�tag����:%d, ʧ�ܸ���:%d", nSuccessTagCount, nFailTagCount);
	else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "��������, �ɹ�tag����:%d, ʧ�ܸ���:%d", nSuccessTagCount, nFailTagCount);
	return 0;
}

// �õ�tag�����Ϣ���������Ա��������pbserver
// ���ػ���������>=0��Ч
int CTagProcessTask::GetTagDataBuffer(CDataTag *pTag, char *szTagBuffer, int nBufLen, unsigned int nCurSec, unsigned int nCurMSec, const char *szTagValue, int nTagDataLen)
{
	const char *szTagName = pTag->strName.c_str();
	int nDataType = pTag->nDataType;
	int nSec = nCurSec;
	unsigned short nMSec = nCurMSec;
	// ���ʱ��Ϊ���򷵻ص�ǰʱ��

	char *pData = szTagBuffer;

	// tag��
	int nLen = strlen(szTagName);
	memcpy(pData, &nLen, sizeof(int));
	pData += sizeof(int);
	memcpy(pData, szTagName, nLen);
	pData += nLen;

	// tag��������
	memcpy(pData, &nDataType, sizeof(int));
	pData += sizeof(int);

	// tagֵ�ĳ��Ⱥ�tagֵ����
	memcpy(pData, &nTagDataLen, sizeof(int));
	pData += sizeof(int);
	if (nTagDataLen > 0)
	{
		memcpy(pData, szTagValue, nTagDataLen);
		pData += nTagDataLen;
	}

	// tag��ͺ�����
	memcpy(pData, &nSec, sizeof(unsigned int));
	pData += sizeof(unsigned int);
	memcpy(pData, &nMSec, sizeof(unsigned short));
	pData += sizeof(unsigned short);

	// tag����
	int nQuality = pTag->curVal.nQuality;
	memcpy(pData, &nQuality, sizeof(int));
	pData += sizeof(int);

	return pData - szTagBuffer;
}

int FromBase64StrToBlob(const char *szBase64Str, char *szRetBlob, int nRetBlobBufLen, int *pnBlobActLen){
	return 0;
}

#define BITS_PER_BYTE 8
// szStrData��szBinData����ָ��ͬһƬ�ڴ�����
int TagValFromString2Bin(const char *szTagName, int nTagDataType, const char *szStrData, char *szBinData, int nBinDataBufLen, int *pnResultBinDataBytes)
{
	// �ȴ����������ַ���
	int nStrDataLen = strlen(szStrData); // �ַ����ĳ���
	if (nTagDataType == TAG_DT_TEXT)
	{
		PKStringHelper::Safe_StrNCpy(szBinData, szStrData, nBinDataBufLen);
		*pnResultBinDataBytes = nStrDataLen; // 4bytes 
		return 0;
	}

	// �ٴ���������blob
	if (nTagDataType == TAG_DT_BLOB)
	{
		int nBlobActLen = 0;
		FromBase64StrToBlob(szStrData, szBinData, nBinDataBufLen, &nBlobActLen);
		*pnResultBinDataBytes = nBlobActLen; // 4bytes
		return 0;
	}

	// szStrData��szBinData����ָ��ͬһƬ�ڴ�����
	char szStrValue[PK_NAME_MAXLEN + 1] = { 0 }; // ֻ����ֵ���ͣ�����ռ�úܶ�ռ�
	PKStringHelper::Safe_StrNCpy(szStrValue, szStrData, sizeof(szStrValue));
	memset(szBinData, 0, nBinDataBufLen);

	// ������ֵ���ַ���ת��Ϊ����������
	int nDataBytes = 0;
	int nDataBits = 0;
	stringstream ssValue;
	switch (nTagDataType){
	case TAG_DT_BOOL:
	{
		unsigned char bValue = ::atoi(szStrValue) != 0; // 0 or 1
		memcpy(szBinData, &bValue, sizeof(unsigned char));
		nDataBits = 1;
		nDataBytes = 1;
		break;
	}
	case TAG_DT_CHAR:
	{
		char cValue = szStrValue[0]; // �ַ�
		memcpy(szBinData, &cValue, sizeof(char));
		nDataBits = BITS_PER_BYTE;
		nDataBytes = 1;
		break;
	}
	case TAG_DT_UCHAR:
	{
		unsigned char ucValue = szStrValue[0]; // �ַ�
		memcpy(szBinData, &ucValue, sizeof(unsigned char));
		nDataBits = BITS_PER_BYTE;
		nDataBytes = 1;
		break;
	}
	case TAG_DT_INT16:
	{
		short sValue = ::atoi(szStrValue); // signed short
		memcpy(szBinData, &sValue, sizeof(short));
		nDataBits = sizeof(short) * BITS_PER_BYTE;
		nDataBytes = sizeof(short);
		break;
	}
	case TAG_DT_UINT16:
	{
		unsigned short usValue = ::atoi(szStrValue); // signed short
		memcpy(szBinData, &usValue, sizeof(unsigned short));
		nDataBits = sizeof(unsigned short) * BITS_PER_BYTE;
		nDataBytes = sizeof(unsigned short);
		break;
	}
	case TAG_DT_INT32:
	{
		ACE_INT32 lValue = ::atol(szStrValue); // signed short
		memcpy(szBinData, &lValue, sizeof(ACE_INT32));
		nDataBits = sizeof(ACE_INT32) * BITS_PER_BYTE; // 4bytes
		nDataBytes = sizeof(ACE_INT32);
		break;
	}
	case TAG_DT_UINT32:
	{
		ACE_UINT32 ulValue = 0; // signed short
		sscanf(szStrValue, "%u", ulValue);
		memcpy(szBinData, &ulValue, sizeof(ACE_UINT32));
		nDataBits = sizeof(ACE_UINT32) * BITS_PER_BYTE; // 4bytes
		nDataBytes = sizeof(ACE_UINT32);
		break;
	}
	case TAG_DT_INT64:
	{
		ACE_INT64 int64Value = 0;
		ssValue << szStrValue;
		ssValue >> int64Value;
		memcpy(szBinData, &int64Value, sizeof(ACE_INT64));
		nDataBits = sizeof(ACE_INT64) * BITS_PER_BYTE;
		nDataBytes = sizeof(ACE_INT64);
		break;
	}
	case TAG_DT_UINT64:
	{
		ACE_UINT64 uint64Value = 0;
		ssValue << szStrValue;
		ssValue >> uint64Value;
		memcpy(szBinData, &uint64Value, sizeof(ACE_UINT64));
		nDataBits = sizeof(ACE_UINT64) * BITS_PER_BYTE;
		nDataBytes = sizeof(ACE_UINT64);
		break;
	}
	case TAG_DT_FLOAT:
	{
		float fValue = ::atof(szStrValue); // signed short
		memcpy(szBinData, &fValue, sizeof(float));
		nDataBits = sizeof(float) * BITS_PER_BYTE; // 4bytes
		nDataBytes = sizeof(float);
		break;
	}
	case TAG_DT_DOUBLE:
	{
		double fValue = ::atof(szStrValue); // signed short
		memcpy(szBinData, &fValue, sizeof(double));
		nDataBits = sizeof(double) * BITS_PER_BYTE; // 4bytes
		nDataBytes = sizeof(double);
		break;
	}
	default:
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "TAG(%s) unknown datatype(typeid:%d) in TagValFromString2Bin", szTagName, nTagDataType);
		*pnResultBinDataBytes = 0;
		nDataBytes = 0;
		return -1;
	}
	*pnResultBinDataBytes = nDataBytes;
	return 0;
}
