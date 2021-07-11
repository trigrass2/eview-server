/**************************************************************
 *  Filename:    logictriggeragent.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �¼�����Դ�����࣬�������������¼�����Դ��Ϣ.
 *
 *  @author:     caichunlei
 *  @version     05/21/2008  caichunlei  Initial Version
**************************************************************/
#if !defined(AFX_LOGICTRIGGERAGENT_HXX__C0E48E9D_4849_4E1C_BF1A_012392718073__INCLUDED_)
#define AFX_LOGICTRIGGERAGENT_HXX__C0E48E9D_4849_4E1C_BF1A_012392718073__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "logicvar.hxx"

#include "logictrigger.hxx"

class CLogicTriggerAgent  
{
public:
	CLogicTriggerAgent();
	virtual ~CLogicTriggerAgent();

	//����¼�����Դ��Ϣ
	void	AddLogicTrigger(CLogicTrigger *pLogicTrigger);
	//�ͷ������¼�����Դ��Ϣ
	void	DestoryLogicTriggerList();
	//��������¼�����Դ�Ƿ����㴥������
	void	CheckAllTriggers();
	//�õ������б�
	void	GetVarList(CLogicVarList* pLogicVarList);
	// �õ��߼����ʽ�б�
	CLogicTriggerList *GetLogicTriggerList(){return &m_LogicTriggerList;};
private:
	//�¼�����Դ�б�
	CLogicTriggerList m_LogicTriggerList;
};

#endif // !defined(AFX_LOGICTRIGGERAGENT_HXX__C0E48E9D_4849_4E1C_BF1A_012392718073__INCLUDED_)
