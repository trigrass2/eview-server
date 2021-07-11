/**************************************************************
 *  Filename:    logicvar.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �߼����ʽ����.
 *
 *  @author:     caichunlei
 *  @version     08/13/2008  caichunlei  Initial Version
**************************************************************/
#if !defined(AFX_LOGICVAR_HXX__68B50276_7E32_4BEC_BCAE_639DC9D992AE__INCLUDED_)
#define AFX_LOGICVAR_HXX__68B50276_7E32_4BEC_BCAE_639DC9D992AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ace/Containers.h"
#include "common/eviewdefine.h"

class CLogicVar  
{
public:
	CLogicVar();
	virtual ~CLogicVar();

	//��������(3�λ�4��ʽ)
	char	m_szVarFullName[ICV_LONGOBJTAGNAME_MAXLEN];
};

typedef ACE_DLList<CLogicVar> CLogicVarList;
typedef ACE_DLList_Iterator<CLogicVar> CLogicVarListIterator;
#endif // !defined(AFX_LOGICVAR_HXX__68B50276_7E32_4BEC_BCAE_639DC9D992AE__INCLUDED_)
