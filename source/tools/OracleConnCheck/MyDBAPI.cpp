#include "MyDBAPI.h"
#include <fstream>  
#include <streambuf>  
#include <iostream>  
#include<algorithm>
#include <string>
#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
//#include "sqlapi/SQLAPI.h"

#if defined (_WIN32)
// Define the pathname separator characters for Win32 (ugh).
#  define PK_DIRECTORY_SEPARATOR_STR "\\"
#  define PK_DIRECTORY_SEPARATOR_CHAR '\\'
#else
// Define the pathname separator characters for UNIX.
#  define PK_DIRECTORY_SEPARATOR_STR "/"
#  define PK_DIRECTORY_SEPARATOR_CHAR '/'
#endif /* _WIN32 */

#include "pkdbapi/pkdbapi.h"
#pragma comment(lib, "pkdbapi")
// 127.0.0.1:3306@park

string trim(const string& str)
{
	string::size_type pos = str.find_first_not_of(' ');
	if (pos == string::npos)
	{
		return str;
	}
	string::size_type pos2 = str.find_last_not_of(' ');
	if (pos2 != string::npos)
	{
		return str.substr(pos, pos2 - pos + 1);
	}
	return str.substr(pos);
}

CMyDbApi::CMyDbApi(string strPath)
{
	m_lConnectionId = 0;
	m_bIsConnected = 0;
//	m_nDatabaseTypeId = 0;
	m_strConfPath = strPath;
	m_nMinLogConn = 2;
	m_nTimeSpan = 10000;
//	strDBType = strConnStr = strUserName = strPassword = strCodingSet = "";
}
CMyDbApi::CMyDbApi()
{
	m_lConnectionId = 0;
//	m_nDatabaseTypeId = 0;
	m_strConfPath = "";
//	strDBType = strConnStr = strUserName = strPassword = strCodingSet = "";
}

CMyDbApi::~CMyDbApi()
{
	//Exit();
}

int CMyDbApi::InitLocalDB()
{
	int	nTimeOut = 3;
	string strConfPath = PKComm::GetConfigPath();
	string strSqlitePath = strConfPath + PK_DIRECTORY_SEPARATOR_STR + "wstransfer.db";
	int nRet = SQLConnect(/*m_lConnectionId,*/ strSqlitePath.c_str(), "", "", SA_SQLite_Client, nTimeOut, ""); // sqlite 无论UTF8或者GB2312，都需要在读取出数据后，调用UTF8GB2312进行转换
	if(nRet == 0)
		PKLog.LogMessage(PK_LOGLEVEL_DEBUG, "连接本地数据库:%s成功", strSqlitePath.c_str(), nRet);
	else
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "连接本地数据库:%s, 错误码:%d", strSqlitePath.c_str(), nRet);

	return nRet;
}
//检查表是否存在
bool CMyDbApi::CheckTargetTableExist( string strTableName )
{
	char szSql[128] = {0};
	if(atoi(strDBType.c_str()) == SA_Oracle_Client)
	{	
		sprintf_s(szSql, "select count(*) from user_tables where table_name = %s", strTableName.c_str());
		return  SQLExecute(szSql)==1 ? true : false;
	}
	
	vector<vector<string>> vec_Row;
	sprintf(szSql, "select * from '%s' limit 1", strTableName.c_str());
	int nRet = SQLExecute(szSql, vec_Row);

	return nRet == PK_SUCCESS ? true :false ;
}
/*
dbtype=mysql
	connection=;
username=root
	password=r123456
*/
int CMyDbApi::Init()
{
	// 得到配置文件的路径
	string strConfigPath = PKComm::GetConfigPath();
	strConfigPath = strConfigPath + PK_DIRECTORY_SEPARATOR_STR + this->m_strConfPath;

	std::ifstream textFile(strConfigPath.c_str());  
	if(!textFile.is_open())  // 不存在该文件，则认为是sqlite：eview.db
	{  
		// PKLog.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return InitLocalDB();  
	}

	// 计算文件大小  
	textFile.seekg(0, std::ios::end);  
	std::streampos len = textFile.tellg();  
	textFile.seekg(0, std::ios::beg);  

	// 方法1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());  

	textFile.close();  

	string strConnStr = "";
	string strUserName = "";
	string strPassword = "";
	string strTimeout = "";
	string strCodingSet = "";
	while(strConfigData.length() > 0)
	{
		string strLine = "";
		int nPosLine = strConfigData.find('\n');
		if(nPosLine > 0)
		{
			strLine = strConfigData.substr(0, nPosLine);
			strConfigData = strConfigData.substr(nPosLine + 1);
		}
		else
		{
			strLine = strConfigData;
			strConfigData = "";
		}

		strLine = trim(strLine);
		if(strLine.empty())
			continue;

		// x=y
		int nPosSection = strLine.find('=');
		if(nPosSection <= 0) // 无=号或=号开头
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s, line: %s invalid, need =!", strConfigPath.c_str(), strLine.c_str());
			continue;
		}

		string strSecName = strLine.substr(0, nPosSection);
		string strSecValue = strLine.substr(nPosSection + 1);
		strSecName = trim(strSecName);
		strSecValue = trim(strSecValue);
		std::transform(strSecName.begin(), strSecName.end(), strSecName.begin(), ::tolower);
		if(strSecName.compare("dbtype") == 0)
			m_strDatabaseType = strDBType = strSecValue;
		else if(strSecName.compare("connection") == 0)
			strConnStr = strSecValue;
		else if(strSecName.compare("username") == 0)
			strUserName = strSecValue;
		else if(strSecName.compare("password") == 0)
			strPassword = strSecValue;
		else if(strSecName.compare("timeout") == 0)
			strTimeout = strSecValue;
		else if (strSecName.compare("codingset") == 0)
			strCodingSet = strSecValue;
		else if (strSecName.compare("logminconn") == 0)
			m_nMinLogConn = atoi(strSecValue.c_str());
		else if (strSecName.compare("timespan") == 0)
			m_nTimeSpan = atoi(strSecValue.c_str());
		else
		{
			//PKLog.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s, key:%s unknown!", strConfigPath.c_str(), strSecName.c_str());
			continue;
		}
	}

	m_nDataBaseType = GetDatabaseTypeId(strDBType.c_str());
	if (m_nDataBaseType == -1)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s, 不支持的数据库类型:%s!", strConfigPath.c_str(), strDBType.c_str());
		return -1;
	}

	int nTimeOut = ::atoi(strTimeout.c_str());
	if(nTimeOut <= 0)
		nTimeOut = 3;

	int nRet = -1;
	// MYSQL 虽然是UTF8，但必须设置为GB2312才能读取中文
	if(strCodingSet.empty())
		strCodingSet = "utf8";

	if (m_nDataBaseType == SA_SQLite_Client)
		nRet = InitLocalDB();
	//nRet = SQLConnect(m_lConnectionId, strConnStr.c_str(), strUserName.c_str(), strPassword.c_str(), nDatabaseTypeId, nTimeOut, "");
	else // 其他数据库
	{
		nRet = SQLConnect(strConnStr.c_str(), strUserName.c_str(), strPassword.c_str(), m_nDataBaseType, nTimeOut, strCodingSet.c_str());
		if(nRet == 0)
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "连接数据库(%s,连接串:%s)成功", strDBType.c_str(), strConnStr.c_str());
		else
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "连接数据库(%s,连接串:%s)失败, 返回:%d", strDBType.c_str(), strConnStr.c_str(), nRet);
	}
	if (nRet != PK_SUCCESS)
	{
		m_bIsConnected = false;
	}
	else
		m_bIsConnected = true;
	return nRet;
}

int CMyDbApi::Exit()
{
	if(m_lConnectionId > 0)
	{
		SQLDisconnect(/*m_lConnectionId*/);
		m_lConnectionId = 0;
	}

	UnInitialize();
	return 0;
}

/************************************************************************/
/*创建表
  由于查询语句没有表字段的类型，所以都用varchar

*/
/************************************************************************/
int CMyDbApi::ExeCreateTable( string strTableName, vector<string> vec_fieldName, map<string,string> map_fieldName )
{
	char szSql[512] = {0};
	char szFields[512] = {0};
	vector<string>::iterator itFiledName = vec_fieldName.begin();
	for(; itFiledName != vec_fieldName.end() ; itFiledName++)
	{
		strcat(szFields, (*itFiledName).c_str());
		strcat(szFields, " ");
		strcat(szFields,map_fieldName[*itFiledName].c_str());
		strcat(szFields, ",");
	}
	szFields[strlen(szFields)-1]='\0';
	sprintf(szSql, "create table '%s'(%s)", strTableName.c_str(), szFields);
	return this->SQLExecute(/*m_lConnectionId,*/ szSql);
	PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"新建表%s成功",strTableName.c_str());
}

// int CMyDbApi::ExeCreateTable( string strTargetTableName, vector<string> vec_FieldName, vector<string> vec_FieldType)
// {
// 	char szSql[512] = {0};
// 	char szFields[512] = {0};
// 	for (vector<vector<string>>::iterator itFieldType = vec_ValueType.begin(); itFieldType!=vec_ValueType.end(); itFieldType++)
// 	{
// 		strcat(szFields, (*itFieldType)[0].c_str());
// 		strcat(szFields,(*itFieldType)[1].c_str());
// 		strcat(szFields, ",");
// 	}
// 	szFields[strlen(szFields)-1]='\0';
// 	sprintf(szSql, "create table %s(%s)", strTargetTableName.c_str(), szFields);
// 	return this->SQLExecute(/*m_lConnectionId,*/ szSql);
// }

/************************************************************************/
/*                                                                      */
/************************************************************************/
int CMyDbApi::ExeInsertValues( string strTableName, vector<vector<string>> rows )
{
	char szSql[1024] = {0};
	for (vector<vector<string>>::iterator itFieldValues = rows.begin() ; itFieldValues!=rows.end() ; itFieldValues++)
	{
		char szRowValue[512] = {0};
		for (vector<string>::iterator itRowValues = (*itFieldValues).begin() ; itRowValues!=(*itFieldValues).end() ; itRowValues++)
		{
			if (itRowValues->data() == "")
			{
				strcat(szRowValue, "' '");
			}
			else
			{
				char szValue[64] = {0};
				sprintf(szValue, "'%s'", (*itRowValues).c_str());
				strcat(szRowValue, szValue);
			}
			strcat(szRowValue,",");
		}
		szRowValue[strlen(szRowValue)-1] = '\0';
		sprintf(szSql, "insert into '%s' values(%s)", strTableName.c_str() ,szRowValue);
		SQLExecute(/*m_lConnectionId,*/ szSql);
		memset(szRowValue, 0x0, sizeof(szRowValue));
		memset(szSql, 0x0, sizeof(szSql));
	}
	return 0;
}

int CMyDbApi::CheckConnection()
{
	if (!m_bIsConnected)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "未连接:%s", m_strConfPath.c_str());
		int nRet = Init();
		if (nRet != PK_SUCCESS)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "尝试连接，失败:%s", m_strConfPath.c_str());
			return -1;
		}
		else
		{
			PKLog.LogMessage(PK_LOGLEVEL_INFO, "尝试连接，成功:%s", m_strConfPath.c_str());
			return 0;
		}
	}
	return 0;
}
