/**************************************************************
 *  Filename:    CVOutputBuffer.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Implementation Of CCVOutputBuffer.
 *
 *  @author:     chenzhiquan
 *  @version     07/16/2008  chenzhiquan  Initial Version
**************************************************************/
#include "common/pkIOBuffer.h"


/**
 *  Constructor.
 *  nMaxBufferLengthΪԤ�����buffer���ȣ�
 *    �������ڵ������賤�ȣ�Ĭ�� 512��
 *    �����ֵС��16������Ĭ��Ϊ16.
 *
 *  @param  -[in]  size_t  nMaxBufferLength: [Ԥ�����buffer����]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer::CCVOutputBuffer( size_t nMaxBufferLength /*= CV_COM_BUFFER_DEFAULT_BUFFER_LENGTH*/ )
: m_OutputCDR((nMaxBufferLength < ACE_CDR::MAX_ALIGNMENT * 2) ? ACE_CDR::MAX_ALIGNMENT : nMaxBufferLength - ACE_CDR::MAX_ALIGNMENT, BYTE_ORDER_NORMAL)
{
	
}

CCVOutputBuffer::CCVOutputBuffer(char *szBuffer, size_t nMaxBufferLength)
: m_OutputCDR(szBuffer, nMaxBufferLength, BYTE_ORDER_NORMAL)
{

}


/**
 *  Destructor.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer::~CCVOutputBuffer()
{
	
}

/**
 *  ��ȡBuffer���ѱ����buffer����.
 *
 *  @return д���buffer����.
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
size_t CCVOutputBuffer::GetLength()
{
	return m_OutputCDR.length();	
}

/**
 *  ��ȡ�����������µ�buffer����.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
size_t CCVOutputBuffer::GetWriteAvilableLength()
{
	ACE_Message_Block* pMsgBlock = const_cast<ACE_Message_Block*>(m_OutputCDR.current());
	if (pMsgBlock != m_OutputCDR.begin())
	{
		return 0;
	}

	char* pEnd = pMsgBlock->end();
	char* pWrPtr = pMsgBlock->wr_ptr();
	return pEnd - pWrPtr;
}

/**
 *  ��ȡ������ָ��.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
const char* CCVOutputBuffer::GetByteStream()
{
	return m_OutputCDR.buffer();	
}

/**
 *  ��������λ.
 *  �������true��ʾδ�д��������
 *    �������false��ʾǰ��Ĳ����д�������
 *    һ����������ԭ���ǻ�����������.
 *
 *  @return true/false.
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
bool CCVOutputBuffer::GetGoodBit()
{
	return m_OutputCDR.good_bit() && m_OutputCDR.begin() == m_OutputCDR.current();	
}

/**
 *  д��64Ϊ�޷�������.
 *
 *  @param  -[in]  ACE_UINT64  ulData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_UINT64 ulData)
{
	m_OutputCDR.write_ulonglong(ulData);
	return *this;
}

/**
 *  д��64Ϊ�з�������..
 *
 *  @param  -[in]  ACE_INT64  lData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_INT64 lData)
{
	m_OutputCDR.write_longlong(lData);
	return *this;
}

/**
 *  д�븡����.
 *
 *  @param  -[in]  float  fData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( float fData)
{
	m_OutputCDR.write_float(fData);
	return *this;	
}

/**
 *  д��double��.
 *
 *  @param  -[in]  double  dData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( double dData)
{
	m_OutputCDR.write_double(dData);
	return *this;
}

/**
 *  д���޷���8bit����.
 *
 *  @param  -[in,out]  unsigned char  ucData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( unsigned char ucData)
{
	m_OutputCDR.write_octet(ucData);
	return *this;
}

/**
 *  д��8bit����.
 *
 *  @param  -[in]  unsigned char  cData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( char cData)
{
	m_OutputCDR.write_char(cData);
	return *this;
}

/**
 *  д��16λ�޷�����.
 *
 *  @param  -[in]  ACE_UINT16  usData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_UINT16 usData)
{
	m_OutputCDR.write_ushort(usData);	
	return *this;
}

/**
 *  д��16Ϊ��������.
 *
 *  @param  -[in]  ACE_INT16  sData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_INT16 sData)
{
	m_OutputCDR.write_short(sData);	
	return *this;
}

/**
 *  д��32λ�޷�����.
 *
 *  @param  -[in,out]  ACE_UINT32  unData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_UINT32 unData)
{
	m_OutputCDR.write_ulong(unData);
	return *this;
}

/**
 *  д��32λ��������.
 *
 *  @param  -[in]  ACE_INT32  nData: [д�������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_INT32 nData)
{
	m_OutputCDR.write_long(nData);
	return *this;
}

/**
 *  д�볤��ΪnSize��buffer.
 *  ���������ֽ���ת����һ�������ַ�����.
 *
 *  @param  -[in]  void*  pBuffer: [������ָ��]
 *  @param  -[in]  size_t  nSize: [��������С]
 *  @return true/false.
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
bool CCVOutputBuffer::Insert( void *pBuffer, size_t nSize )
{
	m_OutputCDR.write_char_array((char *)pBuffer, nSize);
	return GetGoodBit();
}

/**
 *  ����buffer.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
void CCVOutputBuffer::Reset()
{
	m_OutputCDR.reset();	
}



