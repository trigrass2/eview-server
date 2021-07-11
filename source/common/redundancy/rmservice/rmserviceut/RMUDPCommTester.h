#include <cppunit/extensions/HelperMacros.h>

#include "../RMPeerCommunicator.h"

class CRMUDPCommTester : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CRMUDPCommTester );
	CPPUNIT_TEST(testAddIPAddr);
	CPPUNIT_TEST(testAddPortAddr);
	CPPUNIT_TEST(testSendAndRecvBuffer);
	CPPUNIT_TEST(testInitialize_InitAndSendRecvStatus);
	CPPUNIT_TEST(testInitialize_InitFail);
	CPPUNIT_TEST(test1);
	CPPUNIT_TEST(test2);
	CPPUNIT_TEST(test3);
	CPPUNIT_TEST(test4);
	CPPUNIT_TEST(test5);
	CPPUNIT_TEST(test6);
	CPPUNIT_TEST(test7);
	CPPUNIT_TEST(test8);
	CPPUNIT_TEST(test9);
	CPPUNIT_TEST_SUITE_END();

public:
	void testAddIPAddr();
	void testAddPortAddr();

	void testSendAndRecvBuffer();

	void testInitialize_InitAndSendRecvStatus();

	void testInitialize_InitFail();
	void setUp();
	
	void tearDown();


	void test1();
	void test2();
	void test3();
	void test4();
	void test5();
	void test6();
	void test7();
	void test8();
	void test9();


private:
	CRMUDPComm *pRMUDP;
};

