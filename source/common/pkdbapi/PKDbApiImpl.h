/**************************************************************
 *  Filename:    CPKDbApiImpl.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: CPKDbApiImpl扩展类的头文件.
 *
 *  @author:     zhangqiang
 *  @version     10/26/2012  zhangqiang  Initial Version
**************************************************************/
#pragma once
#include "SACommandEx.h"
#include <vector>
#include <map>
#include <string>
using namespace std;

class CPKDbApiImpl
{
public:
  CPKDbApiImpl(void *pPkLog = NULL);
	~CPKDbApiImpl();
	//连接与断开数据库
	
public:
	int	GetDatabaseTypeId(const char *szDataBaseName);
	virtual long SQLConnectEx(const char* connectionString, const char* userName, const char* password, long databaseType, long lTimeOut, const char* szCodingSet);
	virtual long SQLDisconnect();

public:
	//使用事务集
	virtual long SQLTransact();
	virtual long SQLCommit();
	virtual long SQLRollback();
	virtual void UnInitialize();
	long IsConnected(const char *szTestTableName = NULL);
public:
	// 游标方式查询
	virtual long SQLQuery(long& queryID, const char* SQLExpression);
	virtual long SQLGetRecord(long queryID, long recordNumber, std::map<string, string>* fieldValue);
	virtual long SQLNumRows(long queryID, long& numRows);
	virtual long SQLFirst(long queryID, std::map<string, string>* fieldValue);
	virtual long SQLNext(long queryID, std::map<string, string>* fieldValue);
	virtual long SQLPrev(long queryID, std::map<string, string>* fieldValue);
	virtual long SQLLast(long queryID, std::map<string, string>* fieldValue);
	virtual long SQLEnd(long queryID);
	//得到字段值
	virtual long SQLGetValueByField(long queryID, const char* fieldName, string& fieldValue);
	virtual long SQLGetValueByIndex(long queryID, long fieldIndex, string& fieldValue);

public:
	//执行语句
  virtual long SQLExecute(const char* SQLExpression, string *pstrErr);
	//返回查询结果
  virtual long SQLExecute(const char* SQLExpression, vector< vector<string> >& resultArray, string *pstrErr);
  virtual long SQLExecute(const char* SQLExpression, vector< vector<string> >& resultArray, vector<string>& fieldValueVector, vector<int>& fieldTypeVector, string *pstrErr);

  virtual long SQLExecute(const char* SQLExpression, list< list<string> >& resultArray, string *pstrErr);
  virtual long SQLExecute(const char* SQLExpression, list< list<string> >& resultArray, list<string>& fieldValueVector, list<int>& fieldTypeVector, string *pstrErr);

private:
	SAConnection *m_pConnection;
	int	m_nDatabaseTypeId;
	string m_strConnectionString;
	string m_strUserName;
	string m_strPassword;
	string m_strCodingSet;
	long m_lConnTimeout;	 // 连接超时

private:
	long m_generateID;

	std::map<long, SACommandEx*>		m_mapConnectionQuery;

	long SetSaveDataMode(SACommandEx* pCmd);
	long GetNewID();
	
	SAConnection* GetConnection();
	SACommandEx* GetCommand(long queryID);
	long GetFieldValue(SACommandEx* pCmd, std::map<string, string>* fieldValue);

	std::map<long, vector< vector<string> > > m_queryMap;
	std::map<long, long> m_cursorMap;//记录游标位置
	std::map<long, vector<string> > m_columnMap;
};
