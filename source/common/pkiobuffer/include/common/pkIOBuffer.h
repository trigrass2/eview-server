/**************************************************************
 *  Filename:    CVIOBuffer.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: CCVInputBuffer CCVOutputBuffer�������칹ƽ̨���ֽ���Ͷ�������
 *    ��������ʹ�õ�ACE��ACE_InputCDR��ACE_OutputCDR��ACE��ʹ����CORBA�Ĺ���������
 *    ����룬���õ���8�ֽڶ��뷽ʽ�������������ͻ��˷����ͨѶʱ�����ܴ����ֽ�
 *    ��Ͷ������⣩��ʹ������������Ա����ֽ����structure����������������. ����
 *    ��ʹ�õĶ���ACE_XXX���͵����ݣ���Ҫ��Ϊ�˱���ƽ̨��һ�µ��µ��ֳ���һ�����⣬
 *    һ������£�short���ͺ�int���͵��ֳ����ǹ̶��ģ�16��32��������long�͵��ֳ���
 *    64λƽ̨����64λ�����Ϊ�˱����������������������ͨѶʱ��Ҫʹ��long�ͣ�����
 *    ����ʹ��ACE_XXX���͵����ݡ�����ʹ�÷������Բο�TestCVIOBufferUT
 *
 *  @author:     chenzhiquan
 *  @version     07/16/2008  chenzhiquan  Initial Version
**************************************************************/
#ifndef _COMMON_CV_IO_BUFFER_H_
#define _COMMON_CV_IO_BUFFER_H_

#include <ace/CDR_Stream.h>
#define CV_COM_BUFFER_DEFAULT_IO_BUFFER_LENGTH 512

//////////////////////////////////////////////////////////////////////////
// Input Buffer Stream Helper

class CCVInputBuffer
{
private:
	ACE_InputCDR	m_InputCDR;	// Input CDR

public:

	//@cmember - default constructor with max buffer length 
	CCVInputBuffer(const char* szBuffer, size_t nBufferLength);

	//@cmember - copy constructor
	CCVInputBuffer(const CCVInputBuffer& rhs);

	//@cmember - destructor
	virtual ~CCVInputBuffer(){};

	//@cmember - return length of buffer left for read
	size_t GetLength();

	//@cmember - return length of buffer readed
	size_t GetReadedLength();

	//@cmember - returns the decapsulated bytestream Read Only method
	//const char*  GetByteStream();

	//@cmember - Duplicate A Buffer
	CCVInputBuffer *Duplicate();

	//@cmember - return error flag for given type CCVComBuffer::INPUT_STATUS / CCVComBuffer::OUTPUT_STATUS
	bool GetGoodBit();

	//@cmember - Reset Buffer for reuse.
	void Reset();

	//deserialisation
    // @cmember Extract an unsigned char from the buffer.
    CCVInputBuffer& operator >> (unsigned char& );
    
	// @cmember Extract a char from the buffer.
    CCVInputBuffer& operator >> (char& );
    
	// @cmember Extract an unsigned short int from the buffer.
    CCVInputBuffer& operator >> (ACE_UINT16& );
    
	// @cmember Extract a short int from the buffer.
    CCVInputBuffer& operator >> (ACE_INT16& );
    
	// @cmember Extract an unsigned int from the buffer.
    CCVInputBuffer& operator >> (ACE_UINT32& );
    
	// @cmember Extract an int from the buffer.
    CCVInputBuffer& operator >> (ACE_INT32& );
    
	// @cmember Extract an unsigned long int from the buffer.
    CCVInputBuffer& operator >> (ACE_UINT64& );
    
	// @cmember Extract a long int from the buffer.
    CCVInputBuffer& operator >> (ACE_INT64& );
    
	// @cmember Extract a float from the buffer.
    CCVInputBuffer& operator >> (float& );
    
	// @cmember Extract a double precision float from the buffer.
    CCVInputBuffer& operator >> (double& );

    // @cmember Extract several bytes from the buffer.
    bool Extract(void*, size_t);

};

//////////////////////////////////////////////////////////////////////////
// Output Buffer Stream Helper
class CCVOutputBuffer
{
private:
	ACE_OutputCDR	m_OutputCDR;	// Output CDR

	CCVOutputBuffer(const CCVOutputBuffer& rhs){}; // Do Not allow copy

public:

	//@cmember - default constructor with max buffer length 
	CCVOutputBuffer(size_t nMaxBufferLength = CV_COM_BUFFER_DEFAULT_IO_BUFFER_LENGTH);
	CCVOutputBuffer(char *szBuffer, size_t nMaxBufferLength);

	//@cmember - destructor
	virtual ~CCVOutputBuffer();

	//@cmember - return length of buffer left for write
	size_t GetWriteAvilableLength();

	//@cmember - return length of buffer left for read
	size_t GetLength();

	//@cmember - returns the decapsulated bytestream Read Only method
	const char*  GetByteStream();

	//@cmember - return error flag for given type CCVComBuffer::INPUT_STATUS / CCVComBuffer::OUTPUT_STATUS
	bool GetGoodBit();

	//serialisation
	//@cmember - serialise a ation
    CCVOutputBuffer& operator << (unsigned char);
    
	// @cmember Serialize a char into the buffer.
    CCVOutputBuffer& operator << (char);
    
	// @cmember Serialize an unsigned short int into the buffer.
    CCVOutputBuffer& operator << (ACE_UINT16);
    
	// @cmember Serialize a short int into the buffer.
    CCVOutputBuffer& operator << (ACE_INT16);
    
	// @cmember Serialize an unsigned int into the buffer.
    CCVOutputBuffer& operator << (ACE_UINT32);
    
	// @cmember Serialize an int into the buffer.
    CCVOutputBuffer& operator << (ACE_INT32);
    
	// @cmember Serialize a long int into the buffer.
    CCVOutputBuffer& operator << (ACE_UINT64);
   
	// @cmember Serialize a long int into the buffer.
	CCVOutputBuffer& operator << (ACE_INT64);

	// @cmember Serialize a float into the buffer.
    CCVOutputBuffer& operator << (float);
    
	// @cmember Serialize a double precision float into the buffer.
    CCVOutputBuffer& operator << (double);
    
	// @cmember Serialize several bytes into the buffer.
    bool Insert(void*, size_t);

	//@cmember delete overdue bytes from the buffer.
	void Reset();
};

#if defined(_WIN32) && (_MSC_VER < 1300)
#	define BYTE_ORDER_NORMAL	1
#else
#	define BYTE_ORDER_NORMAL	ACE_CDR::BYTE_ORDER_LITTLE_ENDIAN
#endif

#endif //_COMMON_CV_IO_BUFFER_H_
