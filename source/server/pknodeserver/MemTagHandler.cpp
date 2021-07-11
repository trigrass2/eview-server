/**************************************************************
**************************************************************/
#include "MainTask.h"
#include "MemTagHandler.h"
#include "NodeCommonDef.h"
#include "TagProcessTask.h"
#include "RedisFieldDefine.h"

#include <string>
extern CPKLog g_logger;
using namespace std;

/**
*  ���캯��.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CMemTagHandler::CMemTagHandler()
{
	m_pTimerTask = NULL;
}

/**
*  ��������.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CMemTagHandler::~CMemTagHandler()
{
}

// ��ʼ��,��Ҫ�ǳ�ֵ
int CMemTagHandler::InitTags()
{
	// ��ʱ��ȡ����ģ�������ݣ����н����ݷ��ͻ�ȥ
	map<string, CMemTag *>::iterator itTag = MAIN_TASK->m_mapName2MemTag.begin();
	for (; itTag != MAIN_TASK->m_mapName2MemTag.end(); itTag++)
	{
		CMemTag *pMemTag = itTag->second;
		pMemTag->m_strMemValue = pMemTag->strInitValue;
		pMemTag->curVal.nQuality = 0;
	}
	return 0;
}

// ����
void CMemTagHandler::StartTimer()
{
	ACE_Time_Value	tvPollRate;		// ɨ�����ڣ���λms
	tvPollRate.msec(1000*60);	// ģ����ֵ��ÿ60��仯һ��
	ACE_Time_Value tvPollPhase;
	tvPollPhase.msec(0);
	tvPollPhase += ACE_Time_Value::zero;
	m_pTimerTask->m_pReactor->schedule_timer((ACE_Event_Handler *)(CMemTagHandler *)this, NULL, tvPollPhase, tvPollRate);
}

void CMemTagHandler::StopTimer()
{
	m_pTimerTask->m_pReactor->cancel_timer((ACE_Event_Handler *)(CMemTagHandler *)this);
}

int CMemTagHandler::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	if (MAIN_TASK->m_mapName2MemTag.size() <= 0)
		return 0;

	// ��ʱ��ȡ����ģ�������ݣ����н����ݷ��ͻ�ȥ
	vector<CDataTag *> vecMemTag;
	map<string, CMemTag *>::iterator itTag = MAIN_TASK->m_mapName2MemTag.begin();
	for (; itTag != MAIN_TASK->m_mapName2MemTag.end(); itTag++)
	{
		vecMemTag.push_back(itTag->second);
	}

	m_pTimerTask->SendTagsData(vecMemTag);
	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���� %d ���ڴ������ֵ!", MAIN_TASK->m_mapName2MemTag.size());

	vecMemTag.clear();
	return 0;
}

int CMemTagHandler::WriteTag(CDataTag *pTag, char *szTagData, int nTagDataLen)
{
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "ִ�п��Ƴɹ�,�ڴ����: %s, ֵ: %s!", pTag->strName.c_str(), szTagData);

	CMemTag *pMemTag = (CMemTag *)pTag;
	pMemTag->m_strMemValue = szTagData; // ����޸���һ��û��Ч��
	pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, szTagData, 0); // �������Ч��
	
	if (strcmp(szTagData, TAG_QUALITY_INIT_STATE_STRING) == 0) // ?
	{
		pMemTag->curVal.nQuality = TAG_QUALITY_INIT_STATE;
	}
	else if (strcmp(szTagData, TAG_QUALITY_DRIVER_NOT_ALIVE_STRING) == 0) // *
	{
		pMemTag->curVal.nQuality = TAG_QUALITY_DRIVER_NOT_ALIVE;
	}
	else if (strcmp(szTagData, TAG_QUALITY_COMMUNICATE_FAILURE_STRING) == 0) // *
	{
		pMemTag->curVal.nQuality = TAG_QUALITY_COMMUNICATE_FAILURE;
	}
	else if (strcmp(szTagData, TAG_QUALITY_UNKNOWN_REASON_STRING) == 0) // *
	{
		pMemTag->curVal.nQuality = TAG_QUALITY_UNKNOWN_REASON;
	}
	else
	{
		pTag->curVal.AssignStrValueAndQuality(pTag->nDataTypeClass, szTagData, TAG_QUALITY_GOOD);
		//pMemTag->curVal.nQuality = TAG_QUALITY_GOOD;
	}

	// �������͸�����
	vector<CDataTag *> vecTags;
	vecTags.push_back(pTag);
	m_pTimerTask->SendTagsData(vecTags);
	vecTags.clear();

	return 0;
}

