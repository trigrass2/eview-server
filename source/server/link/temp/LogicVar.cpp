/**************************************************************
*  Filename:    LogicVar.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �߼����ʽ����.
*
*  @author:     caichunlei
*  @version     08/13/2008  caichunlei  Initial Version
**************************************************************/
#include "logicvar.hxx"

/**
*  ���캯��.
*
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
CLogicVar::CLogicVar()
{
	memset(m_szVarFullName, 0x00, ICV_LONGOBJTAGNAME_MAXLEN);
}

/**
*  ��������.
*
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
CLogicVar::~CLogicVar()
{
	
}
