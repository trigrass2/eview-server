/**************************************************************
 *  Filename:    CalcLogicExpr.cxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ���ݼ����߼����ʽ��Ӧ�Ľڵ����ͼ�����ʽ���.
 *
 *  @author:     caichunlei
 *  @version     05/29/2008  caichunlei  Initial Version
 *  @version     06/10/2008  caichunlei  ��ӳ�ʼ�����������ز�ͬ�Ķ�̬��
**************************************************************/
#include "common/OS.h"

#include "calclogicexpr.hxx"
#include <algorithm>
#include "common/pkGlobalHelper.h"

extern bool double_equal(double d1, double d2);
//typedef CVarProxy* (*CVarProxy_Creator) (void);

/**
 *  ���캯��.
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
 *  ��������.
 *
 *
 *  @version     05/29/2008  caichunlei  Initial Version.
 */
CCalcLogicExpr::~CCalcLogicExpr()
{
		//�ͷ�ָ��
// 	if(m_pfn_VarProxy_Fini)
// 	{
// 		//delete m_pVarProxy;
// 		m_pfn_VarProxy_Fini(m_pVarProxy);
// 	}
// 
		//�رն�̬��
	m_WorkingDLL.close();
}

/**
 *  ��ȡ�¼�����Դ��ȡ����ֵ��������Ϣ.
 *
 *  @param  -[in]      CCRSDBOperation&  theDBOper:  [�ṩ���ݿ����]
 *  @param  -[in,out]  T_LogicTriggerCfgInfoList&  theLogicTriggerCfgInfoList: [������Ϣ�б�]
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
		
		//�õ���¼����
		int nRowCount = t.numRows();
		for(int i = 0; i < nRowCount; i++)
		{
			//�õ���n�е���Ϣ
			t.setRow(i);		
			
			//��������
			T_LogicTriggerCfgInfo* pT_LogicTriggerCfg = new T_LogicTriggerCfgInfo;
			pT_LogicTriggerCfg->m_nCfgID = atoi(t.fieldValue(0));
			PKStringHelper::Safe_StrNCpy(pT_LogicTriggerCfg->m_szCfgName, t.fieldValue(1), sizeof(pT_LogicTriggerCfg->m_szCfgName));
			//Safe_CopyString(pT_LogicTriggerCfg->m_szGetVarValueDll, t.fieldValue(2), sizeof(pT_LogicTriggerCfg->m_szGetVarValueDll));
			//����̬�����ָ�ΪȫСд
			const char* pszDllName = t.fieldValue(2);
			int nLen = (strlen(pszDllName) <= ICV_LONGFILENAME_MAXLEN - 1) ? strlen(pszDllName) : (ICV_LONGFILENAME_MAXLEN - 1);
			std::transform(pszDllName, pszDllName+nLen, pT_LogicTriggerCfg->m_szGetVarValueDll, (int(*)(int))tolower);
			pT_LogicTriggerCfg->m_szGetVarValueDll[nLen] = '\0';	// ע���ַ���������			
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
 *  ��ʼ������.
 *
 *
 *  @version     06/10/2008  caichunlei  Initial Version.
 *  @version     06/10/2008  caichunlei  ���ݿⲿ��ʹ�õ���ʵ��.
 *  @version     06/10/2008  caichunlei  ���ݿⲿ�ָ�Ϊ�ǵ���ʵ�֣�����ʵ�ֻ��쳣.
 */
int CCalcLogicExpr::InitCalc()
{
	PKLog.SetLogFileName("CRS_LogicTrigger");
	int nRet = ICV_SUCCESS;
	//��������
	CCRSDBOperation theCRSDBOper;
	T_LogicTriggerCfgInfoList theLogicTriggerCfgList;
	
	//�����ݿ�,��ȡ������Ϣ
	theCRSDBOper.OpenCRSDB();
	ReadLogicTriggerCfgInfos(theCRSDBOper, theLogicTriggerCfgList);

	//��һ����������Ϣ
	T_LogicTriggerCfgInfoListIterator iter(theLogicTriggerCfgList);
	while(!iter.done())
	{
		T_LogicTriggerCfgInfo* pT_LogicTriggerCfg = NULL;
		pT_LogicTriggerCfg = iter.next();

		//ͬһʱ��ֻ��һ����̬��ɱ�����
		if(pT_LogicTriggerCfg->m_bIsActive)
		{
			//���ض�̬��
			nRet = LoadGetVarValueDll(pT_LogicTriggerCfg->m_szGetVarValueDll);
			//����ѭ��
			break;
		}
		else
			iter++;
	}//while(!iter.done ())

	//�ͷ���Դ
	for (iter.first(); !iter.done(); iter ++)
	{
		delete iter.next();
	}

	theCRSDBOper.CloseCRSDB();
	//����
	return nRet;
}

/**
 *  ���ػ�ȡ�����Ķ�̬��.
 *
 *  @param  -[in]  const char*  szDllName: [��̬������]
 *
 *  @version     06/10/2008  caichunlei  Initial Version.
 *  @version     06/20/2008  caichunlei  �޸���־��Ϣ.
 */
int CCalcLogicExpr::LoadGetVarValueDll(const char* szDllName)
{
	PKLog.LogMessage(PK_LOGLEVEL_DEBUG, 
		_("CCalcLogicExpr:the dll name of acquire varable is %s!"), szDllName);
		//���ش���Դ���õĶ�̬��
		int nRetVal = m_WorkingDLL.open(szDllName);
		if(nRetVal != 0)
		{
			//��ӡ��־��Ϣ��
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
				_("CCalcLogicExpr:failed to load var proxy dll (%s)!"), szDllName);//��ȡ��������̬��(%s)ָ��ʧ��
			return EC_ICV_CRS_GETDLLFUNCFAIL;
		}

		//�õ�����Դ��Ӧ��ָ��
		m_pVarProxy = m_pfn_VarProxy_Init();
		//����
		return ICV_SUCCESS;
}

/**
 *  ��ȡ������ǰֵ�ĺ���.
 *
 *  @param  -[in]  char*  pVariName: [��������]
 *  @param  -[out]  double&  dbValue: [����ֵ]
 *
 *  @version     05/29/2008  caichunlei  Initial Version.
 */
double CCalcLogicExpr::GetDoubleVar(const char* szVarName )
{
	double dbValue = 0.f;
	return m_pfn_VarProxy_GetVar(m_pVarProxy, szVarName, &dbValue);
}

/**
 *  ������ʽ���(true��false).
 *
 *  @param  -[in,out]  nodeType*  pNodeType: [comment]
 *
 *  @version     05/29/2008  caichunlei  Initial Version.
 */
bool CCalcLogicExpr::CalculateBool(nodeType *pNodeType)
{
		//������ʽ����Ϊ0������false�����򷵻�true
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

