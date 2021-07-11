/**************************************************************
 *  Filename:    SystemConfig.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 读取配置信息.
 *
 *  @author:     liuqifeng
 *  @version     04/13/2009  liuqifeng  Initial Version
**************************************************************/
#include "ace/ACE.h"
#include "ace/OS_NS_strings.h"
#include "ace/DLL.h"
#include "SystemConfig.h"
#include "pkcomm/pkcomm.h"
#include "eviewcomm/eviewcomm.h"
#include  "common/eviewdefine.h"
#include "pklog/pklog.h"
#include "common/PKMiscHelper.h"
#include <algorithm>

#include "pkeviewdbapi/pkeviewdbapi.h"
#pragma comment(lib, "pkeviewdbapi")
extern CPKLog g_logger;
// 用户自定义的Modbus配置文件.DLL名为mdobusservicecfg.dll

#define NULLASSTRING(x) x==NULL?"":x
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSystemConfig::CSystemConfig()
{
	m_pForwardInfo = new ModbusServerInfo();
}

CSystemConfig::~CSystemConfig()
{
	m_pForwardInfo->m_vecAIAddrToDit.clear();
	m_pForwardInfo->m_vecAOAddrToDit.clear();
	m_pForwardInfo->m_vecDIAddrToDit.clear();
	m_pForwardInfo->m_vecDOAddrToDit.clear();
	m_pForwardInfo->m_vecUnkwnAddrToDit.clear();
	m_pForwardInfo->strConnType = m_pForwardInfo->strConnParam = "";
	delete m_pForwardInfo;
}

bool lessAddrMap(const ModbusServiceTag * s1, const ModbusServiceTag * s2)
{
	return s1->nStartRegisterNo < s2->nStartRegisterNo;
}

int CSystemConfig::GetRegisterType(const char *szRegisterType)
{
	int nRegisterType = MODBUS_REGISTERTYPE_UNKOWN;
	if(ACE_OS::strcasecmp(szRegisterType,MODBUS_REGISTERTYPE_NAME_AI) == 0)
	{
		nRegisterType = MODBUS_REGISTERTYPE_AI;
	}
	else if(ACE_OS::strcasecmp(szRegisterType, MODBUS_REGISTERTYPE_NAME_AO) == 0)
	{
		nRegisterType = MODBUS_REGISTERTYPE_AO;
	}
	else if(ACE_OS::strcasecmp(szRegisterType, MODBUS_REGISTERTYPE_NAME_DI) == 0)
	{
		nRegisterType = MODBUS_REGISTERTYPE_DI;
	}
	else if(ACE_OS::strcasecmp(szRegisterType, MODBUS_REGISTERTYPE_NAME_DO) == 0)
	{
		nRegisterType = MODBUS_REGISTERTYPE_DO;
	}
	return nRegisterType;
}

// 根据tag数据类型、长度、对应的寄存器类型，得到寄存器长度.nLenBytes传入时已经计算好字节数
// 对于字符串需要长度，其他类型保留为0即可。每个数据类型占用固定长度的寄存器，BIT必须用DI或DO，其他必须是AI或AO，字符串和BLOB为(1+长度)/2，其他类型都是固定长度（1...4）个AI/AO寄存器
int CSystemConfig::GetRegisterNum(int nDataType, int nRegisterType, int nLenBytes)
{
	if(nDataType == TAG_DT_UNKNOWN) // 非法数据类型
		return 0;

	if(nDataType == TAG_DT_BOOL) // BOOL型的，无论寄存器什么类型，最多只占1个寄存器
		return 1;

	// 后面的都是模拟量，需要计算实际的字节数
	int nDataTypeLenBits = PKMiscHelper::GetTagDataTypeLen(nDataType, nLenBytes);
	int nDataTypeLenBytes = nDataTypeLenBits / 8;
	if(nRegisterType == MODBUS_REGISTERTYPE_AI || nRegisterType == MODBUS_REGISTERTYPE_AO) // 如果AI或AO
		return (1+nLenBytes)/2;
	else
		return nDataTypeLenBytes * 8;
}

// create table 
int CSystemConfig::ReadConfig()
{
	m_pForwardInfo->m_vecAIAddrToDit.clear();
	m_pForwardInfo->m_vecAOAddrToDit.clear();
	m_pForwardInfo->m_vecDIAddrToDit.clear();
	m_pForwardInfo->m_vecDOAddrToDit.clear();
	m_pForwardInfo->m_vecUnkwnAddrToDit.clear();

	int nRet = PK_SUCCESS;
	char szTip[PK_PARAM_MAXLEN] = {0};

	CPKEviewDbApi PKEviewDbApi;
	nRet = PKEviewDbApi.Init();
	if(nRet != 0)
	{
		PKEviewDbApi.Exit();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "初始化数据库失败!");
		return -1;
	}

	char szSql[2048] = {0};
	long lQueryId = -1;

	m_pForwardInfo->strConnType = "tcp";
	m_pForwardInfo->strConnParam = "502";

	// 读取端口号和连接方式
    vector<vector<string> > vecConn;
	sprintf(szSql, "select id,conntype,connparam,description from t_modserver_info where enable is null or enable <> 0");
	string strError;
	nRet = PKEviewDbApi.SQLExecute(szSql, vecConn, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "未配置modbus server连接方式和连接，读取数据库失败,sql:%s, error:%s", szSql, strError.c_str());
	}
	else
	{
		if (vecConn.size() <= 0)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "未配置modbus server连接方式和连接，读取数据库得到有效记录条数为0,sql:%s", szSql);
		}
		else
		{
			vector<string> &row = vecConn[0];
			int iCol = 0;

			string strId = NULLASSTRING(row[iCol].c_str());
			iCol ++;
			string strConnType = NULLASSTRING(row[iCol].c_str());
			iCol ++;
			string strConnParam = NULLASSTRING(row[iCol].c_str());
			iCol ++;
		
			m_pForwardInfo->strConnType = strConnType;
			m_pForwardInfo->strConnParam = strConnParam;
			// g_logger.LogMessage(PK_LOGLEVEL_INFO, "modbus server, conntype:%s, connparam:%s", strConnType.c_str(), strConnParam.c_str());
		}
	}//if(nRet != 0 && vecConn.size() > 0)

	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "modbus server连接方式:%s,连接参数:%s", m_pForwardInfo->strConnType.c_str(), m_pForwardInfo->strConnParam.c_str());
	////////////////////////读取配置的转发的modbus点
    vector<vector<string> > vecTag;
	sprintf(szSql, "select id,tagname,datatype,datatypelen,registertype,startregisterno,description from t_modserver_tag");
	nRet = PKEviewDbApi.SQLExecute(szSql, vecTag, &strError);
	if(nRet != 0)
	{
		PKEviewDbApi.Exit();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "查询数据库失败:%s, error:%s", szSql, strError.c_str());
		return -2;
	}

	if(vecTag.size() <= 0)
	{
		PKEviewDbApi.Exit();
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "未找到任何可用的驱动, SQL:%s!", szSql);
		return -1;
	}

	for(int iDev = 0; iDev < vecTag.size(); iDev ++)
	{
		vector<string> &row = vecTag[iDev];
		int iCol = 0;

		string strId = NULLASSTRING(row[iCol].c_str());
		iCol ++;
		string strTagName = NULLASSTRING(row[iCol].c_str());
		iCol ++;
		string strDataType = NULLASSTRING(row[iCol].c_str());
		iCol ++;
		string strDataTypeLen = NULLASSTRING(row[iCol].c_str());
		iCol ++;
		string strRegisterType = NULLASSTRING(row[iCol].c_str());
		iCol ++;
		string strStartAddr = NULLASSTRING(row[iCol].c_str());
		iCol ++;
		string strDescription = NULLASSTRING(row[iCol].c_str());
		iCol ++;
		if(strTagName.empty())
		{
			PKStringHelper::Snprintf(szTip, sizeof(szTip), "tag define invalid,id:%s,name:%s",  strId.c_str(),strTagName.c_str());
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, szTip);
			continue;
		}

		ModbusServiceTag *pModbusServiceTag = new ModbusServiceTag();
		PKStringHelper::Safe_StrNCpy(pModbusServiceTag->pModbusTag->szName, strTagName.c_str(), sizeof(pModbusServiceTag->pModbusTag->szName));
		pModbusServiceTag->nRegisterType = GetRegisterType(strRegisterType.c_str());
		pModbusServiceTag->nStartRegisterNo = ::atoi(strStartAddr.c_str());
		int nDataType = 0;
		int nDataTypeLenBits = 0;
		PKMiscHelper::GetTagDataTypeAndLen(strDataType.c_str(), strDataTypeLen.c_str(), &nDataType, &nDataTypeLenBits);
		if (nDataType == TAG_DT_UNKNOWN)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "tag:%s datatype:%s not supported, assume int32!!", strTagName.c_str(), strDataType.c_str());
			nDataType = TAG_DT_INT32;
			nDataTypeLenBits = 32; // 32位4个字节
		}

		pModbusServiceTag->nDataType = nDataType;
		pModbusServiceTag->nDataTypeLen = (nDataTypeLenBits + 7) / 8; // 按字节为单位
		pModbusServiceTag->nRegisterNum = GetRegisterNum(nDataType, pModbusServiceTag->nRegisterType, pModbusServiceTag->nDataTypeLen); // 算得寄存器个数
		pModbusServiceTag->nEndRegisterNo = pModbusServiceTag->nStartRegisterNo + pModbusServiceTag->nRegisterNum - 1;

		if(pModbusServiceTag->nRegisterType == MODBUS_REGISTERTYPE_AI)
			m_pForwardInfo->m_vecAIAddrToDit.push_back(pModbusServiceTag);
		else if(pModbusServiceTag->nRegisterType == MODBUS_REGISTERTYPE_AO)
			m_pForwardInfo->m_vecAOAddrToDit.push_back(pModbusServiceTag);
		else if(pModbusServiceTag->nRegisterType == MODBUS_REGISTERTYPE_DI)
			m_pForwardInfo->m_vecDIAddrToDit.push_back(pModbusServiceTag);
		else if(pModbusServiceTag->nRegisterType == MODBUS_REGISTERTYPE_DO)
			m_pForwardInfo->m_vecDOAddrToDit.push_back(pModbusServiceTag);
		else
			m_pForwardInfo->m_vecUnkwnAddrToDit.push_back(pModbusServiceTag);
		// 加入到m_vecAllTags，以便调用modbusdata.dll的Init接口使用
		m_vecAllTags.push_back(pModbusServiceTag->pModbusTag);
	}

	PKEviewDbApi.Exit();

	//升序排序
	sort(m_pForwardInfo->m_vecDOAddrToDit.begin(), m_pForwardInfo->m_vecDOAddrToDit.end(),lessAddrMap) ; //或者sort(ctn.begin(), ctn.end())   默认情况为升序
	sort(m_pForwardInfo->m_vecAOAddrToDit.begin(), m_pForwardInfo->m_vecAOAddrToDit.end(),lessAddrMap) ; //或者sort(ctn.begin(), ctn.end())   默认情况为升序
	sort(m_pForwardInfo->m_vecDIAddrToDit.begin(), m_pForwardInfo->m_vecDIAddrToDit.end(),lessAddrMap) ; //或者sort(ctn.begin(), ctn.end())   默认情况为升序
	sort(m_pForwardInfo->m_vecAIAddrToDit.begin(), m_pForwardInfo->m_vecAIAddrToDit.end(),lessAddrMap) ; //或者sort(ctn.begin(), ctn.end())   默认情况为升序
	g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "load config tagcount:%d,AI:%d,AO:%d,DI:%d,DO:%d,Unknown:%d", m_vecAllTags.size(),
		m_pForwardInfo->m_vecAIAddrToDit.size(),m_pForwardInfo->m_vecAOAddrToDit.size(),m_pForwardInfo->m_vecDIAddrToDit.size(),
		m_pForwardInfo->m_vecDOAddrToDit.size(),m_pForwardInfo->m_vecUnkwnAddrToDit.size());
	return nRet;
}

