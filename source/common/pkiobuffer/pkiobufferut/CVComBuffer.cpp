/**************************************************************
 *  Filename:    CVComBuffer.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Implement Of CCVComBuffer.
 *
 *  @author:     chenzhiquan
 *  @version     07/15/2008  chenzhiquan  Initial Version
**************************************************************/
#include <ace/OS_NS_string.h>
#include "CVComBuffer.h"
#include "errcode/ErrCode_iCV_Common.hxx"
#include "common/CommHelper.h"

/**
 *  Constructor.
 *
 *  @param  -[in]  size_t  nMaxBufferLength: [Buffer Length Allocated]
 *
 *  @version     07/15/2008  chenzhiquan  Initial Version.
 */
CCVComBuffer::CCVComBuffer(size_t nMaxBufferLength /* = CV_COM_BUFFER_DEFAULT_BUFFER_LENGTH */) 
		: m_pInputCDR(NULL), m_pOutputCDR(NULL)
{
	if (nMaxBufferLength < ACE_CDR::MAX_ALIGNMENT * 2 )
	{
		nMaxBufferLength = ACE_CDR::MAX_ALIGNMENT;
	}
	else
	{
		nMaxBufferLength -= ACE_CDR::MAX_ALIGNMENT;
	}
	//if (nMaxBufferLength > 0)
	//{
	m_pOutputCDR = new ACE_OutputCDR(nMaxBufferLength);
	//m_pBuffer = m_pOutputCDR->buffer();

	m_pInputCDR = new ACE_InputCDR(m_pOutputCDR->buffer(), m_pOutputCDR->current()->capacity());

	m_pRefCount = new size_t;
	*m_pRefCount = 1;
	//m_pInputCDR = new ACE_InputCDR(m_pOutputCDR->begin());
	//}
}

/**
 *  Copy Constructor.
 *  非深拷贝而只是拷贝内部对象指针和buffer指针，新对象读取和写入会改变原有对象.
 *
 *  @param  -[in,out]  const CCVComBuffer&  rhs: [comment]
 *
 *  @version     07/15/2008  chenzhiquan  Initial Version.
 */
CCVComBuffer::CCVComBuffer( const CCVComBuffer& rhs ) : m_pInputCDR(NULL), m_pOutputCDR(NULL)
{
	m_pOutputCDR = rhs.m_pOutputCDR;
	m_pInputCDR = rhs.m_pInputCDR;
	//m_pBuffer = m_pBuffer;
	//m_bNeedDelete = false;
	m_pRefCount = rhs.m_pRefCount;
	*m_pRefCount ++;
}

CCVComBuffer::~CCVComBuffer()
{
	Fini();
}

void CCVComBuffer::Fini()
{
	*m_pRefCount -- ;
	if (*m_pRefCount == 0)
	{
		SAFE_DELETE(m_pOutputCDR);
		SAFE_DELETE(m_pInputCDR);
		SAFE_DELETE(m_pRefCount);
	}	
}


CCVComBuffer& CCVComBuffer::operator = (const CCVComBuffer& rhs)
{
	Fini();

	m_pOutputCDR = rhs.m_pOutputCDR;
	m_pInputCDR = rhs.m_pInputCDR;
	//m_pBuffer = m_pBuffer;
	//m_bNeedDelete = false;
	m_pRefCount = rhs.m_pRefCount;
	*m_pRefCount ++;
	return *this;
}
size_t CCVComBuffer::GetTotalBufferLength()
{
	//if (m_pOutputCDR)
	//{
	//}
	//size_t nSize = m_pOutputCDR->total_length();
	return m_pOutputCDR->current()->capacity();
	//return m_pOutputCDR->current()->end() - m_pOutputCDR->current()->base();
}

const char* CCVComBuffer::GetByteStream()
{
	return m_pOutputCDR->buffer();	
}


CCVComBuffer& CCVComBuffer::operator<<( unsigned char ucData)
{
	m_pOutputCDR->write_octet(ucData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator<<( char cData)
{
	m_pOutputCDR->write_char(cData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator<<( ACE_UINT16 usData)
{
	m_pOutputCDR->write_ushort(usData);	
	return *this;
}

CCVComBuffer& CCVComBuffer::operator<<( ACE_INT16 sData)
{
	m_pOutputCDR->write_short(sData);	
	return *this;
}

CCVComBuffer& CCVComBuffer::operator<<( ACE_UINT32 unData)
{
	m_pOutputCDR->write_ulong(unData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator<<( ACE_INT32 nData)
{
	m_pOutputCDR->write_long(nData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( unsigned char& ucData)
{
	m_pInputCDR->read_octet(ucData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( char& cData)
{
	m_pInputCDR->read_char(cData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( ACE_UINT16& usData)
{
	m_pInputCDR->read_ushort(usData);	
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( ACE_INT16& sData)
{
	m_pInputCDR->read_short(sData);	
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( ACE_UINT32& unData)
{
	m_pInputCDR->read_ulong(unData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( ACE_INT32& nData)
{
	m_pInputCDR->read_long(nData);
	return *this;
}


CCVComBuffer& CCVComBuffer::operator<<( ACE_UINT64 ulData)
{
	m_pOutputCDR->write_ulonglong(ulData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator<<( ACE_INT64 lData)
{
	m_pOutputCDR->write_longlong(lData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator<<( float fData)
{
	m_pOutputCDR->write_float(fData);
	return *this;	
}

CCVComBuffer& CCVComBuffer::operator<<( double dData)
{
	m_pOutputCDR->write_double(dData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( ACE_UINT64 &lData)
{
	m_pInputCDR->read_ulonglong(lData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( ACE_INT64 &lData)
{
	m_pInputCDR->read_longlong(lData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( float &fData)
{
	m_pInputCDR->read_float(fData);
	return *this;
}

CCVComBuffer& CCVComBuffer::operator>>( double &dData)
{
	m_pInputCDR->read_double(dData);
	return *this;
}

void CCVComBuffer::ClearData()
{
	m_pOutputCDR->reset();
	ACE_Message_Block* pMsgBlock = (ACE_Message_Block*) m_pInputCDR->start();
	pMsgBlock->rd_ptr(pMsgBlock->base());
}

/**
 *  Set Buffer To CVComBuffer.
 *  设置新的buffer给CVComBuffer，
 *    此时将会与原有的Buffer脱离关系，
 *    原有buffer引用计数减一，
 *    如果原有buffer引用计数为0则自动销毁原有的buffer.
 *
 *  @param  -[in,out]  char*  pBuffer: [comment]
 *  @param  -[in,out]  size_t  nBufferLength: [comment]
 *
 *  @version     07/15/2008  chenzhiquan  Initial Version.
 */
int CCVComBuffer::SetByteStream( char* pBuffer, size_t nBufferLength )
{
	Fini();

	m_pRefCount = new size_t;
	*m_pRefCount = 1;

	m_pOutputCDR = new ACE_OutputCDR(pBuffer, nBufferLength);
	m_pInputCDR = new ACE_InputCDR(pBuffer, nBufferLength);
	return ICV_SUCCESS;
}

bool CCVComBuffer::GetGoodBit( int nType )
{
	switch(nType)
	{
	case INPUT_STATUS:
		return m_pInputCDR->good_bit();
	case OUTPUT_STATUS:
		return m_pOutputCDR->good_bit() && m_pOutputCDR->begin() == m_pOutputCDR->current();
	default:
		return false;
	}
}

bool CCVComBuffer::Insert( void *pBuffer, size_t nSize )
{
	return m_pOutputCDR->write_char_array((char *)pBuffer, nSize);
}

bool CCVComBuffer::Extract( void *pBuffer, size_t nSize )
{
	return m_pInputCDR->read_char_array((char *)pBuffer, nSize);
}

size_t CCVComBuffer::GetWriteAvilableLength()
{
	ACE_Message_Block* pMsgBlock = (ACE_Message_Block*) m_pOutputCDR->current();
	if (pMsgBlock != m_pOutputCDR->begin())
	{
		return 0;
	}

	//char *pRdPtr = m_pInputCDR->wr_ptr();
	//char *pWrPtr = m_pOutputCDR->rd_ptr();
	//ACE_Message_Block* pMsgBlock = (ACE_Message_Block*) m_pOutputCDR->begin();
	char* pEnd = pMsgBlock->end();
	char* pWrPtr = pMsgBlock->wr_ptr();
	return pEnd - pWrPtr;
	//return m_pOutputCDR->length();
}

size_t CCVComBuffer::GetReadAvilableLength()
{
	char *pRdPtr = m_pInputCDR->rd_ptr();
	char *pWrPtr = m_pOutputCDR->begin()->wr_ptr();
	return pWrPtr - pRdPtr;
}

CCVComBuffer * CCVComBuffer::Duplicate()
{
	size_t nMaxBufferLength = GetTotalBufferLength();
	CCVComBuffer *pBuffer = new CCVComBuffer(nMaxBufferLength);
	ACE_OS::memcpy( pBuffer->m_pOutputCDR->current()->base(), m_pOutputCDR->current()->base(), nMaxBufferLength);
	ACE_Message_Block *pMsgBlock = (ACE_Message_Block *)pBuffer->m_pOutputCDR->current();
	pMsgBlock->wr_ptr(m_pOutputCDR->current()->wr_ptr());
	pMsgBlock->rd_ptr(m_pOutputCDR->current()->rd_ptr());

	pMsgBlock = (ACE_Message_Block *)pBuffer->m_pInputCDR->start();

	pMsgBlock->wr_ptr(m_pInputCDR->start()->wr_ptr());
	pMsgBlock->rd_ptr(m_pInputCDR->start()->rd_ptr());

	return pBuffer;
}

