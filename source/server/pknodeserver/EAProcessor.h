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

#define ALM_TYPE_UNKNOWN    0	// δ֪��������
#define ALM_TYPE_LOLO       1	// �͵ͱ���
#define ALM_TYPE_LO         2	// �ͱ���
#define ALM_TYPE_HI         3	// �߱���
#define ALM_TYPE_HIHI       4 	// �߸߱���
#define ALM_TYPE_ROC        5	// �仯�ʱ���
#define ALM_TYPE_COS		6	// ��λ����
#define ALM_TYPE_OPEN       26	// 0->1����
#define ALM_TYPE_CLOSE      27	// 1->0����

//////////////////// ����״̬�궨�� //////////////////////////
#define ALM_STATUS_OK	0	// no alarm(acked & recovered)			0000 0000
#define ALM_STATUS_ACK	1	// acked alarm(ȷ��)					0000 0001
#define ALM_STATUS_RTN	2	// return to normal(�ָ�)				0000 0010
#define ALM_STATUS_ALM	3	// in alarm��������						0000 0011

#define ALM_STATUS_ALM_MASK		1	// ����δ�ָ���MASK				0000 0001
#define ALM_STATUS_UNACK_MASK	2	// ����δȷ�ϵ�MASK				0000 0010

#include "json/json.h"
#include "json/writer.h"
//////// �����ṹ�壬ֻ���������Ҫ����ü������� //////////
typedef struct _PK_ALARM
{
	char 			szAlmSeq[PK_NAME_MAXLEN];		// alarm sequence ȡ������ʱ�������ʱ�������
	char			szNodeName[PK_NAME_MAXLEN];	// node name string ptr
	char			szTagName[PK_NAME_MAXLEN];		// tag name string ptr
	unsigned char	ucType;			// alarm type
	unsigned char	ucStatus;		// alarm status

	_PK_ALARM()
	{
		szAlmSeq[0] = '\0'; // ȡ������ʱ�������ʱ�������
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
	// key�������������ֶ�
	char				szTagName[PK_NAME_MAXLEN];			// tag name
	int					nAlarmCat;							// �������
	int					nAlarmIndex;						// �ñ�������ڵ�����������

	char				szAlarmCat[PK_NAME_MAXLEN];			// alarm category���糬�ޱ��������ڶ�ֵ����
	char				szAlarmType[PK_NAME_MAXLEN];		// alarm type,�糬���ޣ����ڶ�ֵ

	char				szCurValue[PK_NAME_MAXLEN];			// current alarm value.��ʹboolҲת��Ϊdouble��ʾ
	char				szPreValue[PK_NAME_MAXLEN];			//��ǰ����/ȷ��֮ǰ��ֵ 
	char				szNodeName[PK_NAME_MAXLEN];			// node name
	char                szTagDesc[PK_DESC_MAXLEN];			//tag description
	char				szConfirmPerson[PK_NAME_MAXLEN];	// field name
	char				szEgu[PK_DESC_MAXLEN];				// unit name (EGU)

	char                szSubSystemName[PK_NAME_MAXLEN];	//subsystem name
	char				szDriverName[PK_DESC_MAXLEN];			// tag���Ӧ����������
	char				szDeviceName[PK_DESC_MAXLEN];		// tag���Ӧ���豸����

	char				szExtParam1[PK_PARAM_MAXLEN];		//extend param
	char				szExtParam2[PK_PARAM_MAXLEN];		//extend param
	char				szExtParam3[PK_PARAM_MAXLEN];		//extend param
	char				szExtParam4[PK_PARAM_MAXLEN];		//extend param

	int					nPriority;		// priority of alarm
	unsigned int		nRepCount;			// �ڱ�����ȷ��֮ǰ�����������Ĵ��� repetition time of alarm
	bool				bAlarming;		// �Ƿ��ѻָ�
	bool				bConfirmed;		// �Ƿ���ȷ��

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

	bool HasAlarm(CAlarmTag *pAlarmTag); // �Ƿ��б�������ȷ���ѻָ��ҳ���һ��ʱ������Ϊ����û���ˣ�������Ϊ����������

	int ProcessTagAlarms(CDataTag *pTag, bool bConfirmAlarmAction);
	void OnEARecover(CAlarmingInfo *pAlarmingInfo, bool bPublishAlarm, unsigned int nRecoverSec = 0, unsigned int nRecoverMSec = 0);
	void OnEAProduce(CAlarmingInfo *pAlarmInfo, const char *szThresoldOnProduce, bool bPublishAlarm, unsigned int nProduceSec = 0, unsigned int nProduceMSec = 0, int nRepetitions = -1);
	void OnEAConfirm(CAlarmTag *pAlarmTag, char *szAlarmProduceTime, char *szConfirmer, bool bPublishAlarm, unsigned int nConfirmSec = 0, unsigned int nConfirmMSec = 0);
	void OnEAConfirmOne(CAlarmingInfo *pAlarmingInfo, const char *szConfirmer, bool bPublishAlarm, unsigned int nConfirmSec, unsigned int nConfirmMSec);

	void OnSuppressAlarm(CAlarmTag *pAlarmTag, char *szProduceTime, string strSuppressTime, string strUserName);

	int CheckAlarmsOfTag(CDataTag *pTag, bool &bAlarmChanged); // ���ر����Ƿ�ı�

	// �б����������ָ���ȷ��֮�󣬽��µı�����Ϣpublish��ʵʱ�����������
	int PublishTagAlarming(CAlarmingInfo *pAlarmingInfo, char* szActionType);

	// ���µı���������ָ��󣬽��µĶ�����Ϣ������������Ϣ��д�뵽�ڴ����ݿ�
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
