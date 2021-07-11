#include "TestCVComBufferUT.h"

CPPUNIT_TEST_SUITE_REGISTRATION( CTestCVComBufferUT );

void CTestCVComBufferUT::setUp()
{
	m_pCVComBuffer = new CCVComBuffer();
}

void CTestCVComBufferUT::tearDown()
{
	delete m_pCVComBuffer;
}

void CTestCVComBufferUT::TestReadWrite()
{
	char cData = 15;
	char cDataOut;
	*m_pCVComBuffer<< cData;
	*m_pCVComBuffer>>cDataOut;
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);

	unsigned char ucData = 245;
	unsigned char ucDataOut;
	*m_pCVComBuffer<< ucData;
	*m_pCVComBuffer>> ucDataOut;
	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);

	unsigned int unData = 245;
	unsigned int unDataOut;
	*m_pCVComBuffer<< unData;
	*m_pCVComBuffer>> unDataOut;
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);
}

void CTestCVComBufferUT::TestReadWriteMulti()
{
	char cData = 15;
	char cDataOut;
	
	unsigned char ucData = 245;
	unsigned char ucDataOut;
	
	unsigned int unData = 245;
	unsigned int unDataOut;

	*m_pCVComBuffer << ucData << cData << unData;
	*m_pCVComBuffer >> ucDataOut >> cDataOut >> unDataOut;


	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);

	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);

	*m_pCVComBuffer << ucData << unData << cData;
	*m_pCVComBuffer >> ucDataOut >> unDataOut >> cDataOut;

	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);
}

void CTestCVComBufferUT::TestReadWrite64()
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
	
	*m_pCVComBuffer << ullData << unData;
	*m_pCVComBuffer >> ullDataOut >> unDataOut;

	CPPUNIT_ASSERT_EQUAL(ullData, ullDataOut);
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);
}

void CTestCVComBufferUT::TestReadWriteFloatDouble()
{
	float fData = 12123.90f;
	float fDataOut;
	*m_pCVComBuffer << fData;
	*m_pCVComBuffer >> fDataOut;
	CPPUNIT_ASSERT_EQUAL(fDataOut, fData);

	double dData = 12122343.90;
	double dDataOut;
	*m_pCVComBuffer << dData;
	*m_pCVComBuffer >> dDataOut;
	CPPUNIT_ASSERT_EQUAL(dDataOut, dData);
}

void CTestCVComBufferUT::TestClearData()
{
	char cData = 15;
	char cDataOut;
	
	unsigned char ucData = 245;
	unsigned char ucDataOut;
	
	unsigned int unData = 245;
	unsigned int unDataOut;
	
	*m_pCVComBuffer << ucData << cData << unData;
	*m_pCVComBuffer >> ucDataOut >> cDataOut >> unDataOut;
	
	
	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);

	m_pCVComBuffer->ClearData();

	ucDataOut = 0;


	*m_pCVComBuffer >> ucDataOut >> cDataOut >> unDataOut;
	
	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);

}

void CTestCVComBufferUT::TestGetBufferLength()
{
	size_t nSize  = m_pCVComBuffer->GetTotalBufferLength();
	CPPUNIT_ASSERT(nSize == CV_COM_BUFFER_DEFAULT_BUFFER_LENGTH);
	
	CCVComBuffer buffer2(1);
	nSize = buffer2.GetTotalBufferLength();
	CPPUNIT_ASSERT(nSize == 16);
	CCVComBuffer buffer1(0);
	nSize = buffer1.GetTotalBufferLength();
	CPPUNIT_ASSERT(nSize == 16);

	CCVComBuffer buffer3(15);
	nSize = buffer3.GetTotalBufferLength();
	CPPUNIT_ASSERT(nSize == 16);

	CCVComBuffer buffer4(16);
	nSize = buffer4.GetTotalBufferLength();
	CPPUNIT_ASSERT(nSize == 16);

	CCVComBuffer buffer5(17);
	nSize = buffer5.GetTotalBufferLength();
	CPPUNIT_ASSERT(nSize == 17);
}

void CTestCVComBufferUT::TestCopyConstructor()
{
	char cData = 15;
	char cDataOut;
	
	unsigned char ucData = 245;
	unsigned char ucDataOut;
	
	unsigned int unData = 245;
	unsigned int unDataOut;
	
	*m_pCVComBuffer << ucData << cData << unData;

	CCVComBuffer buffer = (*m_pCVComBuffer); // Invoke Copy Constructor


	buffer >> ucDataOut >> cDataOut >> unDataOut;
	
	
	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);
}

void CTestCVComBufferUT::TestAssign()
{
	char cData = 15;
	char cDataOut;
	
	unsigned char ucData = 245;
	unsigned char ucDataOut;
	
	unsigned int unData = 245;
	unsigned int unDataOut;
	
	*m_pCVComBuffer << ucData << cData << unData;
	
	CCVComBuffer buffer;
	buffer = (*m_pCVComBuffer); // Invoke Assign Operator
	
	
	buffer >> ucDataOut >> cDataOut >> unDataOut;
	
	
	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);	
}

void CTestCVComBufferUT::TestInsertExtract()
{
	char *szBuffer = "TestBuffer";
	char szBufferOut[11];
	m_pCVComBuffer->Insert(szBuffer, strlen(szBuffer) + 1);
	m_pCVComBuffer->Extract(szBufferOut, strlen(szBuffer) + 1);

	int nRet  = memcmp(szBuffer, szBufferOut, strlen(szBuffer));
	CPPUNIT_ASSERT(nRet == 0);

	nRet = strcmp(szBuffer, szBufferOut);
	CPPUNIT_ASSERT(nRet == 0);
}

void CTestCVComBufferUT::TestDuplicate()
{
	char cData = 15;
	char cDataOut;
	
	unsigned char ucData = 245;
	unsigned char ucDataOut;
	
	unsigned int unData = 245;
	unsigned int unDataOut;
	
	*m_pCVComBuffer << ucData << cData << unData;
	
	CCVComBuffer *pBuffer = m_pCVComBuffer->Duplicate();
	
	*pBuffer >> ucDataOut >> cDataOut >> unDataOut;
	
	
	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);	
}

void CTestCVComBufferUT::TestGetSetByteStream()
{
	char cData = 15;
	char cDataOut;
	
	unsigned char ucData = 245;
	unsigned char ucDataOut;
	
	unsigned int unData = 245;
	unsigned int unDataOut;
	
	*m_pCVComBuffer << ucData << unData << cData ;
	
	CCVComBuffer buffer;
	buffer.SetByteStream((char *)m_pCVComBuffer->GetByteStream(), m_pCVComBuffer->GetReadAvilableLength());
	
	buffer >> ucDataOut >> unDataOut >> cDataOut;
	
	CPPUNIT_ASSERT_EQUAL(ucData, ucDataOut);
	
	CPPUNIT_ASSERT_EQUAL(cData, cDataOut);
	
	CPPUNIT_ASSERT_EQUAL(unData, unDataOut);
}

void CTestCVComBufferUT::TestOverflow()
{
	CCVComBuffer buffer(8);
	double dData = 18.0;
	char cData = 19;
	buffer << dData << cData;
	CPPUNIT_ASSERT(buffer.GetGoodBit(CCVComBuffer::OUTPUT_STATUS));
	buffer << dData;
	CPPUNIT_ASSERT(!buffer.GetGoodBit(CCVComBuffer::OUTPUT_STATUS));
}

void CTestCVComBufferUT::TestReadOverFlow()
{
	CCVComBuffer buffer(8);
	double dData = 18.0;
	char cData = 19;
	buffer << dData << cData;
	CPPUNIT_ASSERT(buffer.GetGoodBit(CCVComBuffer::OUTPUT_STATUS));
	buffer >> dData >> dData;
	buffer >> dData;
	CPPUNIT_ASSERT(!buffer.GetGoodBit(CCVComBuffer::INPUT_STATUS));
}
