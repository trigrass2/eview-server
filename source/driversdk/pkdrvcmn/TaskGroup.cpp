/**************************************************************
*  Filename:    TaskGroup.cpp
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: �豸����Ϣʵ����.
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
#define	PROCESSQUEUE_SIMPLE_SET_RECORD_LENGTH	4 * 1024				// ÿ�����̹������Ŀ���������С

extern	CPKLog			g_logger;

#ifndef MAX_PATH
#define MAX_PATH		260
#endif // MAX_PATH

// �����߳�MainTask�����豸���̵߳����ACE_Message_Block�е�����֪ͨ��Ϣ
#define MSG_DEVICE_CONTROL					0
#define MSG_TaskGroup_ONLINECFG				1

template<class T> bool MapKeyCmpTaskGroup(T &left, T &right)
{
    return left.first < right.first;
}

extern int TagValFromString2Bin(PKTAG *pTag, int nTagDataType, const char *szStrValue, char *szBinData, int nBinDataBufLen, int *pnResultBinDataBits);
/**
*  ���캯��.
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

	m_nTagVTQBufferLen = 1 * 1024 * 1024;	// 1��TAG����ڴ��С 
	m_pTagVTQBuffer = new char[m_nTagVTQBufferLen];	// 1��TAG����ڴ�ָ��

	m_pTagValueBuffer = new char[m_nTagVTQBufferLen];	// 1��TAG����ڴ�ָ��
	m_nTagValueBufferLen = m_nTagVTQBufferLen;	// 1��TAG����ڴ��С;
}

void CTaskGroup::DestroyDevices()
{
    // ����DeviceList��ɾ�����е��豸
    std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
    for (; iterDev != m_mapDevices.end(); iterDev++)
    {
        CDevice *pDeviceTmp = (CDevice *)iterDev->second;
        // SAFE_DELETE(pDeviceTmp); �˴����˳�ʱ���쳣
    }
    m_mapDevices.clear();
}

/**
*  ��������.
*
*
*  @version     05/30/2008  shijunpu  Initial Version.
*/
CTaskGroup::~CTaskGroup()
{
    // g_logger.LogMessage(PK_LOGLEVEL_INFO, "��ʼ����DevGroup=%s", m_drvObjBase.m_strName.c_str());

    // ����DeviceList��ɾ�����е��豸
    DestroyDevices();

    // ��������豸ɾ��֮�󣬷����豸�����ݿ��е�ACE_Event_Handler�����reator��Cancel_timer������reactor��ɾ�����쳣
    if (m_pReactor)
    {
        delete m_pReactor;
        m_pReactor = NULL;
    } 
}

/**
*  �����߳�
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
    // ��ȡ����ʱ��������ۼ��˺ܶඨʱ����Ϣ�����޷��˳�
    std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
    for (; iterDev != m_mapDevices.end(); iterDev++)
    {
        iterDev->second->DestroyTimers();
    }

    ACE_Task<ACE_MT_SYNCH>::reactor()->end_reactor_event_loop(); // ֹͣ��Ϣѭ��

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

// Group�߳�����ʱ
void CTaskGroup::OnStart()
{
	// ����һ�����͸���ͨ���Ƿ��������ӵ�״̬��nodeserver
	ACE_Time_Value	tvPollRate;		// ɨ�����ڣ���λms
	tvPollRate.msec(5000);
	ACE_Time_Value tvPollPhase;
	tvPollPhase.msec(0);
	tvPollPhase += ACE_Time_Value::zero;
	m_pReactor->schedule_timer((ACE_Event_Handler *)this, NULL, tvPollPhase, tvPollRate);

    // �߳�Ӧ���Ѿ�����,��ʱ�����豸�����ӡ����豸���������ݿ�Ķ�ʱ
    std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
    for (; iterDev != m_mapDevices.end(); iterDev++)
        iterDev->second->Start();
}

// Group�߳�ֹͣʱ
void CTaskGroup::OnStop()
{
	m_pReactor->cancel_timer((ACE_Event_Handler *)(CUserTimer *)this);

    // �߳�Ӧ���Ѿ�����,��ʱ�����豸�����ӡ����豸���������ݿ�Ķ�ʱ
    std::map<std::string, CDevice*>::iterator iterDev = m_mapDevices.begin();
    for (; iterDev != m_mapDevices.end(); iterDev++)
    {
        iterDev->second->Stop();
    }
    
	DestroyDevices();


	ACE_Task<ACE_MT_SYNCH>::reactor()->end_reactor_event_loop(); // ֹͣ��Ϣѭ��
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "driver %s taskgroup[%s] exit...", g_strDrvName.c_str(), this->m_drvObjBase.m_strName.c_str());
    m_pReactor->close();
}

void CTaskGroup::OnOnlineConfigModified(CTaskGroup *pNewGrp)
{
    m_drvObjBase.m_strDesc = pNewGrp->m_drvObjBase.m_strDesc;

    map<string, CDevice*> &mapNewDevices = pNewGrp->m_mapDevices;
    map<string, CDevice*> &mapCurDevices = this->m_mapDevices;

    // ���������ú���������֮��Ĳ����Ϣ
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

    //�����޸ĵ��豸�����ǲ�ȡ��ɾ������ӣ����Խ���ǰ�豸�б����޸ĵ��豸�ŵ�ɾ���б���
    //�Ѵ������ж�ȡ�����豸�б����޸ĵ��豸�ŵ�����б���
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

    //�޸�ɾ���߼���ɾ�������ɿ�ָ���쳣��edit by shijunpu @20130426

    //2���ͷ��ڴ棬��ɾ�����豸�����е��豸�б����Ƴ�
    for (map<string, CDevice*>::iterator iterDel = mapToDelete.begin(); iterDel != mapToDelete.end(); iterDel++)
    {
        iterDel->second->Stop();
        SAFE_DELETE(iterDel->second);
        m_mapDevices.erase(iterDel->first);
    }

    // �����µ��豸
    int nSupportMultiLink = 0;
    for (map<string, CDevice*>::iterator iterAdd = mapToAdd.begin(); iterAdd != mapToAdd.end(); iterAdd++)
    {
        iterAdd->second->m_pTaskGroup = this;
        m_mapDevices.insert(map<string, CDevice*>::value_type(iterAdd->first, iterAdd->second));
        mapNewDevices.erase(iterAdd->first);
        m_mapDevices[iterAdd->first]->Start();
    }

    // �޸��豸
    m_pMainTask->ModifyDevices(mapCurDevices, mapToModify);

    //���µ������豸�Ѿ���������ɾ���ͱ���豸�ﴦ������ֱ�ӽ���device map����
    pNewGrp->m_mapDevices.clear();
    // pNewGrp->Stop();
    SAFE_DELETE(pNewGrp);
}

// Ͷ��һ������������豸��������
//*  @version	8/20/2012  shijunpu  ���軺����Ϣ��Maintask�̶߳����У�ɾ������ѽ����.
void CTaskGroup::PostControlCmd(CDevice *pDevice, PKTAG *pTag, string strTagValue)
{
	// ���ܴ���pTag�У���Ϊ����δ�ػ���Ч�������Ҫһ����ʱ�洢�ı���
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

    // ���������
    memcpy(pMsg->wr_ptr(), &nCmdCode, sizeof(int));
    pMsg->wr_ptr(sizeof(int));

	ACE_UINT64 nPointer = 0;
   // char szTmp[100] = { 0 };
    // �豸��ַ
    //sprintf(szTmp, "%d", pDevice);
    //ACE_UINT nPointer = ::atoll(szTmp); //reinterpret_cast<int>(pNewTaskGroup);
	nPointer = (ACE_UINT64)pDevice;
    memcpy(pMsg->wr_ptr(), &nPointer, sizeof(ACE_UINT64));
    pMsg->wr_ptr(sizeof(ACE_UINT64));

    // tag��ַ
    //sprintf(szTmp, "%ld", pTag);
    //nPointer = ::atoll(szTmp); //reinterpret_cast<int>(pNewTaskGroup);
	nPointer = (ACE_UINT64)pTag;
    memcpy(pMsg->wr_ptr(), &nPointer, sizeof(ACE_UINT64));
    pMsg->wr_ptr(sizeof(ACE_UINT64));

    // ����Ⱥ�����
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

// Ͷ��һ������������豸��������
void CTaskGroup::PostGroupOnlineModifyCmd(CTaskGroup *pNewTaskGroup)
{
    int nCtrlMsgLen = sizeof(int) + sizeof(int);
    ACE_Message_Block *pMsg = new ACE_Message_Block(nCtrlMsgLen);

    int nCmdCode = MSG_TaskGroup_ONLINECFG;
    // ������������
    memcpy(pMsg->wr_ptr(), &nCmdCode, sizeof(int));
    pMsg->wr_ptr(sizeof(int));

    // ��������ȡ�reinterpret_cast<int>(pNewTaskGroup);����HP Unix�¶����벻���������Ҫ����������szzs
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
*  @version	8/20/2012  shijunpu  �޸��ж϶������Ƿ��������ж��������󣬵����ж����Ϣʱ�������������һ����Ϣ.
*/
int CTaskGroup::handle_output(ACE_HANDLE fd /*= ACE_INVALID_HANDLE*/)
{
	HandleWriteCmd();
   
    return 0;
}

/**
*  $(Desp) ����������������Ϣ�����ܴ������߱�������������߼��쳣.
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

            // �豸��ַ
            ACE_UINT64 nPointer = 0;
            memcpy(&nPointer, pMsgBlk->rd_ptr(), sizeof(ACE_UINT64));
            pMsgBlk->rd_ptr(sizeof(ACE_UINT64));
            CDevice *pDevice = (CDevice *)nPointer;

            memcpy(&nPointer, pMsgBlk->rd_ptr(), sizeof(ACE_UINT64));
            pMsgBlk->rd_ptr(sizeof(ACE_UINT64));
            PKTAG *pTag = (PKTAG *)nPointer;

            // ����Ⱥ����ݡ�Ŀǰ֧�ֵ����󳤶�Ϊ127��Blob��Text����󳤶ȣ�
            PKTAGDATA tagData;
            memcpy(&tagData.nDataLen, pMsgBlk->rd_ptr(), sizeof(int));
            pMsgBlk->rd_ptr(sizeof(int));
            if (tagData.nDataLen > 0)
            {
				tagData.szData = new char[tagData.nDataLen + 1];
				memset(tagData.szData, 0, tagData.nDataLen + 1);
                memcpy(tagData.szData, pMsgBlk->rd_ptr(), tagData.nDataLen);
                pMsgBlk->rd_ptr(tagData.nDataLen);
                tagData.szData[tagData.nDataLen] = '\0'; // ȷ�����һ����0�������ַ���ʱ�Ĵ���
            }

			pDevice->OnWriteCommand(pTag, &tagData);
            pMsgBlk->release();
        }
        else
        {
            //��ˢ����Ϣ�ŵ������У���handle_output�����ﴦ��
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
    // ������������״̬��shm
    sprintf(szVTQStatus, "\"v\":\"\",\"t\":\"%s\",\"q\":\"%d\",\"m\":\"%s\"", szTime, lStatus, szErrorTip);
	return PublishKVSimple2NodeServer(ACTIONTYPE_DRV2SERVER_SET_KV_SIMPLE, szKey, szVTQStatus, strlen(szVTQStatus));
}

// �����ʱ�����������ڸ��£�����״̬�㡢�������ӵ�
int CTaskGroup::handle_timeout(const ACE_Time_Value &current_time, const void *act /*= 0*/)
{
	// ��ȡ����ʱ��������ۼ��˺ܶඨʱ����Ϣ�����޷��˳�
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

		g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "�豸: device.%s.enableconnect, ֵ:%s!", pDevice->m_strName.c_str(), szVal);
	}
		
	return 0;
}
