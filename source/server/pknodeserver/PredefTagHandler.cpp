/**************************************************************
**************************************************************/
#include "MainTask.h"
#include "PredefTagHandler.h"
#include "NodeCommonDef.h"
#include "TagProcessTask.h"
#include <string>
extern CPKLog g_logger;
using namespace std;

/**
*  ���캯��.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CPredefTagHandler::CPredefTagHandler()
{
	m_pTagprocessTask = NULL;
}

/**
*  ��������.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CPredefTagHandler::~CPredefTagHandler()
{
}

// ��ʼ��,��Ҫ�ǳ�ֵ
int CPredefTagHandler::InitTags()
{
	// ��ʱ��ȡ����ģ�������ݣ����н����ݷ��ͻ�ȥ
	map<string, CPredefTag *>::iterator itTag = MAIN_TASK->m_mapName2PredefTag.begin();
	for (; itTag != MAIN_TASK->m_mapName2PredefTag.end(); itTag++)
	{
		CPredefTag *pTag = itTag->second;
		pTag->m_strPredefValue = pTag->strInitValue;
		if (pTag->strName.compare("sys.tagnum") == 0)
		{
			char szValue[256];
			sprintf(szValue, "%d", MAIN_TASK->m_mapName2Device.size());
			pTag->m_strPredefValue = szValue;
			pTag->curVal.nQuality = 0;
		}
	}
	return 0;
}

// ����
void CPredefTagHandler::StartTimer()
{
	ACE_Time_Value	tvPollRate;		// ɨ�����ڣ���λms
	tvPollRate.msec(1000);			// ģ����ֵ��ÿ��仯һ��
	ACE_Time_Value tvPollPhase;
	tvPollPhase.msec(0);
	tvPollPhase += ACE_Time_Value::zero;
	m_pTagprocessTask->m_pReactor->schedule_timer((ACE_Event_Handler *)(CPredefTagHandler *)this, NULL, tvPollPhase, tvPollRate);
}

void CPredefTagHandler::StopTimer()
{
	m_pTagprocessTask->m_pReactor->cancel_timer((ACE_Event_Handler *)(CPredefTagHandler *)this);
}

int CPredefTagHandler::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	if (MAIN_TASK->m_mapName2PredefTag.size() <= 0)
		return 0;

	return 0;
	// ��ʱ��ȡ����ģ�������ݣ����н����ݷ��ͻ�ȥ
	vector<CDataTag *> vecMemTag;
	map<string, CPredefTag *>::iterator itTag = MAIN_TASK->m_mapName2PredefTag.begin();
	for (; itTag != MAIN_TASK->m_mapName2PredefTag.end(); itTag++)
	{
		vecMemTag.push_back(itTag->second);
	}

	m_pTagprocessTask->SendTagsData(vecMemTag);
	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���� %d ��Ԥ���������ֵ!", MAIN_TASK->m_mapName2PredefTag.size());

	vecMemTag.clear();
	return 0;
}

int CPredefTagHandler::WriteTag(CDataTag *pTag, char *szTagData, int nTagDataLen)
{
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "ִ�п���ʧ��,Ԥ����������ܱ�����:%s,value:%s!", pTag->strName.c_str(), szTagData);
	return 0;
}

