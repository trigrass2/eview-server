/**************************************************************
*  Filename:    SockHandler.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: EAProcessor.h.
*
*  @author:     shijupu
*  @version     05/29/2015  shijupu  Initial Version
**************************************************************/

#ifndef _TAG_EXPR_CALC_H_
#define _TAG_EXPR_CALC_H_

#include "exprcalc/ExprCalc.h"
#include <map>
#include <string>
#include <vector>

class CDataTag;
// ���ʽ���㡣��֧�ֵ��߳�
class CTagExprCalc:public CExprCalc
{
public:
	CTagExprCalc();
	virtual ~CTagExprCalc();

	virtual bool ValidateVarName(const char*);
	virtual double GetVarValue(const char* szVar);
public:
	map<string, CDataTag*>	m_mapName2Tag; 
};


#endif // _TAG_EXPR_CALC_H_
