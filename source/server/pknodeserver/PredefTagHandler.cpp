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
*  构造函数.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CPredefTagHandler::CPredefTagHandler()
{
	m_pTagprocessTask = NULL;
}

/**
*  析构函数.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CPredefTagHandler::~CPredefTagHandler()
{
}

// 初始化,主要是初值
int CPredefTagHandler::InitTags()
{
	// 定时读取所有模拟点的数据，进行将数据发送回去
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

// 按照
void CPredefTagHandler::StartTimer()
{
	ACE_Time_Value	tvPollRate;		// 扫描周期，单位ms
	tvPollRate.msec(1000);			// 模拟数值，每秒变化一次
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
	// 定时读取所有模拟点的数据，进行将数据发送回去
	vector<CDataTag *> vecMemTag;
	map<string, CPredefTag *>::iterator itTag = MAIN_TASK->m_mapName2PredefTag.begin();
	for (; itTag != MAIN_TASK->m_mapName2PredefTag.end(); itTag++)
	{
		vecMemTag.push_back(itTag->second);
	}

	m_pTagprocessTask->SendTagsData(vecMemTag);
	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "发送 %d 个预定义变量的值!", MAIN_TASK->m_mapName2PredefTag.size());

	vecMemTag.clear();
	return 0;
}

int CPredefTagHandler::WriteTag(CDataTag *pTag, char *szTagData, int nTagDataLen)
{
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "执行控制失败,预定义变量不能被控制:%s,value:%s!", pTag->strName.c_str(), szTagData);
	return 0;
}

