/**************************************************************
*  Filename:    SockHandler.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: EAProcessor.h.
*
*  @author:     shijupu
*  @version     05/29/2015  shijupu  Initial Version
**************************************************************/

#ifndef _NODESERVER_COMMON_DEFINE_H_
#define _NODESERVER_COMMON_DEFINE_H_

#include <ace/Basic_Types.h>
#include <ace/Time_Value.h>
#include "pkcomm/pkcomm.h"
#include "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "TagExprCalc.h"
#include "json/json.h"
#include <string>
#include <vector>
#include <map>
using namespace std;

extern CPKLog g_logger;

#define EPSINON		0.000001	// һ����С��ֵ�����ڸ������Ƚ�

//�����ж�����
#define ALARM_JUDGE_HHH_TEXT								"HHH"		// ���߸߸��ޱ��������ڵ��ڸ���ֵ�򱨾�, >=HHH
#define ALARM_JUDGE_HH_TEXT									"HH"		// ���߸��ޱ������ڵ��ڸ���ֵ��С��HHH�򱨾�, HHH>value>=HH
#define ALARM_JUDGE_H_TEXT									"H"			// �����ޱ��������ڵ��ڸ���ֵ��С��HH�򱨾� ,HH>value>=H
#define ALARM_JUDGE_L_TEXT									"L"			// �����ޱ�����С�ڵ��ڸ���ֵ�Ҵ���LL�򱨾�,LL<value<=L
#define ALARM_JUDGE_LL_TEXT									"LL"		// ���͵��ޱ�����С�ڵ��ڸ���ֵ�Ҵ���LLL�򱨾�,LLL<value<=LL
#define ALARM_JUDGE_LLL_TEXT								"LLL"		// ���͵͵��ޱ�����С�ڵ��ڸ���ֵ�򱨾�,value<=LLL
#define ALARM_JUDGE_FX_TEXT									"FX"		// �൱��EQ��������ʹ��, �𲽷���, FX, FX2, FX3
#define ALARM_JUDGE_OFF_TEXT								"OFF"		// ��1�仯Ϊ0�򱨾����������ͱ���֧�ָ��жϷ�����������ʹ��
#define ALARM_JUDGE_ON_TEXT									"ON"		// ��0�仯Ϊ1�򱨾����������ͱ���֧�ָ��жϷ�����������ʹ��
#define ALARM_JUDGE_EQ_TEXT									"EQ"		// ���ڸ÷�ֵ������֧��EQn��������EQ��EQ2��EQ3��EQ4......
#define ALARM_JUDGE_NE_TEXT									"NE"		// �ڲ����ڸ÷�ֵ������֧��NEn��������NE��NE2��NE3��NE4......
#define ALARM_JUDGE_GT_TEXT									"GT"		// ����theshold�򱨾��� value>threshold
#define ALARM_JUDGE_GTE_TEXT								"GTE"		// ���ڵ���theshold�򱨾��� value>=threshold
#define ALARM_JUDGE_LT_TEXT									"LT"		// С��theshold�򱨾��� value<threshold
#define ALARM_JUDGE_LTE_TEXT								"LTE"		// С�ڵ���theshold�򱨾��� value<=threshold
#define ALARM_JUDGE_GTELT_TEXT								"GTELT"		// ���ڵ���theshold��С��threshold2�� threshold<=value<threshold2���������Զ��Ÿ�����������ֵ   
#define ALARM_JUDGE_GTLT_TEXT								"GTLT"		// ����theshold��С��threshold2�� threshold<value<threshold2���������Զ��Ÿ�����������ֵ
#define ALARM_JUDGE_GTLTE_TEXT								"GTLTE"		// ����theshold��С�ڵ���threshold2�� threshold<value<=threshold2���������Զ��Ÿ�����������ֵ
#define ALARM_JUDGE_GTELTE_TEXT								"GTELTE"	// ���ڵ���theshold��С�ڵ���threshold2�� threshold<=value<=threshold2���������Զ��Ÿ�����������ֵ
#define ALARM_JUDGE_CONTAIN_TEXT							"CONTAIN"	// value����threshld��ֵ�򱨾�
#define ALARM_JUDGE_IN_TEXT									"IN"		// value����һ��ֵ֮�е�һ���򱨾�������ֵ�Զ��Ÿ�����֧���ַ�������ֵ����
#define ALARM_JUDGE_CHANGETOEQ_TEXT							"TOEQ"		// �仯�����ڸ÷�ֵʱ����������֧��EQn��������EQ��EQ2��EQ3��EQ4......
#define ALARM_JUDGE_QUALITYBAD_TEXT							"BAD"		// ����ΪBAD������������q ��=0��
#define ALARM_JUDGE_CHANGETOQUALITYBAD_TEXT					"TOBAD"		// ������GOOD�仯ΪBAD������������q==0 ��Ϊq ��=0��

#define ALARM_JUDGE_INVALID									0		// �Ƿ�
#define ALARM_JUDGE_HHH										1
#define ALARM_JUDGE_HH										2
#define ALARM_JUDGE_H										3
#define ALARM_JUDGE_L										4
#define ALARM_JUDGE_LL										5
#define ALARM_JUDGE_LLL										6
#define ALARM_JUDGE_FX										32
#define ALARM_JUDGE_OFF										33
#define ALARM_JUDGE_ON										34
#define ALARM_JUDGE_EQ										11		// ����
#define ALARM_JUDGE_NE										12		// ������
#define ALARM_JUDGE_GT										13		// ����
#define ALARM_JUDGE_GTE										14		// ���ڵ���
#define ALARM_JUDGE_LT										15		// С��
#define ALARM_JUDGE_LTE										16		// С�ڵ���
#define ALARM_JUDGE_GTELT									17		// ���ڵ��� �� С��   
#define ALARM_JUDGE_GTLT									18		// ���� �� С��  
#define ALARM_JUDGE_GTLTE									19		// ���� �� С�ڵ���
#define ALARM_JUDGE_GTELTE									20		// ���ڵ��� �� С�ڵ���
#define ALARM_JUDGE_CONTAIN									21		// ����
#define ALARM_JUDGE_IN										22		// ������һ������ֵ�Զ��Ÿ���
#define ALARM_JUDGE_CHANGETOEQ								23		// ��1����Чֵ�仯�����ڸ÷�ֵʱ����������֧��EQn��������EQ��EQ2��EQ3��EQ4......
#define ALARM_JUDGE_QUALITYBAD								24		// ����ΪBAD������������q ��=0��
#define ALARM_JUDGE_CHANGETOQUALITYBAD						25		// ������GOOD�仯ΪBAD������������q==0 ��Ϊq ��=0��

#define	DATATYPE_CLASS_OTHER		0	// �磺blob������δ֪��������
#define	DATATYPE_CLASS_INTEGER		1	// ���������ͣ�������bool��int8��int16��int32��int64��uint8��uint16��uint32��uint64
#define	DATATYPE_CLASS_REAL			2	// С�������ͣ�������float��double
#define	DATATYPE_CLASS_TEXT			3	// �ı����ͣ��������ַ���

// ��������
#define TAGTYPE_DEVICE				0
#define	TAGTYPE_CALC				1
#define	TAGTYPE_MEMVAR				2
#define	TAGTYPE_SIMULATE					3
#define	TAGTYPE_ACCUMULATE_TIME		4
#define	TAGTYPE_ACCUMULATE_COUNT	5

#define DRIVER_TYPE_PHYSICALDEVICE	0		// �����豸����

#define DRIVER_TYPE_SIMULATE		-5		// ģ������
#define DRIVER_TYPE_MEMORY			-1		// �ڴ��������
#define DRIVER_NAME_SIMULATE		"simdriver"
#define DRIVER_NAME_MEMORY			"memvardriver"
#define DEVICE_NAME_SIMULATE		"simdevice"
#define DEVICE_NAME_MEMORY			"memdevice"

class SERVERTAG_DATA
{
public:
	HighResTime		tmTime;
	double			dbValue;						// ʵ�����ͣ�float��real
	ACE_INT64		nValue;							// ������������bool��
	string			strValue;						// �ַ�������
	int				nQuality;

public:
	SERVERTAG_DATA();
	void AssignTagValue(int nDataTypeClass, SERVERTAG_DATA *pRightData);
	void AssignStrValueAndQuality(int nDataTypeClass, string &strTagValue, int nNewQuality);
	void AssignStrValueAndQuality(int nDataTypeClass, const char *szTagValue, int nNewQuality); // �����ַ������͵�ֵ
	void GetStrValue(int nDataTypeClass, string &strTagValue); // �����ַ������͵�ֵ
};

class CAlarmTag;
class CAlarmAreaTag;
class CMyDriver;
class CDataTag;
class CDeviceTag;
class CMyDevice;
class CProcessQueue;
// �������������������ԣ��ٸı䲿�������޸�֮��Ҳ�����в����޸�
class CMyDevice
{
public:
	unsigned int		m_nId;
	string				m_strName;
	string				m_strDesc;
	bool				m_bDisable;
	int					m_nDirverId;
	string				m_strSimulate; // Ϊ�ձ�ʾ��ģ�⣬������ʾģ��

	// ��������ڱ������в�ʹ��
	string				m_strParam1;
	string				m_strParam2;
	string				m_strParam3;
	string				m_strParam4;
	string				m_strProjectId;
	vector<CDeviceTag *> m_vecDeviceTag;
	CMyDriver		*	m_pDriver;

public:
	CMyDevice();
	virtual ~CMyDevice();
};

class CMyPysicalDevice :public CMyDevice
{
public:
	string				m_strConnType;
	string				m_strConnParam;
	int					m_nConnTimeout;
	string				m_strTaskNo;	// ��ͬ������ſ�����ͬһ���߳��С�Ϊ�ձ�ʾ��ͬ���߳�
public:
	CMyPysicalDevice();
	virtual ~CMyPysicalDevice();
};

class CMySimulateDevice :public CMyDevice
{
public:

public:
	CMySimulateDevice();
	virtual ~CMySimulateDevice();
};

class CMyMemoryDevice :public CMyDevice
{
public:

public:
	CMyMemoryDevice();
	virtual ~CMyMemoryDevice();
};

// ��������
class CMyDriver
{
public:
	unsigned int		m_nId;
	int					m_nType;	// �豸������ģ������������ڴ��������
	string				m_strName;
	string				m_strDesc;
	map<string, CMyDevice *> m_mapDevice;

public:
	CMyDriver();
	virtual ~CMyDriver();
};

class CMyDriverPhysicalDevice :public CMyDriver
{
public:	
	string				m_strModuleName;
	CProcessQueue *		m_pDrvShm;		// ������Ӧ�Ĺ����ڴ�

public:
	CMyDriverPhysicalDevice();
	virtual ~CMyDriverPhysicalDevice();
};

class CMyDriverMemory :public CMyDriver
{
public:
	CMyDriverMemory();
	virtual ~CMyDriverMemory();
};

class CMyDriverSimulate :public CMyDriver
{
public:
	CMyDriverSimulate();
	virtual ~CMyDriverSimulate();
};


// �洢label��Ϣ
class CLabel
{
public:
	int nControlVal;
	string strLabelName;
	string strDesc;
	CLabel();
};

class CAlarmInfo
{
public:
	int m_nAlarmCount;			// δȷ�ϻ���δ�ָ��ı�������
	int m_nAlarmingCount;		// δ�ָ��ı�������
	int m_nAlarmUnConfirmedCount;	// δȷ�ϵı�������
	int m_nAlarmConfirmedCount;		// �Ѿ�ȷ�ϵı�������
	int m_nAlarmingUnAckCount;		// �Ѿ�ȷ�ϵı�������

	int		m_nMaxAlarmLevel;		// ��������������б�������߼���
	int		m_nMinAlarmLevel;		// ��������������б�������߼���

	CAlarmInfo()
	{
		m_nAlarmingCount = m_nAlarmUnConfirmedCount = m_nAlarmConfirmedCount = m_nAlarmCount = m_nAlarmingUnAckCount = 0;
		m_nMaxAlarmLevel = 0;
		m_nMinAlarmLevel = 1000000;
	}
	CAlarmInfo & operator=(CAlarmInfo &right)
	{
		this->m_nAlarmingUnAckCount = right.m_nAlarmingUnAckCount;
		this->m_nAlarmCount = right.m_nAlarmCount;
		this->m_nAlarmingCount = right.m_nAlarmingCount;
		this->m_nAlarmUnConfirmedCount = right.m_nAlarmUnConfirmedCount;
		this->m_nAlarmConfirmedCount = right.m_nAlarmConfirmedCount;
		this->m_nMaxAlarmLevel = right.m_nMaxAlarmLevel;
		this->m_nMinAlarmLevel = right.m_nMinAlarmLevel;
		return *this;
	}
	CAlarmInfo & operator+=(CAlarmInfo &right)
	{
		int nAlarmingUnAckCount = right.m_nAlarmingUnAckCount > 0 ? 1 : 0;  // ֻͳ������!
		int nAlarmCount = right.m_nAlarmCount > 0 ? 1 : 0;  // ֻͳ������!
		int nAlarmingCount = right.m_nAlarmingCount > 0 ? 1 : 0; // ֻͳ������!
		int nAlarmAckedCount = right.m_nAlarmConfirmedCount > 0 ? 1 : 0; // ֻͳ������!
		int nAlarmUnAckCount = right.m_nAlarmUnConfirmedCount > 0 ? 1 : 0; // ֻͳ������!

		this->m_nAlarmingUnAckCount += nAlarmingUnAckCount; // ֻͳ������!
		this->m_nAlarmCount += nAlarmCount; // ֻͳ������!
		this->m_nAlarmingCount += nAlarmingCount; // ֻͳ������!
		this->m_nAlarmConfirmedCount += nAlarmAckedCount; // ֻͳ������!
		this->m_nAlarmUnConfirmedCount += nAlarmUnAckCount; // ֻͳ������!

		this->m_nMaxAlarmLevel = max(this->m_nMaxAlarmLevel, right.m_nMaxAlarmLevel);
		if (right.m_nMinAlarmLevel > 0 && this->m_nMinAlarmLevel > 0)
			this->m_nMinAlarmLevel = min(this->m_nMinAlarmLevel, right.m_nMinAlarmLevel);
		else if (right.m_nMinAlarmLevel > 0)
			this->m_nMinAlarmLevel = right.m_nMinAlarmLevel;
		return *this;
	}

	void ResetValue()
	{
		m_nAlarmingUnAckCount = m_nAlarmingCount = m_nAlarmUnConfirmedCount = m_nAlarmConfirmedCount = m_nAlarmCount = 0;
		m_nMaxAlarmLevel = 0;
		m_nMinAlarmLevel = 0;
	}

	void GenerateAlarmInfo2Json(Json::Value &jsonAlarmInfo);
};
class CAlarmType
{
public:
	string m_strName;
	string m_strId;
	string m_strDescription;
	bool	m_bAutoConfirm; // �Ƿ�Ҫ�Զ�ȷ�ϱ������ڻָ�����ʱ��

public:
	CAlarmInfo alarmInfo; // ͳ����Ϣ
	CAlarmType()
	{
		m_strName = m_strId = m_strDescription = "";
		m_bAutoConfirm = false;
	}
	CAlarmType &operator=(CAlarmType &right)
	{
		this->m_strName = right.m_strName;
		this->m_strId = right.m_strId;
		this->m_strDescription = right.m_strDescription;
		this->alarmInfo = right.alarmInfo;
		this->m_bAutoConfirm = right.m_bAutoConfirm;
		return *this;
	}
};

class CMyClass;
class CMyClassProp;
class CMyClassPropAlarm;
class CMyObject;
class CMyObjectProp;
class CMyObjectPropAlarm;

class CMyClass
{
public:
	int		m_nId;
	string	m_strName;
	string	m_strDesc;
	int		m_nParentId;
	string	strParentName;
	int		m_nProjectId;
	map<string, CMyClassProp *> m_mapClassProp;

public:
	CMyClass();
};

// ÿ�������൱��һ��TAG�㣬������������Щ�ֶ�
class CMyClassProp
{
public:
	unsigned int		m_nId;
	string				m_strName;//�����Ե�����
	string				m_strDesc;

	string				m_strDataType;
	int					m_nDataType;
	int					m_nDataTypeClass; // ��ֵ/�ַ���
	int					m_nDataLen; // �ַ������ͻ�blob��Ҫ����
	string				m_strAddress; //��ĵ�ַ����eg.400001#10 or 400001:1.������豸���ݵ�ַҲ���������snmp��int32��uint32��counter32��gauge32��timeticks ��octet��oid
	string 				m_strSubsysName;   // ��ϵͳid
	int					m_nLabelId;       // ��ǩID
	string				m_strLabelName;       // ��ǩID	
	string				m_strParam;			// param�ֶΣ�Ԥ���ֶ�
	int					m_nByteOrderNo;	// �ֽ�����44332211
	bool				m_bEnableControl;
	string				m_strHisDataInterval;     // ��ʷ���ݱ�������
	string				m_strTrendDataInterval;     // �������ݱ�����

	int					m_nPollRateMS;		// ˢ���ٶ�
	int					m_nPropTypeId;		// �������ͣ����ݵ㡢����㡢�ۼƴ����㡢�ۼ�ʱ���
	string				m_strInitValue;

	bool				m_bEnableRange;   // ����Χ����
	double				m_dbRangeFactor;	// ��Χ�����������ĳ���ϵ����y=b*x+c��c
	double				m_dbRangePlusValue; // ��Χ����������������ֵ��y=b*x+c��c

	map<string, CMyClassPropAlarm *> m_mapAlarms;
	CMyClass*			m_pClass;	// ��������

public:
	CMyClassProp();
	int CalcRangeParam(string strEnableRange, string strRawMin, string strRawMax, string strOutMin, string strOutMax);
};

class CMyClassPropAlarm
{
public:
	int					m_nId;
	bool				m_bEnable;
	int					m_nLevel;
	string				m_strDescription;    // ������������ʾ�ڿͻ��˻��汨���б��е����ƣ�����Ǳ����㣬���ֶ�Ϊ�գ�ȡ�õ��Ӧ��tag������ƣ����Ǳ������򣬸��ֶ���д�����������������
	string				m_strJudgeMethod;
	string				m_strAlarmTypeName; // �����������ƣ��糬���޾���
	int					m_nDeadtimeProduce; // ��������������ʱ��
	int					m_nDeadtimeRecover; // �ָ�����������ʱ��
	CMyClassProp *		m_pClassProp; // 
	// �ַ������͵���ֵ����
	string				strThresholds;	// �ַ������͵ģ���EQ��HAS��IN ������ֵ���ַ�����ʾ

	CMyClassPropAlarm();
};

// �������Եı�������
class CMyObjectPropAlarm
{
public:
	int					m_nId; // ��Щ�Ǵ�������̳����ģ�����û��ID������˵IDΪ0
	bool				m_bEnable;
	int					m_nLevel;
	string				m_strDescription;    // ������������ʾ�ڿͻ��˻��汨���б��е����ƣ�����Ǳ����㣬���ֶ�Ϊ�գ�ȡ�õ��Ӧ��tag������ƣ����Ǳ������򣬸��ֶ���д�����������������
	string				m_strPropName; // �����������ƣ��糬���޾���
	string				m_strJudgeMethod;
	string				m_strAlarmTypeName;
	CMyObjectProp *		m_pObjectProp; // 
	// �ַ������͵���ֵ����
	string				strThresholds;	// �ַ������͵ģ���EQ��HAS��IN ������ֵ���ַ�����ʾ

	CMyObjectPropAlarm();
};

// �������������������ԣ��ٸı䲿�������޸�֮��Ҳ�����в����޸�
class CMyObjectProp
{
public:
	unsigned int		m_nId;
	string				m_strDesc;

	//string				m_strDataType;
	//int					m_nDataLen; // �ַ������ͻ�blob��Ҫ����
	string				m_strAddress; //��ĵ�ַ����eg.400001#10 or 400001:1.������豸���ݵ�ַҲ���������snmp��int32��uint32��counter32��gauge32��timeticks ��octet��oid
	string 				m_strSubsysName;   // ��ϵͳid
	string				m_strParam;			// param�ֶΣ�Ԥ���ֶ�

	int					m_nByteOrderNo;	// �ֽ�����44332211
	bool				m_bEnableControl;
	string				m_strHisDataInterval;
	string				m_strTrendDataInterval;
	int					m_nPropTypeId;		// �������ͣ����ݵ㡢����㡢�ۼƴ����㡢�ۼ�ʱ���
	string				m_strInitValue;
	int					m_nPollRateMS;		// ˢ���ٶ�

	bool				m_bEnableRange;   // ����Χ����
	double				m_dbRangeFactor;	// ��Χ�����������ĳ���ϵ����y=b*x+c��c
	double				m_dbRangePlusValue; // ��Χ����������������ֵ��y=b*x+c��c

	map<string, CMyObjectPropAlarm *> m_mapAlarms;
	CMyClassProp *		n_pClassProp;		// ��Ӧ��������
	CMyObject *			m_pObject;			// ��������
public:
	CMyObjectProp();

	int CalcRangeParam(string strEnableRange, string strRawMin, string strRawMax, string strOutMin, string strOutMax);
};

//  ����ṹ��
class CDataTag;
class CMyObject
{
public:
	int		nId;				// ObjectId
	//������������Ϣ
	string  strName;			// ��������
	string	strSubsysName;		// ��ϵͳ����
	int		nSubsysId;			// ��ϵͳ����
	string	strDriverName;		// ��������
	string	strDeviceName;		// �豸����
	int		nDeviceId;			// �豸����
	string	strDescription;		// ����

	string	strParam1;
	string	strParam2;
	string	strParam3;
	string	strParam4;
	vector<CDataTag *>			vecTags; // ����ӵ�е����Ը���
	map<string, CMyObjectProp *>	mapProps;
	CMyClass * pClass;
	CMyDevice * m_pDevice;

	// ������ʵʱ��̬��Ϣ
	map<string, CAlarmInfo *>	m_mapAlarmTypeName2Info;	// ��������ӳ�䵽������Ϣ������
	CAlarmInfo	m_alarmInfo;	// ��ǰ��ʵʱ������Ϣ

	int     nQuality;			// ��������

public:
	CMyObject();
	void UpdateToDataMemDB(Json::FastWriter &jsonWriter, char *szCurTime = NULL); // ���¶���memdb0
	void UpdateToConfigMemDB(Json::FastWriter &jsonWriter);
};

// ����ͨ�õ�TAG�����Ϣ����������ַ��Ϣ
class CDataTag
{
public:
	string				strName;//�������
	string				strDesc;//
	string				strNodeName; // �ڵ��������Ϊ�ձ�ʾ�ڱ��ص�local,��Ҫ��֤��Ψһ
	int					nTagType;	// ��������:ģ��㣬��ͨ�㣬����㣬�ۼƵ�ʱ��㣬�ۼƴ�����

	int					nPollrateMS; // ˢ��Ƶ��

	string				strDataType;
	int					nTagDataLen;  // TAG�ĳ��ȣ����û�г�������ȱʡ4���ֽڿ��ǣ�����
	int					nDataType;	//�����������
	int					nDataTypeClass; // �������ͷ��ࣺ������С�����ַ��� ����
	string				strInitValue;	// ���ݿ�ĳ�ʼֵ

	int					nSubsysID;   // ��ϵͳid
	int					nLabelID;       // ��ǩID
	string				strSubsysName;
	int					nByteOrderNo;	// �ֽ�����44332211
	bool				bEnableAlarm;   // �Ƿ�������
	bool				bEnableControl;
	bool				bEnableRange;   // ����Χ����
	double				m_dbRangeFactor;	// ��Χ�����������ĳ���ϵ����y=b*x+c��c
	double				m_dbRangePlusValue; // ��Χ����������������ֵ��y=b*x+c��c

	string				strHisDataInterval;     // ��ʷ���ݱ�����
	string				strTrendDataInterval;   // �������ݱ�����
	string				strParam;			// param�ֶΣ�Ԥ���ֶ�
	SERVERTAG_DATA		curVal;		// ��ǰtag���ֵ
	SERVERTAG_DATA		lastVal;	// ��һ��tag���ֵ

	CMyObject			*pObject;	// ��������
	CMyObjectProp		*pObjProp;
	CMyDevice			*pDevice;	// �����豸����Ҫ�����豸ģʽ

	vector<CAlarmTag *>		vecAllAlarmTags;	// ���б���
	vector<CDataTag *>		vecCalcTags;		// ����������Ķ��������б������ж���Щ����������Ҫ�������¼���

	// ��ǰ������Ϣ
	map<string, CAlarmInfo *> mapAlarmTypeName2AlarmInfo; //  �����������Ƶ�������ǰ��Ϣ�ṹ���map
	CAlarmInfo				m_curAlarmInfo;		// ��ǰ������Ϣ

public:
	CDataTag();
	static int GetDataTypeId(const char *szDataTypeName);
	static int GetDataTypeClass(int nDataTypeId);
	void GetJsonStringToDataMemDB(Json::FastWriter &jsonWriter, char *szCurTime, string &strJsonValue);
	int CalcRangeParam(string strEnableRange, string strRawMin, string strRawMax, string strOutMin, string strOutMax);
	int CalcRangeValue(int nValue);
	ACE_INT64 CalcRangeValue(ACE_INT64 fValue);
	ACE_UINT64 CalcRangeValue(ACE_UINT64 fValue);
	float CalcRangeValue(float fValue);
	double CalcRangeValue(double dbValue);
};

// ���ڿ����й���Ҫ��tag����Ϣ
class CDeviceTag :public CDataTag
{
public:
	string				strAddress; //��ĵ�ַ����eg.400001#10 or 400001:1.������豸���ݵ�ַҲ���������snmp��int32��uint32��counter32��gauge32��timeticks ��octet��oid
	// int					nDeviceID;

	CDeviceTag();
};

// ����ģ��tag����Ϣ. �������豸����Ϊģ��
class CSimTag :public CDeviceTag
{
public:
	bool m_bAutoInc; // �Ƿ�����. ������͹�����������ֹͣ�������տ�ʼʱ���Simulate���ǿ����������
	string m_strDeviceSimValue;
public:
	double			m_dbSimValue;						// ʵ�����ͣ�float��real
	double			m_dbSimIncStep;						// ÿ�����ڵ���������ֵ�ô�С��������Simulate�ֶ�
	string			m_strSimValue;						// �ַ�������
public:
	CSimTag();
};

// �ڴ�tag����Ϣ
class CMemTag :public CDataTag
{
public:
	string			m_strMemValue;						// �ַ�������
public:
	CMemTag();
};

// Ԥ����tag��
class CPredefTag :public CDataTag
{
public:
	string			m_strPredefValue;						// ģ���豸��ֵ
public:
	CPredefTag();
};


class CCalcTag :public CDataTag
{
public:
	CTagExprCalc exprCalc;
	string		strAddress; // �洢�˱��ʽ
	ACE_Time_Value tvLastLogCalcError; // �ϴμ�������ʱ�䣬��¼�����������ظ���ӡ��ÿ30���ӡһ�μ���
	int			nCalcErrorNumLog;		// ���ϴδ�����־��ӡ�������ɼ������Ĵ���
	CCalcTag();
	virtual ~CCalcTag(){};
};

class CTagAccumulate :public CCalcTag // �ۼƵ㡣�ۼƵ����һ��������ʽ����ֵ�õ����ۼ�ʱ����ۼƴ��� ���۲�
{
public:
	ACE_Time_Value	tvLastValue;			// �ϴ��ۼƵ��ʱ�� �ϴεĵ�ֵ
	bool			bLastSatified;	// �ϴ��Ƿ���������
	ACE_UINT64		lValue;			// �ϴ��ۼƵ��ֵ��ʱ����������

	CTagAccumulate();
	virtual ~CTagAccumulate(){};
};

class CTagAccumulateTime :public CTagAccumulate // �ۼ�ʱ���. �ۼ�ʱ������һ��������ʽ����ֵ�õ����ۼ�ʱ����ۼƴ��� ���۲�
{
public:
	CTagAccumulateTime(){
		nTagType = VARIABLE_TYPE_ACCUMULATE_TIME;
	}
	virtual ~CTagAccumulateTime(){};
};

class CTagAccumulateCount :public CTagAccumulate // �ۼƿ��ش�����. �ۼƿ��ص����һ��������ʽ����ֵ�õ����ۼ�ʱ����ۼƴ��� ���۲�
{
public:
	CTagAccumulateCount(){
		nTagType = VARIABLE_TYPE_ACCUMULATE_COUNT;
	}
	virtual ~CTagAccumulateCount(){};
};

class CAlarmingInfo
{
public:
	unsigned char	bAlarming;		// �Ƿ����ڱ���.����boolean������תΪjsonʱ���true������1
	unsigned char	bConfirmed;		// �����Ƿ��Ѿ�ȷ��.����boolean������תΪjsonʱ���true������1
	string			strValueOnProduce;	// ��������ʱ��ֵ
	string			strValueOnRecover;	// �����ָ�ʱ��ֵ
	string			strValueOnConfirm;	// ����ȷ��ʱ��ֵ
	string			strThresholdOnProduce; // ��������ʱ������ֵ

	HighResTime		tmProduceFirst;	// ��һ�α����Ĳ���ʱ��
	HighResTime		tmProduce;	// ���µı�������ʱ��
	HighResTime		tmConfirm;	// ȷ��ʱ��
	HighResTime		tmRecover;	// �����ָ�ʱ��
	int				nRepetitions;	// ����������������δȷ��ǰ���������Ĵ�����

	string			strConfirmUserName;
	
	// ���Ƿ����Ʊ������������Ȳ������������ò������������п��Զ�̬д��Ĳ���
	string				strParam1;		// ����Ԥ������1
	string				strParam2;		// ����Ԥ������2
	string				strParam3;		// ����Ԥ������3
	string				strParam4;		// ����Ԥ������4

	CAlarmTag		*pAlarmTag;	// ������
public:
	CAlarmingInfo();
};

// ���ڿ����й���Ҫ��tag����Ϣ
class CAlarmTag //:public CTag
{
public:
	bool				bEnable;
	int					nLevel;
	string				strAlarmTypeName; // �����������ƣ��糬���޾���

	string				strJudgeMethod;		// EQ5
	int					nJudgeMethodType;	// EQ
	string				strJudgeMethodIndex;	// 5 . ���Ϊ0���ʾEQ

	// �ַ������͵���ֵ����
	string				strThreshold;	// �ַ������͵ģ���EQ��HAS��IN ������ֵ���ַ�����ʾ
	string				strThreshold2;	// �ַ������͵ģ���EQ��HAS��IN ������ֵ���ַ�����ʾ
	// �������͵���ֵ����
	ACE_INT64			nThreshold;		// ֻ��Ҫ�ж�1����ֵʱ��ֵ���� ���� �� ����
	ACE_INT64			nThreshold2;	// ֻ��Ҫ�ж�1����ֵʱ��ֵ���� ���� �� С��
	// С�����͵���ֵ����
	double				dbThreshold;    // ֻ��Ҫ�ж�1����ֵʱ��ֵ���� ���� �� ����
	double				dbThreshold2;   // ֻ��Ҫ�ж�1����ֵʱ��ֵ���� ���� �� С��

	// in �жϷ��������� �� �ַ���
	std::vector<ACE_INT64>		vecThresholdInt;   // �ı�������in����
	std::vector<std::string>	vecThresholdText;   // �ַ���������in����
	std::vector<double>			vecThresholdReal;	// С��������in����
	string				strDesc;

	CDataTag			*pTag;			// ָ���Ӧ��tag��

	vector<CAlarmingInfo *> vecAlarming;
	CAlarmInfo			m_alarmInfo;	// �ñ������Ӧ�ı�����Ϣ
public:
	CAlarmTag();
	void GetThresholdAsString(string &strThresholdDesc);
	void GetThresholdAsString_Int(string &strThresholdDesc);
	void GetThresholdAsString_Real(string &strThresholdDesc);
	void GetThresholdAsString_String(string &strThresholdDesc);

public:
	static int GetJudgeMethodTypeAndIndex(const char *szJudgeMethod, int &nJudgeMethodType, string &strJudgeMethodIndex);
	// ������ֵ���ݲ�ͬ���Ͳ�ִ�ŵ���ͬ�ĳ�Ա������
	int SplitThresholds_Real(const char *szThresholdText);
	// ������ֵ���ݲ�ͬ���Ͳ�ִ�ŵ���ͬ�ĳ�Ա������
	int SplitThresholds_Integer(const char *szThresholdText);
	int SplitThresholds_Text(const char *szThresholdText);
	int SplitThresholds(const char *szThresholdText);
};


#endif // _NODESERVER_COMMON_DEFINE_H_
