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
 *  nMaxBufferLength为预分配的buffer长度，
 *    尽量大于等于所需长度，默认 512。
 *    如果该值小于16，长度默认为16.
 *
 *  @param  -[in]  size_t  nMaxBufferLength: [预分配的buffer长度]
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
 *  获取Buffer中已保存的buffer长度.
 *
 *  @return 写入的buffer长度.
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
size_t CCVOutputBuffer::GetLength()
{
	return m_OutputCDR.length();	
}

/**
 *  获取缓冲区中余下的buffer长度.
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
 *  获取缓冲区指针.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
const char* CCVOutputBuffer::GetByteStream()
{
	return m_OutputCDR.buffer();	
}

/**
 *  检查错误标记位.
 *  如果返回true表示未有错误产生，
 *    如果返回false表示前面的操作有错误发生，
 *    一般产生错误的原因都是缓冲区不够长.
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
 *  写入64为无符号整数.
 *
 *  @param  -[in]  ACE_UINT64  ulData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_UINT64 ulData)
{
	m_OutputCDR.write_ulonglong(ulData);
	return *this;
}

/**
 *  写入64为有符号整数..
 *
 *  @param  -[in]  ACE_INT64  lData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_INT64 lData)
{
	m_OutputCDR.write_longlong(lData);
	return *this;
}

/**
 *  写入浮点数.
 *
 *  @param  -[in]  float  fData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( float fData)
{
	m_OutputCDR.write_float(fData);
	return *this;	
}

/**
 *  写入double数.
 *
 *  @param  -[in]  double  dData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( double dData)
{
	m_OutputCDR.write_double(dData);
	return *this;
}

/**
 *  写入无符号8bit整数.
 *
 *  @param  -[in,out]  unsigned char  ucData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( unsigned char ucData)
{
	m_OutputCDR.write_octet(ucData);
	return *this;
}

/**
 *  写入8bit整数.
 *
 *  @param  -[in]  unsigned char  cData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( char cData)
{
	m_OutputCDR.write_char(cData);
	return *this;
}

/**
 *  写入16位无符号数.
 *
 *  @param  -[in]  ACE_UINT16  usData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_UINT16 usData)
{
	m_OutputCDR.write_ushort(usData);	
	return *this;
}

/**
 *  写入16为带符号数.
 *
 *  @param  -[in]  ACE_INT16  sData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_INT16 sData)
{
	m_OutputCDR.write_short(sData);	
	return *this;
}

/**
 *  写入32位无符号数.
 *
 *  @param  -[in,out]  ACE_UINT32  unData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_UINT32 unData)
{
	m_OutputCDR.write_ulong(unData);
	return *this;
}

/**
 *  写入32位带符号数.
 *
 *  @param  -[in]  ACE_INT32  nData: [写入的数据]
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
CCVOutputBuffer& CCVOutputBuffer::operator<<( ACE_INT32 nData)
{
	m_OutputCDR.write_long(nData);
	return *this;
}

/**
 *  写入长度为nSize的buffer.
 *  （不进行字节序转换，一般用于字符串）.
 *
 *  @param  -[in]  void*  pBuffer: [缓冲区指针]
 *  @param  -[in]  size_t  nSize: [缓冲区大小]
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
 *  重置buffer.
 *
 *
 *  @version     07/16/2008  chenzhiquan  Initial Version.
 */
void CCVOutputBuffer::Reset()
{
	m_OutputCDR.reset();	
}



