/**************************************************************
 *  Filename:    logictrigger.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �¼�����Դ��.
 *
 *  @author:     caichunlei
 *  @version     05/21/2008  caichunlei  Initial Version
 *  @version     06/13/2008  caichunlei  ���¼�����Դ�ĺ궨���������ϵͳ�ĺ궨����
**************************************************************/
#if !defined(AFX_LOGICTRIGGER_HXX__23F314A9_ADE0_44FC_BFCC_7BD813E15B65__INCLUDED_)
#define AFX_LOGICTRIGGER_HXX__23F314A9_ADE0_44FC_BFCC_7BD813E15B65__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ace/Containers.h"

#include "pklog/pklog.h"
#include "errcode/ErrCode_iCV_Common.hxx"
#include "errcode/ErrCode_iCV_CRS.h"

#include "link/TriggerIntrl.h"
#include "link/linkdefine.h"

#include "link/nodedefines.hxx"
#include "logicvar.hxx"
#include "calclogicexpr.hxx"

class CLogicTrigger
{
public:
	CLogicTrigger(int nLogicType);
	virtual ~CLogicTrigger();
	//ɾ���ڵ�ָ��
	void	DeleteNodeType(nodeType *pNodeType);

	//��鴥��Դ�Ƿ����㴥������
	void	CheckTrigger();
	//���ݱ仯ʱ�Ĵ�����
	void	DataChangeLogic();
	//���ʽΪ��ʱ�ĵĴ�����
	void	OnceTrueLogic();
	//���ʽΪ��ʱ�Ĵ�����
	void	OnceFalseLogic();
	//���ʽ��Ϊ��ʱ�ĵĴ�����
	void	AlwaysTrueLogic();
	//���ʽ��Ϊ��ʱ�ĵĴ�����
	void	AlwaysFalseLogic();
	//�õ����ʽ�еı����б�
	void	GetVarList(CLogicVarList* pLogicVarList);
	//���ݸ��ڵ�õ����ʽ�еı����б�
	void	GetVarListByNodeType(nodeType *pNodeType, CLogicVarList* pLogicVarList);
	//��ӱ������Ƶ������б�
	void	AddNameToVarList(const char* pVarName, CLogicVarList* pLogicVarList);
public:
	//�¼�����Դ���߼����ʽ
	char	m_szLogicExpr[CV_LOGICEXPR_MAXLEN];
	//�¼�����Դ���ͣ���LogicType�Ķ���
	int		m_nLogicType;
	//�߼����ʽ����ʱ�䣬���¼�����Դ����ΪLOGIC_ALWAYSTRUE��LOGIC_ALWAYSFALSEʱ��Ч
	int		m_nKeepTime;
	//�����߼����ʽ�ı���
	CCalcLogicExpr*	m_pCalcLogicExpr;

	//���ʽ��Ӧ�Ľڵ�����ָ��
	nodeType*	m_pNodeType;
	//���ʽ�жϽ��
	bool	m_bCheckResult;
	//���α��ʽ����ֵ
	double	m_dCurrValue;
	//�ϴα��ʽ����ֵ
	double	m_dLastValue;
	//�ϴ��߼����ʽ�ж��Ƿ�ɹ�
	bool	m_bLastCalcRet;
	//�Ƿ�������Ϊ�����Ϊ��
	bool	m_bIsSatisfy;
	//��Ϊ�����Ϊ�ٵĿ�ʼʱ��
	time_t	m_lSatisfyTime;

public:
	//����ԴID
	int		m_nEventID;
	//����Դ����
	char	m_szTriggerName[CV_CRSCOMMNAME_MAXLEN];
	//����Դ����
	char	m_szTriggerDesc[CV_CRSDESC_MAXLEN];
	
};

typedef ACE_DLList<CLogicTrigger> CLogicTriggerList;
typedef ACE_DLList_Iterator<CLogicTrigger> CLogicTriggerListIterator;

#endif // !defined(AFX_LOGICTRIGGER_HXX__23F314A9_ADE0_44FC_BFCC_7BD813E15B65__INCLUDED_)
