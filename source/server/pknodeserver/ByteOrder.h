/**************************************************************
 *  Filename:    byteorder.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ��ֵ�ͱ��������ֽ���ת��ͨ�ÿ�
 *
 *  @author:     yangbo
 *  @version     03/15/2013  yangbo  Initial Version
**************************************************************/

#ifndef _BYTEORDER_H_
#define _BYTEORDER_H_

enum CVSWAPTYPE
{
	CV_NOSWAP	= 0,	// ��ת(12,1234,12345678)
	CV_SWAP2_12 = 1,	// 12�� ����Ҫת����ȱʡ
	CV_SWAP2_21 = 3,	// 21

	CV_SWAP4_1234 = 4,	// 1234�� ����Ҫת����ȱʡ
	CV_SWAP4_2143 = 5,	// 2143
	CV_SWAP4_3412 = 6,	// 3412
	CV_SWAP4_4321 = 7,	// 4321

	CV_SWAP8_12345678 = 8,	// 12345678�� ����Ҫת����ȱʡ
	CV_SWAP8_21436587 = 9,	// 21436587
	CV_SWAP8_34127856 = 10,	// 34127856
	CV_SWAP8_43218765 = 11,	// 43218765
	CV_SWAP8_56781234 = 12,	// 56781234
	CV_SWAP8_65872143 = 13,	// 65872143
	CV_SWAP8_78563412 = 14,	// 78563412
	CV_SWAP8_87654321 = 15,	// 87654321
};

enum CVSWAPERR
{
	EC_SWAP_SUCCESS = 0,		// �޴���
	EC_SWAP_INVALIDPARAM = 1,	// ��Ч����
	EC_SWAP_PARSE_FAILURE = 2,	// �����ַ�������
	EC_SWAP_MISMATCH = 3,		// ���Ͳ�ƥ��
};

class CByteOrder
{
public:
	/**
	 *  �����ַ�����ȡ�ֽ���ת������
	 *
	 *  @param  -[in]  const char *szSchema: [�����ַ���]
	 *  @param  -[out] int *pnType: [ת���������������]
	 *
	 *  @retrun - int ������ȡ���
	 *
	 *  @version     03/15/2013  yangbo  Initial Version
	 */
	static int GetSwapType(const char *szByteOrderName);


	/**
	 *  �������ͽ����ֽ���ת��
	 *
	 *  @param  -[in]  NTFRegRecord  RecArarry[]: [array of ntf reg record]
	 *  @param  -[in]  long  nLength: [length of RecArray]
	 *
	 *  @version     03/15/2013  yangbo  Initial Version
	 */
	static int SwapBytes(int nSwapType, unsigned char *szDataBuff, int nDataLen, int nDataType);
};


#endif // _BYTEORDER_H_


