/************************************************************************
 *	Filename:		CVDRIVERCOMMON.CPP
 *	Copyright:		Shanghai Peak InfoTech Co., Ltd.
 *
 *	Description:	$(Desp) .
 *
 *	@author:		shijunpu
 *	@version		??????	shijunpu	Initial Version
 *  @version	9/17/2012  shijunpu  增加设置元素大小接口 .
*************************************************************************/
#include "pkdata/pkdata.h"
#include "common/eviewdefine.h"
#include "pkmemdbapi/pkmemdbapi.h"
#include "json/json.h"
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "pkcrashdump/pkcrashdump.h"

#pragma comment(lib, "jsoncpp")

#define BITS_PER_BYTE 8

#ifdef WIN32
#ifndef NO_EXCEPTION_ATTACHER
#include "common/RegularDllExceptionAttacher.h"
#pragma comment(lib, "ExceptionReport.lib")
#pragma comment(lib, "dbghelp.lib")
// global member to capture exception so as to analyze 
static CRegularDllExceptionAttacher	g_RegDllExceptAttacher;
#endif//#ifndef NO_EXCEPTION_ATTACHER
#endif

#include <sstream>
#include <string>
using namespace std;

//PKDATA_EXPORTS int pkUpdate(PKHANDLE hHandle, char *szTagName, char *szTagVal);	// 仅仅更新内存数据库，不做控制
//PKDATA_EXPORTS int pkMUpdate(PKHANDLE hHandle, PKDATA tagData[], int nTagNum);	// 仅仅更新内存数据库，不做控制

#include <fstream>  
#include <streambuf>  
#include <iostream>  

string Json2String(Json::Value &jsonVal)
{
	if (jsonVal.isNull())
		return "";

	string strRet = "";
	if (jsonVal.isString())
	{
		strRet = jsonVal.asString();
		return strRet;
	}

	if (jsonVal.isInt())
	{
		int nVal = jsonVal.asInt();
		char szTmp[32];
		sprintf(szTmp, "%d", nVal);
		strRet = szTmp;
		return strRet;
	}

	if (jsonVal.isDouble())
	{
		double dbVal = jsonVal.asDouble();
		char szTmp[32];
		sprintf(szTmp, "%f", dbVal);
		strRet = szTmp;
		return strRet;
	}

	if (jsonVal.isBool())
	{
		bool bVal = jsonVal.asBool();
		char szTmp[32];
		sprintf(szTmp, "%d", bVal ? 1 : 0);
		strRet = szTmp;
		return strRet;
	}

	return "";
}

// 得到PKMemDB的端口号PKMemDB的密码
int GetPKMemDBPort(int &nListenPort, string &strPassword)
{
	nListenPort = PKMEMDB_LISTEN_PORT;
	strPassword = PKMEMDB_ACCESS_PASSWORD;

	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_OS_DIR_SEPARATOR + "pkmemdb.conf";
	std::ifstream textFile(strConfigPath.c_str());
	if (!textFile.is_open())  // 不存在该文件，则认为是sqlite：eview.db
	{
		// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return -1;
	}

	// 计算文件大小  
	textFile.seekg(0, std::ios::end);
	std::streampos len = textFile.tellg();
	textFile.seekg(0, std::ios::beg);

	// 方法1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());
	textFile.close();
	vector<string> vecLine = PKStringHelper::StriSplit(strConfigData, "\n");
	for (int iLine = 0; iLine < vecLine.size(); iLine++)
	{
		string strLine = vecLine[iLine];
		vector<string> vecContent = PKStringHelper::StriSplit(strLine, " ");
		if (vecContent.size() < 2)
			continue;
		string strKey = vecContent[0];
		string strValue = vecContent[1];
		if (strKey.compare("port") == 0)
			nListenPort = ::atoi(strValue.c_str());
		if (strKey.compare("requirepass") == 0)
			strPassword = strValue.c_str();
	}
	return 0;
}


// 这里的IP是指要连接的内存数据库IP。szUserName：初始化模拟登录的用户名称。szPassword：初始化模拟登录的用户密码，不是内存数据库的密码
PKDATA_EXPORTS PKHANDLE pkInit(char *szIp, char *szUserName, char *szPassword)
{
	char szRedisIp[PK_NAME_MAXLEN] = {0};
	if(!szIp || strlen(szIp) == 0)
		strcpy(szRedisIp, "127.0.0.1");
	else
		PKStringHelper::Safe_StrNCpy(szRedisIp, szIp, sizeof(szRedisIp));

	string strPassword = PKMEMDB_ACCESS_PASSWORD;
	int	nPort = PKMEMDB_LISTEN_PORT;
	GetPKMemDBPort(nPort, strPassword);

	CMemDBClientSync *pMemDBClient = new CMemDBClientSync(); // redis读写
	pMemDBClient->initialize(szRedisIp, nPort, strPassword.c_str(), 0); // 并不会连接
	return (PKHANDLE)pMemDBClient;
}

PKDATA_EXPORTS int pkExit(PKHANDLE hHandle)// 发生错误时需要重新
{
	if(hHandle == NULL)
		return -1;
	CMemDBClientSync *pMemDBClient = (CMemDBClientSync *)hHandle;
	pMemDBClient->finalize();
	pMemDBClient = NULL;
	return 0;
}

// 仅仅支持格式：对象.属性.字段   、  点名.字段
// 字段必须有！！！
/*
int GetTagAndProp(char *szTagProp, string &strTagName, string &strFieldName)
{
	string strTagNameProp = szTagProp;
	int nPos = strTagNameProp.find_last_of('.');
	if(nPos >= 0)
	{
		strTagName = strTagNameProp.substr(0, nPos);
		strFieldName = strTagNameProp.substr(nPos + 1);
	}
	else
	{
		strTagName = strTagNameProp;
		strFieldName = "value";
	}
	return 0;
}
*/

// 获取内存数据库的值。每个值：Tag1.value  //{"v":"","t":"2015-12-02 19:14:04.035","q":"1","m":"read failcount:4"}
// szTagProp, 形式1：objectname.propname，形式2：objectname.propname.fieldname
// S160.temperature;  objectname.propname.fieldname, S160.temperature.v
PKDATA_EXPORTS int pkGet(PKHANDLE hHandle, const char *szObjectPropName, const char *szFieldName, char *szDataBuff, int nDataBufLen, int *pnOutDataLen)
{
	if (hHandle == NULL || !szObjectPropName || strlen(szObjectPropName) == 0 || !szDataBuff || nDataBufLen <= 0)
		return -1;

	int nRet = 0;
	CMemDBClientSync *pRedisRW = (CMemDBClientSync *)hHandle;
	//GetTagAndProp(szTagProp, strTagName, strFieldName);
	string strValue;
	pRedisRW->get(szObjectPropName, strValue);
	if(strValue.length() == 0)
	{
		*pnOutDataLen = 0;
		szDataBuff[0] = '\0';
		return 0;
	}

	// 如果不需要指定域值（fieldname），则返回整个对象
	if (szFieldName == NULL || strlen(szFieldName) == 0)
	{
		*pnOutDataLen = strValue.length();
		if (*pnOutDataLen > nDataBufLen - 1) // 最多取缓冲区长度-1
			*pnOutDataLen = nDataBufLen - 1;
		memcpy(szDataBuff, strValue.c_str(), *pnOutDataLen);
		szDataBuff[*pnOutDataLen] = '\0';
		return 0;
	}

	// 值必须是json格式
	Json::Reader reader;
	Json::Value jsonTagVal;
	if (!reader.parse(strValue, jsonTagVal, false))
	{
		*pnOutDataLen = 0;
		szDataBuff[0] = '\0';
		return -1000;

	}

	// 找到域值
	Json::Value  jsonField = jsonTagVal[szFieldName];
	if(jsonField.isNull())
	{
		*pnOutDataLen = 0;
		szDataBuff[0] = '\0';
		return -1001;
	}

	string strFieldValue = jsonField.asString();
	*pnOutDataLen = strFieldValue.length();
	if(*pnOutDataLen > nDataBufLen - 1) // 最多取缓冲区长度-1
		*pnOutDataLen = nDataBufLen - 1;

	memcpy(szDataBuff, strFieldValue.c_str(), *pnOutDataLen);
	szDataBuff[*pnOutDataLen] = '\0';

	return 0;
}

// 控制命令
// szFieldName为空则表示 "v"
PKDATA_EXPORTS int pkControl(PKHANDLE hHandle, const char *szObjectPropName, const char *szFieldName, const char *szData)// 发生错误时需要重新
{
	if (!szObjectPropName || !szData || strlen(szObjectPropName) == 0 || strlen(szData) == 0)
		return -100; // 参数非法

	CMemDBClientSync *pRedisRW = (CMemDBClientSync *)hHandle;
	Json::Value root;
	Json::Value oneTag;

	//string strTagName;
	string strFieldName = "";
	if (szFieldName != NULL && strlen(szFieldName) > 0)
		strFieldName = szFieldName;

	//GetTagAndProp(szTagField, strTagName, strFieldName);
	
	oneTag["name"] = szObjectPropName;
	oneTag["field"] = strFieldName.c_str();
	oneTag["value"] = szData;
	root.append(oneTag);

	string strCtrlCmd = root.toStyledString();
	int nRet = pRedisRW->publish(CHANNELNAME_CONTROL, strCtrlCmd.c_str());
	return nRet;
}

// 获取多个内存数据库的值。每个值形式：{"v":"","t":"2015-12-02 19:14:04.035","q":"1","m":"read failcount:4"}
PKDATA_EXPORTS int pkMGet(PKHANDLE hHandle, PKDATA tagData[], int nTagNum)// 发生错误时需要重新
{
	if (hHandle == NULL)
	{
		for (int i = 0; i < nTagNum; i++) // 清空
			tagData[i].szData[0] = 0;
		return -1;
	}
	CMemDBClientSync *pRedisRW = (CMemDBClientSync *)hHandle;

	int nRet = 0;
	vector<string> keys;
	vector<string> fields;
	for(int i = 0; i < nTagNum; i ++)
	{
		PKDATA *pkTagVal = &tagData[i];

		string strTagName = pkTagVal->szObjectPropName;
		string strFieldName = pkTagVal->szFieldName;
		//GetTagAndProp(pkTagVal->szTagFieldName, strTagName, strFieldName);
		keys.push_back(strTagName);
		fields.push_back(strFieldName);
	}

	// 批量获取数据的值
	vector<string> vecValues;
	nRet = pRedisRW->mget(keys, vecValues);
	if (nTagNum != vecValues.size())
	{
		for (int i = 0; i < nTagNum; i++) // 清空
			tagData[i].szData[0] = 0;
		return -2;
	}

	for (int i = 0; i < nTagNum; i++)
	{
		PKDATA *pkTagVal = &tagData[i];
		if(nRet != 0)
		{
			pkTagVal->SetData('\0', 1);
 			pkTagVal->nStatus = nRet;
			continue;
		}

		string &strValue = vecValues[i];
		Json::Reader reader;
		Json::Value jsonTagVal;
		if (!reader.parse(strValue, jsonTagVal, false)) // 失败时取整个字符串
		{
			pkTagVal->nStatus = nRet;
			pkTagVal->SetData(strValue.c_str(), strValue.length());
			continue;
		}

		//printf("pkMGet(%d/%d) = %s, but q is not INT/NULL/STRING---\n", i, nTagNum, strValue.c_str());

		Json::Value  jsonFieldQuality = jsonTagVal["q"];
		if (jsonFieldQuality.isNull()) // 质量Q的获取
		{
			pkTagVal->nStatus = -1000000;
		}
		else if (jsonFieldQuality.isInt())
			pkTagVal->nStatus = jsonFieldQuality.asInt();
		else if (jsonFieldQuality.isString())
			pkTagVal->nStatus = ::atoi(jsonFieldQuality.asCString());
		else
		{
			//printf("pkMGet(%d/%d) = %s, but q is not INT/NULL/STRING\n", i, nTagNum, strValue.c_str());
			pkTagVal->nStatus = -1000000;
		}
		//printf("pkMGet(%d/%d) = %s, but q is not INT/NULL/STRING---END\n", i, nTagNum, strValue.c_str());

		string strFieldName = fields[i]; // 可能是 v，也可能是 t 或q。如果是空则认为是 v
		//if (strFieldName.empty()) // 如果是空则认为是 v
		//	strFieldName = "v";
		if (!strFieldName.empty())
		{
			Json::Value  jsonFieldValue = jsonTagVal[strFieldName];

			if (!jsonFieldValue.isNull()) // 如果指定的域存在，如v、t、q
			{
				//printf("pkMGet(%d/%d) = %s, valuetype:%d\n", i, nTagNum, strValue.c_str(), jsonFieldValue.type());
				string strFieldValue = Json2String(jsonFieldValue);
				//printf("pkMGet(%d/%d) = %s, valuetype:%d---END\n", i, nTagNum, strValue.c_str(), jsonFieldValue.type());
				pkTagVal->SetData(strFieldValue.c_str(), strFieldValue.length());
			}
			else // 有指定的属性(如v或t），但读取到的值中不包含该属性
			{
				if (pkTagVal->nStatus == 0) // 如果找不到指定域的值，则质量为BAD
					pkTagVal->nStatus = -100001;
				PKStringHelper::Safe_StrNCpy(pkTagVal->szData, strValue.c_str(), pkTagVal->nDataBufLen); // 找不到域时，取所有字符串
			}
		}
		else // 没有指定属性的值，此时复制整个{v,t,q}，质量为其中的q的质量
		{
			PKStringHelper::Safe_StrNCpy(pkTagVal->szData, strValue.c_str(), pkTagVal->nDataBufLen); // 找不到域时，取所有字符串
		}
	}

	return nRet;
}

// 多个变量的控制
PKDATA_EXPORTS int pkMControl(PKHANDLE hHandle, PKDATA tagData[], int nTagNum)// 发生错误时需要重新
{
	CMemDBClientSync *pRedisRW = (CMemDBClientSync *)hHandle;
	Json::Value root;
	for(int i = 0; i < nTagNum; i ++)
	{
		PKDATA *pTagVal = &tagData[i];
		Json::Value oneTag;

		string strTagName = pTagVal->szObjectPropName;
		string strFieldName = pTagVal->szFieldName;
		// GetTagAndProp(pkTagVal->szTagFieldName, strTagName, strFieldName);

		oneTag["name"] = strTagName.c_str();
		oneTag["field"] = strFieldName.c_str();
		oneTag["value"] = pTagVal->szData;
		root.append(oneTag);
	}
	string strCtrlCmds = root.toStyledString();
	int nRet = pRedisRW->publish(CHANNELNAME_CONTROL, strCtrlCmds.c_str());
	return nRet;
}

// 仅仅更新内存数据库的数值。通常不需要调用，因为更新完毕后，马上会被更新回来
PKDATA_EXPORTS int pkUpdate(PKHANDLE hHandle, char *szObjectPropName, char *szTagVal, int nQuality)
{
	if (!hHandle || !szObjectPropName || !szTagVal || strlen(szObjectPropName) == 0)
		return -1001; // 参数非法

	char szTime[200] = { 0 };
	PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
	Json::Value oneTag;
	oneTag["n"] = szObjectPropName;
	oneTag["v"] = szTagVal;
	oneTag["q"] = nQuality;
	oneTag["t"] = szTime;
	string strJson = oneTag.toStyledString();

	CMemDBClientSync *pRedisRW = (CMemDBClientSync *)hHandle;
	int nRet = pRedisRW->set(szObjectPropName, strJson.c_str());
	return nRet;
}

// 仅仅批量更新内存数据库的数值。 这个也暂时不提供
//PKDATA_EXPORTS int pkMUpdate(PKHANDLE hHandle, PKDATA tagData[], int nTagNum)
//{
//	if(hHandle == NULL)
//		return -1;
//
//	vector<string> keys;
//	vector<string> values;
//	for(int i = 0; i < nTagNum; i ++)
//	{
//		PKDATA *pkTagVal = &tagData[i];
//		keys.push_back(pkTagVal->szObjectPropName);
//		values.push_back(pkTagVal->szData);
//	}
//	string strKeys, strValues;
//	CVecHelper::Vector2String(keys, strKeys);
//	CVecHelper::Vector2String(values, strValues);
//	CMemDBClientSync *pRedisRW = (CMemDBClientSync *)hHandle;
//	int nRet = pRedisRW->mset(strKeys.c_str(), strValues.c_str());
//	return nRet;
//}
