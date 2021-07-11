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
*  构造函数.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CSimTagHandler::CSimTagHandler()
{
	m_pTagProcessTask = NULL;
}

/**
*  析构函数.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CSimTagHandler::~CSimTagHandler()
{
}

// 初始化,主要是初值
int CSimTagHandler::InitTags()
{
	// 定时读取所有模拟点的数据，进行将数据发送回去
	map<string, CSimTag *>::iterator itTag = MAIN_TASK->m_mapName2SimTag.begin();
	for (; itTag != MAIN_TASK->m_mapName2SimTag.end(); itTag++)
	{
		CSimTag *pTag = itTag->second;
		pTag->m_bAutoInc = true; // 刚开始时认为是自增的，因为数据库SIMULATE字段肯定不是空
		pTag->m_dbSimIncStep = ::atof(pTag->m_strDeviceSimValue.c_str());
		if (pTag->nDataTypeClass == DATATYPE_CLASS_INTEGER || pTag->nDataTypeClass == DATATYPE_CLASS_REAL)
			pTag->m_dbSimValue = ::atof(pTag->strInitValue.c_str());
		else
			pTag->m_strSimValue = pTag->strInitValue;
		pTag->curVal.nQuality = 0;
	}
	return 0;
}

// 按照
void CSimTagHandler::StartTimer()
{
	ACE_Time_Value	tvPollRate;		// 扫描周期，单位ms
	tvPollRate.msec(1000);			// 模拟数值，每秒变化一次
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
	// 定时读取所有模拟点的数据，进行将数据发送回去
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
					sprintf(szValue, "text%d", (int)pTag->m_dbSimValue); // 无缺省值则为：text1234
				else
					sprintf(szValue, "%s%d", pTag->strInitValue.c_str(),(int)pTag->m_dbSimValue); // initValue1234
			}
			pTag->m_strSimValue = szValue;
		}
		vecTags.push_back(pTag);
	}

	// 立即发送给服务
	m_pTagProcessTask->SendTagsData(vecTags);
	vecTags.clear();
	g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "发送 %d 个模拟变量的值!", MAIN_TASK->m_mapName2SimTag.size());
	return 0;
}

int CSimTagHandler::WriteTag(CDataTag *pTag, char *szTagData, int nTagDataLen)
{
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "执行控制成功,模拟变量: %s, 值: %s!", pTag->strName.c_str(), szTagData);

	CSimTag *pSimTag = (CSimTag *)pTag;
	pSimTag->m_bAutoInc = false; // 不再允许自增

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

	// 立即发送给服务
	vector<CDataTag *> vecTags;
	vecTags.push_back(pTag);
	m_pTagProcessTask->SendTagsData(vecTags);
	vecTags.clear();
	return 0;
}

