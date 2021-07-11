#include <cppunit/config/SourcePrefix.h>

#include "RMUDPCommTester.h"
#include <ace/Init_ACE.h>

CPPUNIT_TEST_SUITE_REGISTRATION( CRMUDPCommTester);

void CRMUDPCommTester::setUp()
{
	pRMUDP = new CRMUDPComm();
}

void CRMUDPCommTester::tearDown()
{
	if (pRMUDP)
	{
		delete pRMUDP;
	}
}

void CRMUDPCommTester::testAddIPAddr()
{
	printf("testAddIPAddr\n");

	ACE_UINT32 ulIP= 0x12345678;
	ACE_INET_Addr addr((u_short)0, ulIP);

	pRMUDP->AddIPAddr(addr.get_host_addr());
	CPPUNIT_ASSERT( pRMUDP->m_vecIP.size() == 1 );
	CPPUNIT_ASSERT( pRMUDP->m_vecIP[0] == ulIP);
	ACE_UINT32 ulIP2= 0x87123456;
	ACE_INET_Addr addr2((u_short)0, ulIP2);

	pRMUDP->AddIPAddr(addr2.get_host_addr());
	CPPUNIT_ASSERT( pRMUDP->m_vecIP.size() == 2 );
	CPPUNIT_ASSERT( pRMUDP->m_vecIP[1] == ulIP2);

}

void CRMUDPCommTester::testAddPortAddr()
{
	printf("testAddPortAddr\n");
	u_short usPort0 = 12345;
	u_short usPort1 = 65535;
	pRMUDP->AddPortAddr(usPort0);

	CPPUNIT_ASSERT( pRMUDP->m_vecPort.size() == 1 );
	pRMUDP->AddPortAddr(usPort1);
	CPPUNIT_ASSERT( pRMUDP->m_vecPort.size() == 2 );

	CPPUNIT_ASSERT( pRMUDP->m_vecPort[0] == usPort0 );
	CPPUNIT_ASSERT( pRMUDP->m_vecPort[1] == usPort1 );
}

void CRMUDPCommTester::testSendAndRecvBuffer()
{
	printf("testSendAndRecvBuffer\n");
	return;
	ACE_INET_Addr addrRecv((u_short)61234);
	ACE_SOCK_Dgram udp;
	long nErr = 0;
	nErr = udp.open(addrRecv);
	if (nErr != 0)
	{
		printf("Port Open Failed.\n");
		return ;
	}

	ACE_INET_Addr addr((u_short)61234, "127.0.0.1", AF_UNSPEC);
	CRMUDPComm::tagMsgBlock t0, t1;
	t0.usCtrl = 124;
	t0.usStatus = 798;
	ACE_INET_Addr addrLocal;

	ACE_SOCK_Dgram udpSend(addrLocal);



	nErr = pRMUDP->SendBuffer(&t0, sizeof(t0), udpSend, addr);
	CPPUNIT_ASSERT(nErr == 0);
	nErr = pRMUDP->RecvBuffer(&t1, sizeof(t1), udp);
	CPPUNIT_ASSERT(nErr == 0);
	CPPUNIT_ASSERT( t0.usCtrl == t1.usCtrl);
	CPPUNIT_ASSERT(t1.usStatus == t0.usStatus);

	udp.close();
	udpSend.close();
}

void CRMUDPCommTester::testInitialize_InitAndSendRecvStatus()
{
	printf("testInitialize_InitAndSendRecvStatus\n");
	return;
	long nErr = 0;
	pRMUDP->AddPortAddr(61234);
	pRMUDP->AddIPAddr("127.0.0.1");
	nErr = pRMUDP->Initialize();
	long nCtrl, nStatus;
	int nType;
	std::string strObjName;
	// SCADA_TYPE
	pRMUDP->SendStatus(ICV_RM_SCADA_MSGTYPE,12456, 3445, NULL);
	pRMUDP->GetStatus(nType,nStatus, nCtrl,strObjName);
	CPPUNIT_ASSERT(nType == ICV_RM_SCADA_MSGTYPE);
	CPPUNIT_ASSERT(nStatus == 12456);
	CPPUNIT_ASSERT(nCtrl == 3445);

	// OBJ TYPE
	pRMUDP->SendStatus(ICV_RM_OBJ_MSGTYPE,12456, 3445, "test");
	pRMUDP->GetStatus(nType,nStatus, nCtrl,strObjName);
	CPPUNIT_ASSERT(nType == ICV_RM_OBJ_MSGTYPE);
	CPPUNIT_ASSERT(nStatus == 12456);
	CPPUNIT_ASSERT(nCtrl == 3445);

	// Both send 
	pRMUDP->SendStatus(ICV_RM_SCADA_MSGTYPE,12456, 3445, NULL);
	pRMUDP->SendStatus(ICV_RM_OBJ_MSGTYPE,12456, 3445, "test");
	pRMUDP->GetStatus(nType,nStatus, nCtrl,strObjName);
	CPPUNIT_ASSERT(nType == ICV_RM_SCADA_MSGTYPE);
	CPPUNIT_ASSERT(nStatus == 12456);
	CPPUNIT_ASSERT(nCtrl == 3445);
	pRMUDP->GetStatus(nType,nStatus, nCtrl,strObjName);
	CPPUNIT_ASSERT(nType == ICV_RM_OBJ_MSGTYPE);
	CPPUNIT_ASSERT(nStatus == 12456);
	CPPUNIT_ASSERT(nCtrl == 3445);
}

void CRMUDPCommTester::testInitialize_InitFail()
{
	printf("testInitialize_InitFail\n");
	return;
	ACE_INET_Addr addr(61234);
	ACE_SOCK_Dgram udp(addr);

	long nErr = 0;
	pRMUDP->AddPortAddr(61234);
	nErr = pRMUDP->Initialize();
	
	CPPUNIT_ASSERT(nErr == EC_ICV_RMSERVICE_NO_UDP_PORT_AVALIBLE);
	udp.close();
	nErr = pRMUDP->Initialize();
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	pRMUDP->Finalize();
	pRMUDP->AddPortAddr(61235);
	udp.open(addr);
	nErr = pRMUDP->Initialize();
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	pRMUDP->Finalize();
	udp.close();
}

void CRMUDPCommTester::test1()
{

}

void CRMUDPCommTester::test2()
{

}

void CRMUDPCommTester::test3()
{

}

void CRMUDPCommTester::test4()
{

}

void CRMUDPCommTester::test5()
{

}

void CRMUDPCommTester::test6()
{

}

void CRMUDPCommTester::test7()
{

}

void CRMUDPCommTester::test8()
{

}

void CRMUDPCommTester::test9()
{

}