#include <cppunit/extensions/HelperMacros.h>

#include "common/pkIOBuffer.h"

class CTestCVInputOutputBufferUT : public CppUnit::TestFixture  
{
	CPPUNIT_TEST_SUITE( CTestCVInputOutputBufferUT );
	CPPUNIT_TEST( TestReadWrite );
	CPPUNIT_TEST( TestReadWriteMulti );
	CPPUNIT_TEST( TestReadWrite64 );
	CPPUNIT_TEST( TestReadWriteFloatDouble );
	CPPUNIT_TEST( TestGetBufferLength );
	CPPUNIT_TEST( TestCopyConstructor );
	CPPUNIT_TEST( TestInsertExtract );
	CPPUNIT_TEST( TestOverflow );
	CPPUNIT_TEST( TestReadOverFlow );
	CPPUNIT_TEST( TestWriteLong );
	CPPUNIT_TEST_SUITE_END();
	
public:
	
	CTestCVInputOutputBufferUT(){};
	virtual ~CTestCVInputOutputBufferUT(){};
	void setUp();
	void tearDown();

	void TestReadWrite();
	void TestReadWriteMulti();
	void TestReadWrite64();
	void TestReadWriteFloatDouble();
	void TestCopyConstructor();
	void TestInsertExtract();
	void TestOverflow();
	void TestReadOverFlow();
	void TestGetBufferLength();
	void TestWriteLong();

};
