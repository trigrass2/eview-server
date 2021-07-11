/**************************************************************
*  Filename:    TaskGroup.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 设备组信息实体类.
*
*  @author:     shijunpu
*  @version     05/28/2008  shijunpu  Initial Version
**************************************************************/
#include "ace/High_Res_Timer.h"
#include "TaskGroup.h"
#include "common/CommHelper.h"
#include "errcode/ErrCode_iCV_DA.hxx"
#include "MainTask.h"
#include "pklog/pklog.h"
#include "pkcomm/pkcomm.h"
#include "MainTask.h"
#include <stdlib.h>
#include <cmath>
#include "pksock/pksock_errcode.h"
#include "shmqueue/FixLenProcessQueue.h"
#include "common/eviewcomm_internal.h"
#define	PROCESSQUEUE_SIMPLE_SET_RECORD_LENGTH	4 * 1024				// 每条进程管理器的控制命令大大小

extern	CPKLog			g_logger;

#ifndef MAX_PATH
#define MAX_PATH		260
#endif // MAX_PATH

// 由主线程MainTask发给设备组线程的命令，ACE_Message_Block中的在线通知消息
#define MSG_DEVICE_CONTROL					0
#define MSG_TaskGroup_ONLINECFG				1

template<class T> bool MapKeyCmpTaskGroup(T &left, T &right)
{
    return left.first < right.first;
}

extern int TagValFromString2Bin(PKTAG *pTag, int nTagDataType, const char *szStrValue, char *szBinData, int nBinDataBufLen, int *pnResultBinDataBits);
/**
*  构造函数.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CTaskGroup::CTaskGroup(const char *szGroupName, const char *szDescription)
{
    if (szGroupName != NULL)
        m_drvObjBase.m_strName = szGroupName;
    if (szDescription != NULL)
        m_drvObjBase.m_strDesc = szDescription;
    // m_pGrpThread = NULL;
    m_pMainTask = NULL; 

    ACE_Select_Reactor *pSelectReactor = new ACE_Select_Reactor();
    m_pReactor = new ACE_Reactor(pSelectReactor, true);
	ACE_Task<ACE_MT_SYNCH>::reactor(m_pReactor);

	m_nQueRecBufferLen = 1 * 1024 * 1024; 
	m_pQueRecBuffer = new char[m_nQueRecBufferLen];

	m_nTagVTQBufferLen = 1 * 1024 * 1024;	// 1个TAG点的内存大小 
	m_pTagVTQBuffer = new char[m_nTagVTQBufferLen];	// 1个TAG点的内存指针

	m_pTagValueBuffer = new char[m_nTagVTQBufferLen];	// 1个TAG点的内存指针
	m_nTagValueBufferLen = m_nTagVTQBufferLen;	// 1个TAG点的内存大小;
}

void CTaskGroup::DestroyDevices()
{
    // 遍历DeviceList，删除所有的设备
    std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
    for (; iterDev != m_mapDevices.end(); iterDev++)
    {
        CDevice *pDeviceTmp = (CDevice *)iterDev->second;
        // SAFE_DELETE(pDeviceTmp); 此处在退出时会异常
    }
    m_mapDevices.clear();
}

/**
*  析构函数.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CTaskGroup::~CTaskGroup()
{
    // g_logger.LogMessage(PK_LOGLEVEL_INFO, "开始析构DevGroup=%s", m_drvObjBase.m_strName.c_str());

    // 遍历DeviceList，删除所有的设备
    DestroyDevices();

    // 必须放在设备删除之后，否则设备和数据块中的ACE_Event_Handler会调用reator的Cancel_timer，导致reactor已删除的异常
    if (m_pReactor)
    {
        delete m_pReactor;
        m_pReactor = NULL;
    } 
}

/**
*  启动线程
*
*
*  @version     07/09/2008  shijunpu  Initial Version.
*/
int CTaskGroup::Start()
{
    m_bExit = false;
    int nRet = this->activate(THR_NEW_LWP | THR_JOINABLE | THR_INHERIT_SCHED);
    if (0 == nRet)
        return PK_SUCCESS;

    return nRet;
}

void CTaskGroup::Stop()
{
    this->m_bExit = true;
    // 先取消定时器，免得累计了很多定时器消息导致无法退出
    std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
    for (; iterDev != m_mapDevices.end(); iterDev++)
    {
        iterDev->second->DestroyTimers();
    }

    ACE_Task<ACE_MT_SYNCH>::reactor()->end_reactor_event_loop(); // 停止消息循环

    this->wait();
    g_logger.LogMessage(PK_LOGLEVEL_WARN, "TaskGroup devicegroup:%s thread exited!", m_drvObjBase.m_strName.c_str());
}

int CTaskGroup::svc()
{
	ACE_Task<ACE_MT_SYNCH>::reactor()->owner(ACE_OS::thr_self());

    OnStart();
    while (!m_bExit)
    {
		ACE_Task<ACE_MT_SYNCH>::reactor()->reset_reactor_event_loop();
		ACE_Task<ACE_MT_SYNCH>::reactor()->run_reactor_event_loop();
    }

    OnStop();

    return 0;
}

// Group线程启动时
void CTaskGroup::OnStart()
{
	// 开启一个发送各个通道是否允许连接的状态到nodeserver
	ACE_Time_Value	tvPollRate;		// 扫描周期，单位ms
	tvPollRate.msec(5000);
	ACE_Time_Value tvPollPhase;
	tvPollPhase.msec(0);
	tvPollPhase += ACE_Time_Value::zero;
	m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, tvPollPhase, tvPollRate);

    // 线程应该已经启动,这时启动设备的连接、在设备中启动数据块的定时
    std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
    for (; iterDev != m_mapDevices.end(); iterDev++)
        iterDev->second->Start();
}

// Group线程停止时
void CTaskGroup::OnStop()
{
	m_pReactor->cancel_timer((ACE_Event_Handler *)(CUserTimer *)this);

    // 线程应该已经启动,这时启动设备的连接、在设备中启动数据块的定时
    std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
    for (; iterDev != m_mapDevices.end(); iterDev++)
    {
        iterDev->second->Stop();
    }
    
	DestroyDevices();


	ACE_Task<ACE_MT_SYNCH>::reactor()->end_reactor_event_loop(); // 停止消息循环
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "driver %s taskgroup[%s] exit...", g_strDrvName.c_str(), this->m_drvObjBase.m_strName.c_str());
    m_pReactor->close();
}

void CTaskGroup::OnOnlineConfigModified(CTaskGroup *pNewGrp)
{
    m_drvObjBase.m_strDesc = pNewGrp->m_drvObjBase.m_strDesc;

    map<string, CDevice*> &mapNewDevices = pNewGrp->m_mapDevices;
    map<string, CDevice*> &mapCurDevices = this->m_mapDevices;

    // 生成新配置和已有配置之间的差别信息
    map<string, CDevice*> mapToDelete, mapToModify, mapToAdd;
    std::set_difference(mapCurDevices.begin(), mapCurDevices.end(),
        mapNewDevices.begin(), mapNewDevices.end(),
        std::inserter(mapToDelete, mapToDelete.begin()), MapKeyCmpTaskGroup<map<string, CDevice*>::value_type>);

    std::set_difference(mapNewDevices.begin(), mapNewDevices.end(),
        mapCurDevices.begin(), mapCurDevices.end(),
        std::inserter(mapToAdd, mapToAdd.begin()), MapKeyCmpTaskGroup<map<string, CDevice*>::value_type>);

    std::set_intersection(mapNewDevices.begin(), mapNewDevices.end(),
        mapCurDevices.begin(), mapCurDevices.end(),
        std::inserter(mapToModify, mapToModify.begin()), MapKeyCmpTaskGroup<map<string, CDevice*>::value_type>);

    //对于修改的设备，我们采取先删除再添加，所以将当前设备列表中修改的设备放到删除列表里
    //把从配置中读取到的设备列表中修改的设备放到添加列表里
    map<string, CDevice*>::iterator iterModify = mapToModify.begin();
    map<string, CDevice*>::iterator iterCur;
    for(; iterModify != mapToModify.end(); )
    {
        if ((iterCur = mapCurDevices.find(iterModify->first)) != mapCurDevices.end())
        {
            if (!(*iterCur->second == *iterModify->second))
            {
                mapToDelete.insert(*iterCur);
                mapToAdd.insert(*iterModify);
                mapToModify.erase(iterModify++);
                continue;
            }
        }
        ++iterModify;
    }

    //修改删除逻辑，删除多次造成空指针异常，edit by shijunpu @20130426

    //2、释放内存，将删除的设备从现有的设备列表中移除
    for (map<string, CDevice*>::iterator iterDel = mapToDelete.begin(); iterDel != mapToDelete.end(); iterDel++)
    {
        iterDel->second->Stop();
        SAFE_DELETE(iterDel->second);
        m_mapDevices.erase(iterDel->first);
    }

    // 增加新的设备
    int nSupportMultiLink = 0;
    for (map<string, CDevice*>::iterator iterAdd = mapToAdd.begin(); iterAdd != mapToAdd.end(); iterAdd++)
    {
        iterAdd->second->m_pTaskGroup = this;
        m_mapDevices.insert(map<string, CDevice*>::value_type(iterAdd->first, iterAdd->second));
        mapNewDevices.erase(iterAdd->first);
        m_mapDevices[iterAdd->first]->Start();
    }

    // 修改设备
    m_pMainTask->ModifyDevices(mapCurDevices, mapToModify);

    //组下的所有设备已经在新增、删除和变更设备里处理，所以直接将该device map重置
    pNewGrp->m_mapDevices.clear();
    // pNewGrp->Stop();
    SAFE_DELETE(pNewGrp);
}

// 投递一个控制命令道设备所在任务
//*  @version	8/20/2012  shijunpu  无需缓存消息到Maintask线程队列中，删除冗余费解代码.
void CTaskGroup::PostControlCmd(CDevice *pDevice, PKTAG *pTag, string strTagValue)
{
	// 不能存入pTag中，因为控制未必会生效，因此需要一个临时存储的变量
    PKTAGDATA tagData;
	tagData.nDataLen = pTag->nDataLen;
	tagData.szData = new char[tagData.nDataLen + 1];
	memset(tagData.szData, 0, tagData.nDataLen + 1);
	tagData.nDataType = pTag->nDataType;

    int nLenBits;
	TagValFromString2Bin(pTag, tagData.nDataType, strTagValue.c_str(), tagData.szData, tagData.nDataLen, &nLenBits);

    int nCtrlMsgLen = sizeof(int) + 2 * sizeof(ACE_UINT64) + (sizeof(int) + tagData.nDataLen);
    ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

    int nCmdCode = MSG_DEVICE_CONTROL;

    // 控制命令长度
    memcpy(pMsg->wr_ptr(), &nCmdCode, sizeof(int));
    pMsg->wr_ptr(sizeof(int));

	ACE_UINT64 nPointer = 0;
   // char szTmp[100] = { 0 };
    // 设备地址
    //sprintf(szTmp, "%d", pDevice);
    //ACE_UINT nPointer = ::atoll(szTmp); //reinterpret_cast<int>(pNewTaskGroup);
	nPointer = (ACE_UINT64)pDevice;
    memcpy(pMsg->wr_ptr(), &nPointer, sizeof(ACE_UINT64));
    pMsg->wr_ptr(sizeof(ACE_UINT64));

    // tag地址
    //sprintf(szTmp, "%ld", pTag);
    //nPointer = ::atoll(szTmp); //reinterpret_cast<int>(pNewTaskGroup);
	nPointer = (ACE_UINT64)pTag;
    memcpy(pMsg->wr_ptr(), &nPointer, sizeof(ACE_UINT64));
    pMsg->wr_ptr(sizeof(ACE_UINT64));

    // 命令长度和内容
    memcpy(pMsg->wr_ptr(), &tagData.nDataLen, sizeof(int));
    pMsg->wr_ptr(sizeof(int));
    memcpy(pMsg->wr_ptr(), tagData.szData, tagData.nDataLen);
    pMsg->wr_ptr(tagData.nDataLen);

    ACE_Time_Value tvNow = ACE_OS::gettimeofday();
    int nRet = putq(pMsg, &tvNow);
    ACE_Task<ACE_MT_SYNCH>::reactor()->notify(this, ACE_Event_Handler::WRITE_MASK);

    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "CONTROL COMMAND[tag:%s, address:%s, value:%s], success send to device:%s, devicegroup:%s",
        pTag->szName, pTag->szAddress, strTagValue.c_str(), pDevice->m_strName.c_str(), this->m_drvObjBase.m_strName.c_str());
}

// 投递一个控制命令道设备所在任务
void CTaskGroup::PostGroupOnlineModifyCmd(CTaskGroup *pNewTaskGroup)
{
    int nCtrlMsgLen = sizeof(int) + sizeof(int);
    ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

    int nCmdCode = MSG_TaskGroup_ONLINECFG;
    // 控制命令类型
    memcpy(pMsg->wr_ptr(), &nCmdCode, sizeof(int));
    pMsg->wr_ptr(sizeof(int));

    // 控制命令长度。reinterpret_cast<int>(pNewTaskGroup);等在HP Unix下都编译不过，因此需要用以下歪招szzs
    char szTmp[100] = { 0 };
    sprintf(szTmp, "%d", pNewTaskGroup);
    int nPointer = ::atoi(szTmp); //reinterpret_cast<int>(pNewTaskGroup);
    memcpy(pMsg->wr_ptr(), (void *)&nPointer, sizeof(CTaskGroup *));
    pMsg->wr_ptr(sizeof(CTaskGroup *));

    putq(pMsg);
    ACE_Task<ACE_MT_SYNCH>::reactor()->notify(this, ACE_Event_Handler::WRITE_MASK);
}

/**
*  $(Desp) .
*  $(Detail) .
*
*  @param		-[in]  ACE_HANDLE fd : [comment]
*  @return		int.
*
*  @version	11/20/2012  shijunpu  Initial Version.
*  @version	8/20/2012  shijunpu  修复判断队列中是否有数据判断条件错误，导致有多个消息时仅仅处理了最后一条消息.
*/
int CTaskGroup::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/)
{
	HandleWriteCmd();
   
    return 0;
}

/**
*  $(Desp) 这里仅仅处理控制消息，不能处理在线变更，否则会造成逻辑异常.
*  $(Detail) .
*
*  @return		int.
*
*  @version	5/2/2013  shijunpu  Initial Version.
*/
int CTaskGroup::HandleWriteCmd()
{
    ACE_Message_Block *pMsgBlk = NULL;
    std::list<ACE_Message_Block *> listUnHandledMsg;
    ACE_Time_Value tvTimes = ACE_OS::gettimeofday();
    while (getq(pMsgBlk, &tvTimes) >= 0)
    {
        int nCmdCode = MSG_TaskGroup_ONLINECFG;
        memcpy(&nCmdCode, pMsgBlk->rd_ptr(), sizeof(int));
        if (nCmdCode == MSG_DEVICE_CONTROL)
        {
            pMsgBlk->rd_ptr(sizeof(int));

            // 设备地址
            ACE_UINT64 nPointer = 0;
            memcpy(&nPointer, pMsgBlk->rd_ptr(), sizeof(ACE_UINT64));
            pMsgBlk->rd_ptr(sizeof(ACE_UINT64));
            CDevice *pDevice = (CDevice *)nPointer;

            memcpy(&nPointer, pMsgBlk->rd_ptr(), sizeof(ACE_UINT64));
            pMsgBlk->rd_ptr(sizeof(ACE_UINT64));
            PKTAG *pTag = (PKTAG *)nPointer;

            // 命令长度和内容。目前支持点的最大长度为127（Blob和Text的最大长度）
            PKTAGDATA tagData;
            memcpy(&tagData.nDataLen, pMsgBlk->rd_ptr(), sizeof(int));
            pMsgBlk->rd_ptr(sizeof(int));
            if (tagData.nDataLen > 0)
            {
				tagData.szData = new char[tagData.nDataLen + 1];
				memset(tagData.szData, 0, tagData.nDataLen + 1);
                memcpy(tagData.szData, pMsgBlk->rd_ptr(), tagData.nDataLen);
                pMsgBlk->rd_ptr(tagData.nDataLen);
                tagData.szData[tagData.nDataLen] = '\0'; // 确保最后一个是0，方便字符串时的处理
            }

			pDevice->OnWriteCommand(pTag, &tagData);
            pMsgBlk->release();
        }
        else
        {
            //把刷新消息放到队列中，在handle_output函数里处理
            listUnHandledMsg.push_back(pMsgBlk);
        }
    }

    for (std::list<ACE_Message_Block *>::iterator iter = listUnHandledMsg.begin(); iter != listUnHandledMsg.end(); ++iter)
        putq(*iter);

    return 0;
}


int CTaskGroup::UpdateTaskStatus(int lStatus, char *szErrorTip, char *pszTime)
{
    char szTime[PK_HOSTNAMESTRING_MAXLEN] = { '\0' };
    char *pRealTime = pszTime;
    if (pRealTime == NULL)
    {
        PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));
        pRealTime = szTime;
    }
    char szKey[PK_NAME_MAXLEN * 2] = { 0 };
    PKStringHelper::Snprintf(szKey, sizeof(szKey), "%s:_tasks_:%s", g_strDrvName.c_str(), m_drvObjBase.m_strName.c_str());
    char szVTQStatus[PROCESSQUEUE_SIMPLE_SET_RECORD_LENGTH] = { 0 };
    // 更新驱动运行状态到shm
    sprintf(szVTQStatus, "\"v\":\"\",\"t\":\"%s\",\"q\":\"%d\",\"m\":\"%s\"", szTime, lStatus, szErrorTip);
	return PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szVTQStatus, strlen(szVTQStatus));
}

// 这个定时器，仅仅用于更新：连接状态点、允许连接点
int CTaskGroup::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	// 先取消定时器，免得累计了很多定时器消息导致无法退出
	std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
	for (; iterDev != m_mapDevices.end(); iterDev++)
	{
		CDevice *pDevice = iterDev->second;
		const char * szVal = pDevice->GetEnableConnect() ? "1" :"0";
		Drv_SetTagData_Text(pDevice->m_pTagEnableConnect, szVal, 0, 0, 0);
		vector<PKTAG *> vecTags;
		vecTags.push_back(pDevice->m_pTagConnStatus);
		vecTags.push_back(pDevice->m_pTagEnableConnect);
		pDevice->UpdateTagsData(vecTags.data(), vecTags.size());
		vecTags.clear();

		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "设备: device.%s.enableconnect, 值:%s!", pDevice->m_strName.c_str(), szVal);
	}
		
	return 0;
}
