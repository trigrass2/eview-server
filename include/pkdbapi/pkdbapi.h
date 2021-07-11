/**************************************************************
 *  Filename:    CPKDbApi.h
 *  Copyright:   Shanghai Baosight Software Co., Ltd.
 *
 *  Description: CPKDbApi��չ���ͷ�ļ�.
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

// ֧�ֵ����ݿ�����
#define PK_DATABASE_TYPENAME_SQLITE		"sqlite"		// �����ַ���: SQLite ��Ի�����ļ�·��, .\\test.db, ./sqlitetest.db �� d:/sqlitetest.db
#define PK_DATABASE_TYPENAME_MYSQL		"mysql"			// �����ַ���: 192.168.10.2:13306@eview
#define PK_DATABASE_TYPENAME_POSTGRESQL	"postgresql"	// �����ַ���: localhost@test
#define PK_DATABASE_TYPENAME_ORACLE		"oracle"		// �����ַ���: Oracle������
#define PK_DATABASE_TYPENAME_SQLSERVER	"sqlserver"		// �����ַ���: BEDLAM\\SQL2005EX_EN@pubs, srv2@pubs, @test1,C:\\test.sdf
#define PK_DATABASE_TYPENAME_DB2		"db2"			// �����ַ���:
#define PK_DATABASE_TYPENAME_INTERBASE	"interbase"		// �����ַ���: �����ַ�������test
#define PK_DATABASE_TYPENAME_INFORMIX	"informix"		// �����ַ���: 
#define PK_DATABASE_TYPENAME_SYBASE		"sybase"		// �����ַ���: bedlam@test
#define PK_DATABASE_TYPENAME_ODBC		"odbc"			// �����ַ���: odbcad32.exe �е�����Դ���ƣ���ORA92
#define PK_DATABASE_TYPENAME_SQLBASE	"sqlbase"		// �����ַ���: island

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

// �ֶ�����
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
	
	int InitFromConfigFile(const char *szConfigFileName = NULL, const char *szDBSectionName="database"); // �������ļ���ȡȱʡ��,����ͬ�������ļ�.conf, ini��ʽ,�洢��[db]С��������
	int GetDatabaseTypeId();
	virtual long SQLConnect(const char* connectionString, const char* userName, const char* password, long databaseType, long lTimeOutSec, const char* szCodingSet);
	virtual long SQLDisconnect(); //������Ͽ����ݿ�
	virtual void UnInitialize(); // ����ʱ����

    long TestConnected(const char *szTestTableName); // ��������״̬����Ҫ����һ����Ȩ�޵ı������ڲ���ִ��select count(*) from {szTestTableName}��Ӧ�ö�ʱȥִ��������������״̬�Բ��ԡ�
	bool IsConnected(); // ���ϴ�ִ�����ݿ�ִ�����ӻ�TestConnection��SQLConnect���������ݿ�����״̬. �������ʱִ��TestConnected, ��ô����ֵ���ܲ���
	static int	ConvertDBTypeName2Id(const char *szDbTypeName);
public:
	//�ӱ��м�������
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
	//ʹ������
	virtual long SQLTransact();
	virtual long SQLCommit();
	virtual long SQLRollback();

public:
	//���ز�ѯ���
  virtual long SQLExecute(const char* szSQL, string *pStrErr = NULL); //ִ��һ����SELECT��SQL���
  virtual long SQLExecute(const char* szSQL, vector<vector<string> >& vecRow, string *pStrErr = NULL); // ���ز�ѯ���
  virtual long SQLExecute(const char* szSQL, vector<vector<string> >& vecRow, vector<string>& vecFieldName, vector<int>& vecFieldType, string *pStrErr = NULL);// ���ز�ѯ������ֶ������ֶ�����

  virtual long SQLExecute(const char* szSQL, list<list<string> >& vecRow, string *pStrErr = NULL); // ���ز�ѯ���. ������
  virtual long SQLExecute(const char* szSQL, list<list<string> >& vecRow, list<string>& vecFieldName, list<int>& vecFieldType, string *pStrErr = NULL); // ���ز�ѯ������ֶ������ֶ�����. ������
private:
	void *m_pDbApiImpl;
};

