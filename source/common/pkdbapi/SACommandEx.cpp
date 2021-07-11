/**************************************************************
 *  Filename:    SACommandEx.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: SACommand扩展类的实现文件.
 *
 *  @author:     zhangqiang
 *  @version     10/26/2012  zhangqiang  Initial Version
**************************************************************/
#include "SACommandEx.h"

long SACommandEx::GetResultSetLength()
{
	return m_resultSetLength;
}

void SACommandEx::SetResultSetLength(long resultSetLength)
{
	m_resultSetLength = resultSetLength;
}