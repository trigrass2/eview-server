
#include "pkcomm/pkcomm.h"
#include "eviewcomm/eviewcomm.h"
#include "common/eviewcomm_internal.h"
#include "pkdbapi/pkdbapi.h"
#include "pklog/pklog.h"
#include "stdio.h"
#include<stdlib.h>

extern CPKLog g_logger;
// 获取某个模块的版本号
int PKEviewComm::GetVersion(CPKDbApi &dbApi, const char *szModuleName)
{  
	//此处如果执行失败可能需要再次连接数据库;
	char szSql[128] = { 0 };
	sprintf(szSql, "select version from t_version where module='%s'", szModuleName);
	string strError;
	vector<vector<string> > rows;
 	vector<int > lis;
	int nRet = dbApi.SQLExecute(szSql, rows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "GetVersion(%s), execute query %s exception, check table exist, msg:%s, assume: version=0", szModuleName, szSql, strError.c_str());
		return 0;
	}
	if (rows.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "GetVersion(%s), execute query %s failed, check record exist, assume: version=0", szModuleName, szSql);
		return 0;
	}

	vector<string> &row = rows[0];
	string strVersion = row[0];
	for (int i = 0; i < rows.size(); i++)
		rows[i].clear();
	rows.clear();
	int nVersion = ::atoi(strVersion.c_str());
	if (nVersion < 0)
		nVersion = 0;

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "GetVersion(%s), version=%d", szModuleName, nVersion);

	return nVersion;
}

// 加载缺省的参数值
int PKEviewComm::LoadSysParam(CPKDbApi &dbApi, char *szParamName, char *szDefaultValue, string &strParamValue)
{
	strParamValue = szDefaultValue;
	vector<vector<string> > rows;
	vector<string> fields;
	vector<int > lis;
	long lQueryId = 0;
	//此处如果执行失败可能需要再次连接数据库;
	char szSql[2048] = { 0 };
	sprintf(szSql, "select value from t_sys_param where name='%s'", szParamName);
	string strError;
	int nRet = dbApi.SQLExecute(szSql, rows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "LoadSysParam(%s), execute query %s exception, check table exist, msg:%s, assume:[paraname:]%s=[paramalue:]%s", 
			szParamName, szSql, strError.c_str(), szParamName, szDefaultValue);
		return nRet;
	}
	if (rows.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "LoadSysParam(%s), execute query %s failed, check record exist, assume:[paraname:]%s=[paramalue:]%s", 
			szParamName, szSql, szParamName, szDefaultValue);
		return nRet;
	}

	vector<string> &row = rows[0];
	strParamValue = row[0];
	for (int i = 0; i < rows.size(); i ++)
		rows[i].clear();
	rows.clear();

	g_logger.LogMessage(PK_LOGLEVEL_INFO, "LoadSysParam(%s), get [paraname:%s]=[paramalue:]%s", szParamName, szParamName, szDefaultValue);

	return 0;
}
