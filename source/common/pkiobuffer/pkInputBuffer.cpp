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
 *  @param  -[in,out]  const char*  szBuffer: [待解析的buffer]
 *  @param  -[in,out]  size_t  nBufferLength: [缓冲区长度]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer::CCVInputBuffer( const char* szBuffer, size_t nBufferLength /*= CV_COM_BUFFER_DEFAULT_BUFFER_LENGTH*/ )
: m_InputCDR(szBuffer, nBufferLength, BYTE_ORDER_NORMAL)
{
	
}

/**
 *  Copy Constructor.
 *	  使用的是浅拷贝，不会将buffer复制
 *  @param  -[in]  const CCVInputBuffer&  rhs: [被拷贝的CCVInputBuffer]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer::CCVInputBuffer( const CCVInputBuffer& rhs ) //: m_InputCDR( rhs.m_InputCDR.start()->rd_ptr(), rhs.m_InputCDR.length())
: m_InputCDR(rhs.m_InputCDR)
{
	
}

/**
 *  获取未解析的buffer部分的长度.
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
 *  复制InputBuffer.
 *  浅拷贝.
 *
 *  @return 复制后的CCVInputBuffer.
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer * CCVInputBuffer::Duplicate()
{
	return new CCVInputBuffer(*this);
}

/**
 *  获取状态位.
 *  如果返回true表示上次解析成功，否则表示上次解析失败.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
bool CCVInputBuffer::GetGoodBit()
{
	return m_InputCDR.good_bit();	
}

/**
 *  重置InputBuffer，使之可以重新解析.
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
 *  解析出一个64位无符号整数.
 *
 *  @param  -[out]  ACE_UINT64&  lData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_UINT64 &lData)
{
	m_InputCDR.read_ulonglong(lData);
	return *this;
}

/**
 *  解析出一个64为带符号数.
 *
 *  @param  -[out]  ACE_INT64&  lData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_INT64 &lData)
{
	m_InputCDR.read_longlong(lData);
	return *this;
}

/**
 *  解析出一个浮点数.
 *
 *  @param  -[out]  float&  fData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( float &fData)
{
	m_InputCDR.read_float(fData);
	return *this;
}

/**
 *  解析出双精度浮点数.
 *
 *  @param  -[out]  double&  dData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( double &dData)
{
	m_InputCDR.read_double(dData);
	return *this;
}

/**
 *  解析出8位无符号整数.
 *
 *  @param  -[out]  unsigned char&  ucData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( unsigned char& ucData)
{
	m_InputCDR.read_octet(ucData);
	return *this;
}

/**
 *  解析出8位带符号整数.
 *
 *  @param  -[out]  char&  cData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( char& cData)
{
	m_InputCDR.read_char(cData);
	return *this;
}

/**
 *  解析出16位无符号整数.
 *
 *  @param  -[out]  ACE_UINT16&  usData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_UINT16& usData)
{
	m_InputCDR.read_ushort(usData);	
	return *this;
}

/**
 *  解析出16位带符号数.
 *
 *  @param  -[out]  ACE_INT16&  sData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_INT16& sData)
{
	m_InputCDR.read_short(sData);	
	return *this;
}

/**
 *  解析出32位无符号数.
 *
 *  @param  -[out]  ACE_UINT32&  unData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_UINT32& unData)
{
	m_InputCDR.read_ulong(unData);
	return *this;
}

/**
 *  解析出32位带符号数.
 *
 *  @param  -[out]  ACE_INT32&  nData: [解析出的数值]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVInputBuffer& CCVInputBuffer::operator>>( ACE_INT32& nData)
{
	m_InputCDR.read_long(nData);
	return *this;
}

/**
 *  解析出长度为nSize的缓冲区.
 *  （不进行字节序转换）.
 *
 *  @param  -[out]  void*  pBuffer: [解析出的缓冲区指针]
 *  @param  -[out]  size_t  nSize: [缓冲区长度]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
bool CCVInputBuffer::Extract( void *pBuffer, size_t nSize )
{
	m_InputCDR.read_char_array((char *)pBuffer, nSize);
	return GetGoodBit();
}

