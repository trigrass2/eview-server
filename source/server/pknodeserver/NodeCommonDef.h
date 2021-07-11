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

#define EPSINON		0.000001	// 一个很小的值，用于浮点数比较

//报警判断类型
#define ALARM_JUDGE_HHH_TEXT								"HHH"		// 超高高高限报警，大于等于该限值则报警, >=HHH
#define ALARM_JUDGE_HH_TEXT									"HH"		// 超高高限报，大于等于该限值且小于HHH则报警, HHH>value>=HH
#define ALARM_JUDGE_H_TEXT									"H"			// 超高限报警，大于等于该限值且小于HH则报警 ,HH>value>=H
#define ALARM_JUDGE_L_TEXT									"L"			// 超低限报警，小于等于该限值且大于LL则报警,LL<value<=L
#define ALARM_JUDGE_LL_TEXT									"LL"		// 超低低限报警，小于等于该限值且大于LLL则报警,LLL<value<=LL
#define ALARM_JUDGE_LLL_TEXT								"LLL"		// 超低低低限报警，小于等于该限值则报警,value<=LLL
#define ALARM_JUDGE_FX_TEXT									"FX"		// 相当于EQ，不建议使用, 逐步废弃, FX, FX2, FX3
#define ALARM_JUDGE_OFF_TEXT								"OFF"		// 有1变化为0则报警，布尔类型变量支持该判断方法，不建议使用
#define ALARM_JUDGE_ON_TEXT									"ON"		// 由0变化为1则报警，布尔类型变量支持该判断方法，不建议使用
#define ALARM_JUDGE_EQ_TEXT									"EQ"		// 等于该阀值报警，支持EQn，包括：EQ，EQ2，EQ3，EQ4......
#define ALARM_JUDGE_NE_TEXT									"NE"		// 于不等于该阀值报警，支持NEn，包括：NE，NE2，NE3，NE4......
#define ALARM_JUDGE_GT_TEXT									"GT"		// 大于theshold则报警， value>threshold
#define ALARM_JUDGE_GTE_TEXT								"GTE"		// 大于等于theshold则报警， value>=threshold
#define ALARM_JUDGE_LT_TEXT									"LT"		// 小于theshold则报警， value<threshold
#define ALARM_JUDGE_LTE_TEXT								"LTE"		// 小于等于theshold则报警， value<=threshold
#define ALARM_JUDGE_GTELT_TEXT								"GTELT"		// 大于等于theshold且小于threshold2， threshold<=value<threshold2，需配置以逗号隔开的两个阀值   
#define ALARM_JUDGE_GTLT_TEXT								"GTLT"		// 大于theshold且小于threshold2， threshold<value<threshold2，需配置以逗号隔开的两个阀值
#define ALARM_JUDGE_GTLTE_TEXT								"GTLTE"		// 大于theshold且小于等于threshold2， threshold<value<=threshold2，需配置以逗号隔开的两个阀值
#define ALARM_JUDGE_GTELTE_TEXT								"GTELTE"	// 大于等于theshold且小于等于threshold2， threshold<=value<=threshold2，需配置以逗号隔开的两个阀值
#define ALARM_JUDGE_CONTAIN_TEXT							"CONTAIN"	// value包含threshld阀值则报警
#define ALARM_JUDGE_IN_TEXT									"IN"		// value等于一组值之中的一个则报警，这组值以逗号隔开，支持字符串和数值类型
#define ALARM_JUDGE_CHANGETOEQ_TEXT							"TOEQ"		// 变化到等于该阀值时触发报警，支持EQn，包括：EQ，EQ2，EQ3，EQ4......
#define ALARM_JUDGE_QUALITYBAD_TEXT							"BAD"		// 质量为BAD产生报警（即q ！=0）
#define ALARM_JUDGE_CHANGETOQUALITYBAD_TEXT					"TOBAD"		// 质量由GOOD变化为BAD产生报警（即q==0 变为q ！=0）

#define ALARM_JUDGE_INVALID									0		// 非法
#define ALARM_JUDGE_HHH										1
#define ALARM_JUDGE_HH										2
#define ALARM_JUDGE_H										3
#define ALARM_JUDGE_L										4
#define ALARM_JUDGE_LL										5
#define ALARM_JUDGE_LLL										6
#define ALARM_JUDGE_FX										32
#define ALARM_JUDGE_OFF										33
#define ALARM_JUDGE_ON										34
#define ALARM_JUDGE_EQ										11		// 等于
#define ALARM_JUDGE_NE										12		// 不等于
#define ALARM_JUDGE_GT										13		// 大于
#define ALARM_JUDGE_GTE										14		// 大于等于
#define ALARM_JUDGE_LT										15		// 小于
#define ALARM_JUDGE_LTE										16		// 小于等于
#define ALARM_JUDGE_GTELT									17		// 大于等于 且 小于   
#define ALARM_JUDGE_GTLT									18		// 大于 且 小于  
#define ALARM_JUDGE_GTLTE									19		// 大于 且 小于等于
#define ALARM_JUDGE_GTELTE									20		// 大于等于 且 小于等于
#define ALARM_JUDGE_CONTAIN									21		// 包含
#define ALARM_JUDGE_IN										22		// 是其中一个，限值以逗号隔开
#define ALARM_JUDGE_CHANGETOEQ								23		// 由1个有效值变化到等于该阀值时触发报警，支持EQn，包括：EQ，EQ2，EQ3，EQ4......
#define ALARM_JUDGE_QUALITYBAD								24		// 质量为BAD产生报警（即q ！=0）
#define ALARM_JUDGE_CHANGETOQUALITYBAD						25		// 质量由GOOD变化为BAD产生报警（即q==0 变为q ！=0）

#define	DATATYPE_CLASS_OTHER		0	// 如：blob或其他未知数据类型
#define	DATATYPE_CLASS_INTEGER		1	// 整数大类型，包括：bool、int8、int16、int32、int64、uint8、uint16、uint32、uint64
#define	DATATYPE_CLASS_REAL			2	// 小数大类型，包括：float、double
#define	DATATYPE_CLASS_TEXT			3	// 文本类型，包括：字符窜

// 变量类型
#define TAGTYPE_DEVICE				0
#define	TAGTYPE_CALC				1
#define	TAGTYPE_MEMVAR				2
#define	TAGTYPE_SIMULATE					3
#define	TAGTYPE_ACCUMULATE_TIME		4
#define	TAGTYPE_ACCUMULATE_COUNT	5

#define DRIVER_TYPE_PHYSICALDEVICE	0		// 物理设备驱动

#define DRIVER_TYPE_SIMULATE		-5		// 模拟驱动
#define DRIVER_TYPE_MEMORY			-1		// 内存变量驱动
#define DRIVER_NAME_SIMULATE		"simdriver"
#define DRIVER_NAME_MEMORY			"memvardriver"
#define DEVICE_NAME_SIMULATE		"simdevice"
#define DEVICE_NAME_MEMORY			"memdevice"

class SERVERTAG_DATA
{
public:
	HighResTime		tmTime;
	double			dbValue;						// 实数类型，float和real
	ACE_INT64		nValue;							// 各类整数包含bool型
	string			strValue;						// 字符串类型
	int				nQuality;

public:
	SERVERTAG_DATA();
	void AssignTagValue(int nDataTypeClass, SERVERTAG_DATA *pRightData);
	void AssignStrValueAndQuality(int nDataTypeClass, string &strTagValue, int nNewQuality);
	void AssignStrValueAndQuality(int nDataTypeClass, const char *szTagValue, int nNewQuality); // 返回字符串类型的值
	void GetStrValue(int nDataTypeClass, string &strTagValue); // 返回字符串类型的值
};

class CAlarmTag;
class CAlarmAreaTag;
class CMyDriver;
class CDataTag;
class CDeviceTag;
class CMyDevice;
class CProcessQueue;
// 对象属性是所有类属性，再改变部分属性修改之，也可以有部分修改
class CMyDevice
{
public:
	unsigned int		m_nId;
	string				m_strName;
	string				m_strDesc;
	bool				m_bDisable;
	int					m_nDirverId;
	string				m_strSimulate; // 为空表示不模拟，其他表示模拟

	// 下面参数在本程序中不使用
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
	string				m_strTaskNo;	// 相同的任务号开启在同一个线程中。为空表示不同的线程
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

// 驱动属性
class CMyDriver
{
public:
	unsigned int		m_nId;
	int					m_nType;	// 设备驱动，模拟变量驱动，内存变量驱动
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
	CProcessQueue *		m_pDrvShm;		// 驱动对应的共享内存

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


// 存储label信息
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
	int m_nAlarmCount;			// 未确认或者未恢复的报警个数
	int m_nAlarmingCount;		// 未恢复的报警个数
	int m_nAlarmUnConfirmedCount;	// 未确认的报警个数
	int m_nAlarmConfirmedCount;		// 已经确认的报警个数
	int m_nAlarmingUnAckCount;		// 已经确认的报警个数

	int		m_nMaxAlarmLevel;		// 对象的所有属性中报警的最高级别
	int		m_nMinAlarmLevel;		// 对象的所有属性中报警的最高级别

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
		int nAlarmingUnAckCount = right.m_nAlarmingUnAckCount > 0 ? 1 : 0;  // 只统计有无!
		int nAlarmCount = right.m_nAlarmCount > 0 ? 1 : 0;  // 只统计有无!
		int nAlarmingCount = right.m_nAlarmingCount > 0 ? 1 : 0; // 只统计有无!
		int nAlarmAckedCount = right.m_nAlarmConfirmedCount > 0 ? 1 : 0; // 只统计有无!
		int nAlarmUnAckCount = right.m_nAlarmUnConfirmedCount > 0 ? 1 : 0; // 只统计有无!

		this->m_nAlarmingUnAckCount += nAlarmingUnAckCount; // 只统计有无!
		this->m_nAlarmCount += nAlarmCount; // 只统计有无!
		this->m_nAlarmingCount += nAlarmingCount; // 只统计有无!
		this->m_nAlarmConfirmedCount += nAlarmAckedCount; // 只统计有无!
		this->m_nAlarmUnConfirmedCount += nAlarmUnAckCount; // 只统计有无!

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
	bool	m_bAutoConfirm; // 是否要自动确认报警（在恢复操作时）

public:
	CAlarmInfo alarmInfo; // 统计信息
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

// 每个属性相当于一个TAG点，都具有以下这些字段
class CMyClassProp
{
public:
	unsigned int		m_nId;
	string				m_strName;//类属性的名称
	string				m_strDesc;

	string				m_strDataType;
	int					m_nDataType;
	int					m_nDataTypeClass; // 数值/字符串
	int					m_nDataLen; // 字符串类型或blob需要长度
	string				m_strAddress; //点的地址串，eg.400001#10 or 400001:1.如果有设备数据地址也放在这里，如snmp的int32、uint32、counter32、gauge32、timeticks 、octet、oid
	string 				m_strSubsysName;   // 子系统id
	int					m_nLabelId;       // 标签ID
	string				m_strLabelName;       // 标签ID	
	string				m_strParam;			// param字段，预留字段
	int					m_nByteOrderNo;	// 字节序，如44332211
	bool				m_bEnableControl;
	string				m_strHisDataInterval;     // 历史数据保存周期
	string				m_strTrendDataInterval;     // 趋势数据保存间隔

	int					m_nPollRateMS;		// 刷新速度
	int					m_nPropTypeId;		// 属性类型：数据点、计算点、累计次数点、累计时间点
	string				m_strInitValue;

	bool				m_bEnableRange;   // 允许范围换算
	double				m_dbRangeFactor;	// 范围换算计算出来的乘数系数，y=b*x+c的c
	double				m_dbRangePlusValue; // 范围换算计算出来的增加值，y=b*x+c的c

	map<string, CMyClassPropAlarm *> m_mapAlarms;
	CMyClass*			m_pClass;	// 所属对象

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
	string				m_strDescription;    // 中文描述，显示在客户端画面报警列表中的名称，如果是报警点，该字段为空，取该点对应的tag点的名称，若是报警区域，该字段填写报警区域的中文描述
	string				m_strJudgeMethod;
	string				m_strAlarmTypeName; // 报警类型名称，如超上限警告
	int					m_nDeadtimeProduce; // 产生报警的死区时间
	int					m_nDeadtimeRecover; // 恢复报警的死区时间
	CMyClassProp *		m_pClassProp; // 
	// 字符串类型的限值定义
	string				strThresholds;	// 字符串类型的，如EQ、HAS、IN 报警限值的字符串表示

	CMyClassPropAlarm();
};

// 对象属性的报警配置
class CMyObjectPropAlarm
{
public:
	int					m_nId; // 有些是从类里面继承来的，所以没有ID，或者说ID为0
	bool				m_bEnable;
	int					m_nLevel;
	string				m_strDescription;    // 中文描述，显示在客户端画面报警列表中的名称，如果是报警点，该字段为空，取该点对应的tag点的名称，若是报警区域，该字段填写报警区域的中文描述
	string				m_strPropName; // 报警类型名称，如超上限警告
	string				m_strJudgeMethod;
	string				m_strAlarmTypeName;
	CMyObjectProp *		m_pObjectProp; // 
	// 字符串类型的限值定义
	string				strThresholds;	// 字符串类型的，如EQ、HAS、IN 报警限值的字符串表示

	CMyObjectPropAlarm();
};

// 对象属性是所有类属性，再改变部分属性修改之，也可以有部分修改
class CMyObjectProp
{
public:
	unsigned int		m_nId;
	string				m_strDesc;

	//string				m_strDataType;
	//int					m_nDataLen; // 字符串类型或blob需要长度
	string				m_strAddress; //点的地址串，eg.400001#10 or 400001:1.如果有设备数据地址也放在这里，如snmp的int32、uint32、counter32、gauge32、timeticks 、octet、oid
	string 				m_strSubsysName;   // 子系统id
	string				m_strParam;			// param字段，预留字段

	int					m_nByteOrderNo;	// 字节序，如44332211
	bool				m_bEnableControl;
	string				m_strHisDataInterval;
	string				m_strTrendDataInterval;
	int					m_nPropTypeId;		// 属性类型：数据点、计算点、累计次数点、累计时间点
	string				m_strInitValue;
	int					m_nPollRateMS;		// 刷新速度

	bool				m_bEnableRange;   // 允许范围换算
	double				m_dbRangeFactor;	// 范围换算计算出来的乘数系数，y=b*x+c的c
	double				m_dbRangePlusValue; // 范围换算计算出来的增加值，y=b*x+c的c

	map<string, CMyObjectPropAlarm *> m_mapAlarms;
	CMyClassProp *		n_pClassProp;		// 对应的类属性
	CMyObject *			m_pObject;			// 所属对象
public:
	CMyObjectProp();

	int CalcRangeParam(string strEnableRange, string strRawMin, string strRawMax, string strOutMin, string strOutMax);
};

//  对象结构体
class CDataTag;
class CMyObject
{
public:
	int		nId;				// ObjectId
	//下面是配置信息
	string  strName;			// 对象名称
	string	strSubsysName;		// 子系统名称
	int		nSubsysId;			// 子系统名称
	string	strDriverName;		// 驱动名称
	string	strDeviceName;		// 设备名称
	int		nDeviceId;			// 设备名称
	string	strDescription;		// 描述

	string	strParam1;
	string	strParam2;
	string	strParam3;
	string	strParam4;
	vector<CDataTag *>			vecTags; // 对象拥有的属性个数
	map<string, CMyObjectProp *>	mapProps;
	CMyClass * pClass;
	CMyDevice * m_pDevice;

	// 下面是实时动态信息
	map<string, CAlarmInfo *>	m_mapAlarmTypeName2Info;	// 报警类型映射到报警信息的数组
	CAlarmInfo	m_alarmInfo;	// 当前的实时报警信息

	int     nQuality;			// 对象质量

public:
	CMyObject();
	void UpdateToDataMemDB(Json::FastWriter &jsonWriter, char *szCurTime = NULL); // 更新对象到memdb0
	void UpdateToConfigMemDB(Json::FastWriter &jsonWriter);
};

// 用于通用的TAG点的信息，不包括地址信息
class CDataTag
{
public:
	string				strName;//点的名称
	string				strDesc;//
	string				strNodeName; // 节点名，如果为空表示在本地点local,需要保证不唯一
	int					nTagType;	// 变量类型:模拟点，普通点，计算点，累计点时间点，累计次数点

	int					nPollrateMS; // 刷新频率

	string				strDataType;
	int					nTagDataLen;  // TAG的长度，如果没有长度则以缺省4个字节考虑？？？
	int					nDataType;	//点的数据类型
	int					nDataTypeClass; // 数据类型分类：整数、小数、字符串 三类
	string				strInitValue;	// 数据库的初始值

	int					nSubsysID;   // 子系统id
	int					nLabelID;       // 标签ID
	string				strSubsysName;
	int					nByteOrderNo;	// 字节序，如44332211
	bool				bEnableAlarm;   // 是否允许报警
	bool				bEnableControl;
	bool				bEnableRange;   // 允许范围换算
	double				m_dbRangeFactor;	// 范围换算计算出来的乘数系数，y=b*x+c的c
	double				m_dbRangePlusValue; // 范围换算计算出来的增加值，y=b*x+c的c

	string				strHisDataInterval;     // 历史数据保存间隔
	string				strTrendDataInterval;   // 趋势数据保存间隔
	string				strParam;			// param字段，预留字段
	SERVERTAG_DATA		curVal;		// 当前tag点的值
	SERVERTAG_DATA		lastVal;	// 上一次tag点的值

	CMyObject			*pObject;	// 所属对象
	CMyObjectProp		*pObjProp;
	CMyDevice			*pDevice;	// 所属设备。主要用于设备模式

	vector<CAlarmTag *>		vecAllAlarmTags;	// 所有报警
	vector<CDataTag *>		vecCalcTags;		// 本变量参与的二级变量列表，用于判断哪些二级变量需要进行重新计算

	// 当前报警信息
	map<string, CAlarmInfo *> mapAlarmTypeName2AlarmInfo; //  报警类型名称到报警当前信息结构体的map
	CAlarmInfo				m_curAlarmInfo;		// 当前报警信息

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

// 用于控制有关需要的tag点信息
class CDeviceTag :public CDataTag
{
public:
	string				strAddress; //点的地址串，eg.400001#10 or 400001:1.如果有设备数据地址也放在这里，如snmp的int32、uint32、counter32、gauge32、timeticks 、octet、oid
	// int					nDeviceID;

	CDeviceTag();
};

// 用于模拟tag点信息. 由已有设备配置为模拟
class CSimTag :public CDeviceTag
{
public:
	bool m_bAutoInc; // 是否自增. 如果发送过控制命令则停止自增。刚开始时如果Simulate不是空则进行自增
	string m_strDeviceSimValue;
public:
	double			m_dbSimValue;						// 实数类型，float和real
	double			m_dbSimIncStep;						// 每个周期到了自增的值得大小，配置在Simulate字段
	string			m_strSimValue;						// 字符串类型
public:
	CSimTag();
};

// 内存tag点信息
class CMemTag :public CDataTag
{
public:
	string			m_strMemValue;						// 字符串类型
public:
	CMemTag();
};

// 预定义tag点
class CPredefTag :public CDataTag
{
public:
	string			m_strPredefValue;						// 模拟设备的值
public:
	CPredefTag();
};


class CCalcTag :public CDataTag
{
public:
	CTagExprCalc exprCalc;
	string		strAddress; // 存储了表达式
	ACE_Time_Value tvLastLogCalcError; // 上次计算错误的时间，记录下来避免多次重复打印。每30秒打印一次即可
	int			nCalcErrorNumLog;		// 从上次错误日志打印以来，由计算错误的次数
	CCalcTag();
	virtual ~CCalcTag(){};
};

class CTagAccumulate :public CCalcTag // 累计点。累计点包含一个计算表达式，但值用的是累计时间或累计次数 或累差
{
public:
	ACE_Time_Value	tvLastValue;			// 上次累计点的时间 上次的点值
	bool			bLastSatified;	// 上次是否满足条件
	ACE_UINT64		lValue;			// 上次累计点的值（时间秒或次数）

	CTagAccumulate();
	virtual ~CTagAccumulate(){};
};

class CTagAccumulateTime :public CTagAccumulate // 累计时间点. 累计时间点包含一个计算表达式，但值用的是累计时间或累计次数 或累差
{
public:
	CTagAccumulateTime(){
		nTagType = VARIABLE_TYPE_ACCUMULATE_TIME;
	}
	virtual ~CTagAccumulateTime(){};
};

class CTagAccumulateCount :public CTagAccumulate // 累计开关次数点. 累计开关点包含一个计算表达式，但值用的是累计时间或累计次数 或累差
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
	unsigned char	bAlarming;		// 是否正在报警.不用boolean，避免转为json时变成true而不是1
	unsigned char	bConfirmed;		// 报警是否已经确认.不用boolean，避免转为json时变成true而不是1
	string			strValueOnProduce;	// 报警发生时的值
	string			strValueOnRecover;	// 报警恢复时的值
	string			strValueOnConfirm;	// 报警确认时的值
	string			strThresholdOnProduce; // 报警发生时的门限值

	HighResTime		tmProduceFirst;	// 第一次报警的产生时间
	HighResTime		tmProduce;	// 最新的报警产生时间
	HighResTime		tmConfirm;	// 确认时间
	HighResTime		tmRecover;	// 报警恢复时间
	int				nRepetitions;	// 报警发生次数（在未确认前反复发生的次数）

	string			strConfirmUserName;
	
	// 如是否抑制报警、处理方法等参数。不是配置参数，是运行中可以动态写入的参数
	string				strParam1;		// 报警预留参数1
	string				strParam2;		// 报警预留参数2
	string				strParam3;		// 报警预留参数3
	string				strParam4;		// 报警预留参数4

	CAlarmTag		*pAlarmTag;	// 报警点
public:
	CAlarmingInfo();
};

// 用于控制有关需要的tag点信息
class CAlarmTag //:public CTag
{
public:
	bool				bEnable;
	int					nLevel;
	string				strAlarmTypeName; // 报警类型名称，如超上限警告

	string				strJudgeMethod;		// EQ5
	int					nJudgeMethodType;	// EQ
	string				strJudgeMethodIndex;	// 5 . 如果为0则表示EQ

	// 字符串类型的限值定义
	string				strThreshold;	// 字符串类型的，如EQ、HAS、IN 报警限值的字符串表示
	string				strThreshold2;	// 字符串类型的，如EQ、HAS、IN 报警限值的字符串表示
	// 整数类型的限值定义
	ACE_INT64			nThreshold;		// 只需要判断1个限值时的值，如 大于 或 等于
	ACE_INT64			nThreshold2;	// 只需要判断1个限值时的值，如 大于 且 小于
	// 小数类型的限值定义
	double				dbThreshold;    // 只需要判断1个限值时的值，如 大于 或 等于
	double				dbThreshold2;   // 只需要判断1个限值时的值，如 大于 且 小于

	// in 判断符仅对整数 、 字符串
	std::vector<ACE_INT64>		vecThresholdInt;   // 文本类型做in操作
	std::vector<std::string>	vecThresholdText;   // 字符串类型做in操作
	std::vector<double>			vecThresholdReal;	// 小数类型做in操作
	string				strDesc;

	CDataTag			*pTag;			// 指向对应的tag点

	vector<CAlarmingInfo *> vecAlarming;
	CAlarmInfo			m_alarmInfo;	// 该报警点对应的报警信息
public:
	CAlarmTag();
	void GetThresholdAsString(string &strThresholdDesc);
	void GetThresholdAsString_Int(string &strThresholdDesc);
	void GetThresholdAsString_Real(string &strThresholdDesc);
	void GetThresholdAsString_String(string &strThresholdDesc);

public:
	static int GetJudgeMethodTypeAndIndex(const char *szJudgeMethod, int &nJudgeMethodType, string &strJudgeMethodIndex);
	// 将门限值根据不同类型拆分存放到不同的成员变量中
	int SplitThresholds_Real(const char *szThresholdText);
	// 将门限值根据不同类型拆分存放到不同的成员变量中
	int SplitThresholds_Integer(const char *szThresholdText);
	int SplitThresholds_Text(const char *szThresholdText);
	int SplitThresholds(const char *szThresholdText);
};


#endif // _NODESERVER_COMMON_DEFINE_H_
