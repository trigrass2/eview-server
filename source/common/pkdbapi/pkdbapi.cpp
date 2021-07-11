/**************************************************************
 *  Filename:    CPKDbApi.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: CPKDbApi扩展类的实现文件.
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

// CPKDbApi g_PKDbApi;	// 声明一个实例

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
long CPKDbApi::SQLConnect(const char* connectionString, const char* userName, const char* password, long databaseType, long lTimeOut, const char* szCodingSet)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLConnectEx(connectionString, userName, password, databaseType, lTimeOut, szCodingSet);
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
long CPKDbApi::SQLDisconnect()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLDisconnect();
}

/**
*	查询
*
*	@param[in] conncetionID         连接ID,
*	@param[in][out] queryID        返回查询ID
*	@param[in] SQLExpression       sql语句
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApi::SQLQuery(long & queryID, const char* SQLExpression)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLQuery(queryID, SQLExpression);
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
long CPKDbApi::SQLExecute(const char* SQLExpression, vector<vector<string> >& rows, vector<string>& fieldValueVector, vector<int>& fieldTypeVector, string *pStrErr)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLExecute(SQLExpression, rows, fieldValueVector, fieldTypeVector, pStrErr);
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
long CPKDbApi::SQLExecute(const char* SQLExpression, list<list<string> >& rows, list<string>& fieldValueVector, list<int>& fieldTypeVector, string *pStrErr)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLExecute(SQLExpression, rows, fieldValueVector, fieldTypeVector, pStrErr);
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
long CPKDbApi::SQLExecute(const char* SQLExpression, string *pStrErr)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLExecute(SQLExpression, pStrErr);
}

/**
*	开始事务
*
*	@param[in] conncetionID         连接ID
*
*   @return
*		- ==0 成功
*		- !=0 出现异常
*/
long CPKDbApi::SQLTransact()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLTransact();
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
long CPKDbApi::SQLCommit()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLCommit();
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
long CPKDbApi::SQLRollback()
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLRollback();
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
long CPKDbApi::SQLGetRecord(long queryID, long recordNumber, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLGetRecord(queryID, recordNumber, &row);
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
long CPKDbApi::SQLNumRows(long queryID, long& numRows)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLNumRows(queryID, numRows);
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
long CPKDbApi::SQLFirst(long queryID, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLFirst(queryID, &row);
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
long CPKDbApi::SQLNext(long queryID, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLNext(queryID, &row);
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
long CPKDbApi::SQLPrev(long queryID, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLPrev(queryID, &row);
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
long CPKDbApi::SQLLast(long queryID, map<string, string>& row)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLLast(queryID, &row);
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
long CPKDbApi::SQLEnd(long queryID)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLEnd(queryID);
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
long CPKDbApi::SQLGetValueByField(long queryID, const char* fieldName, string& fieldValue)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLGetValueByField(queryID, fieldName, fieldValue);
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
long CPKDbApi::SQLGetValueByIndex(long queryID, long fieldIndex, string& fieldValue)
{
	CPKDbApiImpl *pDbApiImpl = (CPKDbApiImpl *)m_pDbApiImpl;
	return pDbApiImpl->SQLGetValueByIndex(queryID, fieldIndex, fieldValue);
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

