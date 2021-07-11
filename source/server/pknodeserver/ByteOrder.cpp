/**************************************************************
 *  Filename:    byteorder.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 数值型变量进行字节序转换通用库
 *
 *  @author:     yangbo
 *  @version     03/15/2013  yangbo  Initial Version
**************************************************************/
#include "ByteOrder.h"
#include "pkdriver/pkdrvcmn.h"
#include <string>
#include <map>
using namespace std;

class _CByteOrderHelper
{
public:
	_CByteOrderHelper();
public:
	map<string, int> m_mapTypeName2No;
};

_CByteOrderHelper::_CByteOrderHelper()
{
	m_mapTypeName2No.insert(std::make_pair(std::string("12"), (int)CV_SWAP2_12));
	m_mapTypeName2No.insert(std::make_pair(std::string("21"), (int)CV_SWAP2_21));
	m_mapTypeName2No.insert(std::make_pair(std::string("1234"), (int)CV_SWAP4_1234));
	m_mapTypeName2No.insert(std::make_pair(std::string("2143"), (int)CV_SWAP4_2143));
	m_mapTypeName2No.insert(std::make_pair(std::string("3412"),(int)CV_SWAP4_3412));
	m_mapTypeName2No.insert(std::make_pair(std::string("4321"),(int)CV_SWAP4_4321));
	m_mapTypeName2No.insert(std::make_pair(std::string("12345678"), (int)CV_SWAP8_12345678));
	m_mapTypeName2No.insert(std::make_pair(std::string("21436587"), (int)CV_SWAP8_21436587));
	m_mapTypeName2No.insert(std::make_pair(std::string("34127856"),(int)CV_SWAP8_34127856));
	m_mapTypeName2No.insert(std::make_pair(std::string("43218765"),(int)CV_SWAP8_43218765));
	m_mapTypeName2No.insert(std::make_pair(std::string("56781234"),(int)CV_SWAP8_56781234));
	m_mapTypeName2No.insert(std::make_pair(std::string("65872143"),(int)CV_SWAP8_65872143));
	m_mapTypeName2No.insert(std::make_pair(std::string("78563412"),(int)CV_SWAP8_78563412)); 
	m_mapTypeName2No.insert(std::make_pair(std::string("87654321"),(int)CV_SWAP8_87654321));
}

_CByteOrderHelper g_byteOrderHelper;

void SwapUC(unsigned char &a, unsigned char &b)
{
	if (a != b)
	{
		a ^= b;
		b ^= a;
		a ^= b;
	}
}

/**
 *  获取类型.
 *
 *
 *  @version     03/15/2013  yangbo  Initial Version.
 */
int CByteOrder::GetSwapType(const char *szByteOrderName)
{
	if (szByteOrderName == NULL)
		return CV_NOSWAP;

	map<string,int>::iterator itMap = g_byteOrderHelper.m_mapTypeName2No.find(szByteOrderName);
	if (itMap != g_byteOrderHelper.m_mapTypeName2No.end())
	{
		return itMap->second;
	}
	
	return CV_NOSWAP;
}

int CByteOrder::SwapBytes(int nSwapType, unsigned char *szDataBuff, int nDataLen, int nDataType)
{
	int nErr = EC_SWAP_MISMATCH;

	if (nSwapType == CV_NOSWAP || nSwapType == CV_SWAP2_12 || nSwapType == CV_SWAP4_1234 || nSwapType == CV_SWAP8_12345678)
	{
		nErr = EC_SWAP_SUCCESS;
		return nErr;
	}

	if (szDataBuff == NULL)
	{
		switch (nDataType)
		{
		case TAG_DT_INT16:
		case TAG_DT_UINT16:
			// 两字节数据切换
			if (nSwapType == CV_SWAP2_21 || nSwapType == CV_SWAP2_12)
			{
				nErr = EC_SWAP_SUCCESS;
			}
			break;
		case TAG_DT_FLOAT:
		case TAG_DT_UINT32:
		case TAG_DT_INT32:
			// 四字节数据切换
			switch(nSwapType)
			{
			case CV_SWAP4_1234:
			case CV_SWAP4_2143:
			case CV_SWAP4_3412:
			case CV_SWAP4_4321:
				nErr = EC_SWAP_SUCCESS;
				break;
			default:
				break;
			}
			break;
		case TAG_DT_DOUBLE:
		case TAG_DT_INT64:
		case TAG_DT_UINT64:
			// 四字节数据切换
			switch(nSwapType)
			{
			case CV_SWAP8_12345678:
			case CV_SWAP8_21436587:
			case CV_SWAP8_34127856:
			case CV_SWAP8_43218765:
			case CV_SWAP8_56781234:
			case CV_SWAP8_65872143:
			case CV_SWAP8_78563412:
			case CV_SWAP8_87654321:
				nErr = EC_SWAP_SUCCESS;
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		return nErr;
	}

	switch (nDataType)
	{
	case TAG_DT_INT16:
	case TAG_DT_UINT16:
		// 两字节数据切换
		if (nSwapType == CV_SWAP2_21)
		{
			SwapUC(szDataBuff[0], szDataBuff[1]);
			nErr = EC_SWAP_SUCCESS;
		}
		break;
	case TAG_DT_FLOAT:
	case TAG_DT_UINT32:
	case TAG_DT_INT32:
		// 四字节数据切换
		switch(nSwapType)
		{
		case CV_SWAP4_2143:
			SwapUC(szDataBuff[0], szDataBuff[1]);
			SwapUC(szDataBuff[2], szDataBuff[3]);
			nErr = EC_SWAP_SUCCESS;
			break;
		case CV_SWAP4_3412:
			SwapUC(szDataBuff[0], szDataBuff[2]);
			SwapUC(szDataBuff[1], szDataBuff[3]);
			nErr = EC_SWAP_SUCCESS;
			break;
		case CV_SWAP4_4321:
			SwapUC(szDataBuff[0], szDataBuff[3]);
			SwapUC(szDataBuff[1], szDataBuff[2]);
			nErr = EC_SWAP_SUCCESS;
			break;
		default:
			break;
		}
		break;
	case TAG_DT_DOUBLE:
	case TAG_DT_INT64:
	case TAG_DT_UINT64:
		// 四字节数据切换
		switch(nSwapType)
		{
		case CV_SWAP8_21436587:
			SwapUC(szDataBuff[0], szDataBuff[1]);
			SwapUC(szDataBuff[2], szDataBuff[3]);
			SwapUC(szDataBuff[4], szDataBuff[5]);
			SwapUC(szDataBuff[6], szDataBuff[7]);
			nErr = EC_SWAP_SUCCESS;
			break;
		case CV_SWAP8_34127856:
			SwapUC(szDataBuff[0], szDataBuff[2]);
			SwapUC(szDataBuff[1], szDataBuff[3]);
			SwapUC(szDataBuff[4], szDataBuff[6]);
			SwapUC(szDataBuff[5], szDataBuff[7]);
			nErr = EC_SWAP_SUCCESS;
			break;
		case CV_SWAP8_43218765:
			SwapUC(szDataBuff[0], szDataBuff[3]);
			SwapUC(szDataBuff[1], szDataBuff[2]);
			SwapUC(szDataBuff[4], szDataBuff[7]);
			SwapUC(szDataBuff[5], szDataBuff[6]);
			nErr = EC_SWAP_SUCCESS;
			break;
		case CV_SWAP8_56781234:
			SwapUC(szDataBuff[0], szDataBuff[4]);
			SwapUC(szDataBuff[1], szDataBuff[5]);
			SwapUC(szDataBuff[2], szDataBuff[6]);
			SwapUC(szDataBuff[3], szDataBuff[7]);
			nErr = EC_SWAP_SUCCESS;
			break;
		case CV_SWAP8_65872143:
			SwapUC(szDataBuff[0], szDataBuff[5]);
			SwapUC(szDataBuff[1], szDataBuff[4]);
			SwapUC(szDataBuff[2], szDataBuff[7]);
			SwapUC(szDataBuff[3], szDataBuff[6]);
			nErr = EC_SWAP_SUCCESS;
			break;
		case CV_SWAP8_78563412:
			SwapUC(szDataBuff[0], szDataBuff[6]);
			SwapUC(szDataBuff[1], szDataBuff[7]);
			SwapUC(szDataBuff[2], szDataBuff[4]);
			SwapUC(szDataBuff[3], szDataBuff[5]);
			nErr = EC_SWAP_SUCCESS;
			break;
		case CV_SWAP8_87654321:
			SwapUC(szDataBuff[0], szDataBuff[7]);
			SwapUC(szDataBuff[1], szDataBuff[6]);
			SwapUC(szDataBuff[2], szDataBuff[5]);
			SwapUC(szDataBuff[3], szDataBuff[4]);
			nErr = EC_SWAP_SUCCESS;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return nErr;
}

