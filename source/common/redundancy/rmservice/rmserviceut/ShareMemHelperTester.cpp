#include <cppunit/config/SourcePrefix.h>

#include "ShareMemHelperTester.h"
#include <ace/Init_ACE.h>
#include "ace/OS_NS_sys_stat.h"
#include "common/CommHelper.h"
#include "common/eviewdefine.h"
#include "common/RMAPIDef.h"
#include "common/pkcomm.h"

#ifndef ICV_SUCCESS
#define ICV_SUCCESS 0
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( ShareMemHelperTester);

void ShareMemHelperTester::setUp()
{
	pMemHelper = new CShareMemHelper();
	if (NULL == CVComm.GetCVRunTimeDataPath())
	{
		return ;
	}

	string strPath = CVComm.GetCVRunTimeDataPath();
	ACE_OS::mkdir(strPath.c_str());
	strPath += ACE_DIRECTORY_SEPARATOR_STR;
	strPath += DEFAULT_SHM_FILE_NAME;
	pMemHelper->InitializeMM(strPath);
	RMObject rmObj;
	memset(&rmObj,0, sizeof(RMObject));
	strcpy(rmObj.szObjName,"IOMBTCP:device0");
	pMemHelper->AddObject(&rmObj, 0);
}

void ShareMemHelperTester::tearDown()
{
	if (pMemHelper)
	{
		delete pMemHelper;
	}
}

void ShareMemHelperTester::testGetAndSetObjRMStatus()
{
	printf("testGetAndSetObjRMStatus\n");
	long nStatus = 0;

	// ���ö�������״̬ΪINACTIVE
	long nErr = pMemHelper->SetObjRMStatus(RM_STATUS_INACTIVE,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	nErr = pMemHelper->GetObjRMStatus(nStatus,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);

	// ���ö�������״̬ΪACTIVE
	nErr = pMemHelper->SetObjRMStatus(RM_STATUS_ACTIVE,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	nErr = pMemHelper->GetObjRMStatus(nStatus,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);

	// ���ò����ڵ��������״̬
	nErr = pMemHelper->SetObjRMStatus(nStatus, "IOMBTCP");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// ���÷Ƿ����������״̬
	nErr = pMemHelper->SetObjRMStatus(10, "IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// ����SCADA�ڵ�����״̬ΪACTIVE,��ȡ�����ڵ��������״̬
	pMemHelper->SetStatus(RM_STATUS_ACTIVE);
	nErr = pMemHelper->GetObjRMStatus(nStatus,"IOMBTCP");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);

	// ���ø��ڵ�����״̬ΪACTIVE����ȡ�����ڵ��ӽڵ������״̬
	nErr = pMemHelper->SetObjRMStatus(RM_STATUS_ACTIVE,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	nErr = pMemHelper->GetObjRMStatus(nStatus,"IOMBTCP:device0:abcd");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);
}

void ShareMemHelperTester::testGetAndSetObjRMStatusCtrl()
{
	printf("testGetAndSetObjRMStatusCtrl\n");
	long nStatus = 0;
	long nErr = 0;

	// ���ö�����������ΪACTIVE
	nErr = pMemHelper->SetObjRMStatusCtrl(RM_STATUS_CTRL_ACTIVE,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	tagObjRMStatusBlock *pBlock = pMemHelper->FindObjBlockByName("IOMBTCP:device0");
	CPPUNIT_ASSERT(NULL != pBlock);
	nErr = pMemHelper->GetObjRMStatusCtrl((tagRMStatusCtrl*)((char*)pBlock + sizeof(tagObjStatus)+sizeof(tagObjRMStatus)), nStatus);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);

	// ���ò����ڵ�����������������
	nErr = pMemHelper->SetObjRMStatusCtrl(RM_STATUS_CTRL_ACTIVE,"IOMBTCP");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// ���÷Ƿ�����������ֵ
	nErr = pMemHelper->SetObjRMStatusCtrl(10,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);
}

void ShareMemHelperTester::testSetAndGetObjStatus()
{
	printf("testSetAndGetObjStatus\n");
	long nStatus = 0;
	long nErr = 0;

	// ���ö���״̬ΪACTIVE
	nErr = pMemHelper->SetObjStatus(ICV_RM_OBJ_STATUS_NOT_READY,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	nErr = pMemHelper->GetObjStatus(nStatus,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == ICV_RM_OBJ_STATUS_NOT_READY);
	// ���÷Ƿ�״̬
	nErr = pMemHelper->SetObjStatus(10,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// ���ò����ڵĶ���״̬
	nErr = pMemHelper->SetObjStatus(ICV_RM_OBJ_STATUS_NOT_READY,"IOMBTCP");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// ��ȡ�����ڵĶ���״̬
	nErr = pMemHelper->GetObjStatus(nStatus,"IOMBTCP");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);
}

void ShareMemHelperTester::testNeedSync()
{
	printf("testNeedSync\n");
	long nStatus = 0;
	long nErr = 0;
	RMObject rmObj;

	// ���ö�����ͬ��
	bool bNeedSync;
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bNeedSync = false;
	strcpy(rmObj.szObjName,"IOMBTCP:device1");
	pMemHelper->AddObject(&rmObj, 0);
	nErr = pMemHelper->NeedSync("IOMBTCP:device1",bNeedSync);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedSync == false);

	// ���ö�����ͬ�����Ҷ�ڵ�����ģʽ���ԵȽڵ�״̬�
	//memset(&rmObj,0, sizeof(RMObject));
	//rmObj.bNeedSync = true;
	//rmObj.bBeSingle = false;
	//strcpy(rmObj.szObjName,"IOMBTCP:device2");
	//nErr = pMemHelper->AddObject(&rmObj, 0);
	//CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	//tagObjRMStatusBlock objStatusBlock;
	//tagObjRMStatusBlock *pTmpBlock = pMemHelper->FindObjBlockByName("IOMBTCP:device2");
	//CPPUNIT_ASSERT(NULL != pTmpBlock);
	//memcpy(&objStatusBlock, pTmpBlock, sizeof(tagObjRMStatusBlock));
	//objStatusBlock.objRmStatus.nPeerStatus = RM_STATUS_ACTIVE;
	//objStatusBlock.objRmStatus.peerTimeStamp = ACE_OS::gettimeofday();
	//objStatusBlock.objRmStatus.nStatus = RM_STATUS_ACTIVE;
	//objStatusBlock.objRmStatus.timeStamp = ACE_OS::gettimeofday();
	//nErr = pMemHelper->SetObjBlockByIdx(rmObj.nObjBlockIdx,&objStatusBlock);
	//CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	//nErr = pMemHelper->NeedSync("IOMBTCP:device2",bNeedSync);
	//CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	//CPPUNIT_ASSERT(bNeedSync == false);

	// ���ڵ����У���ͬ��
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bNeedSync = true;
	rmObj.bBeSingle = true;
	strcpy(rmObj.szObjName,"IOMBTCP:device3");
	pMemHelper->AddObject(&rmObj, 0);
	nErr = pMemHelper->NeedSync("IOMBTCP:device3",bNeedSync);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedSync == true);

}

void ShareMemHelperTester::testNeedDoSvc()
{
	printf("testNeedDoSvc\n");
	long nStatus = 0;
	long nErr = 0;
	RMObject rmObj;

	// ��ڵ����У���ִ�з���
	bool bNeedDo;
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bBeSingle = false;
	strcpy(rmObj.szObjName,"IOMBTCP:device4");
	pMemHelper->AddObject(&rmObj, 0);
	nErr = pMemHelper->NeedDoSvc("IOMBTCP:device4",bNeedDo);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedDo == true);

	// ���ڵ����У�����״̬ΪINACTIVE����ִ�з���
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bBeSingle = true;
	strcpy(rmObj.szObjName,"IOMBTCP:device5");
	pMemHelper->AddObject(&rmObj, 0);
	pMemHelper->SetObjRMStatus(RM_STATUS_INACTIVE,"IOMBTCP:device5");
	nErr = pMemHelper->NeedDoSvc("IOMBTCP:device5",bNeedDo);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedDo == false);

	// ���ڵ����У�����״̬ΪACTIVE������ִ�з���
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bBeSingle = true;
	strcpy(rmObj.szObjName,"IOMBTCP:device6");
	pMemHelper->AddObject(&rmObj, 0);
	pMemHelper->SetObjRMStatus(RM_STATUS_ACTIVE,"IOMBTCP:device6");
	nErr = pMemHelper->NeedDoSvc("IOMBTCP:device6",bNeedDo);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedDo == true);
}