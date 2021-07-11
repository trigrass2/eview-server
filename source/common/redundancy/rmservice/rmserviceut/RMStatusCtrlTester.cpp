#include <cppunit/config/SourcePrefix.h>

#include "RMStatusCtrlTester.h"
#include <ace/OS_NS_sys_stat.h>
#include <ace/Init_ACE.h>

#define PAUSE_TIME 1
CPPUNIT_TEST_SUITE_REGISTRATION( CRMStatusCtrlTester);

namespace
{
class auto_init
{
public:
	auto_init()
	{
		ACE::init();
	}
	~auto_init()
	{
		ACE::fini();
	}
};

static auto_init ace_auto_init;
}



#define TEST_TEMP_DIR "ForTest"
#define ICV_RMSVC_OBJ_STATUS_TIMEOUT 10


void CRMStatusCtrlTester::setUp()
{
	m_pRMSC = new CRMStatusCtrl();
	CRMUDPComm* pRMUDP = new CRMUDPComm();
	m_pRMSC->SetPeerCommunicator(pRMUDP);
	ACE_OS::mkdir(TEST_TEMP_DIR);
}

void CRMStatusCtrlTester::tearDown()
{
	SAFE_DELETE(m_pRMSC);
	ACE_OS::rmdir(TEST_TEMP_DIR);
}

long CRMStatusCtrlTester::SetUpEnv0()
{
	// Building Test Enviroment.
	((CRMUDPComm*)m_pRMSC->m_pComm)->AddIPAddr("127.0.0.1");
	((CRMUDPComm*)m_pRMSC->m_pComm)->AddPortAddr(61234);

	return m_pRMSC->Initialize(TEST_TEMP_DIR);	
}

void CRMStatusCtrlTester::CleanEnv0()
{
	((CRMUDPComm*)m_pRMSC->m_pComm)->Finalize();
}

void CRMStatusCtrlTester::UpdatePeerTimestamp(int objBlkIdx)
{
	tagObjRMStatusBlock * pObjBlock = m_pRMSC->m_shmHelper.GetObjBlockByIdx(objBlkIdx);

	pObjBlock->objRmStatus.peerTimeStamp = ACE_OS::gettimeofday();
}

void CRMStatusCtrlTester::testDoCtrl_InitRun_With_No_Peer()
{
	printf("testDoCtrl_InitRum_With_No_Peer\n");
	long nErr;
	return;
	m_pRMSC->SetSequence(0);
	m_pRMSC->SetStrategy(100, 3);// Threshold = 3

	long nStatus, nCtrl;

	nErr = SetUpEnv0();

	if (nErr != 0)
	{
		printf("Error Initialize, Exit testDoCtrl Without Run Test!\n");
		return ;
	}

	//1. Testing Send Message- Run #1
	m_pRMSC->DoCtrl();
	std::string objName="";
	int nType;
	nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 1);
	
	//2. Run #2
	m_pRMSC->DoCtrl();
	nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);

	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 2);

	//2. Run #3
	m_pRMSC->DoCtrl();
	nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 3);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);

	//2. Run #4
	m_pRMSC->DoCtrl();
	nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 4);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE);

	CleanEnv0();
}

void CRMStatusCtrlTester::testDoCtrl_Simulate_Peer_With_No_Ctrl()
{
	printf("testDoCtrl_Simulate_Peer_With_No_Ctrl\n");
	long nErr;return;
	m_pRMSC->SetSequence(0);
	m_pRMSC->SetStrategy(100, 3);// Threshold = 3
	
	long nStatus, nCtrl;
	
	nErr = SetUpEnv0();
	
	if (nErr != 0)
	{
		printf("Error Initialize, Exit testDoCtrl Without Run Test!\n");
		return ;
	}
	std::string objName="";
	int nType = ICV_RM_SCADA_MSGTYPE;	
	// 1. Peer Exists. and Send ACTIVE Status when INACTIVE
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());

	m_pRMSC->DoCtrl();
	 ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	//CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 0);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);


	// 2. Peer Exists. and Send INACTIVE Status when ACTIVE
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_nStatus = RM_STATUS_ACTIVE;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE); 
	//CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 0);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);

	// 3. Peer Exists. and Send INACTIVE Status when INACTIVE seq = 0
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_nStatus = RM_STATUS_INACTIVE; // Init State.
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE); // Change To Active Because Of Sequence = 0;
	//CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 0);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
	

	// 4. Peer Exists. and Send ACTIVE Status when ACTIVE  seq = 0
	
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_nStatus = RM_STATUS_ACTIVE;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE); 
	//CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 0);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);


	m_pRMSC->SetSequence(1); // Change seq = 1

	// 5. Peer Exists. and Send INACTIVE Status when INACTIVE seq = 1
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_nStatus = RM_STATUS_INACTIVE; // Init State.
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE); 
	//CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 0);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	
	
	// 6. Peer Exists. and Send ACTIVE Status when ACTIVE  seq = 1
	
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_nStatus = RM_STATUS_ACTIVE;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE); // Change To InActive Because Of Sequence = 1;
	//CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 0);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);

	CleanEnv0();
}

void CRMStatusCtrlTester::testDoCtrl_Simulate_Peer_With_Local_Ctrl()
{
	printf("testDoCtrl_Simulate_Peer_With_Local_Ctrl\n");
	long nErr;
	return;
	m_pRMSC->SetSequence(0);
	m_pRMSC->SetStrategy(100, 3);// Threshold = 3
	
	long nStatus, nCtrl;
	
	nErr = SetUpEnv0();
	
	if (nErr != 0)
	{
		printf("Error Initialize, Exit testDoCtrl Without Run Test!\n");
		return ;
	}
	std::string objName="";
	int nType = ICV_RM_SCADA_MSGTYPE;	
	// 1. Peer Exists and Send INACTIVE Status with NO Ctrl Cmd. when CTRL_INACTIVE comes.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,NULL);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	m_pRMSC->m_shmHelper.SetStatus(RM_STATUS_ACTIVE);
	m_pRMSC->m_shmHelper.SetStatusCtrl(RM_STATUS_CTRL_INACTIVE); // Set Local Ctrl Cmd CTRL_INACTIVE
	m_pRMSC->m_nStatus = RM_STATUS_ACTIVE;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	//CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_INACTIVE);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_ACTIVE); // SHOULD SEND CTRL_ACTIVE cmd to peer.

	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_ACK,NULL);
	m_pRMSC->DoCtrl();	// Recv ACK.
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);	// Status Change TO INACTIVE
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL); 

	// 2. Peer Exists and Send ACTIVE Status with NO Ctrl Cmd. current node is INACTIVE when CTRL_ACTIVE comes.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,NULL);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	m_pRMSC->m_shmHelper.SetStatusCtrl(RM_STATUS_CTRL_ACTIVE); // Set Local Ctrl Cmd CTRL_INACTIVE
	m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	m_pRMSC->m_shmHelper.SetStatus(RM_STATUS_INACTIVE);
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_ACTIVE);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_INACTIVE); // SHOULD SEND CTRL_INACTIVE cmd to peer.
	
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_ACK,NULL);
	m_pRMSC->DoCtrl();	// Recv ACK.
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE);	// Status Change TO INACTIVE
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL); 



	// 3. Peer Exists and Send INACTIVE Status with NO Ctrl Cmd. when CTRL_ACTIVE comes.
	//    Ctrl Cmd Will Not Be Processed.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,NULL);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	m_pRMSC->m_shmHelper.SetStatusCtrl(RM_STATUS_CTRL_ACTIVE); // Set Local Ctrl Cmd CTRL_INACTIVE
	m_pRMSC->m_nStatus = RM_STATUS_ACTIVE;
	m_pRMSC->m_nCtrlExecing = RM_STATUS_CTRL_NIL;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL); // Nothing to do.



	// 4. Peer Exists and Send ACTIVE Status with NO Ctrl Cmd. INACTIVE itself when CTRL_INACTIVE comes.
	//    Ctrl Cmd Will Not Be Processed.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,NULL);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	m_pRMSC->m_shmHelper.SetStatusCtrl(RM_STATUS_CTRL_INACTIVE); // Set Local Ctrl Cmd CTRL_INACTIVE
	m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	m_pRMSC->m_shmHelper.SetStatus(RM_STATUS_INACTIVE);
	m_pRMSC->m_nCtrlExecing = RM_STATUS_CTRL_NIL;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL); // Nothing to do.



	// 5. Peer Exists and Send INACTIVE Status with CTRL_INACTIVE Cmd. ACTIVE itself.
	//    Ctrl Cmd Will Not Be Processed.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_INACTIVE,NULL);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	m_pRMSC->m_nStatus = RM_STATUS_ACTIVE;
	m_pRMSC->m_shmHelper.SetStatus(RM_STATUS_ACTIVE);
	m_pRMSC->m_nCtrlExecing = RM_STATUS_CTRL_NIL;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_ACK); // Nothing to do.


	// 6. Peer Exists and Send INACTIVE Status with CTRL_ACTIVE Cmd. INACTIVE itself.
	//    Ctrl Cmd Will Not Be Processed.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_ACTIVE,NULL);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	m_pRMSC->m_shmHelper.SetStatus(RM_STATUS_INACTIVE);
	m_pRMSC->m_nCtrlExecing = RM_STATUS_CTRL_NIL;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_ACK); // Nothing to do.
	CleanEnv0();
}

void CRMStatusCtrlTester::testDoCtrl_Simulate_Peer_With_Long_Latency()
{
	printf("testDoCtrl_Simulate_Peer_With_Long_Latency\n");
	long nErr;
	return;
	m_pRMSC->SetSequence(0);
	m_pRMSC->SetStrategy(100, 3);// Threshold = 3
	
	long nStatus, nCtrl;
	
	nErr = SetUpEnv0();
	
	if (nErr != 0)
	{
		printf("Error Initialize, Exit testDoCtrl Without Run Test!\n");
		return ;
	}
	std::string objName= "";
	int nType = ICV_RM_SCADA_MSGTYPE;
	// 1. Recieve ACK Not in an interval. When current is CTRL_INACTIVE
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType, RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL, (char*)objName.c_str());
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	m_pRMSC->m_nCtrlExecing = RM_STATUS_CTRL_INACTIVE;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_INACTIVE);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_ACTIVE); // Resend Ctrl Cmd.

	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType, RM_STATUS_ACTIVE, RM_STATUS_CTRL_ACK, (char*)objName.c_str());
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	//m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	//m_pRMSC->m_nCtrl = RM_STATUS_CTRL_INACTIVE;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL); // Do Not Send.
	
	// Resend An ACK  because of two ctrl cmd.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType, RM_STATUS_ACTIVE, RM_STATUS_CTRL_ACK, (char*)objName.c_str());
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	//m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	//m_pRMSC->m_nCtrl = RM_STATUS_CTRL_NIL;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL); // Nothing changed.

/**********CASE 2*****************/
	// 2. Recieve ACK Not in an interval. When current is CTRL_ACTIVE
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	m_pRMSC->m_nCtrlExecing = RM_STATUS_CTRL_ACTIVE;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_ACTIVE);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_INACTIVE); // Resend Ctrl Cmd.
	
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_ACK,(char*)objName.c_str());
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	//m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	//m_pRMSC->m_nCtrl = RM_STATUS_CTRL_ACTIVE;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL); // Do Not Send.
	
	// Resend An ACK  because of two ctrl cmd.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_ACK,(char*)objName.c_str());
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	//m_pRMSC->m_nStatus = RM_STATUS_ACTIVE;
	//m_pRMSC->m_nCtrl = RM_STATUS_CTRL_NIL;
	m_pRMSC->DoCtrl();
	((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
	CPPUNIT_ASSERT(m_pRMSC->m_nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(m_pRMSC->m_nCtrlExecing == RM_STATUS_CTRL_NIL);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
	CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL); // Nothing changed.
	CleanEnv0();
}

long CRMStatusCtrlTester::SetUpEnvWithObj()
{
	// Building Test Enviroment.
	((CRMUDPComm*)m_pRMSC->m_pComm)->AddIPAddr("127.0.0.1");
	((CRMUDPComm*)m_pRMSC->m_pComm)->AddPortAddr(61234);
	RMObject object;
	object.bBeSingle = true;
	object.bNeedSync = true;
	object.nCtrlCount = 0;
	object.nCtrlExec = RM_STATUS_CTRL_NIL;
	object.nObjBlockIdx = 0;
	object.nPrimSeq = 1;
	object.nSend = RM_STATUS_CTRL_NIL;
	object.nStatusCount = 0;
	strcpy(object.szObjName , "IOMBTCP:device0");
	int nErr = m_pRMSC->AddRMObject(&object);
	if(nErr != ICV_SUCCESS)
		return nErr;

	return m_pRMSC->Initialize(TEST_TEMP_DIR);
	
}

void CRMStatusCtrlTester::testDoCtrl_RMObj_InitRun_With_No_Peer()
{
	printf("testDoCtrl_RMObj_InitRun_With_No_Peer\n");
	long nErr;

	m_pRMSC->SetSequence(0);
	m_pRMSC->SetStrategy(100, 3);// Threshold = 3
	long nStatus, nCtrl;

	nErr = SetUpEnvWithObj();

	if (nErr != 0)
	{
		printf("Error Initialize, Exit testDoCtrl Without Run Test!\n");
		return ;
	}
	// RMObject
	RMObject *pObject = &(*(m_pRMSC->m_RMObjectList.begin()));
	tagObjRMStatusBlock * pObjBlock = m_pRMSC->m_shmHelper.GetObjBlockByIdx(pObject->nObjBlockIdx);

	// 条件：启动执行，本节点seqence=0, 对象的primseq为1.
	// 结果：对象的冗余状态为INACTIVE
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	std::string objName="";
	int nType;
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_SCADA_MSGTYPE)
		{
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 1);
		}else if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nCtrl == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

	ACE_OS::sleep(ICV_RMSVC_OBJ_STATUS_TIMEOUT);

	// 对待冗余节点心跳超时，对象冗余状态自动切换为ACTIVE
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_SCADA_MSGTYPE)
		{
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(m_pRMSC->m_nStatusCount == 2);
		}else if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nCtrl == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_ACTIVE);
		}
	}while(nErr == ICV_SUCCESS);
	
	CleanEnv0();
}


void CRMStatusCtrlTester::testDoCtrl_RMObj_Simulate_Peer_With_No_Ctrl()
{
	printf("testDoCtrl_RMObj_Simulate_Peer_With_No_Ctrl\n");
	long nErr;return;
	m_pRMSC->SetSequence(0);
	m_pRMSC->SetStrategy(100, 3);// Threshold = 3
	
	long nStatus, nCtrl;
	
	nErr = SetUpEnvWithObj();
	
	if (nErr != 0)
	{
		printf("Error Initialize, Exit testDoCtrl Without Run Test!\n");
		return ;
	}
	std::string objName="IOMBTCP:device0";
	// RMObject
	RMObject *pObject = &(*(m_pRMSC->m_RMObjectList.begin()));
	tagObjRMStatusBlock * pObjBlock = m_pRMSC->m_shmHelper.GetObjBlockByIdx(pObject->nObjBlockIdx);

	int nType = ICV_RM_OBJ_MSGTYPE;	
	// 1. Peer Exists. and Send ACTIVE Status when INACTIVE
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjStatus(1,(char*)objName.c_str());

	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
		}
	}while(nErr == ICV_SUCCESS);

	// 2. Peer Exists. and Send INACTIVE Status when ACTIVE
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_ACTIVE,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjStatus(1,(char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
		}
	}while(nErr == ICV_SUCCESS);

	// 3. Peer Exists. and Send INACTIVE Status when INACTIVE primseq = 1
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_INACTIVE,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjStatus(1,(char*)objName.c_str());
	// Init State.
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
		}
	}while(nErr == ICV_SUCCESS);

	// 4. Peer Exists. and Send ACTIVE Status when ACTIVE  seq = 0
	
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_ACTIVE,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjStatus(1,(char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
		}
	}while(nErr == ICV_SUCCESS);

	m_pRMSC->SetSequence(1); // Change seq = 1

	// 5. Peer Exists. and Send INACTIVE Status when INACTIVE seq = 1
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_INACTIVE,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjStatus(1,(char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
		}
	}while(nErr == ICV_SUCCESS);
	
	
	// 6. Peer Exists. and Send ACTIVE Status when ACTIVE  seq = 1
	
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_ACTIVE,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjStatus(1,(char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
		}
	}while(nErr == ICV_SUCCESS);

	CleanEnv0();
}


void CRMStatusCtrlTester::testDoCtrl_RMObj_Simulate_Peer_With_Local_Ctrl()
{
	printf("testDoCtrl_RMObj_Simulate_Peer_With_Local_Ctrl\n");
	long nErr;
	return;
	m_pRMSC->SetSequence(0);
	m_pRMSC->SetStrategy(100, 3);// Threshold = 3
	
	long nStatus, nCtrl;
	
	nErr = SetUpEnvWithObj();
	
	if (nErr != 0)
	{
		printf("Error Initialize, Exit testDoCtrl Without Run Test!\n");
		return ;
	}
	// RMObject
	RMObject *pObject = &(*(m_pRMSC->m_RMObjectList.begin()));
	tagObjRMStatusBlock * pObjBlock = m_pRMSC->m_shmHelper.GetObjBlockByIdx(pObject->nObjBlockIdx);

	std::string objName="IOMBTCP:device0";
	int nType = ICV_RM_OBJ_MSGTYPE;	
	// 1. Peer Exists and Send INACTIVE Status with NO Ctrl Cmd. when CTRL_INACTIVE comes.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(ICV_RM_SCADA_MSGTYPE,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(ICV_RM_OBJ_MSGTYPE,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());

	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_ACTIVE,objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatusCtrl(RM_STATUS_CTRL_INACTIVE, objName.c_str()); // Set Local Ctrl Cmd CTRL_INACTIVE
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nReadFlag == ICV_RM_STATUS_CTRL_READED);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_INACTIVE);
		}
	}while(nErr == ICV_SUCCESS);

	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(ICV_RM_OBJ_MSGTYPE,RM_STATUS_ACTIVE, RM_STATUS_CTRL_ACK,(char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();	// Recv ACK.
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nReadFlag == ICV_RM_STATUS_CTRL_READED);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

	// 2. Peer Exists and Send ACTIVE Status with NO Ctrl Cmd. current node is INACTIVE when CTRL_ACTIVE comes.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_INACTIVE,objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatusCtrl(RM_STATUS_CTRL_ACTIVE, objName.c_str()); 
	UpdatePeerTimestamp(pObject->nObjBlockIdx);

	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nReadFlag == ICV_RM_STATUS_CTRL_READED);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_ACTIVE);
		}
	}while(nErr == ICV_SUCCESS);

	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_ACK,(char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();	// Recv ACK.
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nReadFlag == ICV_RM_STATUS_CTRL_READED);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlCount == 0);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

	// 3. Peer Exists and Send INACTIVE Status with NO Ctrl Cmd. when CTRL_ACTIVE comes.
	//    Ctrl Cmd Will Not Be Processed.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_ACTIVE,objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatusCtrl(RM_STATUS_CTRL_ACTIVE, objName.c_str()); // Set Local Ctrl Cmd CTRL_INACTIVE
	//冗余对象更新心跳
	m_pRMSC->m_shmHelper.SetObjStatus(1, objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);

	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nReadFlag == ICV_RM_STATUS_CTRL_READED);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

	// 4. Peer Exists and Send ACTIVE Status with NO Ctrl Cmd. INACTIVE itself when CTRL_INACTIVE comes.
	//    Ctrl Cmd Will Not Be Processed.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_INACTIVE,objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatusCtrl(RM_STATUS_CTRL_INACTIVE, objName.c_str()); // Set Local Ctrl Cmd CTRL_INACTIVE
	//冗余对象更新心跳
	m_pRMSC->m_shmHelper.SetObjStatus(1, objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);

	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nReadFlag == ICV_RM_STATUS_CTRL_READED);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(nCtrl == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

	// 5. Peer Exists and Send INACTIVE Status with CTRL_INACTIVE Cmd. ACTIVE itself.
	//    Ctrl Cmd Will Not Be Processed.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_INACTIVE,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_ACTIVE,objName.c_str());
	//冗余对象更新心跳
	m_pRMSC->m_shmHelper.SetObjStatus(1, objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);

	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nReadFlag == ICV_RM_STATUS_CTRL_READED);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

	// 6. Peer Exists and Send INACTIVE Status with CTRL_ACTIVE Cmd. INACTIVE itself.
	//    Ctrl Cmd Will Not Be Processed.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_ACTIVE,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_INACTIVE,objName.c_str());
	//冗余对象更新心跳
	m_pRMSC->m_shmHelper.SetObjStatus(1, objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			CPPUNIT_ASSERT(pObjBlock->objRMStatusctrl.nReadFlag == ICV_RM_STATUS_CTRL_READED);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

	CleanEnv0();
}

void CRMStatusCtrlTester::testDoCtrl_RMObj_Simulate_Peer_With_Long_Latency()
{
	printf("testDoCtrl_RMObj_Simulate_Peer_With_Long_Latency\n");
	long nErr;
	return;
	m_pRMSC->SetSequence(0);
	m_pRMSC->SetStrategy(100, 3);// Threshold = 3
	
	long nStatus, nCtrl;
	
	nErr = SetUpEnvWithObj();
	
	if (nErr != 0)
	{
		printf("Error Initialize, Exit testDoCtrl Without Run Test!\n");
		return ;
	}
	string objName = "IOMBTCP:device0";
	int nType = ICV_RM_OBJ_MSGTYPE;	
	// RMObject
	RMObject *pObject = &(*(m_pRMSC->m_RMObjectList.begin()));
	tagObjRMStatusBlock * pObjBlock = m_pRMSC->m_shmHelper.GetObjBlockByIdx(pObject->nObjBlockIdx);
	
	// 1. Recieve ACK Not in an interval. When current is CTRL_INACTIVE
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType, RM_STATUS_INACTIVE, RM_STATUS_CTRL_NIL, (char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_INACTIVE, objName.c_str());
	m_pRMSC->m_shmHelper.SetObjStatus(1, objName.c_str());
	pObject->nCtrlExec = RM_STATUS_CTRL_INACTIVE;
	UpdatePeerTimestamp(pObject->nObjBlockIdx);

	m_pRMSC->DoCtrl();

	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_INACTIVE);
		}
	}while(nErr == ICV_SUCCESS);

	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType, RM_STATUS_ACTIVE, RM_STATUS_CTRL_ACK, (char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);
	
	// Resend An ACK  because of two ctrl cmd.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType, RM_STATUS_ACTIVE, RM_STATUS_CTRL_ACK, (char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

/**********CASE 2*****************/
	// 2. Recieve ACK Not in an interval. When current is CTRL_ACTIVE
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_ACTIVE, RM_STATUS_CTRL_NIL,(char*)objName.c_str());
	m_pRMSC->m_shmHelper.SetObjRMStatus(RM_STATUS_INACTIVE, objName.c_str());
	m_pRMSC->m_shmHelper.SetObjStatus(1, objName.c_str());
	pObject->nCtrlExec = RM_STATUS_CTRL_ACTIVE;
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_ACTIVE);
		}
	}while(nErr == ICV_SUCCESS);
	
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_ACK,(char*)objName.c_str());
	//m_pRMSC->m_nStatus = RM_STATUS_INACTIVE;
	//m_pRMSC->m_nCtrl = RM_STATUS_CTRL_ACTIVE;
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);
	
	// Resend An ACK  because of two ctrl cmd.
	((CRMUDPComm*)m_pRMSC->m_pComm)->SendStatus(nType,RM_STATUS_INACTIVE, RM_STATUS_CTRL_ACK,(char*)objName.c_str());
	UpdatePeerTimestamp(pObject->nObjBlockIdx);
	m_pRMSC->DoCtrl();
	do
	{
		nErr = ((CRMUDPComm*)m_pRMSC->m_pComm)->GetStatus(nType,nStatus, nCtrl,objName);
		if(nType == ICV_RM_OBJ_MSGTYPE)
		{
			CPPUNIT_ASSERT(pObjBlock->objRmStatus.nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nSend == RM_STATUS_CTRL_NIL);
			//CPPUNIT_ASSERT(pObject->nStatusCount == 0);
			CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
			CPPUNIT_ASSERT(pObject->nCtrlExec == RM_STATUS_CTRL_NIL);
		}
	}while(nErr == ICV_SUCCESS);

	CleanEnv0();
}
