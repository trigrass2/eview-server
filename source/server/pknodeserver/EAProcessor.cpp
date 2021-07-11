/**************************************************************
 *  Filename:    SockHandler.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: SockHandler.cpp
 *
 *  @author:     shijunpu
 *  @version     05/29/2012  shijunpu  Initial Version
**************************************************************/
// һ�������ı�ʶ����������ɣ�tagname+almcat+almindex

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
#define ICV_FAILURE		-1					// ���󷵻�ֵ

/**
 *  ���캯��.
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
 *  ��������.
 *
 *  @version     05/29/2008  shijunpu  Initial Version.
 */
CEAProcessor::~CEAProcessor()
{
	// m_mapJudgeMethodName2Fields.clear();
}

// ��������Ҫ��ʱ��ɾ�������еı���
void CEAProcessor::WriteTagAlarms(CDataTag *pTag)
{
	Json::Value jsonAlarmings; // ĳ��tag��ĵ�ǰ����״̬
	GetTagAlarmsStatus(pTag, jsonAlarmings);

	if (jsonAlarmings.size() <= 0) // ������Ѿ������ڱ����ˣ���Ҫɾ��!���򱨾����ݻ��Ϊ"null"������java��ȡʱ������쳣��
	{
		MAIN_TASK->m_redisRW1.del(pTag->strName.c_str());
	}
	else
	{
		string strValue = m_jsonWriter.write(jsonAlarmings); // ���ܸ���
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
		if (pAlarmingInfo->bConfirmed) // �����ȷ�ϲ���ȷ��ʱ��
			PKTimeHelper::HighResTime2String(szTmConfirm, sizeof(szTmConfirm), pAlarmingInfo->tmConfirm.nSeconds, pAlarmingInfo->tmConfirm.nMilSeconds);
		if (!pAlarmingInfo->bAlarming) // ����ѻָ����лָ�ʱ��
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
		if (pAlarmTag->pTag->pObjProp) // ����ģʽ����
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

// ��������״̬�����ı�֮�󣬸��µ�ǰ������б���״̬
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

// �ж��������͵�tag��ı���
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
	case ALARM_JUDGE_CHANGETOEQ: // ��Ϊ���ֵ����Ϊ����
		bAlarming = (nCurValue == pAlarmTag->nThreshold);
		bAlarming = (bAlarming && pAlarmTag->pTag->lastVal.nQuality == 0 && pAlarmTag->pTag->lastVal.nValue != nCurValue);
		// ���㣺���ڸ�ֵ��֮ǰ�����ڵ�������������GOOD����֮ǰ�����ڵ�����ֵ����
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

// �ж�С�����͵�tag��ı����������Ƿ񱨾������������趨
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
	case ALARM_JUDGE_CHANGETOEQ://// ���㣺���ڸ�ֵ��֮ǰ�����ڵ�������������GOOD����֮ǰ�����ڵ�����ֵ����
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

// �ж��ı����͵�tag��ı���
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
	case ALARM_JUDGE_CHANGETOEQ: // ���㣺���ڸ�ֵ��֮ǰ�����ڵ�������������GOOD����֮ǰ�����ڵ�����ֵ����
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

// �жϱ���
int CEAProcessor::CheckAlarmsOfTag(CDataTag *pTag, bool &bAlarmChanged)
{
	// ȷ�ϱ���,֮ǰ����CtrlProctask���洦��ģ�������ΪҪ��һ��tag������б�������Ϣ����Ϊ�ڴ����ݿ��һ����¼������Ҫ��
	// һ��������б���״̬��һ���������µ�״̬�ܻ���ϵĸ��ǵ�

	// ���ֵ����Ч�ģ����жϱ���.��������Ȼ����ȷ��
	// if (pTag->lastVal.nQuality != 0 || pTag->curVal.nQuality != 0)
	if (pTag->curVal.nQuality == TAG_QUALITY_INIT_STATE) // ��ʼ״̬��û���κε㣬��ʱ�ǲ�����б����жϵ�
		return 0-1;

	for (unsigned int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];

		// �������δ���ã��򲻽��б����жϺʹ���
		if (!pAlarmTag->bEnable)
			continue;

		char szThreshold[128] = { 0 };
		bool bAlarming = false;

		if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_QUALITYBAD) // �������Ͷ���������������Է���ǰ���ж�
			bAlarming = (pAlarmTag->pTag->curVal.nQuality != 0);
		else if (pAlarmTag->nJudgeMethodType == ALARM_JUDGE_CHANGETOQUALITYBAD)  // �������Ͷ���������������Է���ǰ���ж�
			bAlarming = (pAlarmTag->pTag->curVal.nQuality != 0 && pAlarmTag->pTag->lastVal.nQuality == 0);
		else if (pAlarmTag->pTag->nDataTypeClass == DATATYPE_CLASS_INTEGER)
			bAlarming = JudgeAlarming_IntClass(pAlarmTag, szThreshold, sizeof(szThreshold));
		else if (pAlarmTag->pTag->nDataTypeClass == DATATYPE_CLASS_REAL)
			bAlarming = JudgeAlarming_RealClass(pAlarmTag, szThreshold, sizeof(szThreshold));
		else if (pAlarmTag->pTag->nDataTypeClass == DATATYPE_CLASS_TEXT)
			bAlarming = JudgeAlarming_TextClass(pAlarmTag, szThreshold, sizeof(szThreshold));
		else // DATATYPE_CLASS_OTHER ����������Ͳ���(blob)�����жϱ���
			continue;

		if (bAlarming) // ��ǰ�б���
		{
			if (pAlarmTag->vecAlarming.size() <= 0) // ��ǰ�ޱ��������ڱ����������һ������)
			{
				if (pAlarmTag->vecAlarming.size() >= 3) // ���ֻ����3������
				{
					g_logger.LogMessage(PK_LOGLEVEL_INFO, "������:%s, �жϷ���:%s, ��ǰ���������Ѿ�����3��, Ϊ:%d, ��ɾ���ϵı���!", 
						pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmTag->vecAlarming.size());
					CAlarmingInfo *pAlarmingToDel = pAlarmTag->vecAlarming[pAlarmTag->vecAlarming.size() - 1];
					delete pAlarmingToDel;
					pAlarmingToDel = NULL;
					pAlarmTag->vecAlarming.erase(pAlarmTag->vecAlarming.end() - 1); // ɾ�����һ��
				}

				// �����µı���
				CAlarmingInfo *pAlarmingInfo = new CAlarmingInfo();
				pAlarmTag->vecAlarming.insert(pAlarmTag->vecAlarming.begin(), pAlarmingInfo);
				pAlarmingInfo->pAlarmTag = pAlarmTag;
				OnEAProduce(pAlarmingInfo, szThreshold, true, 0, 0, -1);
				bAlarmChanged = true;
			}
			else if (!pAlarmTag->vecAlarming[0]->bAlarming) // ��ǰ�б��������ڻָ�״̬��δȷ���ѻָ���
			{
				if (pAlarmTag->vecAlarming.size() >= 100)
				{
					g_logger.LogMessage(PK_LOGLEVEL_INFO, "������:%s, �жϷ���:%d, ��ǰ���������Ѿ�����1000��, Ϊ:%d, ��ɾ���ϵı���!", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmTag->vecAlarming.size());
					CAlarmingInfo *pAlarmingToDel = pAlarmTag->vecAlarming[pAlarmTag->vecAlarming.size() - 1];
					delete pAlarmingToDel;
					pAlarmingToDel = NULL;
					pAlarmTag->vecAlarming.erase(pAlarmTag->vecAlarming.end() - 1); // ɾ�����һ��
				}

				CAlarmingInfo *pAlarmingInfo = new CAlarmingInfo(); // ����һ���µı���
				pAlarmTag->vecAlarming.insert(pAlarmTag->vecAlarming.begin(), pAlarmingInfo);
				pAlarmingInfo->pAlarmTag = pAlarmTag;
				OnEAProduce(pAlarmingInfo, szThreshold, true, 0, 0, -1);
				bAlarmChanged = true;
			}
		}
		else // ��ǰ�ޱ���
		{
			if (pAlarmTag->vecAlarming.size() > 0 && pAlarmTag->vecAlarming[0]->bAlarming) // ��ǰ�б���������û�����������һ�������ָ�
			{
				OnEARecover(pAlarmTag->vecAlarming[0], true);
				bAlarmChanged = true;
			}
		}
	} // for (unsigned int i = 0; i < pTag->vecSwitchAlarmTags.size(); i++)

	return 0;
}

// �������µı����ָ�. bPublishAlarm �Ƿ���Ҫ����ʵʱ������Ϣ���ڴ����ݿ⣨����������ʼ�����ݿ�����ϴ�δ��ɵı���ʱ����Ҫ������
void CEAProcessor::OnEARecover(CAlarmingInfo *pAlarmingInfo, bool bPublishAlarm, unsigned int nRecoverSec, unsigned int nRecoverMSec)
{
	pAlarmingInfo;// ������ǵ�ǰ�ı�����֮ǰ�ı����϶����Ѿ��ָ�����
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	string strTagValue;
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, strTagValue);

	if (pAlarmTag->vecAlarming.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "CEAProcessor::OnEARecover()�������ָ���δ�ҵ��õ�ı���!��tag��%s, jm:%s, ��ǰֵΪ��%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str());
		return;
	}

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "CEAProcessor::OnEARecover()��һ�������ָ���tag��%s, jm:%s, ��ǰֵΪ��%s, ֮ǰ��:%d������",
		pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str(), pAlarmTag->vecAlarming.size());

	if (!pAlarmingInfo->bAlarming)
	{
		char szProduceTime[32];
		PKTimeHelper::HighResTime2String(szProduceTime, sizeof(szProduceTime), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "CEAProcessor::OnEARecover()�������ָ������ָñ����Ѿ��ָ�!��tag��%s, jm:%s, time:%d, ��ǰֵΪ:%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmingInfo->tmProduce.nSeconds, strTagValue.c_str());
		return;
	}

	pAlarmingInfo->bAlarming = 0; // �Ѿ��ָ�����״̬
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, pAlarmingInfo->strValueOnRecover); // ֵ�Ѿ���Ԥ�ȸ�ֵ�ˣ�����ֻ��Ҫ��ֵ����

	if (!bPublishAlarm)
	{
		pAlarmingInfo->tmRecover.nSeconds = nRecoverSec;
		pAlarmingInfo->tmRecover.nMilSeconds = nRecoverMSec;// ���ûָ�ʱ��
	}
	else
	{
		pAlarmingInfo->tmRecover.nSeconds = PKTimeHelper::GetHighResTime(&pAlarmingInfo->tmRecover.nMilSeconds);// ���ûָ�ʱ��
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
		// �Զ�ȷ�ϱ�����
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "CEAProcessor::OnEARecover()�������ָ�,�ñ����ı�����������Ϊ�Զ�ȷ��, ���Զ�ȷ�ϱ���, ȷ����:AutoConfirmer! tag��%s, jm:%s, time:%d, ��ǰֵΪ:%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmingInfo->tmProduce.nSeconds, strTagValue.c_str());
		OnEAConfirmOne(pAlarmingInfo, "AutoConfirmer", true, pAlarmingInfo->tmRecover.nSeconds, pAlarmingInfo->tmRecover.nMilSeconds);
	}

	if (pAlarmingInfo->bConfirmed) // �����ȷ���ѻָ�����ô���ò�����ɾ����������
	{
		pAlarmingInfo->nRepetitions = 0;
		for (vector<CAlarmingInfo *>::iterator itAlarming = pAlarmTag->vecAlarming.begin(); itAlarming != pAlarmTag->vecAlarming.end(); itAlarming++)
		{
			if (*itAlarming == pAlarmingInfo)
			{
				pAlarmTag->vecAlarming.erase(itAlarming); // ɾ����������
				break;
			}
		}
	}

	WriteTagAlarms(pAlarmTag->pTag); // ���õ�����б�����Ϣд�뱨���ڴ����ݿ�db1
	if (pAlarmTag->pTag->pObject)
		pAlarmTag->pTag->pObject->UpdateToDataMemDB(m_jsonWriter); // ���õ�����б���д���ڴ����ݿ�db0
	WriteSystemAlarmInfo2MemDB();  // ����ϵͳ������Ϣ

	//MAIN_TASK->m_redisRW0.commit();
	MAIN_TASK->m_redisRW1.commit();
}

// �������µı������²�������(��ǰû�б����� 
// bPublishAlarm �Ƿ���Ҫ����ʵʱ������Ϣ���ڴ����ݿ⣨����������ʼ�����ݿ�����ϴ�δ��ɵı���ʱ����Ҫ������
void CEAProcessor::OnEAProduce(CAlarmingInfo *pAlarmingInfo, const char *szThesholdOnProduce, bool bPublishAlarm, unsigned int nProduceSec, unsigned int nProduceMSec, int nRepetitions)
{
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	string strTagValue;
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, strTagValue);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "procuce a new alarming, tag��%s,jm:%s, value��%s, �õ㵱ǰ����:%d ������", 
		pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str(), pAlarmTag->vecAlarming.size());

	pAlarmingInfo->strThresholdOnProduce = szThesholdOnProduce; // ��������ʱ������ֵ
	pAlarmingInfo->bAlarming = 1; // ����һ����������״̬
	pAlarmingInfo->bConfirmed = 0; // δȷ��״̬
	pAlarmingInfo->strConfirmUserName = "";
	pAlarmingInfo->strValueOnRecover = "";
	pAlarmingInfo->tmRecover.nSeconds = pAlarmingInfo->tmRecover.nMilSeconds = 0; // ���ø�ʱ��
	pAlarmingInfo->tmConfirm.nSeconds = pAlarmingInfo->tmRecover.nMilSeconds = 0; // ����ȷ��ʱ�䣬����ȷ��ʱ�䱣��ԭ���ļ�¼��������ǰ�ظ������Ƿ�Ϊ0��Ӧ������ȷ��
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, pAlarmingInfo->strValueOnProduce); // �����ݿ����ʱ��ʹ�����ݿ��ȡ��ʱ����������ڵ�ʱ��
	if (!bPublishAlarm) // ��ʼ�����ݿ����
	{
		pAlarmingInfo->tmProduce.nSeconds = nProduceSec;
		pAlarmingInfo->tmProduce.nMilSeconds = nProduceMSec;
		pAlarmingInfo->nRepetitions = nRepetitions;
	}
	else
	{
		// �ǲ�������ʱ��Ͳ���ʱ��ֵ��ȡ����֮ǰ�ı�����û�б�ȷ�ϡ������δ��ȷ�ϵı�������ô�ظ�������Ȼ>0
		if (pAlarmingInfo->nRepetitions <= 0) // ˵��֮ǰû�б�����û��δȷ�ϵ��ѻָ�����������������ʱ��
		{
			pAlarmingInfo->tmProduceFirst.nSeconds = PKTimeHelper::GetHighResTime(&pAlarmingInfo->tmProduceFirst.nMilSeconds);// ���õ�һ�α����Ĳ���ʱ��
		}

		pAlarmingInfo->tmProduce.nSeconds = PKTimeHelper::GetHighResTime(&pAlarmingInfo->tmProduce.nMilSeconds);// �������µı�������ʱ��
			
		pAlarmingInfo->nRepetitions++;  // ����������1
	}

	ReCalcTagAlarmStatus(pAlarmTag->pTag);// ����tag���µ�ÿ�����������¼���һ��tag��ı���״̬
	if (bPublishAlarm)
		PublishTagAlarming(pAlarmingInfo,ALARM_PRODUCE);
	
	WriteTagAlarms(pAlarmTag->pTag); // ���õ�����б�����Ϣд�뱨���ڴ����ݿ�db1
	if (pAlarmTag->pTag->pObject)
		pAlarmTag->pTag->pObject->UpdateToDataMemDB(m_jsonWriter); // ���õ�����б���д���ڴ����ݿ�db0

	WriteSystemAlarmInfo2MemDB();  // ����ϵͳ������Ϣ

	MAIN_TASK->m_redisRW1.commit();
	//MAIN_TASK->m_redisRW0.commit();
}

// ȷ�ϱ���
// һ�������ı�ʶ�ţ�tagname+almtype+almindex
// nTypeΪ�õ�ı������ͣ�nIndexΪ�������ڵı���������
// bPublishAlarm �Ƿ���Ҫ����ʵʱ������Ϣ���ڴ����ݿ⣨����������ʼ�����ݿ�����ϴ�δ��ɵı���ʱ����Ҫ������
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


// ȷ�ϱ���
// һ�������ı�ʶ�ţ�tagname+almtype+almindex
// nTypeΪ�õ�ı������ͣ�nIndexΪ�������ڵı���������
// bPublishAlarm �Ƿ���Ҫ����ʵʱ������Ϣ���ڴ����ݿ⣨����������ʼ�����ݿ�����ϴ�δ��ɵı���ʱ����Ҫ������
void CEAProcessor::OnEAConfirmOne(CAlarmingInfo *pAlarmingInfo, const char *szConfirmer, bool bPublishAlarm, unsigned int nConfirmSec, unsigned int nConfirmMSec)
{
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	pAlarmingInfo->strConfirmUserName = szConfirmer;
	string strTagValue;
	pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, strTagValue);
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "OnEAConfirmOne()��ȷ��һ����������.tag��%s,jm:%s,value��%s, ֮ǰ�õ㹲��:%d ������",
		pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str(), pAlarmTag->vecAlarming.size());

	if (bPublishAlarm) // ʵ�ʲ��������ı���
	{
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnEAConfirmOne��ȷ��һ����������.tag��%s,jm:%s,value��%s, before HasAuthByTag",
		//	pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str());
		if (!USERAUTH_TASK->HasAuthByTagAndLoginName(pAlarmingInfo->strConfirmUserName, pAlarmTag->pTag)) // ��Ȩ��
		{
			//g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnEAConfirmOne��ȷ��һ����������.tag��%s,jm:%s,value��%s, after HasAuthByTag",
			//	pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str());
			return;
		}
		//g_logger.LogMessage(PK_LOGLEVEL_INFO, "OnEAConfirmOne��ȷ��һ����������.tag��%s,jm:%s,value��%s, after HasAuthByTag",
		//	pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), strTagValue.c_str());

		// ��������Ȩ�޵������
		pAlarmingInfo->tmConfirm.nSeconds = PKTimeHelper::GetHighResTime(&pAlarmingInfo->tmConfirm.nMilSeconds);// ����ȷ��ʱ��
		pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, pAlarmingInfo->strValueOnConfirm);
		pAlarmingInfo->strConfirmUserName = szConfirmer;
	}
	else // ��ʼ��ȡ�����ı���
	{
		pAlarmingInfo->tmConfirm.nSeconds = nConfirmSec;
		pAlarmingInfo->tmConfirm.nMilSeconds = nConfirmMSec;
		pAlarmTag->pTag->curVal.GetStrValue(pAlarmTag->pTag->nDataTypeClass, pAlarmingInfo->strValueOnConfirm); // ֵ�Ѿ���Ԥ�ȸ�ֵ�ˣ�����ֻ��Ҫ��ֵ����
	}

	pAlarmingInfo->bConfirmed = 1;
	ReCalcTagAlarmStatus(pAlarmTag->pTag); // ����tag���µ�ÿ�����������¼���һ��tag��ı���״̬
	if (bPublishAlarm)
		PublishTagAlarming(pAlarmingInfo, ALARM_CONFIRM);

	if (!pAlarmingInfo->bAlarming) // �����ȷ���ѻָ�����ôɾ����������
	{
		pAlarmingInfo->nRepetitions = 0;
		pAlarmingInfo->nRepetitions = 0;
		for (vector<CAlarmingInfo *>::iterator itAlarming = pAlarmTag->vecAlarming.begin(); itAlarming != pAlarmTag->vecAlarming.end(); itAlarming++)
		{
			if (*itAlarming == pAlarmingInfo)
			{
				pAlarmTag->vecAlarming.erase(itAlarming); // ɾ����������
				break;
			}
		}
		//pAlarmTag->tmProduce.nSeconds = pAlarmTag->tmProduce.nMilSeconds = 0;
	}

	WriteTagAlarms(pAlarmTag->pTag); // ���õ�����б�����Ϣд�뱨���ڴ����ݿ�db1
	if (pAlarmTag->pTag->pObject)
		pAlarmTag->pTag->pObject->UpdateToDataMemDB(m_jsonWriter); // ���õ�����б���д���ڴ����ݿ�db0
	WriteSystemAlarmInfo2MemDB();  // ����ϵͳ������Ϣ

	MAIN_TASK->m_redisRW1.commit();
	//MAIN_TASK->m_redisRW0.commit();
	//g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "��������.tag��%s,jm:%s,maxlevel��%d",
	//	pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), pAlarmTag->pTag->nMaxAlarmLevel);
}

// �����Ʊ���ʱ
void CEAProcessor::OnSuppressAlarm(CAlarmTag *pAlarmTag, char *szProduceTime, string strSuppressTime, string strUserName)
{
	for (int i = 0; i < pAlarmTag->vecAlarming.size(); i++)
	{
		CAlarmingInfo *pAlarmingInfo = pAlarmTag->vecAlarming[i];
		// pAlarmTag->strSuppressTime = strSuppressTime;
		PublishTagAlarming(pAlarmingInfo, ALARM_SUPPRESS);
	}
	WriteTagAlarms(pAlarmTag->pTag); // ���õ�����б�����Ϣд�뱨���ڴ����ݿ�db1
	if (pAlarmTag->pTag->pObject)
		pAlarmTag->pTag->pObject->UpdateToDataMemDB(m_jsonWriter); // ���õ�����б���д���ڴ����ݿ�db0
	WriteSystemAlarmInfo2MemDB();  // ����ϵͳ������Ϣ
}

int CEAProcessor::PublishOneAlarmAction2Web(char *szAlarmActionType, CAlarmingInfo *pAlarmingInfo, char *szTmProduce, char *szTmConfirm, char *szTmRecover)
{
	CAlarmTag *pAlarmTag = pAlarmingInfo->pAlarmTag;
	Json::Value jsonAlarmAction;
	string strDeviceName = "";
	if (pAlarmingInfo->pAlarmTag->pTag->pObject && pAlarmingInfo->pAlarmTag->pTag->pObject->m_pDevice) // ����ģʽ�µ��豸����
		strDeviceName = pAlarmingInfo->pAlarmTag->pTag->pObject->m_pDevice->m_strName;
	else if (pAlarmingInfo->pAlarmTag->pTag->pDevice) // �豸ģʽ�µ��豸����
		strDeviceName = pAlarmingInfo->pAlarmTag->pTag->pDevice->m_strName;
	jsonAlarmAction[ALARM_DEVICE_NAME] = strDeviceName; // �ȸ�ֵ����Ϊ���ݿ��ȱ���豸����һ�У������޷��������

	if (pAlarmingInfo->pAlarmTag->pTag->pObject) // ����ģʽ��
	{
		jsonAlarmAction[ALARM_OBJECT_NAME] = pAlarmingInfo->pAlarmTag->pTag->pObject->strName;
		jsonAlarmAction[ALARM_OBJECT_DESCRIPTION] = pAlarmingInfo->pAlarmTag->pTag->pObject->strDescription;
	}
	else // �豸ģʽ�£����豸������������
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
	if (pAlarmingInfo->bConfirmed && pAlarmingInfo->bAlarming) // ��ȷ�ϣ�δ�ָ���д����ֵ�ͱ���ǰֵ���ָ�ֵΪ0
	{
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)�յ�%s��ȷ�ϱ���������%s", pAlarmingInfo->strConfirmUserName.c_str(), strAlarmTagName.c_str());
	}
	else if (!pAlarmingInfo->bConfirmed && pAlarmingInfo->bAlarming) // δȷ�ϣ�δ�ָ���д����ֵ�ͱ���ǰֵ���ָ�ֵΪ0
	{
		// ����tag�㱨������
		jsonAlarmAction[ALARM_CONFIRMTIME] = "";
		jsonAlarmAction[ALARM_RECOVERTIME] = "";
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)%s.%s����������", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str());
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)%s.%s:produce new alarm", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str());
#endif
	}
	else if (!pAlarmingInfo->bConfirmed && !pAlarmingInfo->bAlarming)  // δȷ�ϣ��ѻָ���д�ָ�ʱ��ֵ������ֵ�ͱ�����ǰֵΪ0
	{
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)%s.%s�������ָ�", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str());
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)%s.%s��alarm recovered", pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str());
#endif
	}
	else if (pAlarmingInfo->bConfirmed && !pAlarmingInfo->bAlarming) // ��ȷ�ϣ��ѻָ��� ��Ҫɾ����������Ϊȱʡֵ
	{
		pAlarmingInfo->nRepetitions = 0;
		//pAlarmTag->nLevel = 0; ������������ܱ���Ϊ0
		pAlarmingInfo->strValueOnProduce = "";
		pAlarmingInfo->strConfirmUserName = "";
		pAlarmingInfo->strValueOnRecover = "";
		pAlarmingInfo->strValueOnConfirm = "";
	
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)������tag:%s,judgemethod:%s,producetime:%s����ȷ���ѻָ�,�ӱ����б��Ƴ�, ȷ���ߣ�%s, ȷ��ʱ�䣺%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, pAlarmingInfo->strConfirmUserName.c_str(), szTmConfirm);
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)alarm[tag:%s,judgemethod:%s,producetime:%s] confirmed and recovered, will remove from alarminglist. confirmer��%s, confirm time��%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, pAlarmingInfo->strConfirmUserName.c_str(), szTmConfirm);
#endif
	}

	// �ѱ���ת��Ϊ����
	Json::Value jsonAlarmActions;
	jsonAlarmActions.append(jsonAlarmAction);
	string strAlarms = m_jsonWriter.write(jsonAlarmActions); // ���ܸ���

	int nRet = MAIN_TASK->m_redisRW0.publish(CHANNELNAME_ALARMING, strAlarms.c_str());

	if(nRet != 0)
	{
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)������tag:%s,judgemethod:%s,producetime:%s�����͵�����֪ͨ����ʧ�ܣ������룺%d�����ݣ�%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, nRet, strAlarms.c_str());
#else
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)alarm[tag:%s,judgemethod:%s,producetime:%s] sent to alarm notify queue failed��retcode��%d, content��%s",
			pAlarmTag->pTag->strName.c_str(), pAlarmTag->strJudgeMethod.c_str(), szTmProduce, nRet, strAlarms.c_str());
#endif
	}

	// ���ͱ�������ʷ�¼���Ϣ������evttype����ֶ�
	jsonAlarmAction[EVENT_TYPE_MSGID] = EVENT_TYPE_ALARM;
	Json::Value jsonEvents;
	jsonEvents.append(jsonAlarmAction);
	string strEvents = m_jsonWriter.write(jsonEvents);

	nRet = MAIN_TASK->m_redisRW0.publish(CHANNELNAME_EVENT, strEvents.c_str());
	if(nRet != 0)
	{
#ifdef _WIN32
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "(PublishOneAlarmAction2Web)������tag:%s,judgemethod:%s,producetime:%s�����͵��¼�֪ͨ����ʧ�ܣ������룺%d�����ݣ�%s",
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
* ��֯��������Ϣת����pkcserver, AlarmRouteTask���д���
*
* @param    -[in]   CAlarmTag *pAlarmTag : [������ṹ�壬�������б�������Ϣ]
* @param    -[in]   char* szType : [����״̬, recover/confirm/produce]
*
* @return   PK_SUCCESS/ICV_FAILURE.
*
* @version  11/19/2016	liucaixia   Initial Version
*
**/
int CEAProcessor::PublishTagAlarming(CAlarmingInfo *pAlarmingInfo, char* szActionType)
{
	// ʱ��ת�������߾���ʱ��ת����ʱ���ַ���
	char szTmProduce[PK_TIMESTRING_MAXLEN] = {'\0'};
	char szTmConfirm[PK_TIMESTRING_MAXLEN] = {'\0'};
	char szTmRecover[PK_TIMESTRING_MAXLEN] = {'\0'};

	// ����ʱ�������е�
	PKTimeHelper::HighResTime2String(szTmProduce, sizeof(szTmProduce), pAlarmingInfo->tmProduce.nSeconds, pAlarmingInfo->tmProduce.nMilSeconds);
	if (pAlarmingInfo->bConfirmed) // �����ȷ�ϲ���ȷ��ʱ��
		PKTimeHelper::HighResTime2String(szTmConfirm, sizeof(szTmConfirm), pAlarmingInfo->tmConfirm.nSeconds, pAlarmingInfo->tmConfirm.nMilSeconds);
	if (!pAlarmingInfo->bAlarming)// ����ѻָ������лָ�ʱ��
		PKTimeHelper::HighResTime2String(szTmRecover, sizeof(szTmRecover), pAlarmingInfo->tmRecover.nSeconds, pAlarmingInfo->tmRecover.nMilSeconds);

	// ���������͵�Web
	PublishOneAlarmAction2Web(szActionType, pAlarmingInfo, szTmProduce, szTmConfirm, szTmRecover);
	return PK_SUCCESS;
}

// system.alarm = {alarmingcount:5,alarmcount:10...}
// alarmtype.�����ޱ��� = {alarmingcount:5,alarmcount:10...}
int CEAProcessor::WriteSystemAlarmInfo2MemDB()
{
	// �����ڴ����ݿ�ı�������, system.alarm = {alarmingcount:5,alarmcount:10...}
	Json::Value jsonAlarmStatus;
	m_curAlarmInfo.GenerateAlarmInfo2Json(jsonAlarmStatus);
	string strValue = m_jsonWriter.write(jsonAlarmStatus); 
	vector<string> vecTagName, vecTagVTQ;
	vecTagName.push_back(TAGNAME_SYSTEM_ALARM);
	vecTagVTQ.push_back(strValue);
	//MAIN_TASK->m_redisRW0.set(TAGNAME_SYSTEM_ALARM, strValue.c_str()); // δ�ύ

	// ÿ����������,д��һ����, alarmtype.�����ޱ��� = {alarmingcount:5,alarmcount:10...}
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

		//MAIN_TASK->m_redisRW0.set(strTagName.c_str(), strValue.c_str()); // δ�ύ
	}

	MAIN_TASK->m_redisRW0.mset(vecTagName, vecTagVTQ); // δ�ύ
	vecTagName.clear();
	vecTagVTQ.clear();
	return 0;
}

/**
* ��һ���������״̬�����ı�ʱ�����¶�Ӧtag��ı�������
*
* @param    -[in]   CDataTag &pTag : [�������Ӧ��tag��]
* @param    -[in]   string strActType : [����״̬, actrecover/actconfirm/actproduce]
*
* @return   PK_SUCCESS/ICV_FAILURE.
*
* @version  11/19/2016	liucaixia   Initial Version
**/
int CEAProcessor::ReCalcTagAlarmStatus(CDataTag *pDataTag)
{
	if(!pDataTag)
		return -1;

	// ����tag�㱨������,tag�㱨������Ϊtag�����б�������󱨾�����
	int nTagAlmCount = 0;
	int nTagAlmUnAckCount = 0;
	int nTagAlarmingCount = 0;
	int nTagAlmAckedCount = 0;
	int nMaxLevel = 0;

	// ȫ����0
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
			if (pAlarmingInfo->bAlarming) // ���ڱ���
			{
				pAlarmTag->m_alarmInfo.m_nAlarmingCount++;
				if (!pAlarmingInfo->bConfirmed) // ��δȷ��
					pAlarmTag->m_alarmInfo.m_nAlarmingUnAckCount++;
				pAlarmTag->m_alarmInfo.m_nMaxAlarmLevel = pAlarmingInfo->pAlarmTag->nLevel; // ��������ֵ������������򱨾�����==������ļ���
				pAlarmTag->m_alarmInfo.m_nMinAlarmLevel = pAlarmingInfo->pAlarmTag->nLevel; // ��������ֵ������������򱨾�����==������ļ���
			}

			if (!pAlarmingInfo->bConfirmed)
				pAlarmTag->m_alarmInfo.m_nAlarmUnConfirmedCount ++;
			if (pAlarmingInfo->bConfirmed)
				pAlarmTag->m_alarmInfo.m_nAlarmConfirmedCount++;
		}

		// ��ֵ��Ϊÿ����������
		if (!pAlarmTag->strAlarmTypeName.empty())
		{
			CAlarmInfo *pAlarmInfo = pDataTag->mapAlarmTypeName2AlarmInfo[pAlarmTag->strAlarmTypeName];
			*pAlarmInfo += pAlarmTag->m_alarmInfo;
		}
		pDataTag->m_curAlarmInfo += pAlarmTag->m_alarmInfo; // �������
	}

	// �������ı�������
	ReCalcObjectAlarmStatus(pDataTag->pObject);
	return PK_SUCCESS;
}

/**
* ��һ���������״̬�����ı�ʱ�����¶�Ӧtag��ı�������
*
* @param    -[in]   CDataTag &pTag : [�������Ӧ��tag��]
* @param    -[in]   string strActType : [����״̬, actrecover/actconfirm/actproduce]
*
* @return   PK_SUCCESS/ICV_FAILURE.
*
* @version  11/19/2016	liucaixia   Initial Version
**/
int CEAProcessor::ReCalcObjectAlarmStatus(CMyObject *pObject)
{
	if(!pObject)
		return -1;

	// ����tag�㱨������,tag�㱨������Ϊtag�����б�������󱨾�����
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
		if(pDataTag->m_curAlarmInfo.m_nAlarmCount <= 0) // �ޱ��������Բ���������
			continue;

		// ������ͬ���͵ı�������Ϣ
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

	// ��������ϵͳ�ı�����Ϣ
	this->ReCalcSystemAlarmStatus();

	return PK_SUCCESS;
}

/**
* ��һ���������״̬�����ı�ʱ�����¶�Ӧtag��ı�������
*
* @param    -[in]   CDataTag &pTag : [�������Ӧ��tag��]
* @param    -[in]   string strActType : [����״̬, actrecover/actconfirm/actproduce]
*
* @return   PK_SUCCESS/ICV_FAILURE.
*
* @version  11/19/2016	liucaixia   Initial Version
**/
int CEAProcessor::ReCalcSystemAlarmStatus()
{
	// ����tag�㱨������,tag�㱨������Ϊtag�����б�������󱨾�����
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

		// ����ͬһ���������͵ı�����Ϣ
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
�޸ı�����������ֵ�Լ��Ƿ�������ѡ�Ϊ0������Ϊ1 �������⣬��threshold���ò��ŵģ�
{"tagname":"tag1",judgemethod:"hh","alarmparam":{"enable":1,"level":10,"threshold":250,"name":"����һ������","alarmtype":"�����ޱ�������"}}
*/
int CEAProcessor::ProcessAlarmParamChange(string &strTagName, string &strJudgemethod, string &strAlarmParam, string &strUserName)
{
	int nRet = 0;
	// ����tag�����ҵ�tag���Ӧ������
	map<string, CDataTag *>::iterator itMap = MAIN_TASK->m_mapName2Tag.find(strTagName);
	if (itMap == MAIN_TASK->m_mapName2Tag.end()){
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ProcessAlarmParamChange(%s,%s,%s,%s)��cannot find taginfo by name", 
			strTagName.c_str(), strJudgemethod.c_str(), strAlarmParam.c_str(), strUserName.c_str());
		nRet = -3;
		return nRet;
	}

	CDataTag *pTag = itMap->second;
	if (pTag == NULL){
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ProcessAlarmParamChange(%s,%s,%s,%s)��found taginfo==NULL",
			strTagName.c_str(), strJudgemethod.c_str(), strAlarmParam.c_str(), strUserName.c_str());
		nRet = -4;
		return nRet;
	}

	Json::Reader jsonReader;
	Json::Value jsonAlarmParam;
	if (!jsonReader.parse(strAlarmParam.c_str(), jsonAlarmParam, false))
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ProcessAlarmParamChange(%s,%s,%s,%s)��alarmParam is not json format",
			strTagName.c_str(), strJudgemethod.c_str(), strAlarmParam.c_str(), strUserName.c_str());
		return -5;
	}

	// ����������б����㣬�����ڱ����ĵ���Ѿ��ָ��ĵ����ȷ��
	for (int i = 0; i < pTag->vecAllAlarmTags.size(); i++)
	{
		CAlarmTag *pAlarmTag = pTag->vecAllAlarmTags[i];
		// δȷ�ϵı���������б�����Ϊ*���ջ��ߺ͸ñ������
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
	// alarmtypename��һ�����,����Ҫ����ת��Ϊid�ģ�
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
