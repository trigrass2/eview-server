/**************************************************************
**************************************************************/
#include "MainTask.h"
#include "SimTagHandler.h"
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
CSimTagHandler::CSimTagHandler()
{
	m_pTagProcessTask = NULL;
}

/**
*  ��������.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CSimTagHandler::~CSimTagHandler()
{
}

// ��ʼ��,��Ҫ�ǳ�ֵ
int CSimTagHandler::InitTags()
{
	// ��ʱ��ȡ����ģ�������ݣ����н����ݷ��ͻ�ȥ
	map<string, CSimTag *>::iterator itTag = MAIN_TASK->m_mapName2SimTag.begin();
	for (; itTag != MAIN_TASK->m_mapName2SimTag.end(); itTag++)
	{
		CSimTag *pTag = itTag->second;
		pTag->m_bAutoInc = true; // �տ�ʼʱ��Ϊ�������ģ���Ϊ���ݿ�SIMULATE�ֶο϶����ǿ�
		pTag->m_dbSimIncStep = ::atof(pTag->m_strDeviceSimValue.c_str());
		if (pTag->nDataTypeClass == DATATYPE_CLASS_INTEGER || pTag->nDataTypeClass == DATATYPE_CLASS_REAL)
			pTag->m_dbSimValue = ::atof(pTag->strInitValue.c_str());
		else
			pTag->m_strSimValue = pTag->strInitValue;
		pTag->curVal.nQuality = 0;
	}
	return 0;
}

// ����
void CSimTagHandler::StartTimer()
{
	ACE_Time_Value	tvPollRate;		// ɨ�����ڣ���λms
	tvPollRate.msec(1000);			// ģ����ֵ��ÿ��仯һ��
	ACE_Time_Value tvPollPhase;
	tvPollPhase.msec(0);
	tvPollPhase += ACE_Time_Value::zero;
	m_pTagProcessTask->m_pReactor->schedule_timer((ACE_Event_Handler *)(CSimTagHandler *)this, NULL, tvPollPhase, tvPollRate);
}

void CSimTagHandler::StopTimer()
{
	m_pTagProcessTask->m_pReactor->cancel_timer((ACE_Event_Handler *)(CSimTagHandler *)this);
}

int CSimTagHandler::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	if (MAIN_TASK->m_mapName2SimTag.size() <= 0)
		return 0;
	// ��ʱ��ȡ����ģ�������ݣ����н����ݷ��ͻ�ȥ
	vector<CDataTag *> vecTags;
	map<string, CSimTag *>::iterator itTag = MAIN_TASK->m_mapName2SimTag.begin();
	for (; itTag != MAIN_TASK->m_mapName2SimTag.end(); itTag++)
	{
		CSimTag *pTag = itTag->second;
		if (pTag->m_bAutoInc)
		{
			pTag->m_dbSimValue += pTag->m_dbSimIncStep;
			char szValue[256] = { 0 };
			if (pTag->nDataTypeClass == DATATYPE_CLASS_INTEGER)
			{
				sprintf(szValue, "%d", (int)pTag->m_dbSimValue);
			}
			else if (pTag->nDataTypeClass == DATATYPE_CLASS_REAL)
			{
				sprintf(szValue, "%.2f", pTag->m_dbSimValue);
			}
			else
			{
				if (pTag->strInitValue.length() <= 0)
					sprintf(szValue, "text%d", (int)pTag->m_dbSimValue); // ��ȱʡֵ��Ϊ��text1234
				else
					sprintf(szValue, "%s%d", pTag->strInitValue.c_str(),(int)pTag->m_dbSimValue); // initValue1234
			}
			pTag->m_strSimValue = szValue;
		}
		vecTags.push_back(pTag);
	}

	// �������͸�����
	m_pTagProcessTask->SendTagsData(vecTags);
	vecTags.clear();
	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���� %d ��ģ�������ֵ!", MAIN_TASK->m_mapName2SimTag.size());
	return 0;
}

int CSimTagHandler::WriteTag(CDataTag *pTag, char *szTagData, int nTagDataLen)
{
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "ִ�п��Ƴɹ�,ģ�����: %s, ֵ: %s!", pTag->strName.c_str(), szTagData);

	CSimTag *pSimTag = (CSimTag *)pTag;
	pSimTag->m_bAutoInc = false; // ������������

	if (strcmp(szTagData, TAG_QUALITY_INIT_STATE_STRING) == 0) // ***
	{
		pSimTag->curVal.nQuality = TAG_QUALITY_INIT_STATE;
	}
	else if (strcmp(szTagData, TAG_QUALITY_DRIVER_NOT_ALIVE_STRING) == 0) // *
	{
		pSimTag->curVal.nQuality = TAG_QUALITY_DRIVER_NOT_ALIVE;
	}
	else if (strcmp(szTagData, TAG_QUALITY_COMMUNICATE_FAILURE_STRING) == 0) // *
	{
		pSimTag->curVal.nQuality = TAG_QUALITY_COMMUNICATE_FAILURE;
	}
	else if (strcmp(szTagData, TAG_QUALITY_UNKNOWN_REASON_STRING) == 0) // *
	{
		pSimTag->curVal.nQuality = TAG_QUALITY_UNKNOWN_REASON;
	}
	else
		pSimTag->curVal.nQuality = TAG_QUALITY_GOOD;

	if (pSimTag->nDataTypeClass == DATATYPE_CLASS_INTEGER || pSimTag->nDataTypeClass == DATATYPE_CLASS_REAL)
		pSimTag->m_dbSimValue = ::atof(szTagData);
	pSimTag->m_strSimValue = szTagData;

	// �������͸�����
	vector<CDataTag *> vecTags;
	vecTags.push_back(pTag);
	m_pTagProcessTask->SendTagsData(vecTags);
	vecTags.clear();
	return 0;
}

