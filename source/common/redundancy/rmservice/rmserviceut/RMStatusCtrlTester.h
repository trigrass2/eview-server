#include <cppunit/extensions/HelperMacros.h>

#include "../RMStatusCtrl.h"

class CRMStatusCtrlTester : public CPPUNIT_NS::TestFixture
{
	CPPUNIT_TEST_SUITE( CRMStatusCtrlTester );
 	CPPUNIT_TEST(testDoCtrl_InitRun_With_No_Peer);
	CPPUNIT_TEST(testDoCtrl_Simulate_Peer_With_No_Ctrl);
	CPPUNIT_TEST(testDoCtrl_Simulate_Peer_With_Local_Ctrl);
	CPPUNIT_TEST(testDoCtrl_Simulate_Peer_With_Long_Latency);

	 //test RMObj
	CPPUNIT_TEST(testDoCtrl_RMObj_InitRun_With_No_Peer);
	CPPUNIT_TEST(testDoCtrl_RMObj_Simulate_Peer_With_No_Ctrl);
	CPPUNIT_TEST(testDoCtrl_RMObj_Simulate_Peer_With_Local_Ctrl);
	CPPUNIT_TEST(testDoCtrl_RMObj_Simulate_Peer_With_Long_Latency);
// 	CPPUNIT_TEST(testInitialize_InitFail);
	CPPUNIT_TEST_SUITE_END();
	
public:
	void testDoCtrl_InitRun_With_No_Peer();
	void testDoCtrl_Simulate_Peer_With_No_Ctrl();
	void testDoCtrl_Simulate_Peer_With_Local_Ctrl();
	void testDoCtrl_Simulate_Peer_With_Long_Latency();
	void testDoCtrl_RMObj_InitRun_With_No_Peer();
	void testDoCtrl_RMObj_Simulate_Peer_With_No_Ctrl();
	void testDoCtrl_RMObj_Simulate_Peer_With_Local_Ctrl();
	void testDoCtrl_RMObj_Simulate_Peer_With_Long_Latency();
	void setUp();
	
	void tearDown();
	
private:
	CRMStatusCtrl *m_pRMSC;

	long SetUpEnv0();
	long SetUpEnvWithObj();
	void CleanEnv0();
	void UpdatePeerTimestamp(int objBlkIdx);

};


