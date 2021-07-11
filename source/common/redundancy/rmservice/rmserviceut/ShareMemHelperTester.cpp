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

	// 设置对象冗余状态为INACTIVE
	long nErr = pMemHelper->SetObjRMStatus(RM_STATUS_INACTIVE,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	nErr = pMemHelper->GetObjRMStatus(nStatus,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_INACTIVE);

	// 设置对象冗余状态为ACTIVE
	nErr = pMemHelper->SetObjRMStatus(RM_STATUS_ACTIVE,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	nErr = pMemHelper->GetObjRMStatus(nStatus,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);

	// 设置不存在的冗余对象状态
	nErr = pMemHelper->SetObjRMStatus(nStatus, "IOMBTCP");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// 设置非法的冗余对象状态
	nErr = pMemHelper->SetObjRMStatus(10, "IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// 设置SCADA节点冗余状态为ACTIVE,获取不存在的冗余对象状态
	pMemHelper->SetStatus(RM_STATUS_ACTIVE);
	nErr = pMemHelper->GetObjRMStatus(nStatus,"IOMBTCP");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);

	// 设置父节点冗余状态为ACTIVE，获取不存在的子节点的冗余状态
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

	// 设置对象冗余命令为ACTIVE
	nErr = pMemHelper->SetObjRMStatusCtrl(RM_STATUS_CTRL_ACTIVE,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	tagObjRMStatusBlock *pBlock = pMemHelper->FindObjBlockByName("IOMBTCP:device0");
	CPPUNIT_ASSERT(NULL != pBlock);
	nErr = pMemHelper->GetObjRMStatusCtrl((tagRMStatusCtrl*)((char*)pBlock + sizeof(tagObjStatus)+sizeof(tagObjRMStatus)), nStatus);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == RM_STATUS_ACTIVE);

	// 设置不存在的冗余对象的冗余命令
	nErr = pMemHelper->SetObjRMStatusCtrl(RM_STATUS_CTRL_ACTIVE,"IOMBTCP");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// 设置非法的冗余命令值
	nErr = pMemHelper->SetObjRMStatusCtrl(10,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);
}

void ShareMemHelperTester::testSetAndGetObjStatus()
{
	printf("testSetAndGetObjStatus\n");
	long nStatus = 0;
	long nErr = 0;

	// 设置对象状态为ACTIVE
	nErr = pMemHelper->SetObjStatus(ICV_RM_OBJ_STATUS_NOT_READY,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	nErr = pMemHelper->GetObjStatus(nStatus,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(nStatus == ICV_RM_OBJ_STATUS_NOT_READY);
	// 设置非法状态
	nErr = pMemHelper->SetObjStatus(10,"IOMBTCP:device0");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// 设置不存在的对象状态
	nErr = pMemHelper->SetObjStatus(ICV_RM_OBJ_STATUS_NOT_READY,"IOMBTCP");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);

	// 获取不存在的对象状态
	nErr = pMemHelper->GetObjStatus(nStatus,"IOMBTCP");
	CPPUNIT_ASSERT(nErr != ICV_SUCCESS);
}

void ShareMemHelperTester::testNeedSync()
{
	printf("testNeedSync\n");
	long nStatus = 0;
	long nErr = 0;
	RMObject rmObj;

	// 配置对象不需同步
	bool bNeedSync;
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bNeedSync = false;
	strcpy(rmObj.szObjName,"IOMBTCP:device1");
	pMemHelper->AddObject(&rmObj, 0);
	nErr = pMemHelper->NeedSync("IOMBTCP:device1",bNeedSync);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedSync == false);

	// 配置对象需同步，且多节点运行模式，对等节点状态活动
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

	// 单节点运行，需同步
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

	// 多节点运行，需执行服务
	bool bNeedDo;
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bBeSingle = false;
	strcpy(rmObj.szObjName,"IOMBTCP:device4");
	pMemHelper->AddObject(&rmObj, 0);
	nErr = pMemHelper->NeedDoSvc("IOMBTCP:device4",bNeedDo);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedDo == true);

	// 单节点运行，冗余状态为INACTIVE，需执行服务
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bBeSingle = true;
	strcpy(rmObj.szObjName,"IOMBTCP:device5");
	pMemHelper->AddObject(&rmObj, 0);
	pMemHelper->SetObjRMStatus(RM_STATUS_INACTIVE,"IOMBTCP:device5");
	nErr = pMemHelper->NeedDoSvc("IOMBTCP:device5",bNeedDo);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedDo == false);

	// 单节点运行，冗余状态为ACTIVE，不需执行服务
	memset(&rmObj,0, sizeof(RMObject));
	rmObj.bBeSingle = true;
	strcpy(rmObj.szObjName,"IOMBTCP:device6");
	pMemHelper->AddObject(&rmObj, 0);
	pMemHelper->SetObjRMStatus(RM_STATUS_ACTIVE,"IOMBTCP:device6");
	nErr = pMemHelper->NeedDoSvc("IOMBTCP:device6",bNeedDo);
	CPPUNIT_ASSERT(nErr == ICV_SUCCESS);
	CPPUNIT_ASSERT(bNeedDo == true);
}