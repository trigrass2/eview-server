/**************************************************************
 *  Filename:    CVInputBuffer.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Implementation Of CCVInputBuffer.
 *
 *  @author:     chenzhiquan
 *  @version     07/16/2008  chenzhiquan  Initial Version
**************************************************************/
#include "common/pkIOBuffer.h"

/**
 *  Constructor.
 *
 *  @param  -[in,out]  const char*  szBuffer: [��������buffer]
 *  @param  -[in,out]  size_t  nBufferLength: [����������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer::CCVInputBuffer( const char* szBuffer, size_t nBufferLength /*= CV_COM_BUFFER_DEFAULT_BUFFER_LENGTH*/ )
: m_InputCDR(szBuffer, nBufferLength, BYTE_ORDER_NORMAL)
{
	
}

/**
 *  Copy Constructor.
 *	  ʹ�õ���ǳ���������Ὣbuffer����
 *  @param  -[in]  const CCVInputBuffer&  rhs: [��������CCVInputBuffer]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer::CCVInputBuffer( const CCVInputBuffer& rhs ) //: m_InputCDR( rhs.m_InputCDR.start()->rd_ptr(), rhs.m_InputCDR.length())
: m_InputCDR(rhs.m_InputCDR)
{
	
}

/**
 *  ��ȡδ������buffer���ֵĳ���.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
size_t CCVInputBuffer::GetLength()
{
	return m_InputCDR.length();	
}

size_t CCVInputBuffer::GetReadedLength()
{
	return m_InputCDR.start()->rd_ptr() - m_InputCDR.start()->base();
}

/**
 *  ����InputBuffer.
 *  ǳ����.
 *
 *  @return ���ƺ��CCVInputBuffer.
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer * CCVInputBuffer::Duplicate()
{
	return new CCVInputBuffer(*this);
}

/**
 *  ��ȡ״̬λ.
 *  �������true��ʾ�ϴν����ɹ��������ʾ�ϴν���ʧ��.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
bool CCVInputBuffer::GetGoodBit()
{
	return m_InputCDR.good_bit();	
}

/**
 *  ����InputBuffer��ʹ֮�������½���.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
void CCVInputBuffer::Reset()
{
	ACE_Message_Block* pMsgBlock = const_cast<ACE_Message_Block*>(m_InputCDR.start()) ;
	pMsgBlock->rd_ptr(pMsgBlock->base());
}

/**
 *  ������һ��64λ�޷�������.
 *
 *  @param  -[out]  ACE_UINT64&  lData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_UINT64 &lData)
{
	m_InputCDR.read_ulonglong(lData);
	return *this;
}

/**
 *  ������һ��64Ϊ��������.
 *
 *  @param  -[out]  ACE_INT64&  lData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_INT64 &lData)
{
	m_InputCDR.read_longlong(lData);
	return *this;
}

/**
 *  ������һ��������.
 *
 *  @param  -[out]  float&  fData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( float &fData)
{
	m_InputCDR.read_float(fData);
	return *this;
}

/**
 *  ������˫���ȸ�����.
 *
 *  @param  -[out]  double&  dData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( double &dData)
{
	m_InputCDR.read_double(dData);
	return *this;
}

/**
 *  ������8λ�޷�������.
 *
 *  @param  -[out]  unsigned char&  ucData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( unsigned char& ucData)
{
	m_InputCDR.read_octet(ucData);
	return *this;
}

/**
 *  ������8λ����������.
 *
 *  @param  -[out]  char&  cData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( char& cData)
{
	m_InputCDR.read_char(cData);
	return *this;
}

/**
 *  ������16λ�޷�������.
 *
 *  @param  -[out]  ACE_UINT16&  usData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_UINT16& usData)
{
	m_InputCDR.read_ushort(usData);	
	return *this;
}

/**
 *  ������16λ��������.
 *
 *  @param  -[out]  ACE_INT16&  sData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_INT16& sData)
{
	m_InputCDR.read_short(sData);	
	return *this;
}

/**
 *  ������32λ�޷�����.
 *
 *  @param  -[out]  ACE_UINT32&  unData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_UINT32& unData)
{
	m_InputCDR.read_ulong(unData);
	return *this;
}

/**
 *  ������32λ��������.
 *
 *  @param  -[out]  ACE_INT32&  nData: [����������ֵ]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_INT32& nData)
{
	m_InputCDR.read_long(nData);
	return *this;
}

/**
 *  ����������ΪnSize�Ļ�����.
 *  ���������ֽ���ת����.
 *
 *  @param  -[out]  void*  pBuffer: [�������Ļ�����ָ��]
 *  @param  -[out]  size_t  nSize: [����������]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
bool CCVInputBuffer::Extract( void *pBuffer, size_t nSize )
{
	m_InputCDR.read_char_array((char *)pBuffer, nSize);
	return GetGoodBit();
}

