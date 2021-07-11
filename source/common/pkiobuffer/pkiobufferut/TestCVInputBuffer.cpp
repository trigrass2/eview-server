#include "TestCVInputBuffer.h"

CPPUNIT_TEST_SUITE_REGISTRATION( CTestCVInputOutputBufferUT );

void CTestCVInputOutputBufferUT::setUp()
{
}

void CTestCVInputOutputBufferUT::tearDown()
{
}

void CTestCVInputOutputBufferUT::TestReadWrite()
{
	{
		CCVOutputBuffer OutputBuffer;
		char cData = 15;
		char cDataOut;
		OutputBuffer<< cData;
		CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());
		InputBuffer>>cDataOut;
		CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	}
	{
		CCVOutputBuffer OutputBuffer;
		unsigned char ucData = 245;
		unsigned char ucDataOut;
		OutputBuffer<< ucData;
		CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());
		InputBuffer>> ucDataOut;
		CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);

	}

	{
		CCVOutputBuffer OutputBuffer;
		unsigned int unData = 245;
		unsigned int unDataOut;
		OutputBuffer<< unData;
		CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());
		InputBuffer>> unDataOut;
		CPPUNIT_ASSERT_EQUAL(unData, unDataOut);
		
	}
}

void CTestCVInputOutputBufferUT::TestReadWriteMulti()
{
	char cData = 15;
	char cDataOut;
	
	unsigned char ucData = 245;
	unsigned char ucDataOut;
	
	unsigned int unData = 245;
	unsigned int unDataOut;
	{
		CCVOutputBuffer OutputBuffer;
		OutputBuffer << ucData << cData << unData;
		CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());

		InputBuffer >> ucDataOut >> cDataOut >> unDataOut;
		
		
		CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
		
		CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
		
		CPPUNIT_ASSERT_EQUAL(unData, unDataOut);

	}
	{
		CCVOutputBuffer OutputBuffer;
		OutputBuffer << ucData << unData << cData;
		CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());

		InputBuffer >> ucDataOut >> unDataOut >> cDataOut;
		
		CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
		
		CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
		
		CPPUNIT_ASSERT_EQUAL(unData, unDataOut);

	}
}

void CTestCVInputOutputBufferUT::TestReadWrite64()
{
#ifdef _WIN32
	ACE_UINT64 ullData = 17216961135462248174;
	ACE_INT64 unData = 1721696113546224817;
#else
	ACE_UINT64 ullData = 17216961135462248174LL;
	ACE_INT64 unData = 1721696113546224817LL;
#endif

	ACE_UINT64 ullDataOut;
	ACE_INT64 unDataOut;
	CCVOutputBuffer OutputBuffer;
	OutputBuffer << ullData << unData;
	CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());

	InputBuffer >> ullDataOut >> unDataOut;

	CPPUNIT_ASSERT_EQUAL(ullData, ullDataOut);
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);
}

void CTestCVInputOutputBufferUT::TestReadWriteFloatDouble()
{
	float fData = 12123.90f;
	float fDataOut;
	{
		CCVOutputBuffer OutputBuffer;
		OutputBuffer << fData;
		CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());

		InputBuffer >> fDataOut;
		CPPUNIT_ASSERT_EQUAL(fDataOut, fData);
	}

	double dData = 12122343.90;
	double dDataOut;
	{
		CCVOutputBuffer OutputBuffer;
		OutputBuffer << dData;
		CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());

		InputBuffer >> dDataOut;
		CPPUNIT_ASSERT_EQUAL(dDataOut, dData);
	}
}



void CTestCVInputOutputBufferUT::TestCopyConstructor()
{
	char cData = 15;
	char cDataOut;
	
	unsigned char ucData = 245;
	unsigned char ucDataOut;
	
	unsigned int unData = 245;
	unsigned int unDataOut;
	
	CCVOutputBuffer OutputBuffer;
	OutputBuffer << ucData << cData << unData;
	CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());


	CCVInputBuffer buffer(InputBuffer); // Invoke Copy Constructor


	buffer >> ucDataOut >> cDataOut >> unDataOut;
	
	
	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);
}


void CTestCVInputOutputBufferUT::TestInsertExtract()
{
	char *szBuffer = "TestBuffer";
	char szBufferOut[11];

	CCVOutputBuffer OutputBuffer;
	OutputBuffer.Insert(szBuffer, strlen(szBuffer) + 1);

	CCVInputBuffer InputBuffer(OutputBuffer.GetByteStream(), OutputBuffer.GetLength());
	InputBuffer.Extract(szBufferOut, strlen(szBuffer) + 1);

	int nRet  = memcmp(szBuffer, szBufferOut, strlen(szBuffer));
	CPPUNIT_ASSERT(nRet == 0);

	nRet = strcmp(szBuffer, szBufferOut);
	CPPUNIT_ASSERT(nRet == 0);
}

void CTestCVInputOutputBufferUT::TestOverflow()
{
	CCVOutputBuffer buffer(8);
	double dData = 18.0;
	buffer << dData << dData;
	CPPUNIT_ASSERT(buffer.GetGoodBit());
	buffer << dData;
	CPPUNIT_ASSERT(!buffer.GetGoodBit());
}

void CTestCVInputOutputBufferUT::TestReadOverFlow()
{
	CCVOutputBuffer buffer(8);
	double dData = 18.0;
	char cData = 19;
	buffer << dData << cData;
	CPPUNIT_ASSERT(buffer.GetGoodBit());
	CCVInputBuffer inputBuffer(buffer.GetByteStream(), buffer.GetLength());
	inputBuffer >> dData >> dData;
	inputBuffer >> dData;
	CPPUNIT_ASSERT(!inputBuffer.GetGoodBit());
}

void CTestCVInputOutputBufferUT::TestGetBufferLength()
{
	{
		CCVOutputBuffer buffer(24);
		double dData = 18.0;
		char cData = 19;
		buffer << dData << cData;
		size_t nSize = buffer.GetLength();
		
		CPPUNIT_ASSERT(nSize == 9);
		buffer << dData;
		nSize = buffer.GetLength();
		CPPUNIT_ASSERT(nSize == 24);

	}

	{
		CCVOutputBuffer buffer;
		short sData = 18;
		char cData = 19;
		buffer << sData << cData;
		size_t nSize = buffer.GetLength();
		
		CPPUNIT_ASSERT(nSize == 3);
		buffer << cData;
		nSize = buffer.GetLength();
		CPPUNIT_ASSERT(nSize == 4);
		double dData = 19.0;
		buffer << dData;
		nSize = buffer.GetLength();
		CPPUNIT_ASSERT(nSize == 16);
		
	}
	
}

void CTestCVInputOutputBufferUT::TestWriteLong()
{
	{
		CCVOutputBuffer buffer(24);
		ACE_INT32 lData = 19;
		buffer << lData;
		size_t nSize = buffer.GetLength();
		
		CPPUNIT_ASSERT(nSize == 4);
		
	}
	{
		//CCVOutputBuffer buffer0;
		//CCVOutputBuffer buffer1( buffer0); //TEST COpy constructor
		//CCVOutputBuffer buffer1;
		//buffer1 =( buffer0); //TEST Assign
	}
}