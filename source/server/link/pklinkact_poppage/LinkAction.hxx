/**************************************************************
 *  Filename:    pdbaction.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �������ݿ���ģ��.
 *
 *  @author:     caichunlei
 *  @version     08/22/2008  caichunlei  Initial Version
**************************************************************/
#if !defined(AFX_PDBACTION_HXX__6199A96D_43BA_45A2_8BD1_73528155F701__INCLUDED_)
#define AFX_PDBACTION_HXX__6199A96D_43BA_45A2_8BD1_73528155F701__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "pkmemdbapi/pkmemdbapi.h"

//�������ƶ���ģ������

class CLinkAction
{
public:
	CLinkAction();
	virtual ~CLinkAction();

	CMemDBClientAsync m_redisPublish;

	//����ִ�����ʼ������
	int	InitAction();
	//����������
    int	ExecuteAction(char *szActionId, char *szActionParam, char *szActionParam2, char *szActionParam3, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen);
	
	//��Դ�ͷź���
	int	ExitAction();
};

#endif // !defined(AFX_PDBACTION_HXX__6199A96D_43BA_45A2_8BD1_73528155F701__INCLUDED_)
