/**************************************************************
*  Filename:    SockHandler.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: SockHandler.h.
*
*  @author:     lijingjing
*  @version     05/29/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _EA_PROCESSOR_H_
#define _EA_PROCESSOR_H_

#include "PKNodeServer.h"
#include "pkmemdbapi/pkmemdbapi.h"

#define ALM_TYPE_UNKNOWN    0	// 未知报警类型
#define ALM_TYPE_LOLO       1	// 低低报警
#define ALM_TYPE_LO         2	// 低报警
#define ALM_TYPE_HI         3	// 高报警
#define ALM_TYPE_HIHI       4 	// 高高报警
#define ALM_TYPE_ROC        5	// 变化率报警
#define ALM_TYPE_COS		6	// 变位报警
#define ALM_TYPE_OPEN       26	// 0->1报警
#define ALM_TYPE_CLOSE      27	// 1->0报警

//////////////////// 报警状态宏定义 //////////////////////////
#define ALM_STATUS_OK	0	// no alarm(acked & recovered)			0000 0000
#define ALM_STATUS_ACK	1	// acked alarm(确认)					0000 0001
#define ALM_STATUS_RTN	2	// return to normal(恢复)				0000 0010
#define ALM_STATUS_ALM	3	// in alarm（报警）						0000 0011

#define ALM_STATUS_ALM_MASK		1	// 报警未恢复的MASK				0000 0001
#define ALM_STATUS_UNACK_MASK	2	// 报警未确认的MASK				0000 0010

#include "json/json.h"
#include "json/writer.h"
//////// 报警结构体，只包含插件需要的最常用几种属性 //////////
typedef struct _PK_ALARM
{
	char 			szAlmSeq[PK_NAME_MAXLEN];		// alarm sequence 取发生的时间戳，到时分秒毫秒
	char			szNodeName[PK_NAME_MAXLEN];	// node name string ptr
	char			szTagName[PK_NAME_MAXLEN];		// tag name string ptr
	unsigned char	ucType;			// alarm type
	unsigned char	ucStatus;		// alarm status

	_PK_ALARM()
	{
		szAlmSeq[0] = '\0'; // 取发生的时间戳，到时分秒毫秒
		szNodeName[0] = '\0';
		szTagName[0] = '\0';
		ucType = ALM_TYPE_UNKNOWN;
		ucStatus = ALM_STATUS_OK;
	}
}PK_ALARM;

// ID: tagname + nAlarmcat + nIndex
class CAlarmRec
{
public:
	// key包括以下三个字段
	char				szTagName[PK_NAME_MAXLEN];			// tag name
	int					nAlarmCat;							// 报警类别
	int					nAlarmIndex;						// 该报警类别内的数组索引号

	char				szAlarmCat[PK_NAME_MAXLEN];			// alarm category，如超限报警，等于定值报警
	char				szAlarmType[PK_NAME_MAXLEN];		// alarm type,如超上限，等于定值

	char				szCurValue[PK_NAME_MAXLEN];			// current alarm value.即使bool也转化为double表示
	char				szPreValue[PK_NAME_MAXLEN];			//当前报警/确认之前的值 
	char				szNodeName[PK_NAME_MAXLEN];			// node name
	char                szTagDesc[PK_DESC_MAXLEN];			//tag description
	char				szConfirmPerson[PK_NAME_MAXLEN];	// field name
	char				szEgu[PK_DESC_MAXLEN];				// unit name (EGU)

	char                szSubSystemName[PK_NAME_MAXLEN];	//subsystem name
	char				szDriverName[PK_DESC_MAXLEN];			// tag点对应的驱动名称
	char				szDeviceName[PK_DESC_MAXLEN];		// tag点对应的设备名称

	char				szExtParam1[PK_PARAM_MAXLEN];		//extend param
	char				szExtParam2[PK_PARAM_MAXLEN];		//extend param
	char				szExtParam3[PK_PARAM_MAXLEN];		//extend param
	char				szExtParam4[PK_PARAM_MAXLEN];		//extend param

	int					nPriority;		// priority of alarm
	unsigned int		nRepCount;			// 在报警被确认之前，反复报警的次数 repetition time of alarm
	bool				bAlarming;		// 是否已恢复
	bool				bConfirmed;		// 是否已确认

	HighResTime			tmProduce;			// fire time
	HighResTime			tmConfirm;			// confirm time
	HighResTime			tmRecover;			// recover time

	CAlarmRec()
	{
		memset(szPreValue, 0x00, sizeof(szPreValue));
		memset(szNodeName,0x00, sizeof(szNodeName));
		memset(szTagName, 0x00, sizeof(szTagName));
		memset(szEgu, 0x00, sizeof(szEgu));
		memset(szSubSystemName, 0x00, sizeof(szSubSystemName));
		memset(szDriverName, 0x00, sizeof(szDriverName));
		memset(szDeviceName, 0x00, sizeof(szDeviceName));
		memset(szTagDesc,0x00,sizeof(szNodeName));
		memset(szConfirmPerson, 0x00, sizeof(szNodeName));
		memset(szAlarmCat, 0x00, sizeof(szAlarmCat));
		memset(szAlarmType, 0x00, sizeof(szAlarmType));
		memset(szExtParam1, 0x00, sizeof(szNodeName));
		memset(szExtParam2, 0x00, sizeof(szNodeName));
		memset(szExtParam3, 0x00, sizeof(szNodeName));
		memset(szExtParam4, 0x00, sizeof(szNodeName));
		nRepCount = nPriority = 0;
		bAlarming = bConfirmed = false;
	}
};

typedef struct _ALM_JUDGEMETHOD_FIELD
{
	string strJudgeMethod;

	string strFieldAlarming;
	string strFieldAlmProduceTime;
	string strValOnProduce;
	string strValBeforeProduce;
	string strValAfterProduce;
	string strFieldAlmRecoverTime;
	string strFieldAlmAfterProduce;
	string strFieldAlmConfirmed;
	string strFieldAlmConfirmer;
	string strFieldAlmConfirmTime;
	string strFieldAlmEnable;
	string strFieldThresh;
	string strFieldLevel;
	string strFieldType;
	string strRepetitions;
	string strAlarmName;
	string strAlarmType;
	string strThreshold;
}ALM_JUDGEMETHOD_FIELD;

class CEAProcessor
{
public:
	CEAProcessor() ;
	virtual ~CEAProcessor();

	bool HasAlarm(CAlarmTag *pAlarmTag); // 是否有报警。已确认已恢复且超过一段时间则认为报警没有了，否则认为报警还存在

	int ProcessTagAlarms(CDataTag *pTag, bool bConfirmAlarmAction);
	void OnEARecover(CAlarmingInfo *pAlarmingInfo, bool bPublishAlarm, unsigned int nRecoverSec = 0, unsigned int nRecoverMSec = 0);
	void OnEAProduce(CAlarmingInfo *pAlarmInfo, const char *szThresoldOnProduce, bool bPublishAlarm, unsigned int nProduceSec = 0, unsigned int nProduceMSec = 0, int nRepetitions = -1);
	void OnEAConfirm(CAlarmTag *pAlarmTag, char *szAlarmProduceTime, char *szConfirmer, bool bPublishAlarm, unsigned int nConfirmSec = 0, unsigned int nConfirmMSec = 0);
	void OnEAConfirmOne(CAlarmingInfo *pAlarmingInfo, const char *szConfirmer, bool bPublishAlarm, unsigned int nConfirmSec, unsigned int nConfirmMSec);

	void OnSuppressAlarm(CAlarmTag *pAlarmTag, char *szProduceTime, string strSuppressTime, string strUserName);

	int CheckAlarmsOfTag(CDataTag *pTag, bool &bAlarmChanged); // 返回报警是否改变

	// 有报警产生、恢复或确认之后，将新的报警信息publish到实时报警处理服务
	int PublishTagAlarming(CAlarmingInfo *pAlarmingInfo, char* szActionType);

	// 有新的报警产生或恢复后，将新的对象信息（包含报警信息）写入到内存数据库
	int GetTagAlarmsStatus(CDataTag *pTag, Json::Value &jsonAlarmings);
	void UpdateOneAlarmTagStatus(CAlarmingInfo *pAlarmingInfo, Json::Value &jsonAlarmsStatus);

	int ReCalcTagAlarmStatus(CDataTag *pDataTag);
	int ReCalcObjectAlarmStatus(CMyObject *pObject);
	int ReCalcSystemAlarmStatus();
	int PublishOneAlarmAction2Web(char *szJudgeType, CAlarmingInfo *pAlarmingInfo, char *szTmProduce, char *szTmConfirm, char *szTmRecover);
	int ProcessAlarmParamChange(string &strTagName, string &strJudgemethod, string &strAlarmParam, string &strUserName);
	int UpdateAlarmField2DB(string &strTagName, string &strJudgeMethod, const char *szDBFieldName, string strFieldValue, string &strUserName);

public:
	void WriteTagAlarms(CDataTag *pTag);
	int WriteSystemAlarmInfo2MemDB();

	bool JudgeAlarming_IntClass(CAlarmTag *pAlarmTag, char *szThreshold, int nThresholdLen);
	bool JudgeAlarming_RealClass(CAlarmTag *pAlarmTag, char *szThreshold, int nThresholdLen);
	bool JudgeAlarming_TextClass(CAlarmTag *pAlarmTag, char *szThreshold, int nThresholdLen);

public:
	CAlarmInfo	m_curAlarmInfo;
	Json::FastWriter  m_jsonWriter;
};

#endif // _EA_PROCESSOR_H_
