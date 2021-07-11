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
 *  构造函数.
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
 *  析构函数.
 *
 *  @version     05/29/2008  shijunpu  Initial Version.
 */
CVarCalculator::~CVarCalculator()
{
}


// 计算每个tag点的值，缓存在本地。进程量程换算、计算二次变量，并判断报警
// 每一个要计算的点的值类型，都转换为double型,double的指针为szTagData，后面无法保存了
// 字符串和blob不进行转换
// 需要信息：是否做量程换算、是否判断报警、是否存储历史等，现在本地查找吧
int CVarCalculator::CalcTagValByEgu(CDataTag *pTag, int nDataType, char *szTagData, int nTagDataLen, unsigned int nSec, unsigned int mSec, unsigned int nQuality)
{
	// blob /string/ bool类型的数据，不需要保存
	//if(nDataType != pTag->nDataType || pTag->nDataType == TAG_DT_BOOL || pTag->nDataType == TAG_DT_TEXT || pTag->nDataType == TAG_DT_BLOB)
	//{
	//	return PK_SUCCESS;
	//}

	//if(nTagDataLen != sizeof(double))
	//{
	//	return PK_SUCCESS;
	//}

	//// 一定是数值类型的,且必须存储为double
	//if(nTagDataLen < sizeof(double))
	//{
	//	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "[calctag] tag:%s, datalen=%d, should =8(double size)", pTag->szTagName, nTagDataLen);
	//	return PK_SUCCESS;
	//}

	//// 按照配置方案交换字节序
	//SwapRawDataBytes(szTagData, nTagDataLen, pTag->nDataType, pTag->nByteOrderNo);

	//double *pdCurrent = reinterpret_cast<double *>(szTagData);
	//// 量程转换与计算
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

	//// 2.0 zero Pincher. 计算零点嵌位
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

// 交换字节序，交换完毕
void CVarCalculator::SwapRawDataBytes( char *szTagData, unsigned short usDataLen, unsigned char nDataType, int nByteOrderNo)
{
	// 首先转换为原始数据，再转换为double类型
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
// 		CCalcTag *pCalcTag = (CCalcTag *)pTag->vecCalcTags[iTag];	// 计算变量
// 		if(!pCalcTag)
// 			continue;
// 
// 		int nRet = pCalcTag->CalcExpr(); // 二次变量一定是double型的数据
// 		pCalcTag->curVal.dbValue;
// 	}
	return 0;
}

