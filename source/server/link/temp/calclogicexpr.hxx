/**************************************************************
 *  Filename:    calclogicexpr.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ���ݼ����߼����ʽ��Ӧ�Ľڵ����ͼ�����ʽ���.
 *
 *  @author:     caichunlei
 *  @version     05/29/2008  caichunlei  Initial Version
 *  @version     06/10/2008  caichunlei  ��ӳ�ʼ�����������ز�ͬ�Ķ�̬��
**************************************************************/
#if !defined(AFX_CALCLOGICEXPR_HXX__AB278977_A851_4BF4_9DDF_F23F44D20754__INCLUDED_)
#define AFX_CALCLOGICEXPR_HXX__AB278977_A851_4BF4_9DDF_F23F44D20754__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ace/Task.h"

#include "errcode/ErrCode_iCV_Common.hxx"
#include "errcode/ErrCode_iCV_CRS.h"
#include "pklog/pklog.h"

#include "link/linkdb.h"
#include "link/VarProxyIntrl.h"

//#include "calcexpr.hxx"
#include "exprcalc/OpCalcNumerical.h"
#define CV_CRSCOMMNAME_MAXLEN	128
#define CV_LOGICEXPR_MAXLEN		128
#define CV_CRSDESC_MAXLEN	128
typedef struct
{	
	//������ϢID
	int		m_nCfgID;
	//������Ϣ����
	char	m_szCfgName[CV_CRSCOMMNAME_MAXLEN];
	//��ȡ����ֵ�Ķ�̬������
	char	m_szGetVarValueDll[ICV_LONGFILENAME_MAXLEN];
	//ʹ�øö�̬��
	bool	m_bIsActive;
}T_LogicTriggerCfgInfo;

typedef ACE_DLList<T_LogicTriggerCfgInfo> T_LogicTriggerCfgInfoList;
typedef ACE_DLList_Iterator<T_LogicTriggerCfgInfo> T_LogicTriggerCfgInfoListIterator;

class CCalcLogicExpr: public COpCalcNumerical  
{
public:
	CCalcLogicExpr();
	virtual ~CCalcLogicExpr();

	// �̳���COpCalcNumerical
public:
	//��ȡ������ǰֵ�ĺ���
	virtual double GetDoubleVar(const char* szVar);

	// ���к���
	//��ʼ������
	int		InitCalc();
	//���ػ�ȡ�����Ķ�̬��
	int		LoadGetVarValueDll(const char* szDllName);

	long	FiniTriggerType();

    //��ȡ�¼�����Դ��ȡ����ֵ��������Ϣ.
	int		ReadLogicTriggerCfgInfos(CCRSDBOperation&, T_LogicTriggerCfgInfoList&);
	bool CalculateBool(nodeType *pNodeType);
public:
	//ACE�Ķ�̬�⣬�������ض�̬��õ�����ֵ
	ACE_DLL m_WorkingDLL;
	//��ȡ�����Ĵ�����
	//CVarProxy*	m_pVarProxy;
	HVAR_PROXY	m_pVarProxy;
	PFN_VarProxy_Init	m_pfn_VarProxy_Init;
	PFN_VarProxy_RegVar m_pfn_VarProxy_RegVar;
	PFN_VarProxy_Exit	m_pfn_VarProxy_Fini;
	PFN_VarProxy_GetVar m_pfn_VarProxy_GetVar;
	PFN_VarProxy_Start  m_pfn_VarProxy_Start;
};

#endif // !defined(AFX_CALCLOGICEXPR_HXX__AB278977_A851_4BF4_9DDF_F23F44D20754__INCLUDED_)
