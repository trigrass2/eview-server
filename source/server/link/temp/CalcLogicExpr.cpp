/**************************************************************
 *  Filename:    CalcLogicExpr.cxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 根据计算逻辑表达式对应的节点类型计算表达式结果.
 *
 *  @author:     caichunlei
 *  @version     05/29/2008  caichunlei  Initial Version
 *  @version     06/10/2008  caichunlei  添加初始化函数，加载不同的动态库
**************************************************************/
#include "common/OS.h"

#include "calclogicexpr.hxx"
#include <algorithm>
#include "common/pkGlobalHelper.h"

extern bool double_equal(double d1, double d2);
//typedef CVarProxy* (*CVarProxy_Creator) (void);

/**
 *  构造函数.
 *
 *
 *  @version     05/29/2008  caichunlei  Initial Version.
 */
CCalcLogicExpr::CCalcLogicExpr()
{
	m_pVarProxy = NULL;
	
	m_pfn_VarProxy_Init = NULL;
	m_pfn_VarProxy_RegVar = NULL;
	m_pfn_VarProxy_Fini = NULL;
	m_pfn_VarProxy_GetVar = NULL;
	m_pfn_VarProxy_Start = NULL;
}

/**
 *  析构函数.
 *
 *
 *  @version     05/29/2008  caichunlei  Initial Version.
 */
CCalcLogicExpr::~CCalcLogicExpr()
{
		//释放指针
// 	if(m_pfn_VarProxy_Fini)
// 	{
// 		//delete m_pVarProxy;
// 		m_pfn_VarProxy_Fini(m_pVarProxy);
// 	}
// 
		//关闭动态库
	m_WorkingDLL.close();
}

/**
 *  读取事件触发源获取变量值的配置信息.
 *
 *  @param  -[in]      CCRSDBOperation&  theDBOper:  [提供数据库操作]
 *  @param  -[in,out]  T_LogicTriggerCfgInfoList&  theLogicTriggerCfgInfoList: [配置信息列表]
 *
 *  @version     06/10/2008  caichunlei  Initial Version.
 */
int CCalcLogicExpr::ReadLogicTriggerCfgInfos(CCRSDBOperation& theDBOper, T_LogicTriggerCfgInfoList& theLogicTriggerCfgInfoList)
{
	try
	{
		CppSQLite3Table t = theDBOper.GetTable("select fd_cfg_id, fd_cfg_name, \
										fd_getvarvalue_dllname, fd_is_active \
								from t_crs_eventtriggercfgs order by fd_cfg_id;");
		
		//得到纪录个数
		int nRowCount = t.numRows();
		for(int i = 0; i < nRowCount; i++)
		{
			//得到第n行的信息
			t.setRow(i);		
			
			//声明对象
			T_LogicTriggerCfgInfo* pT_LogicTriggerCfg = new T_LogicTriggerCfgInfo;
			pT_LogicTriggerCfg->m_nCfgID = atoi(t.fieldValue(0));
			PKStringHelper::Safe_StrNCpy(pT_LogicTriggerCfg->m_szCfgName, t.fieldValue(1), sizeof(pT_LogicTriggerCfg->m_szCfgName));
			//Safe_CopyString(pT_LogicTriggerCfg->m_szGetVarValueDll, t.fieldValue(2), sizeof(pT_LogicTriggerCfg->m_szGetVarValueDll));
			//将动态库名字改为全小写
			const char* pszDllName = t.fieldValue(2);
			int nLen = (strlen(pszDllName) <= ICV_LONGFILENAME_MAXLEN - 1) ? strlen(pszDllName) : (ICV_LONGFILENAME_MAXLEN - 1);
			std::transform(pszDllName, pszDllName+nLen, pT_LogicTriggerCfg->m_szGetVarValueDll, (int(*)(int))tolower);
			pT_LogicTriggerCfg->m_szGetVarValueDll[nLen] = '\0';	// 注意字符串结束符			
			pT_LogicTriggerCfg->m_bIsActive = atoi(t.fieldValue(3)) > 0 ? true : false;
			
			theLogicTriggerCfgInfoList.insert_tail(pT_LogicTriggerCfg);
		}//for(int i = 0; i < nRowCount; i++)
	}
	catch(...)
	{
		PKLog.LogMessage(PK_LOGLEVEL_CRITICAL, 
			_("CCRSDBOperation:ReadLogicTriggerCfgs catches exception!"));	
		return EC_ICV_CRS_EXCEPTION;
	}

	return ICV_SUCCESS;
}
/**
 *  初始化函数.
 *
 *
 *  @version     06/10/2008  caichunlei  Initial Version.
 *  @version     06/10/2008  caichunlei  数据库部分使用单体实现.
 *  @version     06/10/2008  caichunlei  数据库部分改为非单体实现，单体实现会异常.
 */
int CCalcLogicExpr::InitCalc()
{
	PKLog.SetLogFileName("CRS_LogicTrigger");
	int nRet = ICV_SUCCESS;
	//声明变量
	CCRSDBOperation theCRSDBOper;
	T_LogicTriggerCfgInfoList theLogicTriggerCfgList;
	
	//打开数据库,读取配置信息
	theCRSDBOper.OpenCRSDB();
	ReadLogicTriggerCfgInfos(theCRSDBOper, theLogicTriggerCfgList);

	//逐一解析配置信息
	T_LogicTriggerCfgInfoListIterator iter(theLogicTriggerCfgList);
	while(!iter.done())
	{
		T_LogicTriggerCfgInfo* pT_LogicTriggerCfg = NULL;
		pT_LogicTriggerCfg = iter.next();

		//同一时间只有一个动态库可被加载
		if(pT_LogicTriggerCfg->m_bIsActive)
		{
			//加载动态库
			nRet = LoadGetVarValueDll(pT_LogicTriggerCfg->m_szGetVarValueDll);
			//结束循环
			break;
		}
		else
			iter++;
	}//while(!iter.done ())

	//释放资源
	for (iter.first(); !iter.done(); iter ++)
	{
		delete iter.next();
	}

	theCRSDBOper.CloseCRSDB();
	//返回
	return nRet;
}

/**
 *  加载获取变量的动态库.
 *
 *  @param  -[in]  const char*  szDllName: [动态库名称]
 *
 *  @version     06/10/2008  caichunlei  Initial Version.
 *  @version     06/20/2008  caichunlei  修改日志信息.
 */
int CCalcLogicExpr::LoadGetVarValueDll(const char* szDllName)
{
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, 
		_("CCalcLogicExpr:the dll name of acquire varable is %s!"), szDllName);
		//加载触发源配置的动态库
		int nRetVal = m_WorkingDLL.open(szDllName);
		if(nRetVal != 0)
		{
			//打印日志信息；
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, 
				_("CCalcLogicExpr: failed to load dll %s!"), szDllName);
			return EC_ICV_CRS_LOADDLLFAIL;
		}

		m_pfn_VarProxy_Init = (PFN_VarProxy_Init)m_WorkingDLL.symbol(ACE_TEXT("VarProxy_Init"));
		m_pfn_VarProxy_Fini = (PFN_VarProxy_Exit)m_WorkingDLL.symbol(ACE_TEXT("VarProxy_Exit"));
		m_pfn_VarProxy_RegVar = (PFN_VarProxy_RegVar)m_WorkingDLL.symbol(ACE_TEXT("VarProxy_RegVar"));
		m_pfn_VarProxy_GetVar = (PFN_VarProxy_GetVar)m_WorkingDLL.symbol(ACE_TEXT("VarProxy_GetVar"));
		m_pfn_VarProxy_Start = (PFN_VarProxy_Start)m_WorkingDLL.symbol(ACE_TEXT("VarProxy_Start"));
		if(!m_pfn_VarProxy_Init || !m_pfn_VarProxy_Fini || !m_pfn_VarProxy_RegVar || !m_pfn_VarProxy_GetVar || !m_pfn_VarProxy_Start)
		{
			PKLog.LogErrMessage(EC_ICV_CRS_GETDLLFUNCFAIL, 
				_("CCalcLogicExpr:failed to load var proxy dll (%s)!"), szDllName);//获取变量代理动态库(%s)指针失败
			return EC_ICV_CRS_GETDLLFUNCFAIL;
		}

		//得到触发源对应的指针
		m_pVarProxy = m_pfn_VarProxy_Init();
		//返回
		return ICV_SUCCESS;
}

/**
 *  获取变量当前值的函数.
 *
 *  @param  -[in]  char*  pVariName: [变量名称]
 *  @param  -[out]  double&  dbValue: [变量值]
 *
 *  @version     05/29/2008  caichunlei  Initial Version.
 */
double CCalcLogicExpr::GetDoubleVar(const char* szVarName )
{
	double dbValue = 0.f;
	return m_pfn_VarProxy_GetVar(m_pVarProxy, szVarName, &dbValue);
}

/**
 *  计算表达式结果(true或false).
 *
 *  @param  -[in,out]  nodeType*  pNodeType: [comment]
 *
 *  @version     05/29/2008  caichunlei  Initial Version.
 */
bool CCalcLogicExpr::CalculateBool(nodeType *pNodeType)
{
		//如果表达式计算为0，返回false，否则返回true
		double dbResult = DoCalcTree(pNodeType);
		if(double_equal(dbResult, 0))
			return false;
		else
			return true;
}

/**
 *  Finish Trigger Type.
 *
 *
 *  @version     11/01/2008  chenzhiquan  Initial Version.
 */
long CCalcLogicExpr::FiniTriggerType()
{
	if (m_pfn_VarProxy_Fini)
	{
		m_pfn_VarProxy_Fini(m_pVarProxy);
		m_pfn_VarProxy_Fini = NULL;
		m_pfn_VarProxy_Init = NULL;
		m_pfn_VarProxy_GetVar = NULL;
		m_pfn_VarProxy_RegVar = NULL;
		m_pVarProxy = NULL;
	}
	m_WorkingDLL.close();
	return ICV_SUCCESS;
}

