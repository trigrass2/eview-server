/**************************************************************
 *  Filename:    CPKDbApiImpl.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: CPKDbApiImpl扩展类的实现文件.
 *
 *  @author:     zhangqiang
 *  @version     10/26/2012  zhangqiang  Initial Version
**************************************************************/
#include "PKDbApiImpl.h"
#include <string>
#include <string.h>
#include <algorithm>
#include <stdio.h>
//#include "CodingConv.h"
#include "pkcomm/eviewdefine.h"
#include "common_sqlapiwrapper.h"
#include "pklog/pklog.h"
CPKLog g_defaultPKLog;
CPKLog *g_pPKLog = NULL;
#define _(x)	x
using namespace std;

const char *GetDatabaseTypeName(int nDatabaseTypeId)
{
	static char szDBTypeName[32] = {0};
	string strDatabaseType;
	switch(nDatabaseTypeId)
	{
	case SA_Oracle_Client:
		strDatabaseType = "oracle";
		break;
	case SA_ODBC_Client:
		strDatabaseType = "odbc";
		break;
	case SA_SQLServer_Client:
		strDatabaseType = "sqlserver";
		break;
	case SA_InterBase_Client:
		strDatabaseType = "interbase";
		break;
	case SA_DB2_Client:
		strDatabaseType = "db2";
		break;
	case SA_Informix_Client:
		strDatabaseType = "informix";
		break;
	case SA_Sybase_Client:
		strDatabaseType = "sybase";
		break;
	case SA_MySQL_Client:
		strDatabaseType = "mysql";
		break;
	case SA_PostgreSQL_Client:
		strDatabaseType = "postgresql";
		break;
	case SA_SQLite_Client:
		strDatabaseType = "sqlite";
		break;
	default:
		strDatabaseType = "unknown!";
		break;
	}

	strncpy(szDBTypeName, strDatabaseType.c_str(), sizeof(szDBTypeName) - 1);
	return szDBTypeName;
}


CPKDbApiImpl::CPKDbApiImpl(void *pPkLog)
{
	m_generateID = 0;
	m_nDatabaseTypeId = 0;
	m_pConnection = NULL;

    if (pPkLog != NULL)
      g_pPKLog = (CPKLog*)pPkLog;
    else
    {
      g_pPKLog = &g_defaultPKLog;
      g_pPKLog->SetLogFileName("pkdbapi");
    }
    
	//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Dbproxy CPKDbApiImpl.lib initializing succeed."));
}

CPKDbApiImpl::~CPKDbApiImpl()
{
	if (m_pConnection != NULL)
	{
		m_pConnection->Disconnect();
		m_pConnection->Destroy();
		delete m_pConnection;
		m_pConnection = NULL;
	}

	for (map<long, SACommandEx*>::iterator iter = m_mapConnectionQuery.begin(); iter != m_mapConnectionQuery.end(); iter++)
	{
		if (iter->second != NULL)
		{
			delete iter->second;
			iter->second = NULL;
		}
	}

	m_mapConnectionQuery.clear();
	// g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Dbproxy CPKDbApiImpl.lib uninitializing succeed.")); 可能引起异常，因为PKLog有可能先析构
}

/**
*	测试连接数据库
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::IsConnected(const char *szTestTableName)
{
	std::string strError;
	if (m_pConnection == NULL)
	{
		g_pPKLog->LogMessage(PK_LOGLEVEL_DEBUG, _("IsConnected:pConnection == NULL."));
		return -100;
	}

	if (!m_pConnection->isConnected())
		return -1;

	string strTestTable = "";
	if (szTestTableName != NULL)
		strTestTable = szTestTableName;
	else
	{
		if (this->m_nDatabaseTypeId == SA_Oracle_Client)
			strTestTable = "DUAL";
		else
			return -2;
	}
	string strSQL = "select count(*) from " + strTestTable;
	long lExecuteResult = SQLExecute(strSQL.c_str(), NULL);
	return lExecuteResult;
}

/**
*	连接数据库
*
*	@param[in][out] conncetionID         连接ID,
*	@param[in] conncetionString        连接字符串
*	@param[in] userName         用户名
*	@param[in] password         密码
*	@param[in] databasetype         数据库类型
*	@param[in] lTimeOut         连接不上的超时时间
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLConnectEx(const char* connectionString, const char* userName, const char* password, long databaseType, long lTimeOut, const char* szCodingSet)
{
	this->m_strConnectionString = connectionString;
	this->m_strUserName = userName;
	this->m_strPassword = password;
	this->m_strCodingSet = szCodingSet;
	this->m_lConnTimeout = lTimeOut;	 // 连接超时
	this->m_nDatabaseTypeId = databaseType;

	SQLDisconnect();

	try
	{
		m_pConnection = new SAConnection();
		//针对有超时参数的数据库设置超时参数
		switch(databaseType)
		{
			case SA_Oracle_Client:
				if(strlen(szCodingSet) > 0)
					m_pConnection->setOption("NLS_CHAR") = szCodingSet;
				break;
			case SA_SQLServer_Client:
				m_pConnection->setOption("DBPROP_INIT_TIMEOUT") = lTimeOut;
				break;
			case SA_Sybase_Client:
				m_pConnection->setOption("CS_LOGIN_TIMEOUT") = lTimeOut;
				if(strlen(szCodingSet) > 0)
					m_pConnection->setOption("CS_SYB_CHARSET") = szCodingSet;
				break;
			case SA_MySQL_Client:
				m_pConnection->setOption("MYSQL_OPT_CONNECT_TIMEOUT") = lTimeOut;
				if(strlen(szCodingSet) > 0)
					m_pConnection->setOption("CharacterSet") = szCodingSet;
				break;
			case SA_SQLite_Client:
				m_pConnection->setOption("BusyTimeout") = lTimeOut * 1000;
				break;
			case SA_InterBase_Client:
				if(strlen(szCodingSet) > 0)
					m_pConnection->setOption("isc_dpb_lc_ctype") = szCodingSet;
				break;
			case SA_PostgreSQL_Client:
				if(strlen(szCodingSet) > 0)
					m_pConnection->setOption("ClientEncoding") = szCodingSet;
				break;
			default:
				break;
		}
		//pConnection->setOption("SQL_ATTR_CONCURRENCY") = "SQL_CONCUR_LOCK";
		//pConnection->setOption("SQL_ATTR_CURSOR_TYPE") = "SQL_CURSOR_KEYSET_DRIVEN";
		m_pConnection->setOption("SQL_ATTR_CURSOR_SCROLLABLE") = "SQL_SCROLLABLE";
		m_nDatabaseTypeId = databaseType;
		m_pConnection->Connect(connectionString, userName, password, (SAClient_t)databaseType);
		if (m_pConnection->isConnected())
		{
			m_pConnection->setAutoCommit(SA_AutoCommitOn);
			//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Succeed to connect to DB, connection ID is %d"), connectionID);//连接数据库成功，连接ID为%d
			g_pPKLog->LogMessage(PK_LOGLEVEL_NOTICE, "Succeed to connect to DB, connection string is %s", connectionString);//连接数据库成功，连接ID为%d)
			return 0;
		}
		else
		{
			g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, "连接数据库失败,数据库类型:%s, 连接串:%s, 用户名:%s", GetDatabaseTypeName(databaseType), connectionString, userName);
			delete m_pConnection;
			m_pConnection = NULL;
			return EC_ICV_SQLAPIWRAPPER_CONNECTDBFAILED;
		}
	}
	catch(SAException &x)
	{
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, "连接数据库异常,数据库类型:%s, 连接串:%s, 用户名:%s,Error Message:%s", GetDatabaseTypeName(databaseType), connectionString, userName, x.ErrText().GetBuffer(0));
		delete m_pConnection;
		m_pConnection = NULL;
		//g_pPKLog->LogErrMessage(EC_ICV_SQLAPIWRAPPER_CONNECTDBFAILED, _("Failed to connect to DB, error message: %s"), x.ErrText().GetBuffer(0));
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, "Failed to connect to DB, error message:%s", x.ErrText().GetBuffer(0));
		return EC_ICV_SQLAPIWRAPPER_CONNECTDBFAILED;
	}
}
/**
*	断开数据库
*
*	@param[in] conncetionID         连接ID,
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLDisconnect()
{
	if (m_pConnection == NULL)
		return 0;

	try
	{

		for (map<long, SACommandEx*>::iterator iter = m_mapConnectionQuery.begin(); iter != m_mapConnectionQuery.end(); iter++)
		{
			if (iter->second != NULL)
			{
				delete iter->second;
				iter->second = NULL;
			}
		}
		m_mapConnectionQuery.clear();

		m_pConnection->Disconnect();
		m_pConnection->Destroy();

        delete m_pConnection;
        g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, "PKDbApiImpl SQLDisconnect done");
	}
	catch(SAException &x)
	{
		//g_pPKLog->LogErrMessage(EC_ICV_SQLAPIWRAPPER_DISCONNECTDBFAILED, _("Failed to disconnect DB, error message is %s"), x.ErrText().GetBuffer(0));
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR,"Failed to disconnect DB, error message is %s", x.ErrText().GetBuffer(0));
		delete m_pConnection;
		m_pConnection = NULL;
		return EC_ICV_SQLAPIWRAPPER_DISCONNECTDBFAILED;
	}

	m_pConnection = NULL;
	return 0;
}

/**
*	查询
*
*	@param[in] conncetionID         连接ID,
*	@param[in][out] queryID        查询ID
*	@param[in] SQLExpression       sql语句
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
//long CPKDbApiImpl::SQLSelectBySQL(long& queryID, const char* SQLExpression, vector<string>& vecFieldValue, vector<int>& fieldTypeVector)
//{
//	if (m_pConnection == NULL)
//		return -1;
//
//	vector< vector<string> > rows;
//	long lRet = SQLExecute(queryID, SQLExpression, rows, vecFieldValue, fieldTypeVector);
//	if(lRet == PK_SUCCESS)
//	{
//		m_queryMap.insert(std::make_pair(queryID, rows));
//		m_cursorMap.insert(std::make_pair(queryID, -1l));
//		m_columnMap.insert(std::make_pair(queryID, vecFieldValue));
//	}
//	return lRet;
//}


/**
*	开始事务
*
*	@param[in] conncetionID         连接ID
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLTransact()
{
	if (m_pConnection == NULL)
	{
        //g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("Connection ID does not exist."));//连接ID不存在
		return EC_ICV_SQLAPIWRAPPER_CONNECTIDNOTEXIST;
	}
	try
	{
		m_pConnection->setAutoCommit(SA_AutoCommitOff);
		//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Succeed to begin transact."));
	}
	catch(SAException &x)
	{
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("Failed to begin transact, details is %s."), x.ErrText().GetBuffer(0));
		return EC_ICV_SQLAPIWRAPPER_TRANSACTFAILED;
	}
	return 0;
}
/**
/**
*	提交事务
*
*	@param[in] conncetionID         连接ID
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLCommit()
{
	if (m_pConnection == NULL)
	{
		//g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("ConnectID is not existed."));
		return EC_ICV_SQLAPIWRAPPER_CONNECTIDNOTEXIST;
	}
	try
	{
		m_pConnection->Commit();
		m_pConnection->setAutoCommit(SA_AutoCommitOn);
		//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Succeed to commit transact."));
	}
	catch(SAException &x)
	{
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("Failed to commit transact, details is %s."), x.ErrText().GetBuffer(0));
		return EC_ICV_SQLAPIWRAPPER_COMMITFAILED;
	}
	return 0;
}
/**
*	回滚事务
*
*	@param[in] conncetionID         连接ID
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLRollback()
{
	if (m_pConnection == NULL)
	{
		//g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("ConnectID is not existed."));
		return EC_ICV_SQLAPIWRAPPER_CONNECTIDNOTEXIST;
	}
	try
	{
		m_pConnection->Rollback();
		m_pConnection->setAutoCommit(SA_AutoCommitOn);
		//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Succeed to rollback transact."));
	}
	catch(SAException &x)
	{
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("Failed to roolback transact, details is %s."), x.ErrText().GetBuffer(0));
		return EC_ICV_SQLAPIWRAPPER_ROLLBACKFAILED;
	}
	return 0;
}


/**
 *  根据连接ID获取对应的<queryID,CommandEx>.
 *
 *  @return 
 *  连接总数
 *  @version     10/26/2012  zhangqiang  Initial Version.
 *
 */
long CPKDbApiImpl::GetNewID()
{
	//EnterCriticalSection(&cs);
	m_generateID++;
	long ID = m_generateID;
	//LeaveCriticalSection(&cs);
	return ID;
}

/**
 *  根据连接ID获取对应的连接.
 *
 *  @param  -[in]  connectionID: [comment]
 *  @return 
 *  对应的连接
 *  @version     10/26/2012  zhangqiang  Initial Version.
 *
 */
SAConnection* CPKDbApiImpl::GetConnection()
{
	if (m_pConnection != NULL)
	{
		return m_pConnection;
	}
	else
	{
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, "数据库连接为空");
		return NULL;
	}
}

/**
 *  根据查询ID获取对应的CommandEx.
 *
 *  @param  -[in]  connectionID: [comment]
 *  @param  -[in]  queryID:   [comment]
 *  @return 
 *  对应的CommandEx
 *  @version     10/26/2012  zhangqiang  Initial Version.
 *
 */
SACommandEx* CPKDbApiImpl::GetCommand(long queryID)
{
	map<long, SACommandEx*>::iterator iter;
	iter = m_mapConnectionQuery.find(queryID);
	if (iter != m_mapConnectionQuery.end())
	{
		return iter->second;
	}
	else
	{
		return NULL;
	}
}

/**
 *  获取对应记录的值.
 *
 *  @param  -[in]  SACommandEx* pCmd: [comment]
 *  @param  -[in][out]  fieldValue:   [comment]
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     10/26/2012  zhangqiang  Initial Version.
 *
 */
long CPKDbApiImpl::GetFieldValue(SACommandEx* pCmd, map<string, string>* fieldValue)
{
	if (fieldValue == NULL)
	{
		return -1;
	}
	
	for(map<string, string>::iterator it=fieldValue->begin(); it!=fieldValue->end(); it++)
    {
		string fieldName = it->first;
		if (pCmd != NULL)
		{
			it->second = pCmd->Field(fieldName.c_str()).asString();
		}
	}
	return 0;
}


/**
*	执行sql语句
*
*	@param[in] conncetionID         连接ID,
*	@param[in][out] SQLExpression         sql语句
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLExecute(const char* SQLExpression, string *pstrErr)
{
	if (SQLExpression == NULL || strlen(SQLExpression) == 0)
	{
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("SQLExpression == NULL"));
		//printf("SQLExpression == NULL\n");
		return EC_ICV_SQLAPIWRAPPER_CONNECTIDNOTEXIST;
	}

	if (m_pConnection == NULL)
	{
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("pConnection == NULL."));
		//printf("pConnection == NULL\n");
		return EC_ICV_SQLAPIWRAPPER_CONNECTIDNOTEXIST;
	}

	try
	{
		string strSQL = SQLExpression;
		if (m_nDatabaseTypeId != SA_Oracle_Client) // Oracle 不需要转码，但MySQL和Sqlite都需要
			strSQL = CCodingConv::GB2UTF(strSQL);

		SACommandEx cmd;
		cmd.setConnection(m_pConnection);
		cmd.setCommandText(strSQL.c_str());
		cmd.Execute();
		//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Succeed to execute sql statement."));
	}
	catch (SAException &x)
	{
      if (pstrErr)
        *pstrErr = x.ErrText().GetBuffer(0);

		//g_pPKLog->LogErrMessage(EC_ICV_SQLAPIWRAPPER_EXECSQLFAILED, _("Failed to execute sql statement, details is %s."), x.ErrText().GetBuffer(0));
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, "Failed to execute sql:[%s] statement, details is %s", SQLExpression, x.ErrText().GetBuffer(0));
		return EC_ICV_SQLAPIWRAPPER_EXECSQLFAILED;
	}
	return 0;
}


long CPKDbApiImpl::SQLQuery(long& queryID, const char* SQLExpression)
{
	if (m_pConnection == NULL)
	{
		//g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("connectID is not existed."));
		return EC_ICV_SQLAPIWRAPPER_CONNECTIDNOTEXIST;
	}

	vector< vector<string> > rows;
	vector<int> vecFieldType;
	vector<string> vecFieldValue;

	SACommandEx* pCmd = NULL;
	try
	{
		pCmd = new SACommandEx;
		pCmd->setConnection(m_pConnection);
		pCmd->setOption("UseCursor") = "0";
		pCmd->setOption("UseDynamicCursor") = "0";

		if (m_pConnection->Client() != SA_SQLServer_Client)
		{
			pCmd->setOption("UseCursor") = "1";
			pCmd->setOption("UseDynamicCursor") = "1";
		}

		if (m_pConnection->Client() == SA_ODBC_Client)
		{
			pCmd->setOption("SQL_ATTR_CURSOR_TYPE") = "SQL_CURSOR_STATIC";
			pCmd->setOption("SQL_ATTR_CURSOR_SCROLLABLE") = "SQL_SCROLLABLE";
		}

		string strSQLTmp = SQLExpression;
		string strSQL = CCodingConv::GB2UTF(strSQLTmp);

		pCmd->setCommandText(strSQL.c_str());
		pCmd->Execute();
		queryID = GetNewID();
		m_mapConnectionQuery.insert(map<long, SACommandEx*>::value_type(queryID, pCmd));

		//	bool bGetData = pCmd->FetchFirst();
		//	if(bGetData)
		int nFiledCount = pCmd->FieldCount();
		string strFieldValue("");
		//for (int i = 0; i < lResultSetRows; i ++)
		long lResultSetRows = 0;
		while (pCmd->FetchNext())
		{
			//pCmd->FetchNext();
			vector<string> oneRecord;
			for (int j = 1; j <= nFiledCount; j++)
			{
				strFieldValue = pCmd->Field(j).asString();
				string strTmp = CCodingConv::UTF2GB(strFieldValue);
				oneRecord.push_back(strTmp);
			}
			++lResultSetRows;
			rows.push_back(oneRecord);
		}
		pCmd->SetResultSetLength(lResultSetRows);

		//没有绑定表的情况
		vecFieldValue.clear();
		string strFieldName("");
		for (int i = 1; i <= nFiledCount; i++)
		{
			strFieldName = pCmd->Field(i).Name();
			string strTmp = CCodingConv::UTF2GB(strFieldName);
			vecFieldValue.push_back(strTmp);
		}

		m_queryMap.insert(std::make_pair(queryID, rows));
		m_cursorMap.insert(std::make_pair(queryID, -1l));
		m_columnMap.insert(std::make_pair(queryID, vecFieldValue));

		//pCmd->Cancel();
		delete pCmd;
		pCmd = NULL;

		m_mapConnectionQuery.erase(queryID);
	}
	catch (SAException &x)
	{
		delete pCmd;
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, "Failed to execute sql:[%s] statement, details is %s,retcode:%d", SQLExpression, x.ErrText().GetBuffer(0), EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED);

		//g_pPKLog->LogErrMessage(EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED, _("Failed to query, details is %s,%s"), SQLExpression,x.ErrText().GetBuffer(0));//查询数据失败，详细信息为%s,%s
		return EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED;
	}

	//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Succeed to query, connectID is %d，queryID is %d"), connectionID, queryID);//查询数据成功，连接ID为%d，查询ID为%d
	return 0;
}


long CPKDbApiImpl::SQLExecute(const char* SQLExpression,
  vector< vector<string> >& rows, string *pstrErr)
{
	vector<string> vecFieldValue;
	vector<int> fieldTypeVector;
    return SQLExecute(SQLExpression, rows, vecFieldValue, fieldTypeVector, pstrErr);
}

long CPKDbApiImpl::SQLExecute(const char* SQLExpression, list< list<string> >& rows, string *pstrErr)
{
	list<string> vecFieldValue;
	list<int> fieldTypeVector;
    return SQLExecute(SQLExpression, rows, vecFieldValue, fieldTypeVector, pstrErr);
}


long CPKDbApiImpl::SQLExecute(const char* SQLExpression, 
	vector< vector<string> >& rows, vector<string>& vecFieldValue, vector<int>& fieldTypeVector, string *pstrErr)
{
	if (m_pConnection == NULL)
	{
		//g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("connectID is not existed."));
		return EC_ICV_SQLAPIWRAPPER_CONNECTIDNOTEXIST;
	}

	long queryID = -1;
	SACommandEx* pCmd = NULL;
	try
	{
		pCmd = new SACommandEx;
		pCmd->setConnection(m_pConnection);
		pCmd->setOption("UseCursor") = "0";
		pCmd->setOption("UseDynamicCursor") = "0";

		if (m_pConnection->Client() != SA_SQLServer_Client)
		{
			pCmd->setOption("UseCursor") = "1";
			pCmd->setOption("UseDynamicCursor") = "1";
		}

		if (m_pConnection->Client() == SA_ODBC_Client)
		{
			pCmd->setOption("SQL_ATTR_CURSOR_TYPE") = "SQL_CURSOR_STATIC";
			pCmd->setOption("SQL_ATTR_CURSOR_SCROLLABLE") = "SQL_SCROLLABLE";
		}

		string strSQLTmp = SQLExpression;
		string strSQL = CCodingConv::GB2UTF(strSQLTmp);

		pCmd->setCommandText(strSQL.c_str());
		pCmd->Execute();
		queryID = GetNewID();
		m_mapConnectionQuery.insert(map<long, SACommandEx*>::value_type(queryID, pCmd));

		//	bool bGetData = pCmd->FetchFirst();
		//	if(bGetData)
		int nFiledCount = pCmd->FieldCount();
		string strFieldValue("");
		//for (int i = 0; i < lResultSetRows; i ++)
		long lResultSetRows = 0;
		while(pCmd->FetchNext())
		{
			//pCmd->FetchNext();
			vector<string> oneRecord;
			for(int j = 1; j <= nFiledCount; j++)
			{
				strFieldValue = pCmd->Field(j).asString();
				string strTmp = CCodingConv::UTF2GB(strFieldValue);
				oneRecord.push_back(strTmp);
			}
			++ lResultSetRows;
			rows.push_back(oneRecord);
		}
		pCmd->SetResultSetLength(lResultSetRows);
		//没有绑定表的情况
		vecFieldValue.clear();
		string strFieldName("");
		for(int i = 1; i <= nFiledCount; i++)
		{
			strFieldName = pCmd->Field(i).Name();
			string strTmp = CCodingConv::UTF2GB(strFieldName);
			vecFieldValue.push_back(strTmp);
			int nFieldType = pCmd->Field(i).FieldType();
			fieldTypeVector.push_back(nFieldType);
		}
		
		//pCmd->Cancel();
		delete pCmd;
		pCmd = NULL;

		m_mapConnectionQuery.erase(queryID);
	}
	catch(SAException &x)
	{
      if (pstrErr)
        *pstrErr = x.ErrText().GetBuffer(0);

		delete pCmd;
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, "Failed to execute sql:[%s] statement, details is %s,retcode:%d", SQLExpression, x.ErrText().GetBuffer(0), EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED);

		//g_pPKLog->LogErrMessage(EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED, _("Failed to query, details is %s,%s"), SQLExpression,x.ErrText().GetBuffer(0));//查询数据失败，详细信息为%s,%s
		return EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED;
	}

	//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Succeed to query, connectID is %d，queryID is %d"), connectionID, queryID);//查询数据成功，连接ID为%d，查询ID为%d
	return 0;
}


long CPKDbApiImpl::SQLExecute(const char* SQLExpression, list< list<string> >& rows, list<string>& vecFieldValue, list<int>& fieldTypeVector, string *pstrErr)
{
	if (m_pConnection == NULL)
	{
		//g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, _("connectID is not existed."));
		return EC_ICV_SQLAPIWRAPPER_CONNECTIDNOTEXIST;
	}

	long queryID = -1;
	SACommandEx* pCmd = NULL;
	try
	{
		pCmd = new SACommandEx;
		pCmd->setConnection(m_pConnection);
		pCmd->setOption("UseCursor") = "0";
		pCmd->setOption("UseDynamicCursor") = "0";

		if (m_pConnection->Client() != SA_SQLServer_Client)
		{
			pCmd->setOption("UseCursor") = "1";
			pCmd->setOption("UseDynamicCursor") = "1";
		}

		if (m_pConnection->Client() == SA_ODBC_Client)
		{
			pCmd->setOption("SQL_ATTR_CURSOR_TYPE") = "SQL_CURSOR_STATIC";
			pCmd->setOption("SQL_ATTR_CURSOR_SCROLLABLE") = "SQL_SCROLLABLE";
		}

		string strSQLTmp = SQLExpression;
		string strSQL = CCodingConv::GB2UTF(strSQLTmp);

		pCmd->setCommandText(strSQL.c_str());
		pCmd->Execute();
		queryID = GetNewID();
		m_mapConnectionQuery.insert(map<long, SACommandEx*>::value_type(queryID, pCmd));

		//	bool bGetData = pCmd->FetchFirst();
		//	if(bGetData)
		int nFiledCount = pCmd->FieldCount();
		string strFieldValue("");
		//for (int i = 0; i < lResultSetRows; i ++)
		long lResultSetRows = 0;
		while (pCmd->FetchNext())
		{
			//pCmd->FetchNext();
			list<string> oneRecord;
			for (int j = 1; j <= nFiledCount; j++)
			{
				strFieldValue = pCmd->Field(j).asString();
				string strTmp = CCodingConv::UTF2GB(strFieldValue);
				oneRecord.push_back(strTmp);
			}
			++lResultSetRows;
			rows.push_back(oneRecord);
		}
		pCmd->SetResultSetLength(lResultSetRows);
		//没有绑定表的情况
		vecFieldValue.clear();
		string strFieldName("");
		for (int i = 1; i <= nFiledCount; i++)
		{
			strFieldName = pCmd->Field(i).Name();
			string strTmp = CCodingConv::UTF2GB(strFieldName);
			vecFieldValue.push_back(strTmp);
			int nFieldType = pCmd->Field(i).FieldType();
			fieldTypeVector.push_back(nFieldType);
		}

		//pCmd->Cancel();
		delete pCmd;
		pCmd = NULL;

		m_mapConnectionQuery.erase(queryID);
	}
	catch (SAException &x)
	{
      if (pstrErr)
        *pstrErr = x.ErrText().GetBuffer(0);

		delete pCmd;
		g_pPKLog->LogMessage(PK_LOGLEVEL_ERROR, "Failed to execute sql:[%s] statement, details is %s,retcode:%d", SQLExpression, x.ErrText().GetBuffer(0), EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED);

		//g_pPKLog->LogErrMessage(EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED, _("Failed to query, details is %s,%s"), SQLExpression,x.ErrText().GetBuffer(0));//查询数据失败，详细信息为%s,%s
		return EC_ICV_SQLAPIWRAPPER_QUERYDATAFAILED;
	}

	//g_pPKLog->LogMessage(PK_LOGLEVEL_INFO, _("Succeed to query, connectID is %d，queryID is %d"), connectionID, queryID);//查询数据成功，连接ID为%d，查询ID为%d
	return 0;
}

/**
 *  释放所用到的资源.
 *
 *  @param  -[in]  SACommandEx* pCmd: [comment]
 *  @param  -[in][out]  fieldValue:   [comment]
 *  @return 
 *	- ==0 success
 *	- !=0 failure
 *
 *  @version     10/26/2012  zhangqiang  Initial Version.
 *
 */
void CPKDbApiImpl::UnInitialize()
{
	if (m_pConnection != NULL)
	{
		m_pConnection->Disconnect();
		m_pConnection->Destroy();
	}

	for (map<long, SACommandEx*>::iterator iter = m_mapConnectionQuery.begin(); iter != m_mapConnectionQuery.end(); iter++)
	{
		if (iter->second != NULL)
		{
			delete iter->second;
			iter->second = NULL;
		}
	}

	m_mapConnectionQuery.clear();
}

int CPKDbApiImpl::GetDatabaseTypeId( const char *szDbType )
{
	int nDbType = SA_SQLite_Client;
	string strType = szDbType;
	std::transform(strType.begin(), strType.end(), strType.begin(), ::tolower);
	if(strType.compare("oracle") == 0)
	{
		nDbType = SA_Oracle_Client; 
	}
	else if(strType.compare("odbc") == 0)
	{
		nDbType = SA_ODBC_Client;
	}
	else if(strType.compare("sqlserver") == 0)
	{
		nDbType = SA_SQLServer_Client;
	}
	else if(strType.compare("interbase") == 0)
	{
		nDbType = SA_InterBase_Client;
	}
	else if(strType.compare("db2") == 0)
	{
		nDbType = SA_DB2_Client;
	}
	else if(strType.compare("informix") == 0)
	{
		nDbType = SA_Informix_Client;
	}
	else if(strType.compare("sybase") == 0)
	{
		nDbType = SA_Sybase_Client;
	}
	else if(strType.compare("mysql") == 0)
	{
		nDbType = SA_MySQL_Client;
	}
	else if(strType.compare("postgresql") == 0)
	{
		nDbType = SA_PostgreSQL_Client;
	}
	else if(strType.compare("sqlite") == 0)
	{
		nDbType = SA_SQLite_Client;
	}
	else
		return -1;
	return nDbType;
}

/**
*	得到对应行的记录
*
*	@param[in] conncetionID         连接ID,
*	@param[in] qureyID        查询ID
*	@param[in] recordNumber         记录的行号
*	@param[in][out] fieldValue         表中的字段和对应的值
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLGetRecord(long queryID, long recordNumber, map<string, string>* fieldValue)
{
	if (fieldValue == NULL)
	{
		return -1;
	}
	//根据queryID查询对应的结果集
	map<long, vector< vector<string> > >::iterator iterQuery = m_queryMap.find(queryID);
	if ((*iterQuery).second.size() == 0)
		return EC_ICV_DBProxy_GETRECORDERROR;
	if (iterQuery != m_queryMap.end())
	{
		map<long, long>::iterator iterCursor = m_cursorMap.find(queryID);
		if (recordNumber > (*iterQuery).second.size() || recordNumber <= 0)
			return EC_ICV_DBProxy_GETRECORDERROR;
		if (iterCursor != m_cursorMap.end())
		{
			vector<string> oneRecord = ((*iterQuery).second)[recordNumber - 1];
			(*iterCursor).second = recordNumber - 1;
			int i = 0;
			for (map<string, string>::iterator iterInner = fieldValue->begin(); iterInner != fieldValue->end(); iterInner++)
			{
				(*iterInner).second = oneRecord[i];
				++i;
			}
		}
		else
		{
			return EC_ICV_DBProxy_GETRECORDERROR;
		}
	}
	else
	{
		return EC_ICV_DBProxy_GETRECORDERROR;
	}
	return PK_SUCCESS;
}
/**
*	计算记录的总数
*
*	@param[in][out] conncetionID         连接ID,
*	@param[in] queryID        查询ID
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLNumRows(long queryID, long& numRows)
{
	map<long, vector< vector<string> > >::iterator iter = m_queryMap.find(queryID);
	if (iter != m_queryMap.end())
	{
		numRows = (*iter).second.size();
	}

	return PK_SUCCESS;
}
/**
*	得到第一行的记录
*
*	@param[in] conncetionID         连接ID,
*	@param[in] qureyID        查询ID
*	@param[in][out] fieldValue         表中的字段和对应的值
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLFirst(long queryID, map<string, string>* fieldValue)
{
	if (fieldValue == NULL)
	{
		return -1;
	}
	//根据queryID查询对应的结果集
	map<long, vector< vector<string> > >::iterator iterQuery = m_queryMap.find(queryID);
	if ((*iterQuery).second.size() == 0)
		return EC_ICV_DBProxy_FIRSTERROR;
	if (iterQuery != m_queryMap.end())
	{
		map<long, long>::iterator iterCursor = m_cursorMap.find(queryID);
		if (iterCursor != m_cursorMap.end())
		{
			(*iterCursor).second = 0;
			vector<string> oneRecord = ((*iterQuery).second)[0];
			int i = 0;
			for (map<string, string>::iterator iterInner = fieldValue->begin(); iterInner != fieldValue->end(); iterInner++)
			{
				(*iterInner).second = oneRecord[i];
				++i;
			}
		}
		else
		{
			return EC_ICV_DBProxy_FIRSTERROR;
		}
	}
	else
	{
		return EC_ICV_DBProxy_FIRSTERROR;
	}
	return PK_SUCCESS;
}
/**
*	得到下一行的记录
*
*	@param[in] conncetionID         连接ID,
*	@param[in] qureyID        查询ID
*	@param[in][out] fieldValue         表中的字段和对应的值
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLNext(long queryID, map<string, string>* fieldValue)
{
	if (fieldValue == NULL)
	{
		return -1;
	}
	//根据queryID查询对应的结果集
	map<long, vector< vector<string> > >::iterator iterQuery = m_queryMap.find(queryID);
	if ((*iterQuery).second.size() == 0)
		return EC_ICV_DBProxy_NEXTERROR;
	if (iterQuery != m_queryMap.end())
	{
		map<long, long>::iterator iterCursor = m_cursorMap.find(queryID);
		if (iterCursor != m_cursorMap.end())
		{
			vector<string> oneRecord;
			if ((*iterCursor).second >= long((*iterQuery).second.size()) - 1)
			{
				oneRecord = ((*iterQuery).second)[((*iterQuery).second).size() - 1];
			}
			else
				oneRecord = ((*iterQuery).second)[++(*iterCursor).second];
			int i = 0;
			for (map<string, string>::iterator iterInner = fieldValue->begin(); iterInner != fieldValue->end(); iterInner++)
			{
				(*iterInner).second = oneRecord[i];
				++i;
			}
		}
		else
		{
			return EC_ICV_DBProxy_NEXTERROR;
		}
	}
	else
	{
		return EC_ICV_DBProxy_NEXTERROR;
	}
	return PK_SUCCESS;
}
/**
*	得到前一行的记录
*
*	@param[in] conncetionID         连接ID,
*	@param[in] qureyID        查询ID
*	@param[in][out] fieldValue         表中的字段和对应的值
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLPrev(long queryID, map<string, string>* fieldValue)
{
	if (fieldValue == NULL)
	{
		return -1;
	}
	//根据queryID查询对应的结果集
	map<long, vector< vector<string> > >::iterator iterQuery = m_queryMap.find(queryID);
	if ((*iterQuery).second.size() == 0)
		return EC_ICV_DBProxy_PREVERROR;
	if (iterQuery != m_queryMap.end())
	{
		map<long, long>::iterator iterCursor = m_cursorMap.find(queryID);
		if (iterCursor != m_cursorMap.end())
		{
			vector<string> oneRecord;
			if ((*iterCursor).second <= 0)
			{
				oneRecord = ((*iterQuery).second)[0];
			}
			else
				oneRecord = ((*iterQuery).second)[--(*iterCursor).second];
			int i = 0;
			for (map<string, string>::iterator iterInner = fieldValue->begin(); iterInner != fieldValue->end(); iterInner++)
			{
				(*iterInner).second = oneRecord[i];
				++i;
			}
		}
		else
		{
			return EC_ICV_DBProxy_PREVERROR;
		}
	}
	else
	{
		return EC_ICV_DBProxy_PREVERROR;
	}
	return PK_SUCCESS;
}
/**
*	得到最后一行的记录
*
*	@param[in] conncetionID         连接ID,
*	@param[in] qureyID        查询ID
*	@param[in][out] fieldValue         表中的字段和对应的值
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLLast(long queryID, map<string, string>* fieldValue)
{
	if (fieldValue == NULL)
	{
		return -1;
	}
	//根据queryID查询对应的结果集
	map<long, vector< vector<string> > >::iterator iterQuery = m_queryMap.find(queryID);
	if ((*iterQuery).second.size() == 0)
		return EC_ICV_DBProxy_LASTERROR;

	if (iterQuery != m_queryMap.end())
	{
		map<long, long>::iterator iterCursor = m_cursorMap.find(queryID);
		if (iterCursor != m_cursorMap.end())
		{
			(*iterCursor).second = ((*iterQuery).second).size() - 1;
			vector<string> oneRecord = ((*iterQuery).second)[((*iterQuery).second).size() - 1];
			int i = 0;
			for (map<string, string>::iterator iterInner = fieldValue->begin(); iterInner != fieldValue->end(); iterInner++)
			{
				(*iterInner).second = oneRecord[i];
				++i;
			}
		}
		else
		{
			return EC_ICV_DBProxy_LASTERROR;
		}
	}
	else
	{
		return EC_ICV_DBProxy_LASTERROR;
	}
	return PK_SUCCESS;
}
/**
*	结束此次查询
*
*	@param[in] conncetionID         连接ID,
*	@param[in] qureyID        查询ID
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApiImpl::SQLEnd(long queryID)
{
	//根据queryID查询对应的结果集
	map<long, vector< vector<string> > >::iterator iterQuery = m_queryMap.find(queryID);
	if (iterQuery != m_queryMap.end())
	{
		m_queryMap.erase(iterQuery);
		//
		map<long, long>::iterator iterCursor = m_cursorMap.find(queryID);
		if (iterCursor != m_cursorMap.end())
		{
			m_cursorMap.erase(iterCursor);
		}

	}
	else
	{
		return EC_ICV_DBProxy_ENDERROR;
	}

	return PK_SUCCESS;
}

/**
*	获取表中对应字段的值
*
*	@param[in] conncetionID         连接ID,
*	@param[in] qureyID        查询ID
*	@param[in][out] fieldName         表中的字段
*
*   @return  字段对应的值
*/
long CPKDbApiImpl::SQLGetValueByField(long queryID, const char* fieldName, string& fieldValue)
{
	//根据queryID查询对应的结果集
	map<long, vector< vector<string> > >::iterator iterQuery = m_queryMap.find(queryID);
	if ((*iterQuery).second.size() == 0)
		return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
	if (iterQuery != m_queryMap.end())
	{
		map<long, long>::iterator iterCursor = m_cursorMap.find(queryID);
		map<long, vector<string> >::iterator iterColumn = m_columnMap.find(queryID);
		if (iterCursor != m_cursorMap.end() && iterColumn != m_columnMap.end())
		{
			if (iterCursor->second <0 || iterCursor->second >= iterQuery->second.size())
			{
				return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
			}
			vector<string> oneRecord = ((*iterQuery).second)[(*iterCursor).second];
			int i = 0;
			for (; i < (*iterColumn).second.size(); ++i)
			{
				if ((*iterColumn).second[i].compare(fieldName) == 0)//如果相同
					break;
			}
			if (i < (*iterColumn).second.size())
				fieldValue = oneRecord[i];
			else
			{
				return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
			}
		}
		else
		{
			return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
		}
	}
	else
	{
		return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
	}
	return PK_SUCCESS;
}
/*
*	获取表中对应字段的值
*
*	@param[in] conncetionID         连接ID,
*	@param[in] qureyID        查询ID
*	@param[in][out] fieldName         表中的索引
*
*   @return  索引对应的值
*/
long CPKDbApiImpl::SQLGetValueByIndex(long queryID, long fieldIndex, string& fieldValue)
{
	//根据queryID查询对应的结果集
	map<long, vector< vector<string> > >::iterator iterQuery = m_queryMap.find(queryID);
	if ((*iterQuery).second.size() == 0)
		return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
	if (iterQuery != m_queryMap.end())
	{
		map<long, long>::iterator iterCursor = m_cursorMap.find(queryID);
		map<long, vector<string> >::iterator iterColumn = m_columnMap.find(queryID);
		if (iterCursor != m_cursorMap.end() && iterColumn != m_columnMap.end())
		{
			if (iterCursor->second <0 || iterCursor->second >= iterQuery->second.size())
			{
				return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
			}

			vector<string> oneRecord = ((*iterQuery).second)[(*iterCursor).second];

			if (fieldIndex <= (*iterColumn).second.size() && fieldIndex >= 1)
				fieldValue = oneRecord[fieldIndex - 1];
			else
			{
				return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
			}
		}
		else
		{
			return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
		}
	}
	else
	{
		return EC_ICV_DBProxy_GETVALUEBYFIELDERROR;
	}
	return PK_SUCCESS;
}
