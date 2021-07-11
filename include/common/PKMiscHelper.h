/**************************************************************
 *  Filename:    pkcomm.h
 *  Copyright:   Shanghai Peakinfo Co., Ltd.
 *
**************************************************************/
#ifndef _EVIEW_MISKHELPER_H_
#define _EVIEW_MISKHELPER_H_


class PKMiscHelper  
{
public:
	static int GetTagDataTypeAndLen(const char *szDataype, const char *szDataLen, int *pnDataTypeId, int *pnLenBits);
	static int GetDataTypeId(const char *szDataype);
	static int GetTagDataTypeLen(int nDataype, int nDataTypeLen);
	static int CastDataFromASCII2Bin(char *szTagValue, int nDataType, char *szDestTagData, int nDestTagBufLen, int *pnDestTagDataLen);
	static int CastDataFromBin2ASCII(char *szSrcData, int nSrcSize, int nDataType, char *szDestValue, int nDestValueBufLen);
	static double CastDataFromBin2Double(char *pSrcData, int nSrcSize, int nDataType);
	static int CastDataFromDouble2Bin( double dDouble, unsigned char nDataType, char* pValue, int *pnDataTypeLen);
};

#endif // _PK_COMM_H_
