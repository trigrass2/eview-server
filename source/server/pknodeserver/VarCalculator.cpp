/**************************************************************
 *  Filename:    SockHandler.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: SockHandler.cpp
 *
 *  @author:     shijunpu
 *  @version     05/29/2012  shijunpu  Initial Version
**************************************************************/

#include "VarCalculator.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include <string>
#include <ace/OS_NS_strings.h>
#include "ByteOrder.h"
#include "DataProcTask.h"

/**
 *  ���캯��.
 *
 *  @param  -[in]  ACE_Reactor*  r: [comment]
 *
 *  @version     05/29/2008  shijunpu  Initial Version.
 */
CVarCalculator::CVarCalculator() 
{
	m_pDataProcTask = NULL;
}

/**
 *  ��������.
 *
 *  @version     05/29/2008  shijunpu  Initial Version.
 */
CVarCalculator::~CVarCalculator()
{
}


// ����ÿ��tag���ֵ�������ڱ��ء��������̻��㡢������α��������жϱ���
// ÿһ��Ҫ����ĵ��ֵ���ͣ���ת��Ϊdouble��,double��ָ��ΪszTagData�������޷�������
// �ַ�����blob������ת��
// ��Ҫ��Ϣ���Ƿ������̻��㡢�Ƿ��жϱ������Ƿ�洢��ʷ�ȣ����ڱ��ز��Ұ�
int CVarCalculator::CalcTagValByEgu(CDataTag *pTag, int nDataType, char *szTagData, int nTagDataLen, unsigned int nSec, unsigned int mSec, unsigned int nQuality)
{
	// blob /string/ bool���͵����ݣ�����Ҫ����
	//if(nDataType != pTag->nDataType || pTag->nDataType == TAG_DT_BOOL || pTag->nDataType == TAG_DT_TEXT || pTag->nDataType == TAG_DT_BLOB)
	//{
	//	return PK_SUCCESS;
	//}

	//if(nTagDataLen != sizeof(double))
	//{
	//	return PK_SUCCESS;
	//}

	//// һ������ֵ���͵�,�ұ���洢Ϊdouble
	//if(nTagDataLen < sizeof(double))
	//{
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "[calctag] tag:%s, datalen=%d, should =8(double size)", pTag->szTagName, nTagDataLen);
	//	return PK_SUCCESS;
	//}

	//// �������÷��������ֽ���
	//SwapRawDataBytes(szTagData, nTagDataLen, pTag->nDataType, pTag->nByteOrderNo);

	//double *pdCurrent = reinterpret_cast<double *>(szTagData);
	//// ����ת�������
	//if(pTag->egu.bEnable) 
	//{
	//	if (*pdCurrent > pTag->egu.dbHigh)
	//	{
	//		*pdCurrent = pTag->egu.dbHigh;
	//	}
	//	if (*pdCurrent < pTag->egu.dbLow)
	//	{
	//		*pdCurrent = pTag->egu.dbLow;
	//	}
	//	// 1.0 egu transfer 
	//	*pdCurrent = *pdCurrent * pTag->egu.dbFactor + pTag->egu.dbDiff;
	//}

	//// 2.0 zero Pincher. �������Ƕλ
	//if (pTag->zeroPinch.bEnable)
	//{
	//	if (pTag->zeroPinch.dbCenter - pTag->zeroPinch.dbThreshold < *pdCurrent &&
	//		pTag->zeroPinch.dbCenter + pTag->zeroPinch.dbThreshold > *pdCurrent )
	//	{
	//		*pdCurrent = pTag->zeroPinch.dbCenter;
	//	}
	//}

	return PK_SUCCESS;
}

// �����ֽ��򣬽������
void CVarCalculator::SwapRawDataBytes( char *szTagData, unsigned short usDataLen, unsigned char nDataType, int nByteOrderNo)
{
	// ����ת��Ϊԭʼ���ݣ���ת��Ϊdouble����
	double *pdblTemp = (double *)szTagData;

#define SWAPBLOCKBYTES(datatype, szBuff, datalen, ucType) \
	{ \
	datatype nTemp = 0; \
	nTemp = (datatype) (* pdblTemp); \
	CByteOrder::SwapBytes(nByteOrderNo, (unsigned char*)&nTemp, datalen, nDataType); \
	*pdblTemp = (double)nTemp;  \
	}

	switch(nDataType)
	{
	case TAG_DT_INT16:
		SWAPBLOCKBYTES(short, szDataBuff, usDataLen, ucBlockType);
		break;
	case TAG_DT_UINT16:
		SWAPBLOCKBYTES(unsigned short, szDataBuff, usDataLen, ucBlockType);
		break;
	case TAG_DT_FLOAT:
		SWAPBLOCKBYTES(float, szDataBuff, usDataLen, ucBlockType);
		break;
	case TAG_DT_UINT32:
		SWAPBLOCKBYTES(ACE_UINT32, szDataBuff, usDataLen, ucBlockType);
		break;
	case TAG_DT_INT32:
		SWAPBLOCKBYTES(ACE_INT32, szDataBuff, usDataLen, ucBlockType);
		break;
	case TAG_DT_DOUBLE:
		SWAPBLOCKBYTES(double, szDataBuff, usDataLen, ucBlockType);
		break;
	case TAG_DT_UINT64:
		SWAPBLOCKBYTES(ACE_UINT64, szDataBuff, usDataLen, ucBlockType);
		break;
	case TAG_DT_INT64:
		SWAPBLOCKBYTES(ACE_INT64, szDataBuff, usDataLen, ucBlockType);
		break;
	default:
		break;
	}
}

int CVarCalculator::ProcessIndirectVars(CDataTag *pTag, int nDataType, char *szTagData, int nTagDataLen, unsigned int nSec, unsigned int mSec, unsigned int nQuality)
{
// 	for(int iTag = 0; iTag < (pTag->vecCalcTags.size()); iTag ++)
// 	{
// 		CCalcTag *pCalcTag = (CCalcTag *)pTag->vecCalcTags[iTag];	// �������
// 		if(!pCalcTag)
// 			continue;
// 
// 		int nRet = pCalcTag->CalcExpr(); // ���α���һ����double�͵�����
// 		pCalcTag->curVal.dbValue;
// 	}
	return 0;
}

