/************************************************************************
 *	Filename:		CVDRIVERCOMMON.CPP
 *	Copyright:		Shanghai Peak InfoTech Co., Ltd.
 *
 *	Description:	$(Desp) .
 *
 *	@author:		shijunpu
 *	@version		??????	shijunpu	Initial Version
 *  @version	9/17/2012  shijunpu  ��������Ԫ�ش�С�ӿ� .
*************************************************************************/

#include "TaskGroup.h"
#include "Device.h"
#include "MainTask.h"
#include "pkdriver/pkdrvcmn.h"
#include "pklog/pklog.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "pkcomm/pkcomm.h"
#include "math.h"
#include "pkDriverCommon.h"

extern CPKLog g_logger;

#define BITS_PER_BYTE 8

#ifdef WIN32
#ifndef NO_EXCEPTION_ATTACHER
#include "common/RegularDllExceptionAttacher.h"
#pragma comment(lib, "ExceptionReport.lib")
#pragma comment(lib, "dbghelp.lib")
// global member to capture exception so as to analyze
static CRegularDllExceptionAttacher	g_RegDllExceptAttacher;
#endif//#ifndef NO_EXCEPTION_ATTACHER
#endif

#include <sstream>
#include <string>
using namespace std;

inline bool is_little_endian()
{
    const int i = 1;
    char *ch = (char*)&i;
    return ch[0] && 0x01;
}
bool g_bSysBigEndian = !is_little_endian();

extern "C" PKDRIVER_EXPORTS int Drv_Connect(PKDEVICE *pDevice, int nTimeOutMS)// ��������ʱ��Ҫ����
{
    if(pDevice == NULL || pDevice->_pInternalRef == NULL)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "�����豸ʧ��, �����豸���Ϊ��");
        return -2;
    }

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
	return pCDevice->CheckAndConnect(nTimeOutMS);
}

PKDRIVER_EXPORTS int Drv_Disconnect(PKDEVICE *pDevice)// ��������ʱ��Ҫ�����Ͽ����ӣ��Ա���������
{
    if(pDevice == NULL || pDevice->_pInternalRef == NULL)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "�Ͽ��豸ʧ��, �����豸���Ϊ��");
        return -2;
    }

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
    int nRet = pCDevice->DisconnectFromDevice();
	pCDevice->SetDeviceConnected(false);
	return nRet;
}

// ���ط��͵��ֽڸ���
PKDRIVER_EXPORTS int Drv_Send(PKDEVICE *pDevice, const char *szBuffer, int nBufLen, int nTimeOutMS )
{
	//printf("Send 8111, len:%d, %s\n", nBufLen, pDevice->szName);
    if(pDevice == NULL)
    {
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "���豸��������ʧ��(���� %d ���ֽڣ���ʱ: %d���� ), �����豸���pDevice == NULL", nBufLen, nTimeOutMS);
        return -201;
    }
	//printf("Send 8112, len:%d, %s\n", nBufLen, pDevice->szName);
	if (pDevice->_pInternalRef == NULL)
	{
		Drv_LogMessage(PK_LOGLEVEL_ERROR, "���豸:%s ��������ʧ��(���� %d ���ֽڣ���ʱ: %d����), �����豸pDevice�ڲ�����Ϊ��", pDevice->szName, nBufLen, nTimeOutMS);
		return -202;
	}
	//printf("Send 8113, len:%d, %s\n", nBufLen, pDevice->szName);
	if (nBufLen <= 0)
	{
		Drv_LogMessage(PK_LOGLEVEL_ERROR, "���豸:%s ��������ʧ��(���� %d ���ֽڣ���ʱ: %d����), ����Ļ���������<=0", pDevice->szName, nBufLen, nTimeOutMS);
		return -203;
	}

	//printf("Send 8114, len:%d, %s\n", nBufLen, pDevice->szName);
    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
    int nSent = pCDevice->SendToDevice(szBuffer, nBufLen, nTimeOutMS);
	//printf("Send 8115, len:%d, %s\n", nBufLen, pDevice->szName);
	if (nSent > 0)
	{
		// pCDevice->SetDeviceConnected(true); ���ǵ����ڷ�ʽ�������ݲ���һ��˵�����ӳɹ���������ﲻ��������״̬ΪOK��
		//printf("Send 8116, len:%d, %s\n", nBufLen, pDevice->szName);
	}
	//printf("Send 8117, len:%d, %s\n", nBufLen, pDevice->szName);
	
	if (nSent < 0)
	{
		pCDevice->SetDeviceConnected(false);
	}

    //REMOTELOG_TASK->PostCommunicateLog(pCDevice, true, szBuffer, nBufLen, nTimeOutMS, nSent);
    return nSent;
}

// �����յ����ֽڸ���
PKDRIVER_EXPORTS int Drv_Recv(PKDEVICE *pDevice, char *szBuffer, int nBufLen, int nTimeOutMS)
{
	if (pDevice == NULL)
	{
		Drv_LogMessage(PK_LOGLEVEL_ERROR, "���豸��ȡ����ʧ��(������ %d ���ֽڣ���ʱ: %d����), �����豸���pDevice == NULL", nBufLen, nTimeOutMS);
		return -301;
	}

	if (pDevice->_pInternalRef == NULL)
	{
		Drv_LogMessage(PK_LOGLEVEL_ERROR, "���豸:%s ��ȡ����ʧ��(������ %d ���ֽڣ���ʱ: %d����), �����豸pDevice�ڲ�����Ϊ��", pDevice->szName, nBufLen, nTimeOutMS);
		return -302;
	}

	if (nBufLen <= 0)
	{
		Drv_LogMessage(PK_LOGLEVEL_ERROR, "���豸:%s ��ȡ����ʧ��(������ %d ���ֽڣ���ʱ: %d����), ����Ļ���������<=0", pDevice->szName, nBufLen, nTimeOutMS);
		return -303;
	}

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
    int nRecv = pCDevice->RecvFromDevice(szBuffer, nBufLen, nTimeOutMS);
    //if(nRecv > 0)
        //REMOTELOG_TASK->PostCommunicateLog(pCDevice, false, szBuffer, nRecv, nTimeOutMS, nRecv);
	if (nRecv > 0)
	{
		pCDevice->SetDeviceConnected(true);
	}
    return nRecv;
}

PKDRIVER_EXPORTS void Drv_ClearRecvBuffer(PKDEVICE *pDevice)
{
    if(pDevice == NULL || pDevice->_pInternalRef == NULL)
        return;

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
    pCDevice->ClearRecvBuffer();
}

int FromBase64StrToBlob(const char *szBase64Str, char *szRetBlob, int nRetBlobBufLen, int *pnBlobActLen){
    return 0;
}

int FromBlobToBase64Str(const char *szBlob, int nBlobBufLen, string &strBase64){
    return 0;
}

// szStrData��szBinData����ָ��ͬһƬ�ڴ�����
int TagValFromString2Bin(PKTAG *pTag, int nTagDataType, const char *szStrData, char *szBinData, int nBinDataBufLen, int *pnResultBinDataBits)
{
	// �ȴ����������ַ���
	int nStrDataLen = strlen(szStrData); // �ַ����ĳ���
	if(nTagDataType == TAG_DT_TEXT)
	{
		PKStringHelper::Safe_StrNCpy(szBinData, szStrData, nBinDataBufLen);
		*pnResultBinDataBits = nStrDataLen * BITS_PER_BYTE; // 4bytes 
		return 0;
	} 

	// �ٴ���������blob
	if (nTagDataType == TAG_DT_BLOB) 
	{
		int nBlobActLen = 0;
		FromBase64StrToBlob(szStrData, szBinData, nBinDataBufLen, &nBlobActLen);
		*pnResultBinDataBits = nBlobActLen * BITS_PER_BYTE; // 4bytes
		return 0;
	}

    // szStrData��szBinData����ָ��ͬһƬ�ڴ�����
    char szStrValue[PK_NAME_MAXLEN + 1] = {0}; // ��ֵ���ַ�����ʽ������128���ֽ�
    PKStringHelper::Safe_StrNCpy(szStrValue, szStrData, sizeof(szStrValue));
    memset(szBinData, 0, nBinDataBufLen);

    // ������ֵ���ַ���ת��Ϊ����������
    int nDataBits = 0;
    stringstream ssValue;
    switch(nTagDataType){
    case TAG_DT_BOOL:
    {
        unsigned char bValue = ::atoi(szStrValue) != 0; // 0 or 1
        memcpy(szBinData, &bValue, sizeof(unsigned char));
        nDataBits = 1;
        break;
    }
    case TAG_DT_CHAR:
    {
        char cValue = szStrValue[0]; // �ַ�
        memcpy(szBinData, &cValue, sizeof(char));
        nDataBits = BITS_PER_BYTE;
        break;
    }
    case TAG_DT_UCHAR:
    {
        unsigned char ucValue = szStrValue[0]; // �ַ�
        memcpy(szBinData, &ucValue, sizeof(unsigned char));
        nDataBits = BITS_PER_BYTE;
        break;
    }
    case TAG_DT_INT16:
    {
        short sValue = ::atoi(szStrValue); // signed short
        memcpy(szBinData, &sValue, sizeof(short));
        nDataBits = sizeof(short) * BITS_PER_BYTE;
        break;
    }
    case TAG_DT_UINT16:
    {
        unsigned short usValue = ::atoi(szStrValue); // signed short
        memcpy(szBinData, &usValue, sizeof(unsigned short));
        nDataBits = sizeof(unsigned short) * BITS_PER_BYTE;
        break;
    }
    case TAG_DT_INT32:
    {
        ACE_INT32 lValue = ::atol(szStrValue); // signed short
        memcpy(szBinData, &lValue, sizeof(ACE_INT32));
        nDataBits = sizeof(ACE_INT32) * BITS_PER_BYTE; // 4bytes
        break;
    }
    case TAG_DT_UINT32:
    {
		ACE_UINT32 ulValue = ::atol(szStrValue); // unsigned short
		memcpy(szBinData, &ulValue, sizeof(ACE_UINT32));
        nDataBits = sizeof(ACE_UINT32) * BITS_PER_BYTE; // 4bytes
        break;
    }
    case TAG_DT_INT64:
    {
        ACE_INT64 int64Value = 0;
        ssValue << szStrValue;
        ssValue >> int64Value;
        memcpy(szBinData, &int64Value, sizeof(ACE_INT64));
        nDataBits = sizeof(ACE_INT64) * BITS_PER_BYTE;
        break;
    }
    case TAG_DT_UINT64:
    {
        ACE_UINT64 uint64Value = 0;
        ssValue << szStrValue;
        ssValue >> uint64Value;
        memcpy(szBinData, &uint64Value, sizeof(ACE_UINT64));
        nDataBits = sizeof(ACE_UINT64) * BITS_PER_BYTE;
        break;
    }
    case TAG_DT_FLOAT:
    {
        float fValue = ::atof(szStrValue); // signed short
        memcpy(szBinData, &fValue, sizeof(float));
        nDataBits = sizeof(float) * BITS_PER_BYTE; // 4bytes
        break;
    }
    case TAG_DT_DOUBLE:
    {
        double fValue = ::atof(szStrValue); // signed short
        memcpy(szBinData, &fValue, sizeof(double));
        nDataBits = sizeof(double) * BITS_PER_BYTE; // 4bytes
        break;
    }
    default:
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "TAG(%s) unknown datatype(typeid:%d) in TagValFromString2Bin", pTag->szName, nTagDataType);
        *pnResultBinDataBits = 0;
        return -1;
    }
    *pnResultBinDataBits = nDataBits;
    return 0;
}

///nTagOffsetBits,tag������ڿ���ʼ��ַ��ƫ�ơ�
// �����������鷢�����ģ��������Ͱ��������
template<typename T>
void ConvertBin2Value(bool bDeviceIsBigEndian, const char *szBinData, T &destVal)
{
    int nTypeLen = sizeof(destVal);
    if(g_bSysBigEndian == bDeviceIsBigEndian)
        memcpy(&destVal, szBinData, nTypeLen);
    else
    {
        char szEndianConvtTmp[8];
        memcpy(szEndianConvtTmp, szBinData, nTypeLen);
        for (int  II = 0; II < nTypeLen/2; ++ II)
        {
            char chTmp = szEndianConvtTmp[nTypeLen - II - 1];
            szEndianConvtTmp[nTypeLen - II - 1] = szEndianConvtTmp[II];
            szEndianConvtTmp[II] = chTmp;
        }

        memcpy(&destVal, szEndianConvtTmp, nTypeLen);
    }
}

#define CONVERT_ENDIAN(buff,len)	{ for (int  II = 0; II < len/2; ++ II)  buff[II]=buff[len - II]; }

#define SAFE_COVERT(T)	T _temp; if(g_bSysBigEndian == bDeviceIsBigEndian) memcpy(&_temp, szBinData, sizeof(T)); else \
{char szEndianConvtTmp[8];memcpy(szEndianConvtTmp, szBinData, sizeof(T));CONVERT_ENDIAN(szEndianConvtTmp,sizeof(T)) memcpy(&_temp, szEndianConvtTmp, sizeof(T));}
extern int TagValFromBin2String(bool bDeviceIsBigEndian, int nTagDataType,const char *szInBinData, int nBinDataBufLen, int nTagOffsetBits, string &strTagValue)
{
    int nTagOffsetBytes = (int)(nTagOffsetBits / 8);
    const char *szBinData = szInBinData + nTagOffsetBytes;
    char szStrValue[2048] = {0};
    // ������ֵ���ַ���ת��Ϊ����������
    int nDataBits = 0;
    stringstream ssValue;
    switch(nTagDataType){
    case TAG_DT_BOOL:		// 1 bit value
        {
            int nTagOffsetBitOfOneByte = nTagOffsetBits % 8;
            unsigned char ucValue = (*(unsigned char *)(szBinData));
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", ((ucValue >> nTagOffsetBitOfOneByte) & 0x01) != 0 ); // ȡnTagOffsetOfOneByteλ
            strTagValue = szStrValue;
            break;
        }
    case TAG_DT_CHAR:		// 8 bit signed integer value
        {
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", *(char *)(szBinData));
            strTagValue = szStrValue;
            break;
        }
    case TAG_DT_INT16:		// 16 bit Signed Integer value
        {
            short _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(short)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", _temp );
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
        {
            unsigned short _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(unsigned short)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_FLOAT:		// 32 bit IEEE float
        {
            float _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(float)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%f", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_UCHAR:		// 8 bit unsigned integer value
        PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", *(unsigned char *)(szBinData) );
        strTagValue = szStrValue;
        break;
    case TAG_DT_UINT32:		// 32 bit integer value
        {
            unsigned int _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(unsigned int)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_INT32:		// 32 bit signed integer value
        {
            int _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(int)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_DOUBLE:		// 64 bit double
        {
            double _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(double)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%f", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_INT64:		// 64 bit signed integer value
        //TODO:PKStringHelper::Snprintf(szRawData, "%d", *(__int64 *)(hNtf->szDataBuf) );
        {
            ACE_INT64 _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(ACE_INT64)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%lld", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_UINT64:		// 64 bit unsigned integer value
        {
            ACE_UINT64 _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(ACE_UINT64)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%llu", _temp);
            strTagValue = szStrValue;
        }
        break;
    case TAG_DT_TEXT:
		PKStringHelper::Safe_StrNCpy(szStrValue, szBinData, sizeof(szStrValue));
        strTagValue = szStrValue;
        break;
    case TAG_DT_BLOB:
        FromBlobToBase64Str(szBinData, nBinDataBufLen, strTagValue);
        break;
    default:
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "unknown datatype of %d when convert value from bin to string", nTagDataType);
        return -1;
    }
    return 0;
}

int TagValBuf_ConvertEndian(bool bDeviceIsBigEndian, char *szTagName, int nTagDataType,const char *szInBinData, int nBinDataBufLen, int nTagOffsetBits)
{
    int nTagOffsetBytes = (int)(nTagOffsetBits / 8);
    const char *szBinData = szInBinData + nTagOffsetBytes;
    // ������ֵ���ַ���ת��Ϊ����������
    switch(nTagDataType){
    case TAG_DT_BOOL:		// 1 bit value
    case TAG_DT_CHAR:		// 8 bit signed integer value
    case TAG_DT_TEXT:
    case TAG_DT_BLOB:
            break;
    case TAG_DT_INT16:		// 16 bit Signed Integer value
        {
            short _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
        }
        break;

    case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
        {
            unsigned short _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
        }
        break;

    case TAG_DT_FLOAT:		// 32 bit IEEE float
        {
            float _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
        }
        break;

    case TAG_DT_UCHAR:		// 8 bit unsigned integer valu
        break;
    case TAG_DT_UINT32:		// 32 bit integer value
        {
            unsigned int _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
        }
        break;

    case TAG_DT_INT32:		// 32 bit signed integer value
        {
            int _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
        }
        break;

    case TAG_DT_DOUBLE:		// 64 bit double
        {
            double _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
        }
        break;

    case TAG_DT_INT64:		// 64 bit signed integer value
        //TODO:PKStringHelper::Snprintf(szRawData, "%d", *(__int64 *)(hNtf->szDataBuf) );
        {
            ACE_INT64 _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
        }
        break;

    case TAG_DT_UINT64:		// 64 bit unsigned integer value
        {
            ACE_UINT64 _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
        }
        break;
default:
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "tag:%s, unknown datatype of %d when convert value endian", szTagName, nTagDataType);
        return -1;
    }
    return 0;
}

// ��tag���ֵ���ɲ�ͬ������ת��Ϊdouble��blob�������ͣ������������
int TagDataFromBin2Buf_notuse(bool bDeviceIsBigEndian, int nTagDataType, const char *szInBinData, int nBinDataBufLen, int nTagOffsetBits, string &strTagValue)
{
    int nTagOffsetBytes = (int)(nTagOffsetBits / 8);
    const char *szBinData = szInBinData + nTagOffsetBytes;
    char szStrValue[2048] = {0};
    // ������ֵ���ַ���ת��Ϊ����������
    int nDataBits = 0;
    stringstream ssValue;
    switch(nTagDataType){
    case TAG_DT_BOOL:		// 1 bit value
        {
            int nTagOffsetBitOfOneByte = nTagOffsetBits % 8;
            unsigned char ucValue = (*(unsigned char *)(szBinData));
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", ((ucValue >> nTagOffsetBitOfOneByte) & 0x01) != 0 ); // ȡnTagOffsetOfOneByteλ
            strTagValue = szStrValue;
            break;
        }
    case TAG_DT_CHAR:		// 8 bit signed integer value
        {
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", *(char *)(szBinData));
            strTagValue = szStrValue;
            break;
        }
    case TAG_DT_INT16:		// 16 bit Signed Integer value
        {
            short _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(short)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", _temp );
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_UINT16:		// 16 bit Unsigned Integer value
        {
            unsigned short _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(unsigned short)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_FLOAT:		// 32 bit IEEE float
        {
            float _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(float)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%f", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_UCHAR:		// 8 bit unsigned integer value
        PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", *(unsigned char *)(szBinData) );
        strTagValue = szStrValue;
        break;
    case TAG_DT_UINT32:		// 32 bit integer value
        {
            unsigned int _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(unsigned int)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%u", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_INT32:		// 32 bit signed integer value
        {
            int _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(int)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%d", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_DOUBLE:		// 64 bit double
        {
            double _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(double)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%f", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_INT64:		// 64 bit signed integer value
        //TODO:PKStringHelper::Snprintf(szRawData, "%d", *(__int64 *)(hNtf->szDataBuf) );
        {
            ACE_INT64 _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(ACE_INT64)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%lld", _temp);
            strTagValue = szStrValue;
        }
        break;

    case TAG_DT_UINT64:		// 64 bit unsigned integer value
        {
            ACE_UINT64 _temp;
            ConvertBin2Value(bDeviceIsBigEndian, szBinData, _temp);
            //SAFE_COVERT(ACE_UINT64)
            PKStringHelper::Snprintf(szStrValue, sizeof(szStrValue), "%llu", _temp);
            strTagValue = szStrValue;
        }
        break;
    case TAG_DT_TEXT:
		PKStringHelper::Safe_StrNCpy(szStrValue, szBinData, sizeof(szStrValue));
        strTagValue = szStrValue;
        break;
    case TAG_DT_BLOB:
        FromBlobToBase64Str(szBinData, nBinDataBufLen, strTagValue);
        break;
    default:
        Drv_LogMessage(PK_LOGLEVEL_ERROR, "unknown datatype of %d when convert value from bin to string", nTagDataType);
        return -1;
    }
    return 0;
}

#define MAX_LOGTEMP_SIZE 4096
PKDRIVER_EXPORTS void Drv_LogMessage(int nLogLevel, const char *fmt,... )
{
    char szLogString[MAX_LOGTEMP_SIZE];
    va_list ap;
    va_start(ap,fmt);
    vsnprintf(szLogString,MAX_LOGTEMP_SIZE, fmt,ap);
    va_end(ap);
    g_logger.LogMessage(nLogLevel, szLogString);
}


// �������ݿ�����ݡ�״̬��ʱ���
PKDRIVER_EXPORTS int	Drv_UpdateTagsData(PKDEVICE *pDevice, PKTAG **ppTags, int nTagNum)
{
    if (NULL == pDevice || pDevice->_pInternalRef == NULL)
        return -1;

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
	return pCDevice->UpdateTagsData(ppTags, nTagNum);
}

// ����tag��ĵ�ַ�������ݿ�����ݡ�״̬��ʱ���, ���ظ��³ɹ���tag��ĳ���
PKDRIVER_EXPORTS int	Drv_UpdateTagsDataByAddress(PKDEVICE *pDevice, const char *szTagAddress, const char *szValue, int nValueLen,  unsigned int nTimeSec, unsigned short nTimeMilSec, int nQuality)
{
	PKTAG *ppTags[100000];
	int	nTagNum = Drv_GetTagsByAddr(pDevice, szTagAddress, ppTags, 100000);// ����tag��ַ�ҵ�tag������ṹ��ָ��. ���ظ���
	if (nTagNum <= 0)
		return 0;

	for (int i = 0; i < nTagNum; i++)
	{
		PKTAG *pTag = ppTags[i];
		Drv_SetTagData_Text(pTag, szValue, nTimeSec, nTimeMilSec, nQuality);
	}

	int nRet = Drv_UpdateTagsData(pDevice, ppTags, nTagNum);
	if (nRet == 0) // �ɹ��򷵻�TAG����
		return nTagNum;
	return nRet; // 0��tag��
}

//PKDRIVER_EXPORTS int Drv_GetTagStrValue(PKDEVICE *pDevice, PKTAG *pTag, char *szTagStrVal, int nTagStrValLen, int *pnQuality, unsigned int *pnTimeStampSec, unsigned short *pnTimeStampMSec)
//{
//    szTagStrVal[0] = '\0';
//    *pnQuality = -100;
//    if (NULL == pDevice || pDevice->_pInternalRef == NULL)
//        return 1;
//
//    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
//    PKTAGDATA tagData;
//    PKStringHelper::Safe_StrNCpy(tagData.szName, pTag->szName, sizeof(tagData.szName));
//    int nRet = pCDevice->GetCachedTagData(&tagData);
//    if(nRet != PK_SUCCESS)
//    {
//        if(pnQuality)
//            *pnQuality = -1;
//        if(pnTimeStampSec)
//            *pnTimeStampSec = 0;
//        if(pnTimeStampMSec)
//            *pnTimeStampMSec = 0;
//        return nRet;
//    }
//    string strTagValue;
//    nRet = TagValFromBin2String(g_bSysBigEndian, pTag->nDataType, pTag->szData, pTag->nDataLen, 0, strTagValue);
//    if(nRet != PK_SUCCESS)
//    {
//        if(pnQuality)
//            *pnQuality = -2;
//        if(pnTimeStampSec)
//            *pnTimeStampSec = 0;
//        if(pnTimeStampMSec)
//            *pnTimeStampMSec = 0;
//        return nRet;
//    }
//
//    PKStringHelper::Safe_StrNCpy(szTagStrVal, strTagValue.c_str(), nTagStrValLen);
//    if(pnQuality)
//        *pnQuality = tagData.nQuality;
//    if(pnTimeStampSec)
//        *pnTimeStampSec = tagData.nSecond;
//    if(pnTimeStampMSec)
//        *pnTimeStampMSec = tagData.nMSecond;
//    return PK_SUCCESS;
//}

PKDRIVER_EXPORTS int Drv_TagValStr2Bin(PKTAG *pTag, const char *szTagStrVal, char *szTagValBuff, int nTagValBuffLen,
    int *pnRetValBuffLenBytes, int *pnRetValBuffLenBits)
{
	g_logger.LogMessage(PK_LOGLEVEL_ERROR, "����:%s �ĺ���Drv_TagValStr2Bin������֧�֣��벻Ҫʹ��!", pTag->szName);

    int nTagValBufLenBits = 0;
    int nRet = TagValFromString2Bin(pTag, pTag->nDataType, szTagStrVal, szTagValBuff, nTagValBuffLen, &nTagValBufLenBits);
    if(pnRetValBuffLenBits)
        *pnRetValBuffLenBits = nTagValBufLenBits;

    if(pnRetValBuffLenBytes)
        *pnRetValBuffLenBytes = ceil(nTagValBufLenBits/8.0f);

    return nRet;
}

// ��ȡ���ݿ���Ϣ�����ڿ��豸���а�λ����ʱ���Ȼ�ȡtag���Ӧ�Ŀ��ֵ��Ȼ���tag������λ����
PKDRIVER_EXPORTS int Drv_GetTagData(PKDEVICE *pDevice, PKTAG *pTag, char *szValue, int nValueBufLen, int *pnRetValueLen, unsigned int *pnTimeSec, unsigned short *pnTimeMilSec, int *pnQuality)
{
    if (NULL == pDevice || pDevice->_pInternalRef == NULL)
        return 1;

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
	return pCDevice->GetCachedTagData(pTag, szValue, nValueBufLen, pnRetValueLen, pnTimeSec, pnTimeMilSec, pnQuality);
}

PKDRIVER_EXPORTS void * Drv_CreateTimer(PKDEVICE *pDevice, PKTIMER *timerInfo)
{
    if (NULL == pDevice || pDevice->_pInternalRef == NULL)
        return NULL;

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
    CUserTimer *pUserTimer = pCDevice->CreateAndStartTimer(timerInfo);
	return pUserTimer; // != NULL ? 0 : -1;
}

// ���ص��Ƕ�ʱ��������׵�ַ
PKDRIVER_EXPORTS PKTIMER * Drv_GetTimers(PKDEVICE *pDevice, int *pnTimerNum)
{
    if (NULL == pDevice || pDevice->_pInternalRef == NULL)
        return NULL;

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
    return pCDevice->GetTimers(pnTimerNum);
}

PKDRIVER_EXPORTS int Drv_DestroyTimer(PKDEVICE *pDevice, void *pTimerHandle)
{
    if (NULL == pDevice || pDevice->_pInternalRef == NULL)
        return NULL;

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
	CUserTimer *pCTimer = (CUserTimer *)pTimerHandle;
    return pCDevice->StopAndDestroyTimer(pCTimer);
}

// ����tag���ҵ�tag��ṹ��ָ�롣�����ҵ���PKTAG��ָ��
PKDRIVER_EXPORTS PKTAG*	Drv_GetTagByName(PKDEVICE *pDevice, const char *szTagName)
{
    if (NULL == pDevice || pDevice->_pInternalRef == NULL)
        return NULL;

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
    for(unsigned int i =0; i < pCDevice->m_vecAllTag.size(); i ++)
    {
        PKTAG *pTag = pCDevice->m_vecAllTag[i];
        if(strcmp(szTagName, pTag->szName) == 0)
            return pTag;
    }
    return NULL;
}

// ����tag��ַ�ҵ�tag������ṹ��ָ��. ���ظ���
PKDRIVER_EXPORTS int Drv_GetTagsByAddr(PKDEVICE *pDevice, const char *szTagAddress, PKTAG **ppTags, int nMaxTagNum)
{
    if (NULL == pDevice || pDevice->_pInternalRef == NULL)
        return 0;

    CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
	return pCDevice->GetTagsByAddr(szTagAddress, ppTags, nMaxTagNum);
    //return vecTags.size();
}

// ����ĳ��TAG��ֵ��ʱ�����������blobΪ�����ƣ��������͵�ֵ��ΪASCII �ı�����ʾ
PKDRIVER_EXPORTS int Drv_SetTagData_Text(PKTAG *pTag, const char *szValue, unsigned int nTagSec, unsigned short nTagMSec, int nTagQuality)
{
	pTag->nTimeSec = nTagSec;
	pTag->nTimeMilSec = nTagMSec;
	pTag->nQuality = nTagQuality;
	int nRet = 0;
	if (pTag->nDataType == TAG_DT_BLOB) // �洢���Ѿ��Ƕ�����������
	{
		int nValueLen = strlen(szValue);
		if (nValueLen > pTag->nDataLen)
			nValueLen = pTag->nDataLen;
		if (nValueLen < 0)
			nRet = -1001;
		else
			memcpy(pTag->szData, szValue, nValueLen);
	}
	else
	{
		// Ҫת��Ϊ����������
		int nDataTypeLenBits = pTag->nLenBits; // ������Ȳ�Ӧ�ñ��ı�
		nRet = TagValFromString2Bin(pTag, pTag->nDataType, szValue, pTag->szData, pTag->nDataLen + 1, &nDataTypeLenBits);
	}
	return nRet;
}

// ����ĳ��TAG��ֵ��ʱ�����������blobΪ�����ƣ��������͵�ֵ��ΪASCII �ı�����ʾ
PKDRIVER_EXPORTS int Drv_SetTagData_Binary(PKTAG *pTag, void *szData, int nValueLen, unsigned int nTagSec, unsigned short nTagMSec, int nTagQuality)
{
	pTag->nTimeSec = nTagSec;
	pTag->nTimeMilSec = nTagMSec;
	pTag->nQuality = nTagQuality;
	nValueLen = nValueLen > pTag->nDataLen ? pTag->nDataLen : nValueLen; // ȡСֵ
	memcpy(pTag->szData, szData, nValueLen);
	pTag->szData[nValueLen] = 0;
	return 0;
}

// �����豸��Ч����������ͨѶ����״̬��ʱ�� ������ʱ�仹δ�յ���Ϣ�����Ϊ�豸�Ͽ�
PKDRIVER_EXPORTS int Drv_SetConnectOKTimeout(PKDEVICE *pDevice, int nConnBadTimeoutSec)
{
	if (NULL == pDevice || pDevice->_pInternalRef == NULL)
		return 1;

	CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
	return pCDevice->SetConnectOKTimeout(nConnBadTimeoutSec * 1000);
}

// ���õ�ǰ����״̬Ϊ��OK
PKDRIVER_EXPORTS int Drv_SetConnectOK(PKDEVICE *pDevice)
{
	if (NULL == pDevice || pDevice->_pInternalRef == NULL)
		return 1;

	CDevice *pCDevice = (CDevice *)pDevice->_pInternalRef;
	pCDevice->SetConnectOK();
	return 0;
}

void PKTAG_Reset(PKTAG *pTag)
{
	memset(pTag, 0, sizeof(PKTAG)); 
	pTag->szName = NULL;
	pTag->szAddress = NULL;
	pTag->szParam = NULL; // ���������ļ�ʱ�����ڴ�
	pTag->szData = NULL; // ���������ļ�ʱ�����ڴ�
	pTag->nDataLen = 0;
	pTag->nDataType = TAG_DT_INT16;
	//pTag->szParam = new char[PK_PARAM_MAXLEN + 1];
	//pTag->szData = new char[PK_TAGDATA_MAXLEN + 1];
}

void PKTAG_Free(PKTAG *pTag)
{
	if (!pTag)
		return;

	if (pTag->szName)
		delete pTag->szName;
	pTag->szName = NULL;

	if (pTag->szData)
		delete[] pTag->szData;
	pTag->szData = NULL;
	pTag->nDataLen = 0;

	if (pTag->szParam)
		delete pTag->szParam;
	pTag->szParam = NULL;

	if (pTag->szAddress)
		delete pTag->szAddress;
	pTag->szAddress = NULL;

	delete pTag;
}

void PKDEVICE_Reset(PKDEVICE *pDevice)
{ 
	memset(pDevice, 0, sizeof(PKDEVICE));
	pDevice->szName = new char[PK_NAME_MAXLEN + 1];
	pDevice->szDesc = new char[PK_DESC_MAXLEN + 1];
	pDevice->szConnType = new char[PK_NAME_MAXLEN + 1];
	pDevice->szConnParam = new char[PK_PARAM_MAXLEN + 1];
	pDevice->szParam1 = new char[PK_PARAM_MAXLEN + 1];
	pDevice->szParam2 = new char[PK_PARAM_MAXLEN + 1];
	pDevice->szParam3 = new char[PK_PARAM_MAXLEN + 1];
	pDevice->szParam4 = new char[PK_PARAM_MAXLEN + 1];
	pDevice->szParam5 = new char[PK_PARAM_MAXLEN + 1];
	pDevice->szParam6 = new char[PK_PARAM_MAXLEN + 1];
	pDevice->szParam7 = new char[PK_PARAM_MAXLEN + 1];
	pDevice->szParam8 = new char[PK_PARAM_MAXLEN + 1];

	memset(pDevice->szName, 0, PK_NAME_MAXLEN + 1);
	memset(pDevice->szDesc, 0, PK_DESC_MAXLEN + 1);
	memset(pDevice->szConnType, 0, PK_NAME_MAXLEN + 1);
	memset(pDevice->szConnParam, 0, PK_PARAM_MAXLEN + 1);
	memset(pDevice->szParam1, 0, PK_PARAM_MAXLEN + 1);
	memset(pDevice->szParam2, 0, PK_PARAM_MAXLEN + 1);
	memset(pDevice->szParam3, 0, PK_PARAM_MAXLEN + 1);
	memset(pDevice->szParam4, 0, PK_PARAM_MAXLEN + 1);
	memset(pDevice->szParam5, 0, PK_PARAM_MAXLEN + 1);
	memset(pDevice->szParam6, 0, PK_PARAM_MAXLEN + 1);
	memset(pDevice->szParam7, 0, PK_PARAM_MAXLEN + 1);
	memset(pDevice->szParam8, 0, PK_PARAM_MAXLEN + 1);

	pDevice->pDriver = NULL;
	pDevice->_pInternalRef = NULL;
	memset(pDevice->pUserData, 0, sizeof(void *) * PK_USERDATA_MAXNUM);
	memset(pDevice->nUserData, 0, sizeof(int) * PK_USERDATA_MAXNUM);
	memset(pDevice->szUserData, 0, sizeof(char) * PK_USERDATA_MAXNUM * PK_DESC_MAXLEN);
	pDevice->bEnableConnect = true;
}

void PKDEVICE_Free(PKDEVICE *pDevice)
{
	if (!pDevice)
		return;

	delete[] pDevice->szName;
	delete[] pDevice->szDesc;
	delete[] pDevice->szConnType;
	delete[] pDevice->szConnParam;
	delete[] pDevice->szParam1;
	delete[] pDevice->szParam2;
	delete[] pDevice->szParam3;
	delete[] pDevice->szParam4;
	delete[] pDevice->szParam5;
	delete[] pDevice->szParam6;
	delete[] pDevice->szParam7;
	delete[] pDevice->szParam8;

	delete pDevice;
}

void PKDRIVER_Reset(PKDRIVER *pDriver)
{ 
	memset(pDriver, 0, sizeof(PKDRIVER));

	pDriver->szName = new char[PK_NAME_MAXLEN + 1];
	pDriver->szDesc = new char[PK_DESC_MAXLEN + 1]; 
	pDriver->szParam1 = new char[PK_PARAM_MAXLEN + 1];
	pDriver->szParam2 = new char[PK_PARAM_MAXLEN + 1];
	pDriver->szParam3 = new char[PK_PARAM_MAXLEN + 1];
	pDriver->szParam4 = new char[PK_PARAM_MAXLEN + 1];

	memset(pDriver->szName, 0, PK_NAME_MAXLEN + 1);
	memset(pDriver->szDesc, 0, PK_DESC_MAXLEN + 1);
	memset(pDriver->szParam1, 0, PK_PARAM_MAXLEN + 1);
	memset(pDriver->szParam2, 0, PK_PARAM_MAXLEN + 1);
	memset(pDriver->szParam3, 0, PK_PARAM_MAXLEN + 1);
	memset(pDriver->szParam4, 0, PK_PARAM_MAXLEN + 1); 

	pDriver->ppDevices = NULL;
	pDriver->nDeviceNum = 0;
	pDriver->_pInternalRef = NULL;
	memset(pDriver->pUserData, 0, sizeof(void *) * PK_USERDATA_MAXNUM);
	memset(pDriver->nUserData, 0, sizeof(int) * PK_USERDATA_MAXNUM);
	memset(pDriver->szUserData, 0, sizeof(char) * PK_USERDATA_MAXNUM * PK_DESC_MAXLEN);
}

void PKDRIVER_Free(PKDRIVER *pDriver)
{
	if (!pDriver)
		return;

	delete[] pDriver->szName;
	delete[] pDriver->szDesc;
	delete[] pDriver->szParam1;
	delete[] pDriver->szParam2;
	delete[] pDriver->szParam3;
	delete[] pDriver->szParam4;

	delete pDriver;
}

void PKTIMER_Reset(PKTIMER *pTimer)
{ 
	pTimer->nPeriodMS = pTimer->nPhaseMS = 0;
	pTimer->_pInternalRef = 0;
	int i = 0;
	for (i = 0; i < PK_USERDATA_MAXNUM; i++)
	{
		pTimer->nUserData[i] = 0;
		pTimer->pUserData[i] = 0;
		pTimer->szUserData[i][0] = '\0';
	} 
}

void PKTIMER_Free(PKTIMER *pTimer)
{
	delete pTimer;
}
