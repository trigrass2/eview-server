#include <cppunit/extensions/HelperMacros.h>

#include "CVComBuffer.h"

class CTestCVComBufferUT : public CppUnit::TestFixture  
{
	CPPUNIT_TEST_SUITE( CTestCVComBufferUT );
	CPPUNIT_TEST( TestReadWrite );
	CPPUNIT_TEST( TestReadWriteMulti );
	CPPUNIT_TEST( TestReadWrite64 );
	CPPUNIT_TEST( TestReadWriteFloatDouble );
	CPPUNIT_TEST( TestClearData );
	CPPUNIT_TEST( TestGetBufferLength );
	CPPUNIT_TEST( TestCopyConstructor );
	CPPUNIT_TEST( TestAssign );
	CPPUNIT_TEST( TestInsertExtract );
	CPPUNIT_TEST( TestDuplicate );
	CPPUNIT_TEST( TestGetSetByteStream );
	CPPUNIT_TEST( TestOverflow );
	CPPUNIT_TEST( TestReadOverFlow );
	CPPUNIT_TEST_SUITE_END();
	
public:
	
	CTestCVComBufferUT(){};
	virtual ~CTestCVComBufferUT(){};
	void setUp();
	void tearDown();
	void TestReadWrite();
	void TestReadWriteMulti();
	void TestReadWrite64();
	void TestReadWriteFloatDouble();
	void TestClearData();
	void TestGetBufferLength();
	void TestCopyConstructor();
	void TestAssign();
	void TestInsertExtract();
	void TestDuplicate();
	void TestGetSetByteStream();
	void TestOverflow();

	void TestReadOverFlow();

	CCVComBuffer *m_pCVComBuffer;
};
