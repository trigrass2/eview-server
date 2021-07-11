#include "pkeviewdbapi/pkeviewdbapi.h"
#include <fstream>  
#include <streambuf>  
#include <iostream>  
#include <algorithm>
#include <string>
#include <functional>

#include "pkcomm/pkcomm.h"
#include "pklog/pklog.h"
#include "sqlapi/SQLAPI.h"
#include "common/eviewdefine.h"

extern CPKLog g_logger;

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

// CPKEviewDbApi PKEviewDbApi; // ���ǵ�SQLITE�²��ͷſ��ܻ�����������⣬��˽�����һ������һ�Σ�

// 127.0.0.1:3306@park

inline string& ltrim(string &str) {
    string::iterator p = find_if(str.begin(), str.end(), not1(ptr_fun<int, int>(isspace)));
    str.erase(str.begin(), p);
    return str;
}

inline string& rtrim(string &str) {
    string::reverse_iterator p = find_if(str.rbegin(), str.rend(), not1(ptr_fun<int , int>(isspace)));
    str.erase(p.base(), str.end());
    return str;
}

inline string& trim(string &str) {
    ltrim(rtrim(str));
    return str;
}

CPKEviewDbApi::CPKEviewDbApi()
{
	m_lConnectionId = 0;
}

CPKEviewDbApi::~CPKEviewDbApi()
{
	//Exit(); // ����Disconnect���쳣��
}

int CPKEviewDbApi::InitLocalSQLite(char *szCodingSet)
{
	int	nTimeOut = 3;
	string strConfPath = PKComm::GetConfigPath();
	string strSqlitePath = strConfPath + PK_OS_DIR_SEPARATOR + "eview.db";
	int nRet = SQLConnect(strSqlitePath.c_str(), "", "", SA_SQLite_Client, nTimeOut, szCodingSet); // sqlite ����UTF8����GB2312������Ҫ�ڶ�ȡ�����ݺ󣬵���UTF8GB2312����ת��
	if(nRet == 0)
		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "���ӱ������ݿ�:%s�ɹ�", strSqlitePath.c_str(), nRet);
	else
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���ӱ������ݿ�:%s, ������:%d", strSqlitePath.c_str(), nRet);

	return nRet;
}

/*
dbtype=mysql
	connection=;
username=root
	password=r123456
*/
int CPKEviewDbApi::Init()
{
	// �õ������ļ���·��
	const char *szConfigPath = PKComm::GetConfigPath();
	string strConfigPath = szConfigPath;
	strConfigPath = strConfigPath + PK_DIRECTORY_SEPARATOR_STR + "db.conf";

	std::ifstream textFile(strConfigPath.c_str());  
	if(!textFile.is_open())  // �����ڸ��ļ�������Ϊ��sqlite��eview.db
	{  
		// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s not exist", strConfigPath.c_str());
		return InitLocalSQLite("");  
	}

	// �����ļ���С  
	textFile.seekg(0, std::ios::end);  
	std::streampos len = textFile.tellg();  
	textFile.seekg(0, std::ios::beg);  

	// ����1  
	std::string strConfigData((std::istreambuf_iterator<char>(textFile)), std::istreambuf_iterator<char>());  

	textFile.close();  

	string strConnStr = "";
	string strUserName = "";
	string strPassword = "";
	string strDbType = "";
	string strTimeout = "";
	string strCodingSet = "";
	while(strConfigData.length() > 0)
	{
		strConfigData = trim(strConfigData);
		if (strConfigData.empty())
			break;

		string strLine = "";
		int nPosLine = strConfigData.find('\n');
		if(nPosLine >= 0)
		{
			strLine = strConfigData.substr(0, nPosLine);
			strConfigData = strConfigData.substr(nPosLine + 1);
		}
		else
		{
			strLine = strConfigData;
			strConfigData = "";
		}

		int nPosSection = strLine.find('#');
		if (nPosSection == 0) // #�ſ�ͷ
		{
			continue;
		}

		// x=y
		nPosSection = strLine.find('=');
		if(nPosSection <= 0) // ��=�Ż�=�ſ�ͷ
		{
			// g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s, line: %s invalid, need =!", strConfigPath.c_str(), strLine.c_str());
			continue;
		}

		string strSecName = strLine.substr(0, nPosSection);
		string strSecValue = strLine.substr(nPosSection + 1);
		strSecName = trim(strSecName);
		strSecValue = trim(strSecValue);
		std::transform(strSecName.begin(), strSecName.end(), strSecName.begin(), ::tolower);
		if(strSecName.compare("dbtype") == 0)
			strDbType = strSecValue;
		else if(strSecName.compare("connection") == 0)
			strConnStr = strSecValue;
		else if(strSecName.compare("username") == 0)
			strUserName = strSecValue;
		else if(strSecName.compare("password") == 0)
			strPassword = strSecValue;
		else if(strSecName.compare("timeout") == 0)
			strTimeout = strSecValue;
		else if(strSecName.compare("codingset") == 0)
			strCodingSet = strSecValue;
		else
		{
			//g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s, key:%s unknown!", strConfigPath.c_str(), strSecName.c_str());
			continue;
		}
	}

	int nDatabaseTypeId = ConvertDBTypeName2Id(strDbType.c_str());
	if(nDatabaseTypeId == -1)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "db config file:%s, ��֧�ֵ����ݿ�����:%s!", strConfigPath.c_str(), strDbType.c_str());
		return -1;
	}

	int nTimeOut = ::atoi(strTimeout.c_str());
	if(nTimeOut <= 0)
		nTimeOut = 3;

	int nRet = -1;
	// MYSQL ��Ȼ��UTF8������������ΪGB2312���ܶ�ȡ����
	//if(strCodingSet.empty())
	//	strCodingSet = "utf8";

	if(nDatabaseTypeId == SA_SQLite_Client)
		nRet = InitLocalSQLite((char *)strCodingSet.c_str());
		//nRet = SQLConnect(m_lConnectionId, strConnStr.c_str(), strUserName.c_str(), strPassword.c_str(), nDatabaseTypeId, nTimeOut, "");
	else // �������ݿ�
	{
		nRet = SQLConnect(strConnStr.c_str(), strUserName.c_str(), strPassword.c_str(), nDatabaseTypeId, nTimeOut, strCodingSet.c_str());
		if(nRet == 0)
			g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "�������ݿ�(%s,���Ӵ�:%s)�ɹ�", strDbType.c_str(), strConnStr.c_str());
		else
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�������ݿ�(%s,���Ӵ�:%s)ʧ��, ����2:%d", strDbType.c_str(), strConnStr.c_str(), nRet);
	}
	return nRet;
}

int CPKEviewDbApi::Exit()
{
	SQLDisconnect();

	UnInitialize();
	return 0;
}
