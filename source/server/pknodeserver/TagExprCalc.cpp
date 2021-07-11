/**************************************************************
 *  Filename:    SockHandler.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: SockHandler.cpp
 *
 *  @author:     shijunpu
 *  @version     05/29/2012  shijunpu  Initial Version
**************************************************************/

#include "TagExprCalc.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "PKNodeServer.h"
CTagExprCalc::CTagExprCalc()
{
	m_mapName2Tag.clear();
}

double CTagExprCalc::GetVarValue(const char* szVar )
{
	map<string, CDataTag*>::iterator itTag = m_mapName2Tag.find(szVar);
	if(itTag == m_mapName2Tag.end())
		return 0.f;

	CDataTag *pDataTag = itTag->second;
	
	if (pDataTag->curVal.nQuality != 0)
	{
		char szTip[256] = {0};
		sprintf(szTip, "var:%s,quality is bad:%d", szVar, pDataTag->curVal.nQuality);
		throw TagException(EXPRCALC_EXCEPT_QUALITY_BAD, szTip);
	}

	if (pDataTag->nDataTypeClass == DATATYPE_CLASS_INTEGER)
		return pDataTag->curVal.nValue;
	else if (pDataTag->nDataTypeClass == DATATYPE_CLASS_REAL)
		return pDataTag->curVal.dbValue;
	else
		return 0.f;

}

bool CTagExprCalc::ValidateVarName( const char* szVar )
{
	if (szVar == 0 || strlen(szVar) == 0)
	{
		m_strErrMsg << _("empty variable name\t");
		return false;
	}
	
	return true;
} 

CTagExprCalc::~CTagExprCalc()
{

}
