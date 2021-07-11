#include "ace/OS_NS_strings.h"
#include <ace/Dirent.h>
#include "ace/Default_Constants.h"
#include <algorithm>

#include "pkcomm/pkcomm.h"
#include "eviewcomm/eviewcomm.h"
#include "common/PKMiscHelper.h"
#include <stdint.h>
#include "ace/OS.h"

// 为了兼容性，定义常用的类型定义
#define DT_NAME_BOOLEAN					"boolean"	// 1 bit value
#define DT_NAME_BYTE					"byte"		// 8 bit unsigned integer value==uchar
#define DT_NAME_SHORT					"short"		// 16 bit Signed Integer value
#define DT_NAME_USHORT					"ushort"	// 16 bit UnSigned Integer value
#define DT_NAME_UNSIGNEDSHORT			"unsigned short"	// 16 bit UnSigned Integer value
#define DT_NAME_INT						"int"		// 32 bit Signed Integer value
#define DT_NAME_REAL					"real"		// 32 bit IEEE float
#define DT_NAME_STRING					"string"	// ASCII strisng, maximum: 127,和text相同
#define DT_NAME_TIME					"time"		// 64 bit cv time (second + usecond)
#define DT_NAME_TIME_HMST				"timehmst"	// 4 byte TIME (H:M:S:T)

// 对于字符串和BLOB类型的，不固定长度，返回传入的长度szDataLen *8
int PKMiscHelper::GetTagDataTypeAndLen(const char *szDataype, const char *szDataLen, int *pnDataTypeId, int *pnLenBits)
{
	char nDataType = 0;
	int nLenBits = 0;
	if(PKStringHelper::StriCmp(szDataype, DT_NAME_BOOL) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_BOOLEAN) == 0)
	{
		nDataType = TAG_DT_BOOL;
		nLenBits = 1;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_CHAR) == 0)
	{
		nDataType = TAG_DT_CHAR;
		nLenBits = 8;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UCHAR) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_BYTE) == 0)
	{
		nDataType = TAG_DT_UCHAR;
		nLenBits = 8;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT16) == 0|| PKStringHelper::StriCmp(szDataype, DT_NAME_SHORT) == 0)
	{
		nDataType = TAG_DT_INT16;
		nLenBits = 16;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT16) == 0|| PKStringHelper::StriCmp(szDataype, DT_NAME_USHORT) == 0|| PKStringHelper::StriCmp(szDataype, DT_NAME_UNSIGNEDSHORT) == 0)
	{
		nDataType = TAG_DT_UINT16;
		nLenBits = 16;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT32) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_INT) == 0)
	{
		nDataType = TAG_DT_INT32;
		nLenBits = 32;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT32) == 0)
	{
		nDataType = TAG_DT_UINT32;
		nLenBits = 32;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT64) == 0)
	{
		nDataType = TAG_DT_INT64;
		nLenBits = 64;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT64) == 0)
	{
		nDataType = TAG_DT_UINT64;
		nLenBits = 64;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_FLOAT) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_REAL) == 0)
	{
		nDataType = TAG_DT_FLOAT;
		nLenBits = 32;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_DOUBLE) == 0)
	{
		nDataType = TAG_DT_DOUBLE;
		nLenBits = 64;
	}
	else if (PKStringHelper::StriCmp(szDataype, DT_NAME_FLOAT16) == 0)
	{
		nDataType = TAG_DT_FLOAT16;
		nLenBits = 16;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_TEXT) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_STRING) == 0)
	{
		nDataType = TAG_DT_TEXT;
		int nLenBytes = ::atoi(szDataLen);
		if(nLenBytes <= 0)
			nLenBytes = 64; // 缺省为64个字节
		nLenBits = nLenBytes * 8;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_BLOB) == 0)
	{
		nDataType = TAG_DT_BLOB;
		int nLenBytes = ::atoi(szDataLen);
		if(nLenBytes <= 0)
			nLenBytes = 64;
		nLenBits = nLenBytes * 8;
	}
	else
	{
		// 经常有忘了配置数据类型的情况！因此如果未配置我们默认为是字符串类型的
		nDataType = TAG_DT_TEXT;
		int nLenBytes = ::atoi(szDataLen);
		if(nLenBytes <= 0)
			nLenBytes = 64; // 缺省为64个字节
		nLenBits = nLenBytes * 8;

		//nDataType = TAG_DT_UNKNOWN;
		//nLenBits = 32;
	}

	*pnDataTypeId = nDataType;
	*pnLenBits = nLenBits;
	return nDataType;
}

// 对于字符串和BLOB类型的，不固定长度，返回传入的长度szDataLen *8。返回按位计算的长度。0表示无效返回
int PKMiscHelper::GetTagDataTypeLen(int nDataype, int nDataTypeLen)
{
	char nDataType = 0;
	int nLenBits = 0;
	if(nDataype== TAG_DT_BOOL)
	{
		nLenBits = 1;
	}
	else if(nDataype == TAG_DT_CHAR)
	{
		nLenBits = 8;
	}
	else if(nDataype == TAG_DT_UCHAR)
	{
		nLenBits = 8;
	}
	else if(nDataype == TAG_DT_INT16)
	{
		nLenBits = 16;
	}
	else if(nDataype == TAG_DT_UINT16)
	{
		nLenBits = 16;
	}
	else if(nDataype == TAG_DT_INT32)
	{
		nLenBits = 32;
	}
	else if(nDataype == TAG_DT_UINT32)
	{
		nLenBits = 32;
	}
	else if(nDataype == TAG_DT_INT64)
	{
		nLenBits = 64;
	}
	else if(nDataype == TAG_DT_UINT64)
	{
		nLenBits = 64;
	}
	else if(nDataype == TAG_DT_FLOAT)
	{
		nLenBits = 32;
	}
	else if(nDataype ==  TAG_DT_DOUBLE) 
	{
		nLenBits = 64;
	}
	else if(nDataype == TAG_DT_TEXT) 
	{
		nLenBits = nDataTypeLen * 8;
	}
	else if(nDataype == TAG_DT_BLOB)  
	{
		nLenBits = nDataTypeLen * 8;
	}
	else
	{
		nLenBits = 32;
	}

	return nLenBits;
}

int PKMiscHelper::GetDataTypeId(const char *szDataype)
{
	char nDataType = 0;
	if(PKStringHelper::StriCmp(szDataype, DT_NAME_BOOL) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_BOOLEAN) == 0)
	{
		nDataType = TAG_DT_BOOL;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_CHAR) == 0)
	{
		nDataType = TAG_DT_CHAR;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UCHAR) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_BYTE) == 0)
	{
		nDataType = TAG_DT_UCHAR;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT16) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_SHORT) == 0)
	{
		nDataType = TAG_DT_INT16;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT16) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_USHORT) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_UNSIGNEDSHORT) == 0)
	{
		nDataType = TAG_DT_UINT16;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT32) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_INT) == 0)
	{
		nDataType = TAG_DT_INT32;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT32) == 0)
	{
		nDataType = TAG_DT_UINT32;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_INT64) == 0)
	{
		nDataType = TAG_DT_INT64;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_UINT64) == 0)
	{
		nDataType = TAG_DT_UINT64;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_FLOAT) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_REAL) == 0)
	{
		nDataType = TAG_DT_FLOAT;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_DOUBLE) == 0)
	{
		nDataType = TAG_DT_DOUBLE;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_TEXT) == 0 || PKStringHelper::StriCmp(szDataype, DT_NAME_STRING) == 0)
	{
		nDataType = TAG_DT_TEXT;
	}
	else if(PKStringHelper::StriCmp(szDataype, DT_NAME_BLOB) == 0)
	{
		nDataType = TAG_DT_BLOB;
	}
	else if (PKStringHelper::StriCmp(szDataype, DT_NAME_FLOAT16) == 0)
	{
		nDataType = TAG_DT_FLOAT16;
	}
	else
	{
		nDataType = TAG_DT_UNKNOWN;
	}

	return nDataType;
}

#define SAFE_COVERT(T)	T _temp; memcpy(&_temp, szSrcData, sizeof(T));
/**
 *  Cast Type To ASCII* Buffer.
 *
 *  @param  -[in,out]  string&  str: [string]
 *  @param  -[in,out]  const char*  szSrcData: [source buffer]
 *  @param  -[in,out]  char  cSrcDataType: [src data type]
 *
 *  @version     07/09/2008  chenzhiquan  Initial Version.
 */
int PKMiscHelper::CastDataFromBin2ASCII(char *szSrcData, int nSrcSize, int nDataType, char *szDestValue, int nDestValueBufLen)
{
	const int STR_BUFFER_LENGTH = 256;
	int nLen = nSrcSize;
	if (nDestValueBufLen < nLen)
		nLen = nDestValueBufLen;

	// Modified to avoid buffer overflow
	char szRawData[STR_BUFFER_LENGTH];	
	memset(szRawData, 0, sizeof(szRawData));
	
	const char *szTemp;

	switch(nDataType)
	{
	case TAG_DT_BOOL:		// 1 bit value
		ACE_OS::snprintf(szRawData, sizeof(szRawData), "%u", (*(unsigned char *)(szSrcData)) != 0 );
		break;
	case TAG_DT_CHAR:		// 8 bit signed integer value
		ACE_OS::snprintf(szRawData, sizeof(szRawData), "%d", *(char *)(szSrcData) );
		break;
			
	case TAG_DT_INT16:		// 16 bit Signed Integer value
		{
			SAFE_COVERT(short)
			ACE_OS::snprintf(szRawData, sizeof(szRawData), "%d", _temp );
		}
		break;

	case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
		{
			SAFE_COVERT(unsigned short)
			ACE_OS::snprintf(szRawData, sizeof(szRawData), "%u", _temp);
		}
		break;

	case TAG_DT_FLOAT:		// 32 bit IEEE float
		{
			SAFE_COVERT(float)
			ACE_OS::snprintf(szRawData, sizeof(szRawData), "%f", _temp);
		}
		break;

	case TAG_DT_UCHAR:		// 8 bit unsigned integer value
		ACE_OS::snprintf(szRawData, sizeof(szRawData), "%u", *(unsigned char *)(szSrcData) );
		break;
	case TAG_DT_UINT32:		// 32 bit integer value
		{
			SAFE_COVERT(unsigned int)
			ACE_OS::snprintf(szRawData, sizeof(szRawData), "%u", _temp);
		}
		break;

	case TAG_DT_INT32:		// 32 bit signed integer value
		{
			SAFE_COVERT(int)
			ACE_OS::snprintf(szRawData, sizeof(szRawData), "%d", _temp);
		}
		break;

	case TAG_DT_DOUBLE:		// 64 bit double
		{
			SAFE_COVERT(double)
			ACE_OS::snprintf(szRawData, sizeof(szRawData), "%f", _temp);
		}
		break;

	case TAG_DT_INT64:		// 64 bit signed integer value
		//TODO:ACE_OS::snprintf(szRawData, "%d", *(__int64 *)(hNtf->szDataBuf) );
		{
			SAFE_COVERT(ACE_INT64)
			ACE_OS::snprintf(szRawData, sizeof(szRawData), "%lld", _temp);
		}
		break;

	case TAG_DT_UINT64:		// 64 bit unsigned integer value
			//TODO: ACE_OS::snprintf(szRawData, "%u", *(unsigned __int64 *) _atoi64(hNtf->szDataBuf) );
			//ACE_OS::snprintf(szRawData, "%u", *(ACE_UINT64 *) ACE_OS::atoi(hNtf->szDataBuf) );
		{
			SAFE_COVERT(ACE_UINT64)
			ACE_OS::snprintf(szRawData, sizeof(szRawData), "%llu", _temp);
		}
		break;

	case TAG_DT_BLOB:		// blob, maximum 65535
		// 修改需求，Blob转ASCII，将每一个字节转为两字节ASCII字符显示
		nLen = 0;
		do
		{
			// 无法再分配两个字节进行转换或者已经转换完成
			if (nDestValueBufLen < 2*nLen+3 || nSrcSize<=nLen)
			{
				szDestValue[2*nLen] = '\0';
				break;
			}
			ACE_OS::snprintf(szDestValue+2*nLen, nDestValueBufLen-2*nLen, "%.2X", (unsigned char)(*(szSrcData+nLen)));
			nLen++;
		}while(1);
		return PK_SUCCESS;

	case TAG_DT_TEXT:		// ASCII string, maximum: 255
	default:
		//bStringData = true;
		PKStringHelper::Safe_StrNCpy(szDestValue, szSrcData, nLen);
		if (nDestValueBufLen > nLen)
			szDestValue[nLen] = 0;
		else 
			szDestValue[nDestValueBufLen-1] = 0;

		return PK_SUCCESS;
	}

	PKStringHelper::Safe_StrNCpy(szDestValue, szRawData, nDestValueBufLen);
	return PK_SUCCESS;
}

/**************************************************************************** 
函数名称: str_to_hex 
函数功能: 字符串转换为十六进制 
输入参数: string 字符串 cbuf 十六进制 len 字符串的长度。 
输出参数: 无 
*****************************************************************************/   
static int str_to_hex(char *string, unsigned char *cbuf, int len)  
{  
	unsigned char high, low;  
	int idx, ii=0;  
	for (idx=0; idx<len; idx+=2)   
	{  
		high = string[idx];  
		low = string[idx+1];  

		if(high>='0' && high<='9')  
			high = high-'0';  
		else if(high>='A' && high<='F')  
			high = high - 'A' + 10;  
		else if(high>='a' && high<='f')  
			high = high - 'a' + 10;  
		else  
			return -1;  

		if(low>='0' && low<='9')  
			low = low-'0';  
		else if(low>='A' && low<='F')  
			low = low - 'A' + 10;  
		else if(low>='a' && low<='f')  
			low = low - 'a' + 10;  
		else  
			return -1;  

		cbuf[ii++] = high<<4 | low;  
	}  
	return 0;  
}  
// 将tag数据从ASCII转换为二进制。Blob转ASCII，将每一个字节转为两字节ASCII字符显示
int PKMiscHelper::CastDataFromASCII2Bin(char *szTagValue, int nDataType, char *szDestTagData, int nDestTagBufLen, int *pnDestTagDataLen)
{
	int nTagDataLen = strlen(szTagValue);
	if(nDataType == TAG_DT_BLOB) // base64
		nTagDataLen = nTagDataLen * 3.0f / 4;

	if(nDataType == TAG_DT_UNKNOWN)
	{
		szDestTagData[0] = '\0';
		*pnDestTagDataLen = 0;
		return -1;
	}

	if(nDataType == TAG_DT_TEXT)
	{
		*pnDestTagDataLen = nTagDataLen;
		if(*pnDestTagDataLen > nDestTagBufLen - 1)
			*pnDestTagDataLen = nDestTagBufLen;
		memcpy(szDestTagData, szTagValue, *pnDestTagDataLen);
		szDestTagData[*pnDestTagDataLen] = '\0'; // 留下1个充当结束字符
		return 0;
	}
	// Blob转Bin，将每一个字节转为两字节ASCII字符显示,中间不留空格.ASC为大写
	if(nDataType == TAG_DT_BLOB)
	{
		*pnDestTagDataLen = nTagDataLen/2;
		if(*pnDestTagDataLen > nDestTagBufLen - 1)
			*pnDestTagDataLen = nDestTagBufLen - 1;
		str_to_hex(szTagValue, (unsigned char*)szDestTagData, *pnDestTagDataLen);
		szDestTagData[*pnDestTagDataLen] = '\0'; // 留下1个充当结束字符
		return 0;
	}

	switch(nDataType)
	{
	case TAG_DT_CHAR:		// 8 bit signed integer value
		{
			char uValue = ::atoi(szTagValue);
			memcpy(szDestTagData, &uValue, sizeof(char));
			*pnDestTagDataLen = sizeof(char);
		}
		break;

	case TAG_DT_INT16:		// 16 bit Signed Integer value
		{
			*pnDestTagDataLen = sizeof(short);
			short uValue = ::atoi(szTagValue);
			memcpy(szDestTagData, &uValue, sizeof(short));
			break;
		}
	case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
		{
			unsigned short uValue = ::atoi(szTagValue);
			*pnDestTagDataLen = sizeof(unsigned short);
			memcpy(szDestTagData, &uValue, *pnDestTagDataLen);
			break;
		}
	case TAG_DT_FLOAT:		// 32 bit IEEE float	
		{
			float fValue = ::atof(szTagValue);
			*pnDestTagDataLen = sizeof(float);
			memcpy(szDestTagData, &fValue, *pnDestTagDataLen);
			break;
		}
		break;

	case TAG_DT_BOOL:
	case TAG_DT_UCHAR:		// 1 bit value
		{
			unsigned char uValue = ::atoi(szTagValue);
			*pnDestTagDataLen = sizeof(unsigned char);
			memcpy(szDestTagData, &uValue, *pnDestTagDataLen);
			break;
		}
		break;

	case TAG_DT_UINT32:		// 32 bit integer value
		{
			ACE_UINT32 uValue = ::atoi(szTagValue);
			*pnDestTagDataLen = sizeof(ACE_UINT32);
			memcpy(szDestTagData, &uValue, *pnDestTagDataLen);
			break;
		}

	case TAG_DT_INT32:		// 32 bit signed integer value
		{
			ACE_INT32 uValue = ::atoi(szTagValue);
			*pnDestTagDataLen = sizeof(ACE_INT32);
			memcpy(szDestTagData, &uValue, *pnDestTagDataLen);
			break;
		}

	case TAG_DT_DOUBLE:		// 64 bit double
		{
			double uValue = ::atof(szTagValue);
			*pnDestTagDataLen = sizeof(double);
			memcpy(szDestTagData, &uValue, *pnDestTagDataLen);
			break;
		}

	case TAG_DT_INT64:		// 64 bit signed integer value
		{
			ACE_INT64 uValue = ::atoi(szTagValue);
			*pnDestTagDataLen = sizeof(ACE_INT64);
			memcpy(szDestTagData, &uValue, *pnDestTagDataLen);
			break;
		}

	case TAG_DT_UINT64:		// 64 bit unsigned integer value
		{
			ACE_UINT64 uValue = ::atoi(szTagValue);
			*pnDestTagDataLen = sizeof(ACE_UINT64);
			memcpy(szDestTagData, &uValue, *pnDestTagDataLen);
			break;
		}
	default:
		szDestTagData[0] = '\0';
		*pnDestTagDataLen = 0;
		return -1;
	}

	return 0;
}

// HP UNIX下，64位下指针强制转换必须能被8整除
#define SAFE_COVERT_RETDOUBLE(T)	{T _temp; 	memcpy(&_temp, pSrcData, sizeof(T)); return (double)_temp;}

double PKMiscHelper::CastDataFromBin2Double(char *pSrcData, int nSrcSize, int nDataType)
{
	switch(nDataType)
	{
	case TAG_DT_CHAR:		// 8 bit signed integer value
		SAFE_COVERT_RETDOUBLE(char)
	case TAG_DT_INT16:		// 16 bit Signed Integer value
		SAFE_COVERT_RETDOUBLE(short)
	case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
		SAFE_COVERT_RETDOUBLE(unsigned short)

	case TAG_DT_FLOAT:		// 32 bit IEEE float
		SAFE_COVERT_RETDOUBLE(float)

	case TAG_DT_BOOL:
	case TAG_DT_UCHAR:		// 1 bit value
		SAFE_COVERT_RETDOUBLE(unsigned char);

	case TAG_DT_UINT32:		// 32 bit integer value
		SAFE_COVERT_RETDOUBLE(unsigned int);

	case TAG_DT_INT32:		// 32 bit signed integer value
		SAFE_COVERT_RETDOUBLE(unsigned int)

	case TAG_DT_DOUBLE:		// 64 bit double
		SAFE_COVERT_RETDOUBLE(double)
	case TAG_DT_INT64:		// 64 bit signed integer value
		SAFE_COVERT_RETDOUBLE(ACE_INT64)

	case TAG_DT_UINT64:		// 64 bit unsigned integer value
		SAFE_COVERT_RETDOUBLE(ACE_UINT64)
	default:
		return 0;
	}
}


int PKMiscHelper::CastDataFromDouble2Bin( double dDouble, unsigned char nDataType, char* pValue, int *pnDataTypeLen)
{
	switch(nDataType)
	{
	case TAG_DT_CHAR:		// 8 bit signed integer value
		*(char *)(pValue) = (char)dDouble;
		*pnDataTypeLen = sizeof(char);
		break;

	case TAG_DT_INT16:		// 16 bit Signed Integer value
		*(short *)(pValue) = (short)dDouble;
		*pnDataTypeLen = sizeof(short);
		break;
	case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
		*(unsigned short *)(pValue) = (unsigned short)dDouble;
		*pnDataTypeLen = sizeof(unsigned short);
		break;
	case TAG_DT_FLOAT:		// 32 bit IEEE float	
		*(float *)(pValue) = (float)dDouble;
		*pnDataTypeLen = sizeof(float);
		break;

	case TAG_DT_BOOL:
	case TAG_DT_UCHAR:		// 1 bit value
		*(unsigned char *)(pValue) = (unsigned char)dDouble;
		*pnDataTypeLen = sizeof(unsigned char);
		break;

	case TAG_DT_UINT32:		// 32 bit integer value
		*(ACE_UINT32 *)(pValue) = (unsigned int)dDouble;
		*pnDataTypeLen = sizeof(unsigned int);
		break;

	case TAG_DT_INT32:		// 32 bit signed integer value
		*(ACE_INT32 *)(pValue) = (int)dDouble;
		*pnDataTypeLen = sizeof(ACE_INT32);
		break;

	case TAG_DT_DOUBLE:		// 64 bit double
		*(double *)(pValue) = (double)dDouble;
		*pnDataTypeLen = sizeof(double);
		break;

	case TAG_DT_INT64:		// 64 bit signed integer value
		*(ACE_INT64 *)(pValue) = (ACE_INT64)dDouble;
		*pnDataTypeLen = sizeof(ACE_INT64);
		break;

	case TAG_DT_UINT64:		// 64 bit unsigned integer value
		*(ACE_UINT64 *)(pValue) = (ACE_UINT64)dDouble;
		*pnDataTypeLen = sizeof(ACE_UINT64);
		break;
	default:
		*pnDataTypeLen = 0;
		return -1;
	}

	return 0;
}
