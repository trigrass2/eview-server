/**************************************************************
*  Filename:    LogicVar.cxx
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 逻辑表达式变量.
*
*  @author:     caichunlei
*  @version     08/13/2008  caichunlei  Initial Version
**************************************************************/
#include "logicvar.hxx"

/**
*  构造函数.
*
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
CLogicVar::CLogicVar()
{
	memset(m_szVarFullName, 0x00, ICV_LONGOBJTAGNAME_MAXLEN);
}

/**
*  析构函数.
*
*
*  @version     08/13/2008  caichunlei  Initial Version.
*/
CLogicVar::~CLogicVar()
{
	
}
