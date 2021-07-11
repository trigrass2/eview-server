/**************************************************************
 *  Filename:    CPKDbApi.h
 *  Copyright:   Shanghai Baosight Software Co., Ltd.
 *
 *  Description: CPKDbApi扩展类的头文件.
 *
 *  @author:     zhangqiang
 *  @version     10/26/2012  zhangqiang  Initial Version
**************************************************************/
#pragma once

#ifdef _WIN32
#	ifdef pkdbapi_EXPORTS
#		define PKDBAPI_API __declspec(dllexport)
#	else
#		define PKDBAPI_API __declspec(dllimport)
#	endif
#else
#	define PKDBAPI_API __attribute__ ((visibility ("default")))
#endif//_WIN32

#include <vector>
#include <map>
#include <list>
#include <string>

using namespace std;

// 支持的数据库类型
#define PK_DATABASE_TYPENAME_SQLITE		"sqlite"		// 连接字符串: SQLite 相对或绝对文件路径, .\\test.db, ./sqlitetest.db 或 d:/sqlitetest.db
#define PK_DATABASE_TYPENAME_MYSQL		"mysql"			// 连接字符串: 192.168.10.2:13306@eview
#define PK_DATABASE_TYPENAME_POSTGRESQL	"postgresql"	// 连接字符串: localhost@test
#define PK_DATABASE_TYPENAME_ORACLE		"oracle"		// 连接字符串: Oracle服务名
#define PK_DATABASE_TYPENAME_SQLSERVER	"sqlserver"		// 连接字符串: BEDLAM\\SQL2005EX_EN@pubs, srv2@pubs, @test1,C:\\test.sdf
#define PK_DATABASE_TYPENAME_DB2		"db2"			// 连接字符串:
#define PK_DATABASE_TYPENAME_INTERBASE	"interbase"		// 连接字符串: 名字字符串，如test
#define PK_DATABASE_TYPENAME_INFORMIX	"informix"		// 连接字符串: 
#define PK_DATABASE_TYPENAME_SYBASE		"sybase"		// 连接字符串: bedlam@test
#define PK_DATABASE_TYPENAME_ODBC		"odbc"			// 连接字符串: odbcad32.exe 中的数据源名称，如ORA92
#define PK_DATABASE_TYPENAME_SQLBASE	"sqlbase"		// 连接字符串: island

#define PK_DATABASE_TYPEID_ODBC			1
#define PK_DATABASE_TYPEID_ORACLE		2
#define PK_DATABASE_TYPEID_SQLSERVER	3
#define PK_DATABASE_TYPEID_INTERBASE	4
#define PK_DATABASE_TYPEID_SQLBASE		5
#define PK_DATABASE_TYPEID_DB2			6
#define PK_DATABASE_TYPEID_INFORMIX		7
#define PK_DATABASE_TYPEID_SYBASE		8
#define PK_DATABASE_TYPEID_MYSQL		9
#define PK_DATABASE_TYPEID_POSTGRESQL	10
#define PK_DATABASE_TYPEID_SQLITE		11
#define PK_DATABASE_TYPEID_INVALID		-1

// 字段类型
#define PK_DATABASE_DATATYPE_UNKNOWN	0
#define PK_DATABASE_DATATYPE_BOOL		1
#define PK_DATABASE_DATATYPE_SHORT		2
#define PK_DATABASE_DATATYPE_USHORT		3
#define PK_DATABASE_DATATYPE_LONG		4
#define PK_DATABASE_DATATYPE_ULONG		5
#define PK_DATABASE_DATATYPE_DOUBLE		6
#define PK_DATABASE_DATATYPE_NUMERIC	7
#define PK_DATABASE_DATATYPE_DATETIME	8
#define PK_DATABASE_DATATYPE_INTERVAL	9
#define PK_DATABASE_DATATYPE_STRING		10
#define PK_DATABASE_DATATYPE_BYTES		11
#define PK_DATABASE_DATATYPE_LONGBINARY	12
#define PK_DATABASE_DATATYPE_LONGCHAR	13
#define PK_DATABASE_DATATYPE_BLOB		14
#define PK_DATABASE_DATATYPE_CLOB		15
#define PK_DATABASE_DATATYPE_CURSOR		16
#define PK_DATABASE_DATATYPE_SPECIFICTODBMS	17


class PKDBAPI_API CPKDbApi
{
public:
    CPKDbApi(void *pPkLog = NULL);
	virtual ~CPKDbApi();
	
	int InitFromConfigFile(const char *szConfigFileName = NULL, const char *szDBSectionName="database"); // 从配置文件读取缺省的,进程同名配置文件.conf, ini格式,存储在[db]小节下下面
	int GetDatabaseTypeId();
	virtual long SQLConnect(const char* connectionString, const char* userName, const char* password, long databaseType, long lTimeOutSec, const char* szCodingSet);
	virtual long SQLDisconnect(); //连接与断开数据库
	virtual void UnInitialize(); // 结束时调用

    long TestConnected(const char *szTestTableName); // 测试连接状态，需要给出一个有权限的表名，内部会执行select count(*) from {szTestTableName}。应该定时去执行这个语句检查连接状态对不对。
	bool IsConnected(); // 从上次执行数据库执行连接或TestConnection或SQLConnect以来的数据库连接状态. 如果不定时执行TestConnected, 那么返回值可能不对
	static int	ConvertDBTypeName2Id(const char *szDbTypeName);
public:
	//从表中检索数据
	virtual long SQLQuery(long& lQueryId, const char* szSQL);
	virtual long SQLNumRows(long lQueryId, long& numRows);
	virtual long SQLGetRecord(long lQueryId, long recordNumber, std::map<string, string>& row);
	virtual long SQLFirst(long lQueryId, map<string, string>& row);
	virtual long SQLNext(long lQueryId, map<string, string>& row);
	virtual long SQLPrev(long lQueryId, map<string, string>& row);
	virtual long SQLLast(long lQueryId, map<string, string>& row);
	virtual long SQLEnd(long lQueryId);
	virtual long SQLGetValueByField(long lQueryId, const char* fieldName, string& fieldValue);
	virtual long SQLGetValueByIndex(long lQueryId, long fieldIndex, string& fieldValue);	

public:
	//使用事务集
	virtual long SQLTransact();
	virtual long SQLCommit();
	virtual long SQLRollback();

public:
	//返回查询结果
  virtual long SQLExecute(const char* szSQL, string *pStrErr = NULL); //执行一个非SELECT的SQL语句
  virtual long SQLExecute(const char* szSQL, vector<vector<string> >& vecRow, string *pStrErr = NULL); // 返回查询结果
  virtual long SQLExecute(const char* szSQL, vector<vector<string> >& vecRow, vector<string>& vecFieldName, vector<int>& vecFieldType, string *pStrErr = NULL);// 返回查询结果、字段名、字段类型

  virtual long SQLExecute(const char* szSQL, list<list<string> >& vecRow, string *pStrErr = NULL); // 返回查询结果. 高性能
  virtual long SQLExecute(const char* szSQL, list<list<string> >& vecRow, list<string>& vecFieldName, list<int>& vecFieldType, string *pStrErr = NULL); // 返回查询结果、字段名、字段类型. 高性能
private:
	void *m_pDbApiImpl;
};

