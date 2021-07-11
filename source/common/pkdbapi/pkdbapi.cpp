/**************************************************************
 *  Filename:    CPKDbApi.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: CPKDbApi��չ���ʵ���ļ�.
 *
 *  @author:     zhangqiang
 *  @version     10/26/2012  zhangqiang  Initial Version
**************************************************************/
#include "pkdbapi/pkdbapi.h"
#include "PKDbApiImpl.h"
#include "pklog/pklog.h"
#include <string>
#include <string.h>

#define _(x)	x
using namespace std;

// CPKDbApi g_PKDbApi;	// ����һ��ʵ��

CPKDbApi::CPKDbApi(void *pPkLog)
{
  m_pDbApiImpl = (void *) new CPKDbApiImpl(pPkLog);
}

CPKDbApi::~CPKDbApi()
{
	if(m_pDbApiImpl)
	{
		delete (CPKDbApiImpl *)m_pDbApiImpl;
		m_pDbApiImpl = NULL;
	}
}

/**
*	�������ݿ�
*
*	@param[in][out] conncetionID         ����ID,
*	@param[in] conncetionString        �����ַ���
*	@param[in] userName         �û���
*	@param[in] password         ����
*	@param[in] databasetype         ���ݿ�����
*	@param[in] lTimeOut         ���Ӳ��ϵĳ�ʱʱ��
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLConnect(const char* connectionString, const char* userName, const char* password, long databaseType, long lTimeOut, const char* szCodingSet)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLConnectEx(connectionString, userName, password, databaseType, lTimeOut, szCodingSet);
}
/**
*	�Ͽ����ݿ�
*
*	@param[in] conncetionID         ����ID,
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLDisconnect()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLDisconnect();
}

/**
*	��ѯ
*
*	@param[in] conncetionID         ����ID,
*	@param[in][out] queryID        ���ز�ѯID
*	@param[in] SQLExpression       sql���
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLQuery(long & queryID, const char* SQLExpression)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLQuery(queryID, SQLExpression);
}

/**
*	��ѯ
*
*	@param[in] conncetionID         ����ID,
*	@param[in][out] queryID        ��ѯID
*	@param[in] SQLExpression       sql���
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLExecute(const char* SQLExpression, vector<vector<string> >& rows, string *pStrErr)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
    return pDbApiImpl->SQLExecute(SQLExpression, rows, pStrErr);
}

long CPKDbApi::SQLExecute(const char* SQLExpression, list<list<string> >& rows, string *pStrErr)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
    return pDbApiImpl->SQLExecute(SQLExpression, rows, pStrErr);
}

/**
*	��ѯ
*
*	@param[in] conncetionID         ����ID,
*	@param[in][out] queryID        ��ѯID
*	@param[in] SQLExpression       sql���
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLExecute(const char* SQLExpression, vector<vector<string> >& rows, vector<string>& fieldValueVector, vector<int>& fieldTypeVector, string *pStrErr)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLExecute(SQLExpression, rows, fieldValueVector, fieldTypeVector, pStrErr);
}


/**
*	��ѯ
*
*	@param[in] conncetionID         ����ID,
*	@param[in][out] queryID        ��ѯID
*	@param[in] SQLExpression       sql���
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLExecute(const char* SQLExpression, list<list<string> >& rows, list<string>& fieldValueVector, list<int>& fieldTypeVector, string *pStrErr)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLExecute(SQLExpression, rows, fieldValueVector, fieldTypeVector, pStrErr);
}


/**
*	ִ��sql���
*
*	@param[in] conncetionID         ����ID,
*	@param[in][out] SQLExpression         sql���
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLExecute(const char* SQLExpression, string *pStrErr)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLExecute(SQLExpression, pStrErr);
}

/**
*	��ʼ����
*
*	@param[in] conncetionID         ����ID
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLTransact()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLTransact();
}
/**
/**
*	�ύ����
*
*	@param[in] conncetionID         ����ID
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLCommit()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLCommit();
}
/**
*	�ع�����
*
*	@param[in] conncetionID         ����ID
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLRollback()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLRollback();
}

/**
*	�õ���Ӧ�еļ�¼
*
*	@param[in] conncetionID         ����ID,
*	@param[in] qureyID        ��ѯID
*	@param[in] recordNumber         ��¼���к�
*	@param[in][out] fieldValue         ���е��ֶκͶ�Ӧ��ֵ
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLGetRecord(long queryID, long recordNumber, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLGetRecord(queryID, recordNumber, &row);
}

/**
*	�����¼������
*
*	@param[in][out] conncetionID         ����ID,
*	@param[in] queryID        ��ѯID
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLNumRows(long queryID, long& numRows)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLNumRows(queryID, numRows);
}
/**
*	�õ���һ�еļ�¼
*
*	@param[in] conncetionID         ����ID,
*	@param[in] qureyID        ��ѯID
*	@param[in][out] fieldValue         ���е��ֶκͶ�Ӧ��ֵ
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLFirst(long queryID, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLFirst(queryID, &row);
}

/**
*	�õ���һ�еļ�¼
*
*	@param[in] conncetionID         ����ID,
*	@param[in] qureyID        ��ѯID
*	@param[in][out] fieldValue         ���е��ֶκͶ�Ӧ��ֵ
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLNext(long queryID, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLNext(queryID, &row);
}
/**
*	�õ�ǰһ�еļ�¼
*
*	@param[in] conncetionID         ����ID,
*	@param[in] qureyID        ��ѯID
*	@param[in][out] fieldValue         ���е��ֶκͶ�Ӧ��ֵ
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLPrev(long queryID, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLPrev(queryID, &row);
}
/**
*	�õ����һ�еļ�¼
*
*	@param[in] conncetionID         ����ID,
*	@param[in] qureyID        ��ѯID
*	@param[in][out] fieldValue         ���е��ֶκͶ�Ӧ��ֵ
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLLast(long queryID, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLLast(queryID, &row);
}
/**
*	�����˴β�ѯ
*
*	@param[in] conncetionID         ����ID,
*	@param[in] qureyID        ��ѯID
*
*   @return
*		- ==0 �ɹ�
*		- !=0 �����쳣
*/
long CPKDbApi::SQLEnd(long queryID)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLEnd(queryID);
}

/**
*	��ȡ���ж�Ӧ�ֶε�ֵ
*
*	@param[in] conncetionID         ����ID,
*	@param[in] qureyID        ��ѯID
*	@param[in][out] fieldName         ���е��ֶ�
*
*   @return  �ֶζ�Ӧ��ֵ
*/
long CPKDbApi::SQLGetValueByField(long queryID, const char* fieldName, string& fieldValue)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLGetValueByField(queryID, fieldName, fieldValue);
}
/*
*	��ȡ���ж�Ӧ�ֶε�ֵ
*
*	@param[in] conncetionID         ����ID,
*	@param[in] qureyID        ��ѯID
*	@param[in][out] fieldName         ���е�����
*
*   @return  ������Ӧ��ֵ
*/
long CPKDbApi::SQLGetValueByIndex(long queryID, long fieldIndex, string& fieldValue)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLGetValueByIndex(queryID, fieldIndex, fieldValue);
}

/**
 *  �ͷ����õ�����Դ.
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
void CPKDbApi::UnInitialize()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->UnInitialize();
}

long CPKDbApi::IsConnected(const char *szTestTableName /*= NULL*/)
{
  CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
  return pDbApiImpl->IsConnected(szTestTableName);
}

int CPKDbApi::GetDatabaseTypeId( const char *szDbTypeName )
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->GetDatabaseTypeId(szDbTypeName);
}

