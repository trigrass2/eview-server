#ifndef _COMMON_CV_COM_BUFFER_H_
#define _COMMON_CV_COM_BUFFER_H_

// default 1k buffer
#define CV_COM_BUFFER_DEFAULT_BUFFER_LENGTH 512

#include <ace/CDR_Stream.h>


class CCVComBuffer
{
private:
	//const char		*m_pBuffer;
	ACE_InputCDR	*m_pInputCDR;
	ACE_OutputCDR	*m_pOutputCDR;
	//size_t			m_nMaxBufferLength;
	//bool			m_bNeedDelete;
	size_t			*m_pRefCount;

	void			Fini();

public:

	enum ERROR_TYPE
	{
		INPUT_STATUS,
		OUTPUT_STATUS,
	};


	//@cmember - default constructor with max buffer length 
	CCVComBuffer(size_t nMaxBufferLength = CV_COM_BUFFER_DEFAULT_BUFFER_LENGTH);

	CCVComBuffer(const CCVComBuffer& rhs);

	//@cmember - destructor
	virtual ~CCVComBuffer();

	//@cmember - return total buffer length
	size_t GetTotalBufferLength();

	//@cmember - return length of buffer left for write
	size_t GetWriteAvilableLength();

	//@cmember - return length of buffer left for read
	size_t GetReadAvilableLength();

	//@cmember - returns the decapsulated bytestream Read Only method
	const char*  GetByteStream();

	//@cmember - Duplicate A Buffer
	CCVComBuffer *Duplicate();

	//@cmember - set the buffer 
	int SetByteStream(char* pBuffer, size_t nBufferLength);

	//@cmember - return error flag for given type CCVComBuffer::INPUT_STATUS / CCVComBuffer::OUTPUT_STATUS
	bool GetGoodBit(int nType);

	//operators
	//@cmember - assignment
	CCVComBuffer& operator = (const CCVComBuffer& rhs);

	//serialisation
	//@cmember - serialise a ation
    CCVComBuffer& operator << (unsigned char);
    
	// @cmember Serialize a char into the buffer.
    CCVComBuffer& operator << (char);
    
	// @cmember Serialize an unsigned short int into the buffer.
    CCVComBuffer& operator << (ACE_UINT16);
    
	// @cmember Serialize a short int into the buffer.
    CCVComBuffer& operator << (ACE_INT16);
    
	// @cmember Serialize an unsigned int into the buffer.
    CCVComBuffer& operator << (ACE_UINT32);
    
	// @cmember Serialize an int into the buffer.
    CCVComBuffer& operator << (ACE_INT32);
    
	// @cmember Serialize a unsigned long int into the buffer.
    //CCVComBuffer& operator << (unsigned long);
    
	// @cmember Serialize a long int into the buffer.
    //CCVComBuffer& operator << (long);

	// @cmember Serialize a long int into the buffer.
    CCVComBuffer& operator << (ACE_UINT64);
   
	// @cmember Serialize a long int into the buffer.
	CCVComBuffer& operator << (ACE_INT64);

	// @cmember Serialize a float into the buffer.
    CCVComBuffer& operator << (float);
    
	// @cmember Serialize a double precision float into the buffer.
    CCVComBuffer& operator << (double);
    
	// @cmember Serialize several bytes into the buffer.
    bool Insert(void*, size_t);

	//deserialisation
    // @cmember Extract an unsigned char from the buffer.
    CCVComBuffer& operator >> (unsigned char& );
    
	// @cmember Extract a char from the buffer.
    CCVComBuffer& operator >> (char& );
    
	// @cmember Extract an unsigned short int from the buffer.
    CCVComBuffer& operator >> (ACE_UINT16& );
    
	// @cmember Extract a short int from the buffer.
    CCVComBuffer& operator >> (ACE_INT16& );
    
	// @cmember Extract an unsigned int from the buffer.
    CCVComBuffer& operator >> (ACE_UINT32& );
    
	// @cmember Extract an int from the buffer.
    CCVComBuffer& operator >> (ACE_INT32& );
    
	// @cmember Extract an unsigned long int from the buffer.
    CCVComBuffer& operator >> (ACE_UINT64& );
    
	// @cmember Extract a long int from the buffer.
    CCVComBuffer& operator >> (ACE_INT64& );
    
	// @cmember Extract a float from the buffer.
    CCVComBuffer& operator >> (float& );
    
	// @cmember Extract a double precision float from the buffer.
    CCVComBuffer& operator >> (double& );

    // @cmember Extract several bytes from the buffer.
    bool Extract(void*, size_t);

	//@cmember delete overdue bytes from the buffer.
	void ClearData();
};


#endif// _COMMON_CV_COM_BUFFER_H_

