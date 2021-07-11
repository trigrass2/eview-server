/**************************************************************
*  Filename:    SockHandler.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: EAProcessor.h.
*
*  @author:     shijupu
*  @version     05/29/2015  shijupu  Initial Version
**************************************************************/

#ifndef _VAR_CALCULATOR_H_
#define _VAR_CALCULATOR_H_
#include "PKNodeServer.h"

class CDataProcTask;
class CVarCalculator
{
public:
	CVarCalculator() ;
	virtual ~CVarCalculator();
	CDataProcTask *	m_pDataProcTask;
public:
	int CalcTagValByEgu(CDataTag *pTag, int nDataType, char *szTagData, int nTagDataLen, unsigned int nSec, unsigned int mSec, unsigned int nQuality);
	int ProcessIndirectVars(CDataTag *pTag, int nDataType, char *szTagData, int nTagDataLen, unsigned int nSec, unsigned int mSec, unsigned int nQuality);
	void SwapRawDataBytes(char *szTagData, unsigned short usDataLen, unsigned char nDataType, int nByteOrderNo);
};

#endif // _VAR_CALCULATOR_H_
