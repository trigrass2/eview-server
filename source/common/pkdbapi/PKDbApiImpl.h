/**************************************************************
 *  Filename:    CPKDbApiImpl.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: CPKDbApiImpl��չ���ͷ�ļ�.
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
	//������Ͽ����ݿ�
	
public:
	int	GetDatabaseTypeId(const char *szDataBaseName);
	virtual long SQLConnectEx(const char* connectionString, const char* userName, const char* password, long databaseType, long lTimeOut, const char* szCodingSet);
	virtual long SQLDisconnect();

public:
	//ʹ������
	virtual long SQLTransact();
	virtual long SQLCommit();
	virtual long SQLRollback();
	virtual void UnInitialize();
	long IsConnected(const char *szTestTableName = NULL);
public:
	// �α귽ʽ��ѯ
	virtual long SQLQuery(long& queryID, const char* SQLExpression);
	virtual long SQLGetRecord(long queryID, long recordNumber, std::map<string, string>* fieldValue);
	virtual long SQLNumRows(long queryID, long& numRows);
	virtual long SQLFirst(long queryID, std::map<string, string>* fieldValue);
	virtual long SQLNext(long queryID, std::map<string, string>* fieldValue);
	virtual long SQLPrev(long queryID, std::map<string, string>* fieldValue);
	virtual long SQLLast(long queryID, std::map<string, string>* fieldValue);
	virtual long SQLEnd(long queryID);
	//�õ��ֶ�ֵ
	virtual long SQLGetValueByField(long queryID, const char* fieldName, string& fieldValue);
	virtual long SQLGetValueByIndex(long queryID, long fieldIndex, string& fieldValue);

public:
	//ִ�����
  virtual long SQLExecute(const char* SQLExpression, string *pstrErr);
	//���ز�ѯ���
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
	long m_lConnTimeout;	 // ���ӳ�ʱ

private:
	long m_generateID;

	std::map<long, SACommandEx*>		m_mapConnectionQuery;

	long SetSaveDataMode(SACommandEx* pCmd);
	long GetNewID();
	
	SAConnection* GetConnection();
	SACommandEx* GetCommand(long queryID);
	long GetFieldValue(SACommandEx* pCmd, std::map<string, string>* fieldValue);

	std::map<long, vector< vector<string> > > m_queryMap;
	std::map<long, long> m_cursorMap;//��¼�α�λ��
	std::map<long, vector<string> > m_columnMap;
};
