#include <cppunit/extensions/HelperMacros.h>

#include "../ShareMemHelper.h"

class ShareMemHelperTester : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( ShareMemHelperTester );
	CPPUNIT_TEST(testGetAndSetObjRMStatus);
	CPPUNIT_TEST(testGetAndSetObjRMStatusCtrl);
	CPPUNIT_TEST(testSetAndGetObjStatus);
	CPPUNIT_TEST(testNeedSync);
	CPPUNIT_TEST(testNeedDoSvc);
	CPPUNIT_TEST_SUITE_END();

public:
	void testGetAndSetObjRMStatus();
	void testGetAndSetObjRMStatusCtrl();
	void testSetAndGetObjStatus();
	void testNeedSync();
	void testNeedDoSvc();
	void setUp();
	
	void tearDown();
private:
	CShareMemHelper *pMemHelper;
};

