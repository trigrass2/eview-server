/**************************************************************
 *  Filename:    SockHandler.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: SockHandler.cpp
 *
 *  @author:     shijunpu
 *  @version     05/29/2012  shijunpu  Initial Version
**************************************************************/
// 一个报警的标识由三部分组成：tagname+almcat+almindex

#include "EAProcessor.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <string>
#include "json/json.h"
#include "NodeCommonDef.h"
#include <ace/OS_NS_strings.h>
#include "RedisFieldDefine.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "MainTask.h"
#include "NodeCommonDef.h"
#include "UserAuthTask.h"
#include "PredefTagHandler.h"
#include "common/eviewcomm_internal.h"
#include <map>
#include <math.h>
using namespace std;

int CalcLHLimits(CDataTag *pTag);
int Json2String(Json::Value &json, string &strJson);
extern CPKLog g_logger;
#define ICV_FAILURE		-1					// 错误返回值

/**
 *  构造函数.
 *
 *  @param  -[in]  ACE_Reactor*  r: [comment]
 *
 *  @version     05/29/2008  shijunpu  Initial Version.
 */
CEAProcessor::CEAProcessor() 
{
	int nStatus = PK_SUCCESS;
}

/**
 *  析构函数.
 *
 *  @version     05/29/2008  shijunpu  Initial Version.
 */
CEAProcessor::~CEAProcessor()
{
	// m_mapJudgeMethodName2Fields.clear();
}

// 必须在需要的时候，删除不该有的报警
void CEAProcessor::WriteTagAlarms(CDataTag *pTag)
{
	Json::Value jsonAlarmings; // 某个tag点的当前报警状态
	GetTagAlarmsStatus(pTag, jsonAlarmings);

	if (jsonAlarmings.size() <= 0) // 这个点已经不存在报警了，需要删除!否则报警内容会变为"null"！导致java读取时候产生异常！
	{
		MAIN_TASK->m_redisRW1.del(pTag->strName.c_str());
	}
	else
	{
		string strValue = m_jsonWriter.write(jsonAlarmings); // 性能更高
		MAIN_TASK->m_redisRW1.set(pTag->strName.c_str(), strValue.c_str());
	}
}

void CEAProcessor::UpdateOneAlarmTagStatus(CAlarmingInfo *pAlarmingInfo, Json::Value &jsonAlarmsStatus)
{
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	if (pAlarmingInfo->bAlarming || (!pAlarmingInfo->bConfirmed))
	{
		string strJudgeMethod = pAlarmTag->strJudgeMethod;

		char szTmProduce[PK_TIMESTRING_MAXLEN] = {'\0'};
		char szTmConfirm[PK_TIMESTRING_MAXLEN] = {'\0'};
		char szTmRecover[PK_TIMESTRING_MAXLEN] = {'\0'};

		PKTimeHelper::HighResTime2String(szTmProduce, sizeof(szTmProduce), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
		if (pAlarmingInfo->bConfirmed) // 如果已确认才有确认时间
			PKTimeHelper::HighResTime2String(szTmConfirm, sizeof(szTmConfirm), pAlarmingInfo->tmConfirm.nSeconds, pAlarmingInfo->tmConfirm.nMilSeconds);
		if (!pAlarmingInfo->bAlarming) // 如果已恢复才有恢复时间
			PKTimeHelper::HighResTime2String(szTmRecover, sizeof(szTmRecover), pAlarmingInfo->tmRecover.nSeconds, pAlarmingInfo->tmRecover.nMilSeconds);

		jsonAlarmsStatus[ALARM_TAG_NAME] = pAlarmTag->pTag->strName;
		jsonAlarmsStatus[ALARM_JUDGEMUTHOD] = pAlarmTag->strJudgeMethod;
		jsonAlarmsStatus[ALARM_PRODUCETIME] = szTmProduce;
		if (pAlarmTag->pTag->pObject)
		{
			jsonAlarmsStatus[ALARM_OBJECT_NAME] = pAlarmTag->pTag->pObject->strName;
			jsonAlarmsStatus[ALARM_OBJECT_DESCRIPTION] = pAlarmTag->pTag->pObject->strDescription;
		}
		else
		{
			jsonAlarmsStatus[ALARM_OBJECT_NAME] = "";
			jsonAlarmsStatus[ALARM_OBJECT_DESCRIPTION] = "";
		}
		if (pAlarmTag->pTag->pObjProp) // 对象模式才有
			jsonAlarmsStatus[ALARM_PROP_NAME] = pAlarmTag->pTag->pObjProp->n_pClassProp->m_strName;
		else
			jsonAlarmsStatus[ALARM_PROP_NAME] = pAlarmTag->pTag->strName;

		jsonAlarmsStatus[ALARM_ALARMING] = pAlarmingInfo->bAlarming;
		jsonAlarmsStatus[ALARM_SYSTEMNAME] = pAlarmTag->pTag->strSubsysName;
		jsonAlarmsStatus[ALARM_LEVEL] = pAlarmTag->nLevel;
		jsonAlarmsStatus[ALARM_TYPE] = pAlarmTag->strAlarmTypeName;
		jsonAlarmsStatus[ALARM_VALONPRODUCE] = pAlarmingInfo->strValueOnProduce;
		jsonAlarmsStatus[ALARM_VALONCONFIRM] = pAlarmingInfo->strValueOnConfirm;
		jsonAlarmsStatus[ALARM_VALONRECOVER] = pAlarmingInfo->strValueOnRecover;
		jsonAlarmsStatus[ALARM_CONFIRMED] = pAlarmingInfo->bConfirmed;
		jsonAlarmsStatus[ALARM_CONFIRMER] = pAlarmingInfo->strConfirmUserName;
		jsonAlarmsStatus[ALARM_THRESH] = pAlarmingInfo->strThresholdOnProduce;

		if (pAlarmingInfo->bConfirmed)
			jsonAlarmsStatus[ALARM_CONFIRMTIME] = szTmConfirm;
		else
			jsonAlarmsStatus[ALARM_CONFIRMTIME] = "";
		if (!pAlarmingInfo->bAlarming)
			jsonAlarmsStatus[ALARM_RECOVERTIME] = szTmRecover;
		else
			jsonAlarmsStatus[ALARM_RECOVERTIME] = "";

		jsonAlarmsStatus[ALARM_REPETITIONS] = pAlarmingInfo->nRepetitions;
		jsonAlarmsStatus[ALARM_THRESH] = pAlarmTag->strThreshold;

		jsonAlarmsStatus[ALARM_PARAM1] = pAlarmingInfo->strParam1;
		jsonAlarmsStatus[ALARM_PARAM2] = pAlarmingInfo->strParam2;
		jsonAlarmsStatus[ALARM_PARAM3] = pAlarmingInfo->strParam3;
		jsonAlarmsStatus[ALARM_PARAM4] = pAlarmingInfo->strParam4;
	}
}

// 当报警点状态发生改变之后，更新当前点的所有报警状态
int CEAProcessor::GetTagAlarmsStatus(CDataTag *pTag, Json::Value &jsonAlarmings)
{
	int nRet = 0;

	for (unsigned int i = 0; i < pTag->vecAllAlarmTags.size(); i ++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
		for (int j = 0; j < pAlarmTag->vecAlarming.size(); j++)
		{
			CAlarmingInfo * pAlarmingInfo = pAlarmTag->vecAlarming[j];
			Json::Value jsonAlarming;
			UpdateOneAlarmTagStatus(pAlarmingInfo, jsonAlarming);
			jsonAlarmings.append(jsonAlarming);
		}
		
	}

	return nRet;
}

// 判断整数类型的tag点的报警
bool CEAProcessor::JudgeAlarming_IntClass(CAlarmTag *pAlarmTag, char *szThreshold, int nThresholdLen)
{
	ACE_INT64 nCurValue = pAlarmTag->pTag->curVal.nValue;
	bool bAlarming = false;
	switch (pAlarmTag->nJudgeMethodType)
	{
	case ALARM_JUDGE_HHH:
	case ALARM_JUDGE_HH:
	case ALARM_JUDGE_H:
		bAlarming = (nCurValue >= pAlarmTag->nThreshold && nCurValue < pAlarmTag->nThreshold2); // >= min && < max
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold);
		break;
	case ALARM_JUDGE_L:
	case ALARM_JUDGE_LL:
	case ALARM_JUDGE_LLL:
		bAlarming = (nCurValue > pAlarmTag->nThreshold && nCurValue <= pAlarmTag->nThreshold2); // >min && <= max
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold2);
		break;
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_ON:
	case ALARM_JUDGE_OFF:
	case ALARM_JUDGE_EQ:
		bAlarming = (nCurValue == pAlarmTag->nThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold);
		break;
	case ALARM_JUDGE_CHANGETOEQ: // 变为这个值则认为报警
		bAlarming = (nCurValue == pAlarmTag->nThreshold);
		bAlarming = (bAlarming && pAlarmTag->pTag->lastVal.nQuality == 0 && pAlarmTag->pTag->lastVal.nValue != nCurValue);
		// 满足：等于该值，之前和现在的数据质量都是GOOD，且之前和现在的两个值不等
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold);
		break;
	case ALARM_JUDGE_QUALITYBAD:
		bAlarming = (pAlarmTag->pTag->curVal.nQuality != 0);
		break;
	case ALARM_JUDGE_CHANGETOQUALITYBAD:
		bAlarming = (pAlarmTag->pTag->curVal.nQuality != 0 &&  pAlarmTag->pTag->lastVal.nQuality == 0);
		break;
	case ALARM_JUDGE_NE:
		bAlarming = (nCurValue != pAlarmTag->nThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold);
		break;
	case ALARM_JUDGE_GT:
		bAlarming = (nCurValue > pAlarmTag->nThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold);
		break;
	case ALARM_JUDGE_GTE:
		bAlarming = (nCurValue >= pAlarmTag->nThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold);
		break;
	case ALARM_JUDGE_LT:
		bAlarming = (nCurValue < pAlarmTag->nThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold);
		break;
	case ALARM_JUDGE_LTE:
		bAlarming = (nCurValue <= pAlarmTag->nThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", pAlarmTag->nThreshold);
		break;
	case ALARM_JUDGE_GTELT:
		bAlarming = (nCurValue >= pAlarmTag->nThreshold && nCurValue < pAlarmTag->nThreshold2);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "[%d,%d)", pAlarmTag->nThreshold, pAlarmTag->nThreshold2);
		break;
	case ALARM_JUDGE_GTLT:
		bAlarming = (nCurValue > pAlarmTag->nThreshold && nCurValue < pAlarmTag->nThreshold2);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "(%d,%d)", pAlarmTag->nThreshold, pAlarmTag->nThreshold2);
		break;
	case ALARM_JUDGE_GTLTE:
		bAlarming = (nCurValue > pAlarmTag->nThreshold && nCurValue <= pAlarmTag->nThreshold2);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "(%d,%d]", pAlarmTag->nThreshold, pAlarmTag->nThreshold2);
		break;
	case ALARM_JUDGE_GTELTE:
		bAlarming = (nCurValue >= pAlarmTag->nThreshold && nCurValue <= pAlarmTag->nThreshold2);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "[%d,%d]", pAlarmTag->nThreshold, pAlarmTag->nThreshold2);
		break;
	case ALARM_JUDGE_IN:
		for (int i = 0; i < pAlarmTag->vecThresholdInt.size(); i++)
		{
			ACE_INT64 &nThreshold = pAlarmTag->vecThresholdInt[i];
			if (nThreshold == nCurValue)
			{
				bAlarming = true;
				PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%d", nThreshold);
				break;
			}
		}
		break;
	default:
		bAlarming = false;
		break;
	}
	return bAlarming;
}

// 判断小数类型的tag点的报警，返回是否报警、报警门限设定
bool CEAProcessor::JudgeAlarming_RealClass(CAlarmTag *pAlarmTag, char *szThreshold, int nThresholdLen)
{
	double dbCurValue = pAlarmTag->pTag->curVal.dbValue;
	bool bAlarming = false;

	switch (pAlarmTag->nJudgeMethodType)
	{
	case ALARM_JUDGE_HHH:
	case ALARM_JUDGE_HH:
	case ALARM_JUDGE_H:
		bAlarming = (dbCurValue > pAlarmTag->dbThreshold && dbCurValue <= pAlarmTag->dbThreshold2); // > min && <= max
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold);
		break;
	case ALARM_JUDGE_L:
	case ALARM_JUDGE_LL:
	case ALARM_JUDGE_LLL:
		bAlarming = (dbCurValue >= pAlarmTag->dbThreshold && dbCurValue < pAlarmTag->dbThreshold2); // >=min && < max
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold2);
		break;
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_ON:
	case ALARM_JUDGE_OFF:
	case ALARM_JUDGE_EQ:
		bAlarming = fabs(dbCurValue - pAlarmTag->dbThreshold) < EPSINON;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold);
		break;
	case ALARM_JUDGE_CHANGETOEQ://// 满足：等于该值，之前和现在的数据质量都是GOOD，且之前和现在的两个值不等
		bAlarming = fabs(dbCurValue - pAlarmTag->dbThreshold) < EPSINON;
		bAlarming = (bAlarming && (pAlarmTag->pTag->lastVal.nQuality == 0 && fabs(pAlarmTag->pTag->lastVal.dbValue - dbCurValue) >= EPSINON));
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold);
		break;
	case ALARM_JUDGE_NE:
		bAlarming = fabs(dbCurValue - pAlarmTag->dbThreshold) >= EPSINON;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold);
		break;
	case ALARM_JUDGE_GT:
		bAlarming = (dbCurValue > pAlarmTag->dbThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold);
		break;
	case ALARM_JUDGE_GTE:
		bAlarming = (dbCurValue >= pAlarmTag->dbThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold);
		break;
	case ALARM_JUDGE_LT:
		bAlarming = (dbCurValue < pAlarmTag->dbThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold);
		break;
	case ALARM_JUDGE_LTE:
		bAlarming = (dbCurValue <= pAlarmTag->dbThreshold);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", pAlarmTag->dbThreshold);
		break;
	case ALARM_JUDGE_GTELT:
		bAlarming = (dbCurValue >= pAlarmTag->dbThreshold && dbCurValue < pAlarmTag->dbThreshold2);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "[%f,%f)", pAlarmTag->dbThreshold, pAlarmTag->dbThreshold2);
		break;
	case ALARM_JUDGE_GTLT:
		bAlarming = (dbCurValue > pAlarmTag->dbThreshold && dbCurValue < pAlarmTag->dbThreshold2);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "(%f,%f)", pAlarmTag->dbThreshold, pAlarmTag->dbThreshold2);
		break;
	case ALARM_JUDGE_GTLTE:
		bAlarming = (dbCurValue > pAlarmTag->dbThreshold && dbCurValue <= pAlarmTag->dbThreshold2);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "(%f,%f]", pAlarmTag->dbThreshold, pAlarmTag->dbThreshold2);
		break;
	case ALARM_JUDGE_GTELTE:
		bAlarming = (dbCurValue >= pAlarmTag->dbThreshold && dbCurValue <= pAlarmTag->dbThreshold2);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "[%f,%f]", pAlarmTag->dbThreshold, pAlarmTag->dbThreshold2);
		break;
	case ALARM_JUDGE_IN:
		for (int i = 0; i < pAlarmTag->vecThresholdReal.size(); i++)
		{
			double &dbThreshold = pAlarmTag->vecThresholdReal[i];
			if (fabs(dbCurValue - dbThreshold) < EPSINON)
			{
				bAlarming = true;
				PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%f", dbThreshold);
				break;
			}
		}
		break;
	default:
		bAlarming = false;
		break;
	}

	return bAlarming;
}

// 判断文本类型的tag点的报警
bool CEAProcessor::JudgeAlarming_TextClass(CAlarmTag *pAlarmTag, char *szThreshold, int nThresholdLen)
{
	string &strCurValue = pAlarmTag->pTag->curVal.strValue;
	bool bAlarming = false;
	
	switch (pAlarmTag->nJudgeMethodType)
	{
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_ON:
	case ALARM_JUDGE_OFF:
	case ALARM_JUDGE_EQ:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) == 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", pAlarmTag->strThreshold.c_str());
		break;
	case ALARM_JUDGE_CHANGETOEQ: // 满足：等于该值，之前和现在的数据质量都是GOOD，且之前和现在的两个值不等
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) == 0;
		bAlarming = bAlarming && pAlarmTag->pTag->lastVal.nQuality == 0 && pAlarmTag->pTag->lastVal.strValue.compare(strCurValue) != 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", pAlarmTag->strThreshold.c_str());
		break;	
	case ALARM_JUDGE_NE:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) != 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", pAlarmTag->strThreshold.c_str());
		break;
	case ALARM_JUDGE_GT:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) > 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", pAlarmTag->strThreshold.c_str());
		break;
	case ALARM_JUDGE_GTE:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) >= 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", pAlarmTag->strThreshold.c_str());
		break;
	case ALARM_JUDGE_LT:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) < 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", pAlarmTag->strThreshold.c_str());
		break;
	case ALARM_JUDGE_LTE:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) <= 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", pAlarmTag->strThreshold.c_str());
		break;
	case ALARM_JUDGE_GTELT:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) >= 0 && strCurValue.compare(pAlarmTag->strThreshold2) < 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "[%s,%s)", pAlarmTag->strThreshold.c_str(), pAlarmTag->strThreshold2.c_str());
		break;
	case ALARM_JUDGE_GTLT:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) > 0 && strCurValue.compare(pAlarmTag->strThreshold2) < 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "(%s,%s)", pAlarmTag->strThreshold.c_str(), pAlarmTag->strThreshold2.c_str());
		break;
	case ALARM_JUDGE_GTLTE:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) > 0 && strCurValue.compare(pAlarmTag->strThreshold2) <= 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "(%s,%s]", pAlarmTag->strThreshold.c_str(), pAlarmTag->strThreshold2.c_str());
		break;
	case ALARM_JUDGE_GTELTE:
		bAlarming = strCurValue.compare(pAlarmTag->strThreshold) >= 0 && strCurValue.compare(pAlarmTag->strThreshold2) <= 0;
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "[%s,%s]", pAlarmTag->strThreshold.c_str(), pAlarmTag->strThreshold2.c_str());
		break;
	case ALARM_JUDGE_CONTAIN:
		bAlarming = (strCurValue.find(pAlarmTag->strThreshold) != string::npos);
		if (bAlarming)
			PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", pAlarmTag->strThreshold.c_str());
		break;
	case ALARM_JUDGE_IN:
		for (int i = 0; i < pAlarmTag->vecThresholdText.size(); i++)
		{
			string &strThreshold = pAlarmTag->vecThresholdText[i];
			if (strThreshold.compare(strCurValue) == 0)
			{
				bAlarming = true;
				PKStringHelper::Snprintf(szThreshold, nThresholdLen, "%s", strThreshold.c_str());
				break;
			}
		}
		break;
	default:
		bAlarming = false;
		break;
	}

	return bAlarming;
}

// 判断报警
int CEAProcessor::CheckAlarmsOfTag(CDataTag *pTag, bool &bAlarmChanged)
{
	// 确认报警,之前是在CtrlProctask里面处里的，现在因为要把一个tag点的所有报警点信息更新为内存数据库的一个记录，所以要将
	// 一个点的所有报警状态放一起处理，否则新的状态总会把老的覆盖掉

	// 如果值是无效的，则不判断报警.但报警仍然可以确认
	// if (pTag->lastVal.nQuality != 0 || pTag->curVal.nQuality != 0)
	if (pTag->curVal.nQuality == TAG_QUALITY_INIT_STATE) // 初始状态，没有任何点，此时是不会进行报警判断的
		return 0-1;

	for (unsigned int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];

		// 如果报警未启用，则不进行报警判断和处理
		if (!pAlarmTag->bEnable)
			continue;

		char szThreshold[128] = { 0 };
		bool bAlarming = false;

		if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_QUALITYBAD) // 所有类型都有这个报警，所以放在前面判断
			bAlarming = (pAlarmTag->pTag->curVal.nQuality != 0);
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_CHANGETOQUALITYBAD)  // 所有类型都有这个报警，所以放在前面判断
			bAlarming = (pAlarmTag->pTag->curVal.nQuality != 0 && pAlarmTag->pTag->lastVal.nQuality == 0);
		else if (pAlarmTag->pTag->nDataTypeClass == DATATYPE_CLASS_INTEGER)
			bAlarming = JudgeAlarming_IntClass(pAlarmTag, szThreshold, sizeof(szThreshold));
		else if (pAlarmTag->pTag->nDataTypeClass == DATATYPE_CLASS_REAL)
			bAlarming = JudgeAlarming_RealClass(pAlarmTag, szThreshold, sizeof(szThreshold));
		else if (pAlarmTag->pTag->nDataTypeClass == DATATYPE_CLASS_TEXT)
			bAlarming = JudgeAlarming_TextClass(pAlarmTag, szThreshold, sizeof(szThreshold));
		else // DATATYPE_CLASS_OTHER 如果数据类型不对(blob)，则不判断报警
			continue;

		if (bAlarming) // 当前有报警
		{
			if (pAlarmTag->vecAlarming.size() <= 0) // 以前无报警，现在报警，则产生一个报警)
			{
				if (pAlarmTag->vecAlarming.size() >= 3) // 最多只允许3个报警
				{
					g_logger.LogMessage(PK_LOGLEVEL_INFO, "报警点:%s, 判断方法:%s, 当前报警个数已经超过3个, 为:%d, 将删除老的报警!", 
						pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmTag->vecAlarming.size());
					CAlarmingInfo *pAlarmingToDel = pAlarmTag->vecAlarming[pAlarmTag->vecAlarming.size() - 1];
					delete pAlarmingToDel;
					pAlarmingToDel = NULL;
					pAlarmTag->vecAlarming.erase(pAlarmTag->vecAlarming.end() - 1); // 删除最后一个
				}

				// 产生新的报警
				CAlarmingInfo *pAlarmingInfo = new CAlarmingInfo();
				pAlarmTag->vecAlarming.insert(pAlarmTag->vecAlarming.begin(), pAlarmingInfo);
				pAlarmingInfo->pAlarmTag = pAlarmTag;
				OnEAProduce(pAlarmingInfo, szThreshold, true, 0, 0, -1);
				bAlarmChanged = true;
			}
			else if (!pAlarmTag->vecAlarming[0]->bAlarming) // 以前有报警但处于恢复状态（未确认已恢复）
			{
				if (pAlarmTag->vecAlarming.size() >= 100)
				{
					g_logger.LogMessage(PK_LOGLEVEL_INFO, "报警点:%s, 判断方法:%d, 当前报警个数已经超过1000个, 为:%d, 将删除老的报警!", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmTag->vecAlarming.size());
					CAlarmingInfo *pAlarmingToDel = pAlarmTag->vecAlarming[pAlarmTag->vecAlarming.size() - 1];
					delete pAlarmingToDel;
					pAlarmingToDel = NULL;
					pAlarmTag->vecAlarming.erase(pAlarmTag->vecAlarming.end() - 1); // 删除最后一个
				}

				CAlarmingInfo *pAlarmingInfo = new CAlarmingInfo(); // 增加一个新的报警
				pAlarmTag->vecAlarming.insert(pAlarmTag->vecAlarming.begin(), pAlarmingInfo);
				pAlarmingInfo->pAlarmTag = pAlarmTag;
				OnEAProduce(pAlarmingInfo, szThreshold, true, 0, 0, -1);
				bAlarmChanged = true;
			}
		}
		else // 当前无报警
		{
			if (pAlarmTag->vecAlarming.size() > 0 && pAlarmTag->vecAlarming[0]->bAlarming) // 以前有报警，现在没报警，则产生一个报警恢复
			{
				OnEARecover(pAlarmTag->vecAlarming[0], true);
				bAlarmChanged = true;
			}
		}
	} // for (unsigned int i = 0; i < pTag->vecSwitchAlarmTags.size(); i++)

	return 0;
}

// 产生了新的报警恢复. bPublishAlarm 是否需要发布实时报警信息给内存数据库（程序启动初始从数据库加载上次未完成的报警时不需要发布）
void CEAProcessor::OnEARecover(CAlarmingInfo *pAlarmingInfo, bool bPublishAlarm, unsigned int nRecoverSec, unsigned int nRecoverMSec)
{
	pAlarmingInfo;// 这个就是当前的报警。之前的报警肯定都已经恢复过了
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	string strTagValue;
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, strTagValue);

	if (pAlarmTag->vecAlarming.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "CEAProcessor::OnEARecover()，报警恢复但未找到该点的报警!，tag：%s, jm:%s, 当前值为：%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str());
		return;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "CEAProcessor::OnEARecover()，一个报警恢复，tag：%s, jm:%s, 当前值为：%s, 之前有:%d个报警",
		pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str(), pAlarmTag->vecAlarming.size());

	if (!pAlarmingInfo->bAlarming)
	{
		char szProduceTime[32];
		PKTimeHelper::HighResTime2String(szProduceTime, sizeof(szProduceTime), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "CEAProcessor::OnEARecover()，报警恢复但发现该报警已经恢复!，tag：%s, jm:%s, time:%d, 当前值为:%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmingInfo->tmProduce.nSeconds, strTagValue.c_str());
		return;
	}

	pAlarmingInfo->bAlarming = 0; // 已经恢复，置状态
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, pAlarmingInfo->strValueOnRecover); // 值已经被预先赋值了，这里只需要赋值即可

	if (!bPublishAlarm)
	{
		pAlarmingInfo->tmRecover.nSeconds = nRecoverSec;
		pAlarmingInfo->tmRecover.nMilSeconds = nRecoverMSec;// 设置恢复时间
	}
	else
	{
		pAlarmingInfo->tmRecover.nSeconds = PKTimeHelper::GetHighResTime(&pAlarmingInfo->tmRecover.nMilSeconds);// 设置恢复时间
	}
	ReCalcTagAlarmStatus(pAlarmTag->pTag);
	
	if (bPublishAlarm)
		PublishTagAlarming(pAlarmingInfo, ALARM_RECOVER);

	bool bAutoConfirmAlarm = false;
	map<string, CAlarmType *> &mapAllAlarmType = MAIN_TASK->m_mapName2AlarmType;
	map<string, CAlarmType *>::iterator itAlmType = mapAllAlarmType.find(pAlarmTag->strAlarmTypeName);
	if (itAlmType != mapAllAlarmType.end())
	{
		CAlarmType *pAlarmType = itAlmType->second;
		bAutoConfirmAlarm = pAlarmType->m_bAutoConfirm;
	}

	if (!pAlarmingInfo->bConfirmed && bAutoConfirmAlarm)
	{
		// 自动确认报警！
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "CEAProcessor::OnEARecover()，报警恢复,该报警的报警类型配置为自动确认, 故自动确认报警, 确认人:AutoConfirmer! tag：%s, jm:%s, time:%d, 当前值为:%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmingInfo->tmProduce.nSeconds, strTagValue.c_str());
		OnEAConfirmOne(pAlarmingInfo, "AutoConfirmer", true, pAlarmingInfo->tmRecover.nSeconds, pAlarmingInfo->tmRecover.nMilSeconds);
	}

	if (pAlarmingInfo->bConfirmed) // 如果已确认已恢复，那么重置参数，删除该条报警
	{
		pAlarmingInfo->nRepetitions = 0;
		for (vector<CAlarmingInfo *>::iterator itAlarming = pAlarmTag->vecAlarming.begin(); itAlarming != pAlarmTag->vecAlarming.end(); itAlarming++)
		{
			if (*itAlarming == pAlarmingInfo)
			{
				pAlarmTag->vecAlarming.erase(itAlarming); // 删除该条报警
				break;
			}
		}
	}

	WriteTagAlarms(pAlarmTag->pTag); // 将该点的所有报警信息写入报警内存数据库db1
	if (pAlarmTag->pTag->pObject)
		pAlarmTag->pTag->pObject->UpdateToDataMemDB(m_jsonWriter); // 将该点的所有报警写入内存数据库db0
	WriteSystemAlarmInfo2MemDB();  // 更新系统报警信息

	//MAIN_TASK->m_redisRW0.commit();
	MAIN_TASK->m_redisRW1.commit();
}

// 产生了新的报警。新产生报警(以前没有报警） 
// bPublishAlarm 是否需要发布实时报警信息给内存数据库（程序启动初始从数据库加载上次未完成的报警时不需要发布）
void CEAProcessor::OnEAProduce(CAlarmingInfo *pAlarmingInfo, const char *szThesholdOnProduce, bool bPublishAlarm, unsigned int nProduceSec, unsigned int nProduceMSec, int nRepetitions)
{
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	string strTagValue;
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, strTagValue);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "procuce a new alarming, tag：%s,jm:%s, value：%s, 该点当前共有:%d 个报警", 
		pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str(), pAlarmTag->vecAlarming.size());

	pAlarmingInfo->strThresholdOnProduce = szThesholdOnProduce; // 报警发生时的门限值
	pAlarmingInfo->bAlarming = 1; // 产生一个报警，置状态
	pAlarmingInfo->bConfirmed = 0; // 未确认状态
	pAlarmingInfo->strConfirmUserName = "";
	pAlarmingInfo->strValueOnRecover = "";
	pAlarmingInfo->tmRecover.nSeconds = pAlarmingInfo->tmRecover.nMilSeconds = 0; // 重置该时间
	pAlarmingInfo->tmConfirm.nSeconds = pAlarmingInfo->tmRecover.nMilSeconds = 0; // 重置确认时间，避免确认时间保留原来的记录。无论以前重复计数是否为0都应该予以确认
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, pAlarmingInfo->strValueOnProduce); // 从数据库加载时，使用数据库读取的时间而不是现在的时间
	if (!bPublishAlarm) // 初始从数据库加载
	{
		pAlarmingInfo->tmProduce.nSeconds = nProduceSec;
		pAlarmingInfo->tmProduce.nMilSeconds = nProduceMSec;
		pAlarmingInfo->nRepetitions = nRepetitions;
	}
	else
	{
		// 是不是重置时间和产生时的值，取决于之前的报警有没有被确认。如果有未被确认的报警，那么重复计数必然>0
		if (pAlarmingInfo->nRepetitions <= 0) // 说明之前没有报警且没有未确认的已恢复报警。否则不能重置时间
		{
			pAlarmingInfo->tmProduceFirst.nSeconds = PKTimeHelper::GetHighResTime(&pAlarmingInfo->tmProduceFirst.nMilSeconds);// 设置第一次报警的产生时间
		}

		pAlarmingInfo->tmProduce.nSeconds = PKTimeHelper::GetHighResTime(&pAlarmingInfo->tmProduce.nMilSeconds);// 设置最新的报警产生时间
			
		pAlarmingInfo->nRepetitions++;  // 报警次数加1
	}

	ReCalcTagAlarmStatus(pAlarmTag->pTag);// 根据tag点下的每个报警，重新计算一下tag点的报警状态
	if (bPublishAlarm)
		PublishTagAlarming(pAlarmingInfo,ALARM_PRODUCE);
	
	WriteTagAlarms(pAlarmTag->pTag); // 将该点的所有报警信息写入报警内存数据库db1
	if (pAlarmTag->pTag->pObject)
		pAlarmTag->pTag->pObject->UpdateToDataMemDB(m_jsonWriter); // 将该点的所有报警写入内存数据库db0

	WriteSystemAlarmInfo2MemDB();  // 更新系统报警信息

	MAIN_TASK->m_redisRW1.commit();
	//MAIN_TASK->m_redisRW0.commit();
}

// 确认报警
// 一个报警的标识号：tagname+almtype+almindex
// nType为该点的报警类型，nIndex为该类型内的报警索引号
// bPublishAlarm 是否需要发布实时报警信息给内存数据库（程序启动初始从数据库加载上次未完成的报警时不需要发布）
void CEAProcessor::OnEAConfirm(CAlarmTag *pAlarmTag, char *szAlarmProduceTime, char *szConfirmer, bool bPublishAlarm, unsigned int nConfirmSec, unsigned int nConfirmMSec)
{	
	for (int i = 0; i < pAlarmTag->vecAlarming.size(); i++)
	{
		CAlarmingInfo *pAlarmingInfo = pAlarmTag->vecAlarming[i];
		if (!szAlarmProduceTime || strlen(szAlarmProduceTime) == 0)
			OnEAConfirmOne(pAlarmingInfo, szConfirmer, bPublishAlarm, nConfirmSec, nConfirmMSec);
		else
		{
			HighResTime htProduceTime;
			htProduceTime.nSeconds = PKTimeHelper::String2HighResTime(szAlarmProduceTime, &htProduceTime.nMilSeconds);
			if (htProduceTime.nSeconds == pAlarmingInfo->tmProduce.nSeconds && htProduceTime.nMilSeconds == pAlarmingInfo->tmProduce.nMilSeconds)
			{
				OnEAConfirmOne(pAlarmingInfo, szConfirmer, bPublishAlarm, nConfirmSec, nConfirmMSec);
			}
		}
	}
}


// 确认报警
// 一个报警的标识号：tagname+almtype+almindex
// nType为该点的报警类型，nIndex为该类型内的报警索引号
// bPublishAlarm 是否需要发布实时报警信息给内存数据库（程序启动初始从数据库加载上次未完成的报警时不需要发布）
void CEAProcessor::OnEAConfirmOne(CAlarmingInfo *pAlarmingInfo, const char *szConfirmer, bool bPublishAlarm, unsigned int nConfirmSec, unsigned int nConfirmMSec)
{
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	pAlarmingInfo->strConfirmUserName = szConfirmer;
	string strTagValue;
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, strTagValue);
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "OnEAConfirmOne()，确认一条报警数据.tag：%s,jm:%s,value：%s, 之前该点共有:%d 个报警",
		pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str(), pAlarmTag->vecAlarming.size());

	if (bPublishAlarm) // 实际操作产生的报警
	{
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnEAConfirmOne，确认一条报警数据.tag：%s,jm:%s,value：%s, before HasAuthByTag",
		//	pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str());
		if (!USERAUTH_TASK->HasAuthByTagAndLoginName(pAlarmingInfo->strConfirmUserName, pAlarmTag->pTag)) // 无权限
		{
			//g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnEAConfirmOne，确认一条报警数据.tag：%s,jm:%s,value：%s, after HasAuthByTag",
			//	pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str());
			return;
		}
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnEAConfirmOne，确认一条报警数据.tag：%s,jm:%s,value：%s, after HasAuthByTag",
		//	pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str());

		// 下面是有权限的情况！
		pAlarmingInfo->tmConfirm.nSeconds = PKTimeHelper::GetHighResTime(&pAlarmingInfo->tmConfirm.nMilSeconds);// 设置确认时间
		pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, pAlarmingInfo->strValueOnConfirm);
		pAlarmingInfo->strConfirmUserName = szConfirmer;
	}
	else // 初始读取产生的报警
	{
		pAlarmingInfo->tmConfirm.nSeconds = nConfirmSec;
		pAlarmingInfo->tmConfirm.nMilSeconds = nConfirmMSec;
		pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, pAlarmingInfo->strValueOnConfirm); // 值已经被预先赋值了，这里只需要赋值即可
	}

	pAlarmingInfo->bConfirmed = 1;
	ReCalcTagAlarmStatus(pAlarmTag->pTag); // 根据tag点下的每个报警，重新计算一下tag点的报警状态
	if (bPublishAlarm)
		PublishTagAlarming(pAlarmingInfo, ALARM_CONFIRM);

	if (!pAlarmingInfo->bAlarming) // 如果已确认已恢复，那么删除这条报警
	{
		pAlarmingInfo->nRepetitions = 0;
		pAlarmingInfo->nRepetitions = 0;
		for (vector<CAlarmingInfo *>::iterator itAlarming = pAlarmTag->vecAlarming.begin(); itAlarming != pAlarmTag->vecAlarming.end(); itAlarming++)
		{
			if (*itAlarming == pAlarmingInfo)
			{
				pAlarmTag->vecAlarming.erase(itAlarming); // 删除该条报警
				break;
			}
		}
		//pAlarmTag->tmProduce.nSeconds = pAlarmTag->tmProduce.nMilSeconds = 0;
	}

	WriteTagAlarms(pAlarmTag->pTag); // 将该点的所有报警信息写入报警内存数据库db1
	if (pAlarmTag->pTag->pObject)
		pAlarmTag->pTag->pObject->UpdateToDataMemDB(m_jsonWriter); // 将该点的所有报警写入内存数据库db0
	WriteSystemAlarmInfo2MemDB();  // 更新系统报警信息

	MAIN_TASK->m_redisRW1.commit();
	//MAIN_TASK->m_redisRW0.commit();
	//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "报警数据.tag：%s,jm:%s,maxlevel：%d",
	//	pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmTag->pTag->nMaxAlarmLevel);
}

// 当抑制报警时
void CEAProcessor::OnSuppressAlarm(CAlarmTag *pAlarmTag, char *szProduceTime, string strSuppressTime, string strUserName)
{
	for (int i = 0; i < pAlarmTag->vecAlarming.size(); i++)
	{
		CAlarmingInfo *pAlarmingInfo = pAlarmTag->vecAlarming[i];
		// pAlarmTag->strSuppressTime = strSuppressTime;
		PublishTagAlarming(pAlarmingInfo, ALARM_SUPPRESS);
	}
	WriteTagAlarms(pAlarmTag->pTag); // 将该点的所有报警信息写入报警内存数据库db1
	if (pAlarmTag->pTag->pObject)
		pAlarmTag->pTag->pObject->UpdateToDataMemDB(m_jsonWriter); // 将该点的所有报警写入内存数据库db0
	WriteSystemAlarmInfo2MemDB();  // 更新系统报警信息
}

int CEAProcessor::PublishOneAlarmAction2Web(char *szAlarmActionType, CAlarmingInfo *pAlarmingInfo, char *szTmProduce, char *szTmConfirm, char *szTmRecover)
{
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	Json::Value jsonAlarmAction;
	string strDeviceName = "";
	if (pAlarmingInfo->pAlarmTag->pTag->pObject && pAlarmingInfo->pAlarmTag->pTag->pObject->m_pDevice) // 对象模式下的设备名称
		strDeviceName = pAlarmingInfo->pAlarmTag->pTag->pObject->m_pDevice->m_strName;
	else if (pAlarmingInfo->pAlarmTag->pTag->pDevice) // 设备模式下的设备名称
		strDeviceName = pAlarmingInfo->pAlarmTag->pTag->pDevice->m_strName;
	jsonAlarmAction[ALARM_DEVICE_NAME] = strDeviceName; // 先赋值，因为数据库表缺少设备名这一列，导致无法插入该列

	if (pAlarmingInfo->pAlarmTag->pTag->pObject) // 对象模式下
	{
		jsonAlarmAction[ALARM_OBJECT_NAME] = pAlarmingInfo->pAlarmTag->pTag->pObject->strName;
		jsonAlarmAction[ALARM_OBJECT_DESCRIPTION] = pAlarmingInfo->pAlarmTag->pTag->pObject->strDescription;
	}
	else // 设备模式下，用设备名填充对象名列
	{
		jsonAlarmAction[ALARM_OBJECT_NAME] = strDeviceName;
		jsonAlarmAction[ALARM_OBJECT_DESCRIPTION] = "";
	}

	if (pAlarmTag->pTag->pObjProp)
		jsonAlarmAction[ALARM_PROP_NAME] = pAlarmTag->pTag->pObjProp->n_pClassProp->m_strName;
	else
		jsonAlarmAction[ALARM_PROP_NAME] = pAlarmTag->pTag->strName;

	jsonAlarmAction[ALARM_DESC] = pAlarmTag->strDesc;
	jsonAlarmAction[ALARM_ACTION_TYPE] = szAlarmActionType;
	jsonAlarmAction[ALARM_TAG_NAME] = pAlarmTag->pTag->strName;
	jsonAlarmAction[ALARM_JUDGEMUTHOD] = pAlarmTag->strJudgeMethod;
	jsonAlarmAction[ALARM_SYSTEMNAME] = pAlarmTag->pTag->strSubsysName;
	jsonAlarmAction[ALARM_ALARMING] = pAlarmingInfo->bAlarming;
	jsonAlarmAction[ALARM_VALONRECOVER] = pAlarmingInfo->strValueOnRecover;
	jsonAlarmAction[ALARM_LEVEL] = pAlarmTag->nLevel;
	jsonAlarmAction[ALARM_TYPE] = pAlarmTag->strAlarmTypeName;
	jsonAlarmAction[ALARM_VALONPRODUCE] = pAlarmingInfo->strValueOnProduce;
	jsonAlarmAction[ALARM_VALONCONFIRM] = pAlarmingInfo->strValueOnConfirm;
	jsonAlarmAction[ALARM_THRESH] = pAlarmingInfo->strThresholdOnProduce;

	jsonAlarmAction[ALARM_CONFIRMED] = pAlarmingInfo->bConfirmed;
	jsonAlarmAction[ALARM_CONFIRMER] = pAlarmingInfo->strConfirmUserName;
	jsonAlarmAction[ALARM_CONFIRMTIME] = szTmConfirm;
	jsonAlarmAction[ALARM_PRODUCETIME] = szTmProduce;
	jsonAlarmAction[ALARM_RECOVERTIME] = szTmRecover;
	jsonAlarmAction[ALARM_REPETITIONS] = pAlarmingInfo->nRepetitions;
	jsonAlarmAction[ALARM_PARAM1] = pAlarmingInfo->strParam1;
	jsonAlarmAction[ALARM_PARAM2] = pAlarmingInfo->strParam2;
	jsonAlarmAction[ALARM_PARAM3] = pAlarmingInfo->strParam3;
	jsonAlarmAction[ALARM_PARAM4] = pAlarmingInfo->strParam4;
	
	string strAlarmTagName = pAlarmTag->pTag->strName + "." + pAlarmTag->strJudgeMethod;
	if (pAlarmingInfo->bConfirmed && pAlarmingInfo->bAlarming) // 已确认，未恢复重写报警值和报警前值，恢复值为0
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)收到%s的确认报警操作：%s", pAlarmingInfo->strConfirmUserName.c_str(), strAlarmTagName.c_str());
	}
	else if (!pAlarmingInfo->bConfirmed && pAlarmingInfo->bAlarming) // 未确认，未恢复重写报警值和报警前值，恢复值为0
	{
		// 更新tag点报警属性
		jsonAlarmAction[ALARM_CONFIRMTIME] = "";
		jsonAlarmAction[ALARM_RECOVERTIME] = "";
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)%s.%s：产生报警", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str());
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)%s.%s:produce new alarm", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str());
#endif
	}
	else if (!pAlarmingInfo->bConfirmed && !pAlarmingInfo->bAlarming)  // 未确认，已恢复，写恢复时的值，报警值和报警当前值为0
	{
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)%s.%s：报警恢复", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str());
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)%s.%s：alarm recovered", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str());
#endif
	}
	else if (pAlarmingInfo->bConfirmed && !pAlarmingInfo->bAlarming) // 已确认，已恢复。 需要删除！并重置为缺省值
	{
		pAlarmingInfo->nRepetitions = 0;
		//pAlarmTag->nLevel = 0; 这是配置项，不能被置为0
		pAlarmingInfo->strValueOnProduce = "";
		pAlarmingInfo->strConfirmUserName = "";
		pAlarmingInfo->strValueOnRecover = "";
		pAlarmingInfo->strValueOnConfirm = "";
	
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)报警【tag:%s,judgemethod:%s,producetime:%s】已确认已恢复,从报警列表移除, 确认者：%s, 确认时间：%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, pAlarmingInfo->strConfirmUserName.c_str(), szTmConfirm);
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)alarm[tag:%s,judgemethod:%s,producetime:%s] confirmed and recovered, will remove from alarminglist. confirmer：%s, confirm time：%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, pAlarmingInfo->strConfirmUserName.c_str(), szTmConfirm);
#endif
	}

	// 把报警转换为数组
	Json::Value jsonAlarmActions;
	jsonAlarmActions.append(jsonAlarmAction);
	string strAlarms = m_jsonWriter.write(jsonAlarmActions); // 性能更高

	int nRet = MAIN_TASK->m_redisRW0.publish(CHANNELNAME_ALARMING, strAlarms.c_str());

	if(nRet != 0)
	{
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)报警【tag:%s,judgemethod:%s,producetime:%s】发送到报警通知队列失败，错误码：%d，内容：%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, nRet, strAlarms.c_str());
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)alarm[tag:%s,judgemethod:%s,producetime:%s] sent to alarm notify queue failed，retcode：%d, content：%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, nRet, strAlarms.c_str());
#endif
	}

	// 推送报警的历史事件消息，增加evttype这个字段
	jsonAlarmAction[EVENT_TYPE_MSGID] = EVENT_TYPE_ALARM;
	Json::Value jsonEvents;
	jsonEvents.append(jsonAlarmAction);
	string strEvents = m_jsonWriter.write(jsonEvents);

	nRet = MAIN_TASK->m_redisRW0.publish(CHANNELNAME_EVENT, strEvents.c_str());
	if(nRet != 0)
	{
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)报警【tag:%s,judgemethod:%s,producetime:%s】发送到事件通知队列失败，错误码：%d，内容：%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, nRet, strEvents.c_str());
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)alarm[tag:%s,judgemethod:%s,producetime:%s] sent to event notify queue failed,retcode:%d,content:%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, nRet, strEvents.c_str());
#endif
	}
	//MAIN_TASK->m_redisRW0.commit();
	return PK_SUCCESS;
}

/**
* 组织报警点信息转发到pkcserver, AlarmRouteTask进行处理
*
* @param    -[in]   CAlarmTag *pAlarmTag : [报警点结构体，包含所有报警点信息]
* @param    -[in]   char* szType : [报警状态, recover/confirm/produce]
*
* @return   PK_SUCCESS/ICV_FAILURE.
*
* @version  11/19/2016	liucaixia   Initial Version
*
**/
int CEAProcessor::PublishTagAlarming(CAlarmingInfo *pAlarmingInfo, char* szActionType)
{
	// 时间转换，将高精度时间转换成时间字符串
	char szTmProduce[PK_TIMESTRING_MAXLEN] = {'\0'};
	char szTmConfirm[PK_TIMESTRING_MAXLEN] = {'\0'};
	char szTmRecover[PK_TIMESTRING_MAXLEN] = {'\0'};

	// 产生时间总是有的
	PKTimeHelper::HighResTime2String(szTmProduce, sizeof(szTmProduce), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
	if (pAlarmingInfo->bConfirmed) // 如果已确认才有确认时间
		PKTimeHelper::HighResTime2String(szTmConfirm, sizeof(szTmConfirm), pAlarmingInfo->tmConfirm.nSeconds, pAlarmingInfo->tmConfirm.nMilSeconds);
	if (!pAlarmingInfo->bAlarming)// 如果已恢复，才有恢复时间
		PKTimeHelper::HighResTime2String(szTmRecover, sizeof(szTmRecover), pAlarmingInfo->tmRecover.nSeconds, pAlarmingInfo->tmRecover.nMilSeconds);

	// 将动作发送到Web
	PublishOneAlarmAction2Web(szActionType, pAlarmingInfo, szTmProduce, szTmConfirm, szTmRecover);
	return PK_SUCCESS;
}

// system.alarm = {alarmingcount:5,alarmcount:10...}
// alarmtype.上上限报警 = {alarmingcount:5,alarmcount:10...}
int CEAProcessor::WriteSystemAlarmInfo2MemDB()
{
	// 更新内存数据库的报警对象, system.alarm = {alarmingcount:5,alarmcount:10...}
	Json::Value jsonAlarmStatus;
	m_curAlarmInfo.GenerateAlarmInfo2Json(jsonAlarmStatus);
	string strValue = m_jsonWriter.write(jsonAlarmStatus); 
	vector<string> vecTagName, vecTagVTQ;
	vecTagName.push_back(TAGNAME_SYSTEM_ALARM);
	vecTagVTQ.push_back(strValue);
	//MAIN_TASK->m_redisRW0.set(TAGNAME_SYSTEM_ALARM, strValue.c_str()); // 未提交

	// 每个报警类型,写入一个点, alarmtype.上上限报警 = {alarmingcount:5,alarmcount:10...}
	map<string, CAlarmType *> &mapAlarmType = MAIN_TASK->m_mapName2AlarmType;
	for (map<string, CAlarmType *>::iterator itAlmType = mapAlarmType.begin(); itAlmType != mapAlarmType.end(); itAlmType++)
	{
		CAlarmType *pAlarmType = itAlmType->second;
		string strAlarmType = itAlmType->first;

		Json::Value jsonAlarmType;
		pAlarmType->alarmInfo.GenerateAlarmInfo2Json(jsonAlarmType);

		string strTagName = "alarmtype." + strAlarmType;
		string strValue = m_jsonWriter.write(jsonAlarmType); 
		vecTagName.push_back(strTagName);
		vecTagVTQ.push_back(strValue);

		//MAIN_TASK->m_redisRW0.set(strTagName.c_str(), strValue.c_str()); // 未提交
	}

	MAIN_TASK->m_redisRW0.mset(vecTagName, vecTagVTQ); // 未提交
	vecTagName.clear();
	vecTagVTQ.clear();
	return 0;
}

/**
* 当一个报警点的状态发生改变时，更新对应tag点的报警属性
*
* @param    -[in]   CDataTag &pTag : [报警点对应的tag点]
* @param    -[in]   string strActType : [报警状态, actrecover/actconfirm/actproduce]
*
* @return   PK_SUCCESS/ICV_FAILURE.
*
* @version  11/19/2016	liucaixia   Initial Version
**/
int CEAProcessor::ReCalcTagAlarmStatus(CDataTag *pDataTag)
{
	if(!pDataTag)
		return -1;

	// 更新tag点报警级别,tag点报警级别为tag点所有报警的最大报警级别
	int nTagAlmCount = 0;
	int nTagAlmUnAckCount = 0;
	int nTagAlarmingCount = 0;
	int nTagAlmAckedCount = 0;
	int nMaxLevel = 0;

	// 全部置0
	for (map<string, CAlarmInfo *>::iterator itAlmType = pDataTag->mapAlarmTypeName2AlarmInfo.begin(); itAlmType != pDataTag->mapAlarmTypeName2AlarmInfo.end(); itAlmType++)
		itAlmType->second->ResetValue(); 
	pDataTag->m_curAlarmInfo.ResetValue();

	for (unsigned int iAlarmTag = 0; iAlarmTag < pDataTag->vecAllAlarmTags.size(); iAlarmTag ++)
	{
		CAlarmTag *pAlarmTag = pDataTag->vecAllAlarmTags[iAlarmTag];
		if(!pAlarmTag->bEnable)
			continue;
		pAlarmTag->m_alarmInfo.ResetValue();

		for (int i = 0; i < pAlarmTag->vecAlarming.size(); i++)
		{
			CAlarmingInfo *pAlarmingInfo = pAlarmTag->vecAlarming[i];
			pAlarmTag->m_alarmInfo.m_nAlarmCount++;
			if (pAlarmingInfo->bAlarming) // 正在报警
			{
				pAlarmTag->m_alarmInfo.m_nAlarmingCount++;
				if (!pAlarmingInfo->bConfirmed) // 且未确认
					pAlarmTag->m_alarmInfo.m_nAlarmingUnAckCount++;
				pAlarmTag->m_alarmInfo.m_nMaxAlarmLevel = pAlarmingInfo->pAlarmTag->nLevel; // 报警级别赋值，如果报警了则报警级别==报警点的级别
				pAlarmTag->m_alarmInfo.m_nMinAlarmLevel = pAlarmingInfo->pAlarmTag->nLevel; // 报警级别赋值，如果报警了则报警级别==报警点的级别
			}

			if (!pAlarmingInfo->bConfirmed)
				pAlarmTag->m_alarmInfo.m_nAlarmUnConfirmedCount ++;
			if (pAlarmingInfo->bConfirmed)
				pAlarmTag->m_alarmInfo.m_nAlarmConfirmedCount++;
		}

		// 赋值，为每个报警类型
		if (!pAlarmTag->strAlarmTypeName.empty())
		{
			CAlarmInfo *pAlarmInfo = pDataTag->mapAlarmTypeName2AlarmInfo[pAlarmTag->strAlarmTypeName];
			*pAlarmInfo += pAlarmTag->m_alarmInfo;
		}
		pDataTag->m_curAlarmInfo += pAlarmTag->m_alarmInfo; // 总体相加
	}

	// 计算对象的报警级别
	ReCalcObjectAlarmStatus(pDataTag->pObject);
	return PK_SUCCESS;
}

/**
* 当一个报警点的状态发生改变时，更新对应tag点的报警属性
*
* @param    -[in]   CDataTag &pTag : [报警点对应的tag点]
* @param    -[in]   string strActType : [报警状态, actrecover/actconfirm/actproduce]
*
* @return   PK_SUCCESS/ICV_FAILURE.
*
* @version  11/19/2016	liucaixia   Initial Version
**/
int CEAProcessor::ReCalcObjectAlarmStatus(CMyObject *pObject)
{
	if(!pObject)
		return -1;

	// 更新tag点报警级别,tag点报警级别为tag点所有报警的最大报警级别
	int nQualityGoodNum = 0;
	int nQualityBadOfFirstTag = 0;
	pObject->m_alarmInfo.ResetValue();
	for (map<string, CAlarmInfo *>::iterator itAlmInfo = pObject->m_mapAlarmTypeName2Info.begin(); itAlmInfo != pObject->m_mapAlarmTypeName2Info.end(); itAlmInfo++)
	{
		CAlarmInfo *pObjAlmInfo = itAlmInfo->second;
		pObjAlmInfo->ResetValue();
	}

	for (unsigned int iTag = 0; iTag < pObject->vecTags.size(); iTag ++)
	{
		CDataTag *pDataTag = pObject->vecTags[iTag];
		if(pDataTag->m_curAlarmInfo.m_nAlarmCount <= 0) // 无报警的属性不参与运算
			continue;

		// 更新相同类型的报警的信息
		for (map<string, CAlarmInfo *>::iterator itAlmInfo = pDataTag->mapAlarmTypeName2AlarmInfo.begin(); itAlmInfo != pDataTag->mapAlarmTypeName2AlarmInfo.end(); itAlmInfo++)
		{
			string strAlarmTypeName = itAlmInfo->first;
			CAlarmInfo *pObjAlmInfo = itAlmInfo->second;

			map<string, CAlarmInfo *>::iterator itObjAlmTypeInfo = pObject->m_mapAlarmTypeName2Info.find(strAlarmTypeName);
			if (itObjAlmTypeInfo == pObject->m_mapAlarmTypeName2Info.end())
				continue;
			CAlarmInfo *pObjTypeAlarmInfo = itObjAlmTypeInfo->second;
			*pObjTypeAlarmInfo += *pObjAlmInfo;
		}
		
		pObject->m_alarmInfo += pDataTag->m_curAlarmInfo;
	}

	if (nQualityGoodNum == pObject->vecTags.size())
		pObject->nQuality = 0;
	else
		pObject->nQuality = nQualityBadOfFirstTag;

	// 计算整个系统的报警信息
	this->ReCalcSystemAlarmStatus();

	return PK_SUCCESS;
}

/**
* 当一个报警点的状态发生改变时，更新对应tag点的报警属性
*
* @param    -[in]   CDataTag &pTag : [报警点对应的tag点]
* @param    -[in]   string strActType : [报警状态, actrecover/actconfirm/actproduce]
*
* @return   PK_SUCCESS/ICV_FAILURE.
*
* @version  11/19/2016	liucaixia   Initial Version
**/
int CEAProcessor::ReCalcSystemAlarmStatus()
{
	// 更新tag点报警级别,tag点报警级别为tag点所有报警的最大报警级别
	m_curAlarmInfo.ResetValue();
	map<string, CAlarmType *> &mapAlarmType = MAIN_TASK->m_mapName2AlarmType;
	for (map<string, CAlarmType *>::iterator itAlmType = mapAlarmType.begin(); itAlmType != mapAlarmType.end(); itAlmType++)
	{
		CAlarmType *pAlarmType = itAlmType->second;
		pAlarmType->alarmInfo.ResetValue();
	}

	map<string, CMyObject *> &mapName2Obj = MAIN_TASK->m_mapName2Object;
	for (map<string, CMyObject *>::iterator it = mapName2Obj.begin(); it != mapName2Obj.end(); it++)
	{
		CMyObject *pObject = it->second;
		m_curAlarmInfo += pObject->m_alarmInfo;

		// 更新同一个报警类型的报警信息
		map<string, CAlarmType *> &mapAllAlarmType = MAIN_TASK->m_mapName2AlarmType;
		for (map<string, CAlarmInfo *>::iterator itAlmInfo = pObject->m_mapAlarmTypeName2Info.begin(); itAlmInfo != pObject->m_mapAlarmTypeName2Info.end(); itAlmInfo++)
		{
			string strAlarmTypeName = itAlmInfo->first;
			CAlarmInfo *pObjAlmInfo = itAlmInfo->second;
			map<string, CAlarmType *>::iterator itAlmType = mapAllAlarmType.find(strAlarmTypeName);
			if (itAlmType == mapAllAlarmType.end())
				continue;

			CAlarmType* pAlarmType = itAlmType->second;
			pAlarmType->alarmInfo += *pObjAlmInfo;
		}
	}

	return PK_SUCCESS;
}

/**
修改报警的上下限值以及是否允许报警选项。为0报警和为1 报警除外，其threshold是用不着的！
{"tagname":"tag1",judgemethod:"hh","alarmparam":{"enable":1,"level":10,"threshold":250,"name":"这是一个报警","alarmtype":"上上限报警类型"}}
*/
int CEAProcessor::ProcessAlarmParamChange(string &strTagName, string &strJudgemethod, string &strAlarmParam, string &strUserName)
{
	int nRet = 0;
	// 根据tag名称找到tag点对应的驱动
	map<string, CDataTag *>::iterator itMap = MAIN_TASK->m_mapName2Tag.find(strTagName);
	if (itMap == MAIN_TASK->m_mapName2Tag.end()){
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ProcessAlarmParamChange(%s,%s,%s,%s)，cannot find taginfo by name", 
			strTagName.c_str(), strJudgemethod.c_str(), strAlarmParam.c_str(), strUserName.c_str());
		nRet = -3;
		return nRet;
	}

	CDataTag *pTag = itMap->second;
	if (pTag == NULL){
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ProcessAlarmParamChange(%s,%s,%s,%s)，found taginfo==NULL",
			strTagName.c_str(), strJudgemethod.c_str(), strAlarmParam.c_str(), strUserName.c_str());
		nRet = -4;
		return nRet;
	}

	Json::Reader jsonReader;
	Json::Value jsonAlarmParam;
	if (!jsonReader.parse(strAlarmParam.c_str(), jsonAlarmParam, false))
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ProcessAlarmParamChange(%s,%s,%s,%s)，alarmParam is not json format",
			strTagName.c_str(), strJudgemethod.c_str(), strAlarmParam.c_str(), strUserName.c_str());
		return -5;
	}

	// 遍历点的所有报警点，对正在报警的点或已经恢复的点进行确认
	for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
		// 未确认的报警，如果判别类型为*、空或者和该报警相等
		if (strJudgemethod.compare("*") != 0 && PKStringHelper::StriCmp(strJudgemethod.c_str(), pAlarmTag->strJudgeMethod.c_str()) != 0)
			continue;

		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "modify tag's alarm, tag:%s,judgemethod:%s, alarmparam:%s, user:%s", strTagName.c_str(), strJudgemethod.c_str(), strAlarmParam.c_str(), strUserName.c_str());
		string strFieldName, strFieldValue;
		strFieldName = "threshold";
		if (!jsonAlarmParam[strFieldName].isNull())
		{
			Json2String(jsonAlarmParam[strFieldName], strFieldValue);
			pAlarmTag->strThreshold = strFieldName;
			pAlarmTag->dbThreshold = ::atoi(strFieldValue.c_str());
			CalcLHLimits(pTag);
			UpdateAlarmField2DB(pTag->strName, pAlarmTag->strJudgeMethod, strFieldName.c_str(), strFieldValue, strUserName);
		}

		strFieldName = "enable";
		if (!jsonAlarmParam[strFieldName].isNull())
		{
			Json2String(jsonAlarmParam[strFieldName], strFieldValue);
			if (PKStringHelper::StriCmp(strFieldValue.c_str(), "true") == 0 || PKStringHelper::StriCmp(strFieldValue.c_str(), "1") == 0)
			{
				pAlarmTag->bEnable = true;
				UpdateAlarmField2DB(pTag->strName, pAlarmTag->strJudgeMethod, strFieldName.c_str(), "1", strUserName);
			}
			else
			{
				pAlarmTag->bEnable = false;
				UpdateAlarmField2DB(pTag->strName, pAlarmTag->strJudgeMethod, strFieldName.c_str(), "0", strUserName);
			}
		}

		strFieldName = "priority";
		if (!jsonAlarmParam[strFieldName].isNull())
		{
			Json2String(jsonAlarmParam[strFieldName], strFieldValue);
			pAlarmTag->nLevel = ::atoi(strFieldValue.c_str());
			UpdateAlarmField2DB(pTag->strName, pAlarmTag->strJudgeMethod, strFieldName.c_str(), strFieldValue, strUserName);
		}
		
		strFieldName = "alarmtypename";
		if (!jsonAlarmParam[strFieldName].isNull())
		{
			Json2String(jsonAlarmParam[strFieldName], strFieldValue);
			pAlarmTag->strAlarmTypeName = strFieldValue;
			UpdateAlarmField2DB(pTag->strName, pAlarmTag->strJudgeMethod, strFieldName.c_str(), strFieldValue, strUserName);
		}
	}
	return 0;
}

int CEAProcessor::UpdateAlarmField2DB(string &strTagName, string &strJudgeMethod, const char *szDBFieldName, string strFieldValue, string &strUserName)
{
	string strJudgeMethodLower = strJudgeMethod;
	string strJudgeMethodUpper = strJudgeMethod;
	transform(strJudgeMethodLower.begin(), strJudgeMethodLower.end(), strJudgeMethodLower.begin(), ::tolower);
	transform(strJudgeMethodUpper.begin(), strJudgeMethodUpper.end(), strJudgeMethodUpper.begin(), ::toupper);

	int nRet = 0;

	CPKEviewDbApi PKEviewDbApi;
	nRet = PKEviewDbApi.Init();
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "[UpdateAlarmField2DB]Init Database Failed:%d!tagname:%s, judgemethod:%s, dbfieldname:%s, fieldvalue:%s, username:%s", 
			nRet, strTagName.c_str(), strJudgeMethod.c_str(), szDBFieldName, strFieldValue.c_str(), strUserName.c_str());
		PKEviewDbApi.Exit();
		return -2;
	}

	char szSql[2048] = { 0 };
	// alarmtypename是一个外键,是需要进行转换为id的！
	if (PKStringHelper::StriCmp(szDBFieldName, "alarmtype") == 0 || PKStringHelper::StriCmp(szDBFieldName, "alarmtypename") == 0)
	{
		sprintf(szSql, "update t_device_tagalarm set alarmtype_id=(select id from t_alarm_type where name='%s')  where tag_id in (select id from t_device_tag where name='%s') and (judgemethod='%s' or judgemethod='%s')",
			strFieldValue.c_str(), strTagName.c_str(), strJudgeMethodLower.c_str(), strJudgeMethodUpper.c_str());
	}
	else
	{
		sprintf(szSql, "update t_device_tagalarm set %s='%s' where tag_id in (select id from t_device_tag where name='%s') and (judgemethod='%s' or judgemethod='%s')", szDBFieldName,
			strFieldValue.c_str(), strTagName.c_str(), strJudgeMethodLower.c_str(), strJudgeMethodUpper.c_str());
	}

	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, &strError);
	if (nRet != 0)
	{
		PKEviewDbApi.Exit();
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "[UpdateAlarmField2DB]update database failed:%s!tagname:%s, judgemethod:%s, dbfieldname:%s, fieldvalue:%s, username:%s, error:%s", 
			szSql, strTagName.c_str(), strJudgeMethod.c_str(), szDBFieldName, strFieldValue.c_str(), strUserName.c_str(), strError.c_str());
		return -2;
	}
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "UpdateAlarmField2DB successfully! tagname:%s, judgemethod:%s, dbfieldname:%s, fieldvalue:%s, username:%s",
		strTagName.c_str(), strJudgeMethod.c_str(), szDBFieldName, strFieldValue.c_str(), strUserName.c_str());

	PKEviewDbApi.Exit();
	return 0;
}
