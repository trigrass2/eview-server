#include "common/gettimeofday.h"
#include "NodeCommonDef.h"
#include "eviewcomm/eviewcomm.h"
#include "RedisFieldDefine.h"
#include "MainTask.h"
#include "common/eviewdefine.h"
#include "common/PKMiscHelper.h"

void inttostring(int nValue, string &strValue)
{
	char szValue[128] = { 0 };
	memset(szValue, 0, sizeof(szValue));

	if (nValue == INT_MIN)
		sprintf(szValue, "INT_MIN");
	else if (nValue == INT_MAX)
		sprintf(szValue, "INT_MAX");
	else
		sprintf(szValue, "%d", nValue);
	strValue = szValue;
}

void realtostring(double dbValue, string &strValue)
{
	char szValue[128] = { 0 };

	int nMaxVal = dbValue - DBL_MAX;
	int nMinVal = dbValue + DBL_MAX;
	//if (std::abs(nMinVal) < 100)
	//	strcpy(szValue, "DBL_MIN");
	//else if (std::abs(nMaxVal) < 100)
	//	strcpy(szValue, "DBL_MAX");
	//else
	sprintf(szValue, "%f", dbValue);
	strValue = szValue;
}

SERVERTAG_DATA::SERVERTAG_DATA()
{
	dbValue = 0.0f;
	nValue = 0;
	strValue = TAG_QUALITY_INIT_STATE_STRING;
	nQuality = TAG_QUALITY_INIT_STATE;
}

void SERVERTAG_DATA::GetStrValue(int nDataTypeClass, string &strTagValue) // 返回字符串类型的值
{
	// 先处理质量不好的情况
	if (this->nQuality != 0)
	{
		if (this->nQuality == TAG_QUALITY_INIT_STATE)
			strTagValue = TAG_QUALITY_INIT_STATE_STRING;
		else if (this->nQuality == TAG_QUALITY_COMMUNICATE_FAILURE)
			strTagValue = TAG_QUALITY_COMMUNICATE_FAILURE_STRING;
		else if (this->nQuality == TAG_QUALITY_DRIVER_NOT_ALIVE)
			strTagValue = TAG_QUALITY_DRIVER_NOT_ALIVE_STRING;
		else if (this->nQuality == TAG_QUALITY_UNKNOWN_REASON)
			strTagValue = TAG_QUALITY_UNKNOWN_REASON_STRING;
		else
			strTagValue = "???";
		return;
	}

	// 质量好的情况的处理
	char szStrValue[1024];
	if (nDataTypeClass == DATATYPE_CLASS_INTEGER)
	{
		sprintf(szStrValue, "%ld", this->nValue);
		strTagValue = szStrValue;
	}
	else if (nDataTypeClass == DATATYPE_CLASS_REAL)
	{
		sprintf(szStrValue, "%lf", this->dbValue);
		strTagValue = szStrValue;
	}
	else// if (pTag->nDataTypeClass == DATATYPE_CLASS_TEXT)
	{
		strTagValue = this->strValue;
	}
}

void SERVERTAG_DATA::AssignStrValueAndQuality(int nDataTypeClass, string &strTagValue, int nNewQuality) // 返回字符串类型的值
{
	if (nDataTypeClass == DATATYPE_CLASS_INTEGER)
	{
		this->nValue = ::atoi(strTagValue.c_str());
	}
	else if (nDataTypeClass == DATATYPE_CLASS_REAL)
	{
		this->dbValue = ::atof(strTagValue.c_str());
	}
	else// if (nDataTypeClass == DATATYPE_CLASS_TEXT)
	{
		this->strValue = strTagValue;
	}
	this->nQuality = nNewQuality;
}


void SERVERTAG_DATA::AssignStrValueAndQuality(int nDataTypeClass, const char *szTagValue, int nNewQuality) // 返回字符串类型的值
{
	if (nDataTypeClass == DATATYPE_CLASS_INTEGER)
	{
		this->nValue = ::atoi(szTagValue);
	}
	else if (nDataTypeClass == DATATYPE_CLASS_REAL)
	{
		this->dbValue = ::atof(szTagValue);
	}
	else// if (nDataTypeClass == DATATYPE_CLASS_TEXT)
	{
		this->strValue = szTagValue;
	}
	this->nQuality = nNewQuality;
}

void SERVERTAG_DATA::AssignTagValue(int nDataTypeClass, SERVERTAG_DATA *pRightData) // 返回字符串类型的值
{
	if (nDataTypeClass == DATATYPE_CLASS_INTEGER)
	{
		this->nValue = pRightData->nValue;
		this->nQuality = pRightData->nQuality;
	}
	else if (nDataTypeClass == DATATYPE_CLASS_REAL)
	{
		this->dbValue = pRightData->dbValue;
		this->nQuality = pRightData->nQuality;
	}
	else// if (nDataTypeClass == DATATYPE_CLASS_TEXT)
	{
		this->strValue = pRightData->strValue;
		this->nQuality = pRightData->nQuality;
	}
}

CLabel::CLabel()
{
	nControlVal = 0;
	strLabelName = "";
	strDesc = "";
}

CMyObject::CMyObject()
{
	strName = strSubsysName = strDeviceName = strDescription = "";
	nDeviceId = 0;
	strDeviceName = strDriverName = ""; 
	nQuality = TAG_QUALITY_INIT_STATE;
	vecTags.clear();
	mapProps.clear();
	m_pDevice = NULL;
	pClass = NULL;
	strParam1 = strParam2 = strParam3 = strParam4 = "";
}

void CMyObject::UpdateToDataMemDB(Json::FastWriter &jsonWriter, char *szCurTime)
{
	// 实时数据的VTQ
	Json::Value jsonTagData;
	if (szCurTime)
		jsonTagData[JSONFIELD_TIME] = szCurTime;
	else
	{
		char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
		PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
	}
	jsonTagData[JSONFIELD_QUALITY] = nQuality;
	m_alarmInfo.GenerateAlarmInfo2Json(jsonTagData);
	string strTagData = jsonWriter.write(jsonTagData);
	vector<string> vecTagName, vecTagVTQ;
	vecTagName.push_back(strName);
	vecTagVTQ.push_back(strTagData);
	//MAIN_TASK->m_redisRW0.set(strName.c_str(), strTagData.c_str());

	// 每个报警类型,写入一个点
	for (map<string, CAlarmInfo *>::iterator it = this->m_mapAlarmTypeName2Info.begin(); it != this->m_mapAlarmTypeName2Info.end(); it++)
	{
		string strAlarmType = it->first;
		CAlarmInfo *pAlarmInfo = it->second;

		Json::Value jsonAlarmType;
		pAlarmInfo->GenerateAlarmInfo2Json(jsonAlarmType);

		string strTagName = strName + "." + strAlarmType;
		string strValue = jsonWriter.write(jsonAlarmType); // .toStyledString();

		vecTagName.push_back(strTagName);
		vecTagVTQ.push_back(strValue);
		//MAIN_TASK->m_redisRW0.set(strTagName.c_str(), strValue.c_str()); // 未提交
	}

	MAIN_TASK->m_redisRW0.mset(vecTagName, vecTagVTQ);
	vecTagName.clear();
	vecTagVTQ.clear();
}

// 仅仅在MAINTASK中初始化时调用一次
void CMyObject::UpdateToConfigMemDB(Json::FastWriter &jsonWriter)
{
	Json::Value jsonObject;
	jsonObject["name"] = strName;
	//jsonObject[OBJECTJSONFIELD_TAGCOUNT] = vecTags.size();
	string strConfigData = jsonWriter.write(jsonObject); // .toStyledString();
	MAIN_TASK->m_redisRW2.set(strName.c_str(), strConfigData.c_str());

}
void CAlarmInfo::GenerateAlarmInfo2Json(Json::Value &jsonAlarmInfo)
{
	jsonAlarmInfo[JSONFIELD_MAXALARM_LEVEL] = this->m_nMaxAlarmLevel;
	jsonAlarmInfo[JSONFIELD_MINALARM_LEVEL] = this->m_nMinAlarmLevel;
	jsonAlarmInfo[JSONFIELD_ALMCOUNT] = this->m_nAlarmCount;
	jsonAlarmInfo[JSONFIELD_ALMING_UNACK_COUNT] = this->m_nAlarmingUnAckCount;
	jsonAlarmInfo[JSONFIELD_ALMINGCOUNT] = this->m_nAlarmingCount;
	jsonAlarmInfo[JSONFIELD_ALM_UNACKCOUNT] = this->m_nAlarmUnConfirmedCount;
	jsonAlarmInfo[JSONFIELD_ALM_ACKEDCOUNT] = this->m_nAlarmConfirmedCount;
}

CMyDevice::CMyDevice()
{
	m_nId = 0;
	m_strName = "";
	m_strDesc = "";
	m_strSimulate = "";
	m_bDisable = false;
	m_nDirverId = 0;

	m_strParam1 = "";
	m_strParam2 = "";
	m_strParam3 = "";
	m_strParam4 = "";
	m_strProjectId = "";
	m_pDriver = NULL;
	m_vecDeviceTag.clear();
}

CMyDevice::~CMyDevice()
{
}

CMyPysicalDevice::CMyPysicalDevice()
{
	// 下面参数在本程序中不使用
	m_strConnType = "";
	m_strConnParam = "";
	m_nConnTimeout;
	m_strTaskNo = "";
}
CMyPysicalDevice::~CMyPysicalDevice()
{
}

// 模拟设备
CMySimulateDevice::CMySimulateDevice()
{
	m_strName = DEVICE_NAME_SIMULATE;
}
CMySimulateDevice::~CMySimulateDevice()
{
}

// 内存变量设备
CMyMemoryDevice::CMyMemoryDevice()
{
	m_strName = DEVICE_NAME_MEMORY;
}
CMyMemoryDevice::~CMyMemoryDevice()
{
}

CMyDriver::CMyDriver()
{
	m_nId = 0;
	m_strName = "";
	m_strDesc = "";
	m_mapDevice.clear();
}
CMyDriver::~CMyDriver()
{
}

CMyDriverPhysicalDevice::CMyDriverPhysicalDevice()
{
	m_strModuleName = "";
	m_pDrvShm = NULL;
	m_nType = DRIVER_TYPE_PHYSICALDEVICE;
}
CMyDriverPhysicalDevice::~CMyDriverPhysicalDevice()
{
}

CMyDriverMemory::CMyDriverMemory()
{
	m_nId = DRIVER_TYPE_MEMORY;
	m_nType = DRIVER_TYPE_MEMORY;
	m_strName = DRIVER_NAME_MEMORY;
}
CMyDriverMemory::~CMyDriverMemory()
{
}

CMyDriverSimulate::CMyDriverSimulate()
{
	m_nId = DRIVER_TYPE_SIMULATE;
	m_nType = DRIVER_TYPE_SIMULATE;
	m_strName = DRIVER_NAME_SIMULATE;
}
CMyDriverSimulate::~CMyDriverSimulate()
{
}

CMyClass::CMyClass()
{
	m_nId = m_nProjectId = m_nParentId = -1;
	m_strName = m_strDesc = strParentName = "";
	m_mapClassProp.clear();
};

CMyClassProp::CMyClassProp()
{
	m_nId = 0;
	m_strName = "";
	m_strDesc = "";
	m_strParam = "";
	m_strAddress = "";
	m_strDataType = "";
	m_strSubsysName = "";
	m_strSubsysName = "";
	m_strLabelName = "";
	m_strHisDataInterval = m_strTrendDataInterval = "";
	m_bEnableRange = false;
	m_nByteOrderNo = 0;
	m_strInitValue = "";
	m_nDataLen = 0;

	m_bEnableRange = 0.f;   // 允许范围换算
	m_dbRangeFactor = 0.f;	// 范围换算计算出来的乘数系数，y=b*x+c的c
	m_dbRangePlusValue = 0.f; // 范围换算计算出来的增加值，y=b*x+c的c

	m_nPollRateMS = 0;
	m_pClass = NULL;
}

int CMyClassProp::CalcRangeParam(string strEnableRange, string strRawMin, string strRawMax, string strOutMin, string strOutMax)
{
	if (strEnableRange.compare("1") == 0)
		m_bEnableRange = true;
	else
		m_bEnableRange = false;

	if (m_bEnableRange == false)
		return 0;

	if (this->m_nDataTypeClass == DATATYPE_CLASS_INTEGER)
	{
		int nRawMin = ::atoi(strRawMin.c_str());
		int nRawMax = ::atoi(strRawMax.c_str());
		int nOutMin = ::atoi(strOutMin.c_str());
		int nOutMax = ::atoi(strOutMax.c_str());
		int nRawSpan = nRawMax - nRawMin;
		int nOutSpan = nOutMax - nOutMin;
		if (nRawSpan == 0 || nOutSpan == 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "int class:%s, prop:%s, enable range, value span is ZERO!INVALID! RawMin:%s, RawMax:%s, OutMin:%s, OutMax:%s",
				this->m_pClass->m_strName.c_str(), this->m_strName.c_str(), strRawMin.c_str(), strRawMax.c_str(), strOutMin.c_str(), strOutMax.c_str());
			this->m_bEnableRange = false;
			return -1;
		}
		this->m_dbRangeFactor = (1.0f * nOutSpan) / nRawSpan;
		return 0;
	}

	if (this->m_nDataTypeClass == DATATYPE_CLASS_REAL)
	{
		double dbRawMin = ::atof(strRawMin.c_str());
		double dbRawMax = ::atof(strRawMax.c_str());
		double dbOutMin = ::atof(strOutMin.c_str());
		double dbOutMax = ::atof(strOutMax.c_str());
		double dbRawSpan = dbRawMax - dbRawMin;
		double dbOutSpan = dbOutMax - dbOutMin;
		if (dbRawSpan == 0 || dbOutSpan == 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "real class:%s, prop:%s, enable range, value span is ZERO!INVALID! RawMin:%s, RawMax:%s, OutMin:%s, OutMax:%s",
				this->m_pClass->m_strName.c_str(), this->m_strName.c_str(), strRawMin.c_str(), strRawMax.c_str(), strOutMin.c_str(), strOutMax.c_str());
			this->m_bEnableRange = false;
			return -1;
		}
		this->m_dbRangeFactor = dbOutSpan / dbRawSpan;
		return 0;
	}

	this->m_bEnableRange = false;
	return 0;
}

CMyClassPropAlarm::CMyClassPropAlarm(){
	m_nId = 0;
	m_nLevel = 0;
	m_bEnable = false;
	m_pClassProp = NULL;
	m_strDescription = m_strJudgeMethod = m_strAlarmTypeName = "";
	m_nDeadtimeProduce = m_nDeadtimeRecover = 0;
}

CMyObjectPropAlarm::CMyObjectPropAlarm(){
	m_nId = 0;
	m_nLevel = 0;
	m_bEnable = false;
	m_pObjectProp = NULL;
	m_strDescription = m_strPropName = m_strJudgeMethod = m_strAlarmTypeName = "";
}

CMyObjectProp::CMyObjectProp()
{
	m_nId = 0;
	m_strDesc = "";
	m_strParam = "";
	m_strAddress = "";
	//m_strDataType = "";
	m_strSubsysName = "";
	m_strSubsysName = "";
	m_strHisDataInterval = "";
	m_strTrendDataInterval = "";

	m_bEnableRange = 0.f;   // 允许范围换算
	m_dbRangeFactor = 0.f;	// 范围换算计算出来的乘数系数，y=b*x+c的c
	m_dbRangePlusValue = 0.f; // 范围换算计算出来的增加值，y=b*x+c的c
	m_strInitValue = "";
	m_nPropTypeId = TAGTYPE_DEVICE;
	m_nPollRateMS = 0;
	//m_nDataLen = 0;
	m_pObject = NULL;

	m_mapAlarms.clear();
	m_pObject = NULL;
	n_pClassProp = NULL;
}

int CMyObjectProp::CalcRangeParam(string strEnableRange, string strRawMin, string strRawMax, string strOutMin, string strOutMax)
{
	if (strEnableRange.compare("1") == 0)
		m_bEnableRange = true;
	else
		m_bEnableRange = false;

	if (m_bEnableRange == false)
		return 0;

	if (this->n_pClassProp->m_nDataTypeClass == DATATYPE_CLASS_INTEGER)
	{
		int nRawMin = ::atoi(strRawMin.c_str());
		int nRawMax = ::atoi(strRawMax.c_str());
		int nOutMin = ::atoi(strOutMin.c_str());
		int nOutMax = ::atoi(strOutMax.c_str());
		int nRawSpan = nRawMax - nRawMin;
		int nOutSpan = nOutMax - nOutMin;
		if (nRawSpan == 0 || nOutSpan == 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "int object:%s, prop:%s, enable range, value span is ZERO!INVALID! RawMin:%s, RawMax:%s, OutMin:%s, OutMax:%s",
				this->m_pObject->strName.c_str(), this->n_pClassProp->m_strName.c_str(), strRawMin.c_str(), strRawMax.c_str(), strOutMin.c_str(), strOutMax.c_str());
			this->m_bEnableRange = false;
			return -1;
		}
		this->m_dbRangeFactor = (1.0f * nOutSpan) / nRawSpan;
		return 0;
	}

	if (this->n_pClassProp->m_nDataTypeClass == DATATYPE_CLASS_REAL)
	{
		double dbRawMin = ::atof(strRawMin.c_str());
		double dbRawMax = ::atof(strRawMax.c_str());
		double dbOutMin = ::atof(strOutMin.c_str());
		double dbOutMax = ::atof(strOutMax.c_str());
		double dbRawSpan = dbRawMax - dbRawMin;
		double dbOutSpan = dbOutMax - dbOutMin;
		if (dbRawSpan == 0 || dbOutSpan == 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "real object:%s, prop:%s, enable range, value span is ZERO!INVALID! RawMin:%s, RawMax:%s, OutMin:%s, OutMax:%s",
				this->m_pObject->strName.c_str(), this->n_pClassProp->m_strName.c_str(), strRawMin.c_str(), strRawMax.c_str(), strOutMin.c_str(), strOutMax.c_str());
			this->m_bEnableRange = false;
			return -1;
		}
		this->m_dbRangeFactor = dbOutSpan / dbRawSpan;
		return 0;
	}

	this->m_bEnableRange = false;
	return 0;
}

CDataTag::CDataTag(){
	strName = strDesc = strNodeName = "";//点的名称
	nTagType = VARIABLE_TYPE_DEVICE;	// 0表示数据点，1表示报警点，2表示计算点，3表示报警区点
	
	strInitValue = "";
	strDataType = "";
	strSubsysName = "";
	nTagDataLen = 0;
	pObject = NULL;
	pObjProp = NULL;
	nSubsysID = 0;
	nLabelID = 0;
	nDataType = TAG_DT_INT32;
	nDataTypeClass = DATATYPE_CLASS_OTHER;
	nPollrateMS = 1000;

	nByteOrderNo = 0;
	bEnableAlarm = true;
	bEnableControl = true;
	strHisDataInterval = "";
	strTrendDataInterval = "";

	bEnableRange = false;   // 允许范围换算
	m_dbRangeFactor = 0.f;	// 范围换算计算出来的乘数系数，y=b*x+c的c
	m_dbRangePlusValue = 0.f; // 范围换算计算出来的增加值，y=b*x+c的c

	vecCalcTags.clear();
	vecAllAlarmTags.clear();

	curVal.nQuality = TAG_QUALITY_INIT_STATE; // -30000
	curVal.strValue = TAG_QUALITY_INIT_STATE_STRING; // ***
	lastVal.nQuality = TAG_QUALITY_INIT_STATE; // -30000
	lastVal.strValue = TAG_QUALITY_INIT_STATE_STRING; // ***

	pDevice = NULL;
}

int CDataTag::GetDataTypeId(const char *szDataTypeName)
{
	int nDataTypeId = 0;
	int nDataTypeLenBits;
	PKMiscHelper::GetTagDataTypeAndLen(szDataTypeName, NULL, &nDataTypeId, &nDataTypeLenBits);
	return nDataTypeId;
}

int CDataTag::CalcRangeParam(string strEnableRange, string strRawMin, string strRawMax, string strOutMin, string strOutMax)
{
	if (strEnableRange.compare("1") == 0)
		bEnableRange = true;
	else
		bEnableRange = false;

	if (bEnableRange == false)
		return 0;

	if (this->nDataTypeClass == DATATYPE_CLASS_INTEGER)
	{
		int nRawMin = ::atoi(strRawMin.c_str());
		int nRawMax = ::atoi(strRawMax.c_str());
		int nOutMin = ::atoi(strOutMin.c_str());
		int nOutMax = ::atoi(strOutMax.c_str());
		int nRawSpan = nRawMax - nRawMin;
		int nOutSpan = nOutMax - nOutMin;
		if (nRawSpan == 0 || nOutSpan == 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "int tag:%s, enable range, value span is ZERO!INVALID! RawMin:%s, RawMax:%s, OutMin:%s, OutMax:%s",
				this->strName.c_str(), strRawMin.c_str(), strRawMax.c_str(), strOutMin.c_str(), strOutMax.c_str());
			this->bEnableRange = false;
			return -1;
		}
		this->m_dbRangeFactor = (1.0f * nOutSpan) / nRawSpan;
		return 0;
	}

	if (this->nDataTypeClass == DATATYPE_CLASS_REAL)
	{
		double dbRawMin = ::atof(strRawMin.c_str());
		double dbRawMax = ::atof(strRawMax.c_str());
		double dbOutMin = ::atof(strOutMin.c_str());
		double dbOutMax = ::atof(strOutMax.c_str());
		double dbRawSpan = dbRawMax - dbRawMin;
		double dbOutSpan = dbOutMax - dbOutMin;
		if (dbRawSpan == 0 || dbOutSpan == 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "real tag:%s, enable range, value span is ZERO!INVALID! RawMin:%s, RawMax:%s, OutMin:%s, OutMax:%s",
				this->strName.c_str(), strRawMin.c_str(), strRawMax.c_str(), strOutMin.c_str(), strOutMax.c_str());
			this->bEnableRange = false;
			return -1;
		}
		this->m_dbRangeFactor = dbOutSpan / dbRawSpan;
		return 0;
	}

	this->bEnableRange = false;
	return 0;
}

float CDataTag::CalcRangeValue(float fValue)
{
	if (!this->bEnableRange)
		return fValue;
	return (fValue * m_dbRangeFactor + m_dbRangePlusValue);
}

double CDataTag::CalcRangeValue(double dbValue)
{
	if (!this->bEnableRange)
		return dbValue;
	return (dbValue * m_dbRangeFactor + m_dbRangePlusValue);
}

int CDataTag::CalcRangeValue(int nValue)
{
	if (!this->bEnableRange)
		return nValue;
	return (nValue * m_dbRangeFactor + m_dbRangePlusValue);
}

ACE_INT64 CDataTag::CalcRangeValue(ACE_INT64 nValue)
{
	if (!this->bEnableRange)
		return nValue;
	return (nValue * m_dbRangeFactor + m_dbRangePlusValue);
}

ACE_UINT64 CDataTag::CalcRangeValue(ACE_UINT64 nValue)
{
	if (!this->bEnableRange)
		return nValue;
	return (nValue * m_dbRangeFactor + m_dbRangePlusValue);
}

int CDataTag::GetDataTypeClass(int nDataTypeId)
{
	int nDataTypeClass = DATATYPE_CLASS_OTHER;
	switch (nDataTypeId)
	{
	case TAG_DT_BOOL:
	case TAG_DT_CHAR:
	case TAG_DT_UCHAR:
	case TAG_DT_INT16:
	case TAG_DT_UINT16:
	case TAG_DT_INT32:
	case TAG_DT_UINT32:
	case TAG_DT_INT64:
	case TAG_DT_UINT64:
		nDataTypeClass = DATATYPE_CLASS_INTEGER;
		break;
	case TAG_DT_FLOAT:
	case TAG_DT_DOUBLE:
		nDataTypeClass = DATATYPE_CLASS_REAL;
		break;
	case TAG_DT_TEXT:
		nDataTypeClass = DATATYPE_CLASS_TEXT;
		break;
	case TAG_DT_BLOB:
		nDataTypeClass = DATATYPE_CLASS_OTHER;
		break;
	default:
		break;
	}
	return nDataTypeClass;
}

void CDataTag::GetJsonStringToDataMemDB(Json::FastWriter &jsonWriter, char *szCurTime, string &strJsonValue)
{
	string strTagValue;
	this->curVal.GetStrValue(this->nDataTypeClass, strTagValue);
	Json::Value jsonOneTag;
	jsonOneTag[JSONFIELD_TIME] = szCurTime;
	jsonOneTag[JSONFIELD_QUALITY] = this->curVal.nQuality;
	jsonOneTag[JSONFIELD_MAXALARM_LEVEL] = m_curAlarmInfo.m_nMaxAlarmLevel;
	jsonOneTag[JSONFIELD_VALUE] = strTagValue;
	strJsonValue = jsonWriter.write(jsonOneTag);
}

CDeviceTag::CDeviceTag(){
	strAddress = "";
	pDevice = NULL;
	nTagType = VARIABLE_TYPE_DEVICE;
}

CSimTag::CSimTag()
{
	m_strDeviceSimValue = "";
	m_dbSimValue = 0.f;
	m_strSimValue = "Text";
	m_bAutoInc = true;
	m_dbSimIncStep = 1;
	nTagType = VARIABLE_TYPE_SIMULATE;
}

CMemTag::CMemTag()
{
	m_strMemValue = "";
	nTagType = VARIABLE_TYPE_MEMVAR;
	curVal.AssignStrValueAndQuality(this->nDataTypeClass, this->strInitValue, 0);
	this->lastVal.AssignTagValue(this->nDataTypeClass, &curVal);
}

CPredefTag::CPredefTag()
{
	m_strPredefValue = "";
	nTagType = VARIABLE_TYPE_PREDEFINE;
}

CCalcTag::CCalcTag()
{
	strAddress = "";
	nTagType = VARIABLE_TYPE_CALC;
	tvLastLogCalcError = ACE_Time_Value(0);
	nCalcErrorNumLog = 0;
}

CTagAccumulate::CTagAccumulate(){
	nTagType = VARIABLE_TYPE_ACCUMULATE_TIME;
	tvLastValue = ACE_OS::gettimeofday();
	bLastSatified = false;
	lValue = 0;
	strAddress = "";
}

void CAlarmTag::GetThresholdAsString(string &strThresholdDesc)
{
	if (pTag->nDataTypeClass == DATATYPE_CLASS_INTEGER)
		GetThresholdAsString_Int(strThresholdDesc);
	else if (pTag->nDataTypeClass == DATATYPE_CLASS_REAL)
		GetThresholdAsString_Real(strThresholdDesc);
	else
		GetThresholdAsString_String(strThresholdDesc);
}

CAlarmingInfo::CAlarmingInfo()
{
	bConfirmed = true;
	bAlarming = false;
	nRepetitions = 0;
	strConfirmUserName = "";
	strValueOnProduce = "";
	strValueOnRecover = "";
	strValueOnConfirm = "";
	strThresholdOnProduce = "";
	pAlarmTag = NULL;
	strParam1 = strParam2 = strParam3 = strParam4 = "";
	// tmProduce = tmConfirm = tmRecover = 0;
}

void CAlarmTag::GetThresholdAsString_Int(string &strThresholdDesc)
{
	char szThresholds[256] = { 0 };
	string strThresholdTmp1, strThresholdTmp2;
	inttostring(nThreshold, strThresholdTmp1);
	inttostring(nThreshold2, strThresholdTmp2);

	switch (nJudgeMethodType)
	{
	case ALARM_JUDGE_HHH:
	case ALARM_JUDGE_HH:
	case ALARM_JUDGE_H:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "[%s,%s)", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_L:
	case ALARM_JUDGE_LL:
	case ALARM_JUDGE_LLL:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "(%s,%s]", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_ON:
	case ALARM_JUDGE_OFF:
	case ALARM_JUDGE_EQ:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_CHANGETOEQ:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "->=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_QUALITYBAD:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "==BAD");
		break;	
	case ALARM_JUDGE_CHANGETOQUALITYBAD:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "GOOD->BAD");
		break;
	case ALARM_JUDGE_NE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "!=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_GT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), ">%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_GTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), ">=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_LT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "<%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_LTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "<=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_GTELT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "[%s,%s)", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_GTLT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "(%s,%s)", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_GTLTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "(%s,%s]", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_GTELTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "[%s,%s]", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_IN:
		for (int i = 0; i < vecThresholdInt.size(); i++)
		{
			char szTmp[32] = { 0 };
			ACE_INT64 &nThreshold = vecThresholdInt[i];
			PKStringHelper::Snprintf(szTmp, sizeof(szTmp), "%d,", nThreshold);
			strThresholdDesc = strThresholdDesc + szTmp;
		}
		return;
	}
	strThresholdDesc = szThresholds;
}

void CAlarmTag::GetThresholdAsString_Real(string &strThresholdDesc)
{
	char szThresholds[256] = { 0 };
	string strThresholdTmp1, strThresholdTmp2;
	realtostring(dbThreshold, strThresholdTmp1);
	realtostring(dbThreshold2, strThresholdTmp2);

	switch (nJudgeMethodType)
	{
	case ALARM_JUDGE_HHH:
	case ALARM_JUDGE_HH:
	case ALARM_JUDGE_H:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "[%s,%s)", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_L:
	case ALARM_JUDGE_LL:
	case ALARM_JUDGE_LLL:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "(%s,%s]", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_ON:
	case ALARM_JUDGE_OFF:
	case ALARM_JUDGE_EQ:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_CHANGETOEQ:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "->=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_QUALITYBAD:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "==BAD");
		break;
	case ALARM_JUDGE_CHANGETOQUALITYBAD:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "GOOD->BAD");
		break;
	case ALARM_JUDGE_NE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "!=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_GT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), ">%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_GTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), ">=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_LT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "<%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_LTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "<=%s", strThresholdTmp1.c_str());
		break;
	case ALARM_JUDGE_GTELT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "[%s,%s)", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_GTLT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "(%s,%s)", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_GTLTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "(%s,%s]", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_GTELTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "[%s,%s]", strThresholdTmp1.c_str(), strThresholdTmp2.c_str());
		break;
	case ALARM_JUDGE_IN:
		for (int i = 0; i < vecThresholdReal.size(); i++)
		{
			char szTmp[128] = { 0 };
			double &dbThreshold = vecThresholdReal[i];
			PKStringHelper::Snprintf(szTmp, sizeof(szTmp), "%f,", dbThreshold);
			strThresholdDesc = strThresholdDesc + szTmp;
		}
		return;
	}
	strThresholdDesc = szThresholds;
}

void CAlarmTag::GetThresholdAsString_String(string &strThresholdDesc)
{
	char szThresholds[256] = { 0 };
	switch (nJudgeMethodType)
	{
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_ON:
	case ALARM_JUDGE_OFF:
	case ALARM_JUDGE_EQ:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "=%s", strThreshold.c_str());
		break;
	case ALARM_JUDGE_CHANGETOEQ:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "->=%s", strThreshold.c_str());
		break;
	case ALARM_JUDGE_QUALITYBAD:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "==BAD");
		break;
	case ALARM_JUDGE_CHANGETOQUALITYBAD:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "GOOD->BAD");
		break;
	case ALARM_JUDGE_NE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "!=%s", strThreshold.c_str());
		break;
	case ALARM_JUDGE_GT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), ">%s", strThreshold.c_str());
		break;
	case ALARM_JUDGE_GTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), ">=%s", strThreshold.c_str());
		break;
	case ALARM_JUDGE_LT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "<%s", strThreshold.c_str());
		break;
	case ALARM_JUDGE_LTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "<=%s", strThreshold.c_str());
		break;
	case ALARM_JUDGE_GTELT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "[%s,%s)", strThreshold.c_str(), strThreshold2.c_str());
		break;
	case ALARM_JUDGE_GTLT:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "(%s,%s)", strThreshold.c_str(), strThreshold2.c_str());
		break;
	case ALARM_JUDGE_GTLTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "(%s,%s]", strThreshold.c_str(), strThreshold2.c_str());
		break;
	case ALARM_JUDGE_GTELTE:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "[%s,%s]", strThreshold.c_str(), strThreshold2.c_str());
		break;
	case ALARM_JUDGE_CONTAIN:
		PKStringHelper::Snprintf(szThresholds, sizeof(szThresholds), "%s", strThreshold.c_str());
		break;
	case ALARM_JUDGE_IN:
		string strThresholdsText;
		for (int i = 0; i < vecThresholdText.size(); i++)
		{
			string &strThresholdTmp = vecThresholdText[i];
			strThresholdDesc = strThresholdDesc + strThresholdTmp + ",";
		}
		return;
		break;
	}
	strThresholdDesc = szThresholds;
}

CAlarmTag::CAlarmTag(){
	pTag = NULL;
	bEnable = false;
	nLevel = 0;

	strAlarmTypeName = "";
	strJudgeMethod = "";
	nJudgeMethodType = ALARM_JUDGE_INVALID;
	strJudgeMethodIndex = "";

	dbThreshold = dbThreshold2 = 0.0f;
	nThreshold = nThreshold2 = 0;
	strThreshold = strThreshold2 = "";
	vecThresholdInt.clear();
	vecThresholdText.clear();

	strDesc = "";

	vecAlarming.clear();
}

int CAlarmTag::GetJudgeMethodTypeAndIndex(const char *szJudgeMethod, int &nJudgeMethodType, string &strJudgeMethodIndex)
{
	char *chOne = (char *)szJudgeMethod;
	strJudgeMethodIndex = "";
	string strJudgeMethodType = "";
	bool bJudgeMethodIndexPart = false;
	while (*chOne != '\0') // EQ5--->EQ,5
	{
		if (*chOne >= '0' && *chOne <= '9')
		{
			bJudgeMethodIndexPart = true;
		}

		if (!bJudgeMethodIndexPart)
			strJudgeMethodType = strJudgeMethodType + *chOne;
		else
			strJudgeMethodIndex = strJudgeMethodIndex + *chOne;
		chOne++;
	}

	nJudgeMethodType = ALARM_JUDGE_INVALID;
	if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_H_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_H;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_HH_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_HH;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_HHH_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_HHH;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_L_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_L;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_LL_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_LL;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_LLL_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_LLL;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_FX_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_FX;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_ON_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_ON;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_OFF_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_OFF;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_EQ_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_EQ;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_CHANGETOEQ_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_CHANGETOEQ;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_CHANGETOQUALITYBAD_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_CHANGETOQUALITYBAD;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_QUALITYBAD_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_QUALITYBAD;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_NE_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_NE;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_GT_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_GT;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_GTE_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_GTE;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_LT_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_LT;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_LTE_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_LTE;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_GTELT_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_GTELT;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_GTLT_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_GTLT;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_GTLTE_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_GTLTE;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_GTELTE_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_GTELTE;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_CONTAIN_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_CONTAIN;
	else if (PKStringHelper::StriCmp(strJudgeMethodType.c_str(), ALARM_JUDGE_IN_TEXT) == 0)
		nJudgeMethodType = ALARM_JUDGE_IN;

	return nJudgeMethodType;
}

// 将门限值根据不同类型拆分存放到不同的成员变量中
int CAlarmTag::SplitThresholds_Real(const char *szThresholdText)
{
	if (strlen(szThresholdText) <= 0)
		return -1;

	int nRet = 0;
	switch (nJudgeMethodType)
	{
		// 一个参数
	case ALARM_JUDGE_H:
	case ALARM_JUDGE_HH:
	case ALARM_JUDGE_HHH:
	case ALARM_JUDGE_L:
	case ALARM_JUDGE_LL:
	case ALARM_JUDGE_LLL:
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_EQ:
	case ALARM_JUDGE_CHANGETOEQ:
	case ALARM_JUDGE_NE:
	case ALARM_JUDGE_GT:
	case ALARM_JUDGE_GTE:
	case ALARM_JUDGE_LT:
	case ALARM_JUDGE_LTE:
		dbThreshold = ::atof(szThresholdText);
		break;
	case ALARM_JUDGE_QUALITYBAD: // 不需要门限值
	case ALARM_JUDGE_CHANGETOQUALITYBAD: // 不需要门限值
		dbThreshold = 0.0f;
		break;
	case ALARM_JUDGE_ON:
		dbThreshold = 1.0f;
		break;
	case ALARM_JUDGE_OFF:
		dbThreshold = 0.0f;
		break;
	case ALARM_JUDGE_GTELT:
	case ALARM_JUDGE_GTLT:
	case ALARM_JUDGE_GTLTE:
	case ALARM_JUDGE_GTELTE:
	{
		vector<string> vecThreshold = PKStringHelper::StriSplit(szThresholdText, ",");
		if (vecThreshold.size() <= 1)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s,judgemethod:%d, need 2 thresholds, but only 1 now! %s", pTag->strName.c_str(), nJudgeMethodType, szThresholdText);
			return -2;
		}
		double dbThresholdTmp1 = ::atof(vecThreshold[0].c_str());
		double dbThresholdTmp2 = ::atof(vecThreshold[1].c_str());
		if (dbThresholdTmp1 < dbThresholdTmp2)
		{
			this->dbThreshold = dbThresholdTmp1;
			this->dbThreshold2 = dbThresholdTmp2;
		}
		else
		{
			this->dbThreshold = dbThresholdTmp2;
			this->dbThreshold2 = dbThresholdTmp1;
		}
	}
	break;
	case ALARM_JUDGE_IN:
	{
		vector<string> vecThreshold = PKStringHelper::StriSplit(szThresholdText, ",");
		for (int i = 0; i < vecThreshold.size(); i++)
			this->vecThresholdReal.push_back(::atof(vecThreshold[i].c_str()));
	}
	break;
	default:
		break;
	} // switch (nJudgeMethod)
	return 0;
}

// 将门限值根据不同类型拆分存放到不同的成员变量中
int CAlarmTag::SplitThresholds_Integer(const char *szThresholdText)
{
	if (strlen(szThresholdText) <= 0)
		return -1;

	int nRet = 0;
	switch (nJudgeMethodType)
	{
		// 一个参数
	case ALARM_JUDGE_H:
	case ALARM_JUDGE_HH:
	case ALARM_JUDGE_HHH:
	case ALARM_JUDGE_L:
	case ALARM_JUDGE_LL:
	case ALARM_JUDGE_LLL:
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_EQ:
	case ALARM_JUDGE_CHANGETOEQ:
	case ALARM_JUDGE_NE:
	case ALARM_JUDGE_GT:
	case ALARM_JUDGE_GTE:
	case ALARM_JUDGE_LT:
	case ALARM_JUDGE_LTE:
		this->nThreshold = ::atoi(szThresholdText);
		this->nThreshold2 = -1;
		break;

	case ALARM_JUDGE_QUALITYBAD: // 不需要门限值
	case ALARM_JUDGE_CHANGETOQUALITYBAD: // 不需要门限值
		this->nThreshold = 0;
		this->nThreshold2 = 0;
		break;
	case ALARM_JUDGE_ON:
		this->nThreshold = 1;
		break;
	case ALARM_JUDGE_OFF:
		this->nThreshold = 0;
		break;
	case ALARM_JUDGE_GTELT:
	case ALARM_JUDGE_GTLT:
	case ALARM_JUDGE_GTLTE:
	case ALARM_JUDGE_GTELTE:
	{
		vector<string> vecThreshold = PKStringHelper::StriSplit(szThresholdText, ",");
		if (vecThreshold.size() <= 1)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s,judgemethod:%d, need 2 thresholds, but only 1 now! %s", pTag->strName.c_str(), nJudgeMethodType, szThresholdText);
			return -2;
		}
		double nThresholdTmp1 = ::atoi(vecThreshold[0].c_str());
		double nThresholdTmp2 = ::atoi(vecThreshold[1].c_str());
		if (nThresholdTmp1 < nThresholdTmp2)
		{
			this->nThreshold = nThresholdTmp1;
			this->nThreshold2 = nThresholdTmp2;
		}
		else
		{
			this->nThreshold = nThresholdTmp2;
			this->nThreshold2 = nThresholdTmp1;
		}
	}
	break;
	case ALARM_JUDGE_IN:
	{
		vector<string> vecThreshold = PKStringHelper::StriSplit(szThresholdText, ",");
		for (int i = 0; i < vecThreshold.size(); i++)
			this->vecThresholdInt.push_back(::atoi(vecThreshold[i].c_str()));
	}
	break;
	default:
		break;
	} // switch (nJudgeMethod)
	return 0;
}

int CAlarmTag::SplitThresholds_Text(const char *szThresholdText)
{
	if (strlen(szThresholdText) <= 0)
		return -1;

	int nRet = 0;
	switch (nJudgeMethodType)
	{
		// 一个参数
	case ALARM_JUDGE_H:
	case ALARM_JUDGE_HH:
	case ALARM_JUDGE_HHH:
	case ALARM_JUDGE_L:
	case ALARM_JUDGE_LL:
	case ALARM_JUDGE_LLL:
	case ALARM_JUDGE_FX:
	case ALARM_JUDGE_EQ:
	case ALARM_JUDGE_CHANGETOEQ:
	case ALARM_JUDGE_NE:
	case ALARM_JUDGE_GT:
	case ALARM_JUDGE_GTE:
	case ALARM_JUDGE_LT:
	case ALARM_JUDGE_LTE:
	case ALARM_JUDGE_CONTAIN:
		this->strThreshold = szThresholdText;
		break;
	case ALARM_JUDGE_QUALITYBAD: // 不需要门限值
	case ALARM_JUDGE_CHANGETOQUALITYBAD: // 不需要门限值
		this->strThreshold = "";
		break;
	case ALARM_JUDGE_ON:
		this->strThreshold = "1";
		break;
	case ALARM_JUDGE_OFF:
		this->strThreshold = "0";
		break;
	case ALARM_JUDGE_GTELT:
	case ALARM_JUDGE_GTLT:
	case ALARM_JUDGE_GTLTE:
	case ALARM_JUDGE_GTELTE:
	{
		vector<string> vecThreshold = PKStringHelper::StriSplit(szThresholdText, ",");
		if (vecThreshold.size() <= 1)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s,judgemethod:%d, need 2 thresholds, but only 1 now! %s", pTag->strName.c_str(), nJudgeMethodType, szThresholdText);
			return -2;
		}
		string strThresholdTmp1 = vecThreshold[0];
		string strThresholdTmp2 = vecThreshold[1];
		if (strThresholdTmp1.compare(strThresholdTmp2) < 0)
		{
			this->strThreshold = strThresholdTmp1;
			this->strThreshold2 = strThresholdTmp2;
		}
		else
		{
			this->strThreshold = strThresholdTmp2;
			this->strThreshold2 = strThresholdTmp1;
		}
	}
	break;
	case ALARM_JUDGE_IN:
	{
		vector<string> vecThreshold = PKStringHelper::StriSplit(szThresholdText, ",");
		for (int i = 0; i < vecThreshold.size(); i++)
			this->vecThresholdText.push_back(vecThreshold[i]);
	}
	break;
	default:
		break;
	} // switch (nJudgeMethod)
	return 0;
}

int CAlarmTag::SplitThresholds(const char *szThresholdText)
{
	if (pTag->nDataTypeClass == DATATYPE_CLASS_INTEGER)
		return SplitThresholds_Integer(szThresholdText);
	else if (pTag->nDataTypeClass == DATATYPE_CLASS_REAL)
		return SplitThresholds_Real(szThresholdText);
	else if (pTag->nDataTypeClass == DATATYPE_CLASS_TEXT)
		return SplitThresholds_Text(szThresholdText);
	else
		return -100;
}