/**************************************************************
 *  Filename:    SQLAPIWrapper.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: SQLAPIWrapper扩展类的头文件.
 *
 *  @author:     zhangqiang
 *  @version     10/26/2012  zhangqiang  Initial Version
**************************************************************/
#pragma once
#include "pkdbapi/pkdbapi.h"
using namespace std;

typedef
	enum eSAClient
{
	//! DBMS client is not specified
	SA_Client_NotSpecified,
	//! ODBC
	SA_ODBC_Client,
	//! Oracle
	SA_Oracle_Client,
	//! Microsoft SQL Server
	SA_SQLServer_Client,
	//! InterBase or Firebird
	SA_InterBase_Client,
	//! SQLBase
	SA_SQLBase_Client,
	//! IBM DB2
	SA_DB2_Client,
	//! Informix
	SA_Informix_Client,
	//! Sybase ASE 
	SA_Sybase_Client,
	//! MySQL
	SA_MySQL_Client,
	//! PostgreSQL
	SA_PostgreSQL_Client,
	//! SQLite
	SA_SQLite_Client,
	_SA_Client_Reserverd = (int)(((unsigned int)(-1))/2)
} SAClient_t;

typedef
enum eSADataType
{
	SA_dtUnknown,
	SA_dtBool,
	SA_dtShort,
	SA_dtUShort,
	SA_dtLong,
	SA_dtULong,
	SA_dtDouble,
	SA_dtNumeric,
	SA_dtDateTime,
	SA_dtInterval,
	SA_dtString,
	SA_dtBytes,
	SA_dtLongBinary,
	SA_dtLongChar,
	SA_dtBLob,
	SA_dtCLob,
	SA_dtCursor,
	SA_dtSpecificToDBMS,
	_SA_DataType_Reserved = (int)(((unsigned int)(-1))/2)
} SADataType_t;
class CMyDbApi:public CPKDbApi
{
public:
	CMyDbApi(string strDBPath);
	CMyDbApi();
	~CMyDbApi();

public:
	string m_strConfPath;
 	string strDBType;
	int Init();
	int Exit();
	int InitLocalDB();
	int ExeCreateTable(string strTableName, vector<string> vec_fieldName, map<string,string> vec_fieltype  );
	//int ExeCreateTable(string strTargetTableName, vector<vector<string>>);
	int ExeInsertValues(string strTableName, vector<vector<string>> rows);
public:
	bool CheckTargetTableExist( string strTableName );
	int GetDBType(){ return m_nDataBaseType; }
	int CheckConnection();
public:
	bool m_bIsConnected;
	string m_strDatabaseType;
	int m_nMinLogConn;
	int m_nTimeSpan;
private:
	int m_lConnectionId;
	int m_nDataBaseType;
	string m_strUsername;
	string m_strPassword;
};

