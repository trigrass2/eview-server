/**************************************************************
 *  Filename:    CtrlProcessTask.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �����������.
 *
 *  @author:    shijunpu
**************************************************************/

#include "common/OS.h"
#include <ace/Default_Constants.h>
#include "ace/Thread_Mutex.h"
#include "ace/Guard_T.h"
#include "ace/High_Res_Timer.h"
#include "MainTask.h"
#include "errcode/ErrCode_iCV_DA.hxx"
#include "TaskGroup.h"
#include "Device.h"
#include "pkcomm/pkcomm.h"
#include "pkdriver/pkdrvcmn.h"
#include "math.h"
#include "json/json.h"
#include "common/RMAPI.h"
#include "common/eviewdefine.h"
#include "eviewcomm/eviewcomm.h"
#include "shmqueue/ProcessQueue.h"
#include "common/eviewcomm_internal.h"
#include "pkeviewdbapi/pkeviewdbapi.h"
#include "pklog/pklog.h"
#include "common/PKMiscHelper.h"
#include "tinyxml2/tinyxml2.h"
#include "ByteOrder.h"

extern CPKLog g_logger;
extern bool g_bDriverAlive;

extern CProcessQueue *			g_pQueCtrlFromNodeServer;			// �ͽ��̹���������ͨ�ŵĹ����ڴ����
extern CProcessQueue *			g_pQueData2NodeServer;
 extern void PKTAG_Reset(PKTAG *pTag);
extern void PKDEVICE_Reset(PKDEVICE *pDevice);
extern void PKDRIVER_Reset(PKDRIVER *);
extern void PKTIMER_Reset(PKTIMER *);
extern void PKTAG_Free(PKTAG *pTag);

#define  CHECK_NULL_RETURN_ERR(X) { if(!X) return EC_ICV_DA_CONFIG_FILE_STRUCTURE_ERROR; }

#define	XML_ELEMENTNODE_DEVICE		"device"
#define XML_ELEMENTNODE_NAME		"name"
#define XML_ELEMENTNODE_PARAM1		"param1"
#define XML_ELEMENTNODE_PARAM2		"param2"
#define XML_ELEMENTNODE_PARAM3		"param3"
#define XML_ELEMENTNODE_TASK		"taskno"
#define XML_ELEMENTNODE_CONNTYPE	"conntype"
#define XML_ELEMENTNODE_CONNPARAM	"connparam"
#define XML_ELEMENTNODE_RECVTIMEOUT	"recvtimeout"
#define XML_ELEMENTNODE_CONNTIMEOUT	"conntimeout"

#define XML_ELEMENTNODE_TYPE		"type"

#define TaskGroup_NAME_PREFIX		"TaskGroup"

#define BITSFLOAT_PER_BYTE			8.0f
#define DEFAULT_SLEEP_TIME			50 // ms

template<class T> bool MapKeyCmpMainTask(T &left, T &right)
{
    return left.first < right.first;
}

/**
 *  ���캯��.
 *
 *  @param  -[in,out]  CDriver*  pDriver: [comment]
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMainTask::CMainTask()
{
    m_bStop = false;
    m_bStopped = false;
    m_bManualRefreshConfig = false;
    g_pQueData2NodeServer = NULL;
    g_pQueCtrlFromNodeServer = NULL;
    m_nTagNum = 0;
    m_nDeviceNum = 0;
	m_nQueRecordLen = PROCESSQUEUE_QUEUE_ONRECORD_LENGHT;
	m_pQueRecordBuffer =  new char[m_nQueRecordLen];
}

/**
 *  ��������.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CMainTask::~CMainTask()
{
	delete []m_pQueRecordBuffer;
	m_pQueRecordBuffer = NULL;
}

/**
 *  �麯�����̳���ACE_Task.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
int CMainTask::svc()
{
    ACE_High_Res_Timer::global_scale_factor();

    // �����ȳ�ʼ�����̼�ͨ��
    int nRet = InitQueWithNodeServer();
    if(nRet != PK_SUCCESS)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR,"InitShmConn failed,ret:%d��", nRet);
        return -1;
    }

    nRet = OnStart();

    char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
    PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

    int nStatus = PK_SUCCESS;
    ACE_Time_Value tvSleep;
    tvSleep.msec(DEFAULT_SLEEP_TIME); // 100ms

    while(!m_bStop)
    {
        // �ж��Ƿ����ֹ�������������
        if(m_bManualRefreshConfig)
        {
            HandleOnlineConfig();
            m_bManualRefreshConfig = false;
        }

        //���taskgroup�Ƿ����˳��ģ��еĻ���taskgroup����
        std::map<std::string, CTaskGroup *>::iterator iter = m_driverInfo.m_mapTaskGroups.begin();
        for (; iter != m_driverInfo.m_mapTaskGroups.end(); ++iter)
        {
            if (iter->second->thr_count() == 0)
            {
                g_logger.LogMessage(PK_LOGLEVEL_ERROR,"MainTask devicegroup task:%s exit...", iter->first.c_str());
                iter->second->Stop();
                iter->second->Start();
            }
        }

        char *szRecBuffer = m_pQueRecordBuffer;
        unsigned int nRecLen = 0;
        bool bFoundRecord = false;
        // ��ȡ�����һ���������ݣ�ǰ��ļ�¼ȫ�����׵�
        int nResult = g_pQueCtrlFromNodeServer->DeQueue(szRecBuffer, m_nQueRecordLen, &nRecLen);
        if(nResult == PK_SUCCESS)
            bFoundRecord = true;
        // ����м�¼�����ȡ���һ����¼
        while(bFoundRecord)
        {
            szRecBuffer[nRecLen] = '\0';
            string strCtrlContent = szRecBuffer;
            Json::Reader reader;
            Json::Value root;
            if (!reader.parse(strCtrlContent, root, false))
            {
                g_logger.LogMessage(PK_LOGLEVEL_ERROR, "driver[%s] MainTask recv an msg(%s)!",g_strDrvName.c_str(), strCtrlContent.c_str());
                m_driverInfo.UpdateDriverStatus(-2,"control msg format invalid",szTime);
            }
            else
                ProcessWriteCmds(strCtrlContent, root);

            nResult = g_pQueCtrlFromNodeServer->DeQueue(szRecBuffer, sizeof(szRecBuffer), &nRecLen);
            if(nResult != PK_SUCCESS)
                break;
        }

        ACE_Time_Value tvCycle;
        tvCycle.msec(50);
        ACE_OS::sleep (tvCycle);

        //g_logger.LogMessage(PK_LOGLEVEL_DEBUG, "driver recv an msg:%s", szRecBuffer);
    }

    g_logger.LogMessage(PK_LOGLEVEL_INFO,"driver: %s exit normally!", g_strDrvName.c_str());
    OnStop();

    m_bStopped = true;
    return PK_SUCCESS;
}

// �����豸�����ݿ����Ƶõ������顢�豸�����ݿ�ָ��.��Ҫ����ͬ���豸�����ڶ���������е����
bool CMainTask::GetTaskGroupAndDeviceByTagName(string &strTagName, CTaskGroup *&pTaskGroup, CDevice *&pDevice, PKTAG *&pTag)
{
    bool bFound = false;
    pTaskGroup = NULL;
    pDevice = NULL;
    pTag = NULL;

    map<string, CTaskGroup*>::iterator iter = m_driverInfo.m_mapTaskGroups.begin();
    // �����豸�飬�����豸
    for ( ; iter != m_driverInfo.m_mapTaskGroups.end(); iter++)
    {
        CTaskGroup *pTmpTaskGroup = iter->second;
        map<string, CDevice*>::iterator iterDevice = pTmpTaskGroup->m_mapDevices.begin();
        for ( ; iterDevice != pTmpTaskGroup->m_mapDevices.end(); iterDevice++)
        {
            CDevice *pDeviceTmp = iterDevice->second;

			// �����ǲ�������豸����ͨ��
            std::map<string,PKTAG *>::iterator itTag = pDeviceTmp->m_mapName2Tags.find(strTagName);
            if(itTag != pDeviceTmp->m_mapName2Tags.end())
            {
                pTaskGroup = pTmpTaskGroup;
                pDevice = pDeviceTmp;
                pTag = itTag->second;
                bFound = true;
                break;
            }

			// �ٿ���������ǲ���enableconnect��
			if (strTagName.compare(pDeviceTmp->m_pTagEnableConnect->szName) == 0)
			{
				pTaskGroup = pTmpTaskGroup;
				pDevice = pDeviceTmp;
				pTag = pDeviceTmp->m_pTagEnableConnect;
				bFound = true;
				break;
			}
        }
    }
    return bFound;
}

/**
 *  ����д��������.�������ݰ��������������豸������������ֵ���ַ�����ʽ��
 *
 *  @param  -[in]  OUTPUT_QUEUE_DESC*  pOutput: [comment]
 *
 *  @version     07/10/2008  shijunpu  Initial Version.
 */
int CMainTask::ProcessWriteCmds(string &strCmds, Json::Value &root)
{
    int nStatus = PK_SUCCESS;
    vector<Json::Value *> vectorCmds;
    if (root.isObject())
    {
        vectorCmds.push_back(&root);
    }
    else if(root.isArray())
    {
        for(int i = 0; i < root.size(); i ++)
            vectorCmds.push_back(&root[i]);
    }
    else
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "ctrls(%s),format error��should json array or object��{\"type\":\"control\",\"data\":{\"name\":\"tag1\",\"value\":\"100\"}",strCmds.c_str());
        m_driverInfo.UpdateDriverStatus(-12,"control format invalid", NULL);
        return -12;
    }

    for(vector<Json::Value *>::iterator itCmd = vectorCmds.begin(); itCmd != vectorCmds.end(); itCmd ++){
        Json::Value &jsonCmd = **itCmd;
        ProcessWriteOneCmd(jsonCmd);
    }

    return PK_SUCCESS;
}

/**
 *  ����д��������.�������ݰ��������������豸������������ֵ���ַ�����ʽ��
 *
 *  @param  -[in]  OUTPUT_QUEUE_DESC*  pOutput: [comment]
 *
 *  @version     07/10/2008  shijunpu  Initial Version.
 */
int CMainTask::ProcessWriteOneCmd(Json::Value &ctrlInfo)
{
    int nStatus = PK_SUCCESS;

    // ���ݵ�ַ�õ��豸��Ϣ�������ַ������ð��:����
    Json::Value tagName = ctrlInfo["name"];
    Json::Value tagValue = ctrlInfo["value"];
    if(tagName.isNull() || tagValue.isNull()){
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��������ʱ��������ֵ��data�У�δ�ҵ�name��value����");
        return -1;
    }
    string strTagName = tagName.asString();
    string strTagValue = tagValue.asString();

    if(strTagName.empty() || strTagValue.empty())
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "��������ʱ��δ�ҵ�tag��(%s),tagֵ(%s)��ֵ", strTagName.c_str(), strTagValue.c_str());
        return -1;
    }

    // �����豸�����ݿ飬�õ������顢�豸�����ݿ����
    CTaskGroup *pTaskGroup = NULL;
    CDevice* pDevice = NULL;
    PKTAG *pTag = NULL;
    if(false == GetTaskGroupAndDeviceByTagName(strTagName, pTaskGroup, pDevice,pTag))
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "On Control, can not find task and device by tag: %s", strTagName.c_str());
        return -2;
    }

    // ���ַ�������ֵת��Ϊ�����ƻ��������ͳ�ȥ
    pTaskGroup->PostControlCmd(pDevice, pTag, strTagValue);
    return PK_SUCCESS;
}


// ���߸�������
void CMainTask::HandleOnlineConfig()
{
    //�����dit2�ڴ���tag�������ݿ��ڴ�ӳ�䣬����ÿ��Ҫ�ж�tag��ı仯Ч�ʺܲ�
    g_logger.LogMessage(PK_LOGLEVEL_INFO, "Reload Config File!");
    CDriver driver;
    driver.m_bAutoDeleted = false;	//����TaskGroup�����Զ�ɾ������Ϊ��ɾ�ĵĶ��ڳ����н����˴���

    char szTime[PK_HOSTNAMESTRING_MAXLEN] = {'\0'};
    PKTimeHelper::GetCurHighResTimeString(szTime, sizeof(szTime));

    int nSuccess = LoadConfig(driver);

    if(nSuccess != PK_SUCCESS)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "���������ļ�ʧ�ܣ�������: %d", nSuccess);
        std::map<string, CTaskGroup*>::iterator itTaskGrp = driver.m_mapTaskGroups.begin();
        for(; itTaskGrp != driver.m_mapTaskGroups.end();itTaskGrp ++)
        {
            CTaskGroup* pTaskGrpTmp = itTaskGrp->second;
            g_logger.LogMessage(PK_LOGLEVEL_WARN, "(HandleOnlineConfig)delete TaskGroup name: %s,device num:%d", pTaskGrpTmp->m_drvObjBase.m_strName.c_str(), pTaskGrpTmp->m_mapDevices.size());
            SAFE_DELETE(itTaskGrp->second);
        }

        driver.m_mapTaskGroups.clear();
        m_driverInfo.UpdateDriverOnlineConfigTime(-1, "onlineconfig:loadconfig failed",szTime);
        m_driverInfo.UpdateDriverStatus(-1, "configfile failed",szTime);
        return;
    }
    m_driverInfo.UpdateDriverOnlineConfigTime(0,"",szTime);

    // ���������ú���������֮��Ĳ����Ϣ
    map<string, CTaskGroup*> mapToDelete, mapToModify, mapToAdd;
    std::set_difference(m_driverInfo.m_mapTaskGroups.begin(), m_driverInfo.m_mapTaskGroups.end(),
        driver.m_mapTaskGroups.begin(), driver.m_mapTaskGroups.end(),
        std::inserter(mapToDelete, mapToDelete.begin()), MapKeyCmpMainTask<map<string, CTaskGroup*>::value_type>);

    std::set_intersection(driver.m_mapTaskGroups.begin(), driver.m_mapTaskGroups.end(),
        m_driverInfo.m_mapTaskGroups.begin(), m_driverInfo.m_mapTaskGroups.end(),
        std::inserter(mapToModify, mapToModify.begin()), MapKeyCmpMainTask<map<string, CTaskGroup*>::value_type>);

    std::set_difference(driver.m_mapTaskGroups.begin(), driver.m_mapTaskGroups.end(),
        m_driverInfo.m_mapTaskGroups.begin(), m_driverInfo.m_mapTaskGroups.end(),
        std::inserter(mapToAdd, mapToAdd.begin()), MapKeyCmpMainTask<map<string, CTaskGroup*>::value_type>);

    // �ȴ���ɾ�����豸��
    DelTaskGrps(mapToDelete);
    // �������ӵ��豸��

    m_driverInfo.AddTaskGrps(mapToAdd);
    mapToAdd.clear();

    // �����޸ĵ��豸��,mapToModify ��ŵ����µ��豸��
    ModifyTaskGrps(mapToModify);
}

// ����DIT�е��豸��Ϣ
void CMainTask::ModifyDevices(map<string, CDevice*> &mapCurDevices, map<string, CDevice*> &mapModifyDevices)
{
    map<string, CDevice*>::iterator iterModifyDevice = mapModifyDevices.begin();
    for (; iterModifyDevice != mapModifyDevices.end(); iterModifyDevice++)
    {
        map<string, CDevice*>::iterator iterCurDevice = mapCurDevices.find(iterModifyDevice->first);
        if (iterCurDevice == mapCurDevices.end())
            continue;

        map<string, PKTAG*> mapOldToDelete, mapToModify, mapNewToAdd;
        map<string, PKTAG*> &mapModifyTags = iterModifyDevice->second->m_mapName2Tags;
        map<string, PKTAG*> &mapCurTags = iterCurDevice->second->m_mapName2Tags;

        // ���������ú���������֮��Ĳ����Ϣ
        std::set_difference(mapCurTags.begin(), mapCurTags.end(),
            mapModifyTags.begin(), mapModifyTags.end(),
            std::inserter(mapOldToDelete, mapOldToDelete.begin()), MapKeyCmpMainTask<map<string, PKTAG*>::value_type>);

        std::set_intersection(mapModifyTags.begin(), mapModifyTags.end(),
            mapCurTags.begin(), mapCurTags.end(),
            std::inserter(mapToModify, mapToModify.begin()), MapKeyCmpMainTask<map<string, PKTAG*>::value_type>);

        std::set_difference(mapModifyTags.begin(), mapModifyTags.end(),
            mapCurTags.begin(), mapCurTags.end(),
            std::inserter(mapNewToAdd, mapNewToAdd.begin()), MapKeyCmpMainTask<map<string, PKTAG*>::value_type>);

        // ɾ���������Ѿ�û�е����ݿ�
        for(map<string, PKTAG*>::iterator iterDel = mapOldToDelete.begin(); iterDel != mapOldToDelete.end(); ++iterDel)
        {
            SAFE_DELETE(iterDel->second)
            mapCurTags.erase(iterDel->first);
        }
        mapOldToDelete.clear();

        // �������������µ����ÿ�
        for(map<string, PKTAG*>::iterator iterAdd = mapNewToAdd.begin(); iterAdd != mapNewToAdd.end(); ++iterAdd)
        {
            mapCurTags.insert(map<string, PKTAG*>::value_type(iterAdd->first, iterAdd->second));
        }

        // ����������ʱ��
		iterCurDevice->second->StartCheckConnectTimer();

        //�������޸ĺ�ɾ�������Ѿ��Կ�����˴��������Ҫ��ɾ���豸��ʱ������տ��б�����������ظ�ɾ��
        iterModifyDevice->second->m_mapName2Tags.clear();
        SAFE_DELETE(iterModifyDevice->second);
    }
}

//�Ƚ�����Device�����ж��Ƿ���Ҫ������(DrvConfigLoader��ʹ��)
// ���true��ʾ��Ҫ��������
bool CMainTask::IsDeviceConfigChanged(CDevice* pCurActiveDev, CDevice* pNewDev)
{
    // �����������û�ж���ȽϷ���,�Ƚ����е�����
    if(*pCurActiveDev == *pNewDev)
        return false;
    else
        return true;
}

int CMainTask::ModifyTaskGrps(map<string, CTaskGroup*> &mapModifyTaskGrp)
{
    int nStatus = PK_SUCCESS;

    map<string, CTaskGroup*>::iterator iterNewTaskGrp = mapModifyTaskGrp.begin();
    for(; iterNewTaskGrp != mapModifyTaskGrp.end(); iterNewTaskGrp++)
    {
        map<string, CTaskGroup*>::iterator iterCurTaskGrp = m_driverInfo.m_mapTaskGroups.find(iterNewTaskGrp->first);
        if (iterCurTaskGrp == m_driverInfo.m_mapTaskGroups.end())
            continue;

        iterCurTaskGrp->second->PostGroupOnlineModifyCmd(iterNewTaskGrp->second);
        // iterCurTaskGrp->second->m_ioService.dispatch(boost::bind(&CTaskGroup::OnOnlineConfigModified,iterCurTaskGrp->second,iterNewTaskGrp->second));
    }

    return nStatus;
}

int CMainTask::DelTaskGrps(map<string, CTaskGroup*> &mapDelTaskGrp)
{
    int nStatus = PK_SUCCESS;
    map<string, CTaskGroup*>::iterator itTaskGrp = mapDelTaskGrp.begin();
    for(; itTaskGrp != mapDelTaskGrp.end(); itTaskGrp++)
    {
        itTaskGrp->second->Stop();

        CTaskGroup* pTaskGrp = itTaskGrp->second;
        g_logger.LogMessage(PK_LOGLEVEL_WARN, "(DelTaskGrpsFromShm)delete TaskGroup name: %s,device num:%d", pTaskGrp->m_drvObjBase.m_strName.c_str(), pTaskGrp->m_mapDevices.size());
        SAFE_DELETE(itTaskGrp->second);

        // �ӵ�ǰ������map��ɾ������
        std::map<std::string, CTaskGroup *>::iterator itCurTaskGrp = m_driverInfo.m_mapTaskGroups.find(itTaskGrp->first);
        if(itCurTaskGrp != m_driverInfo.m_mapTaskGroups.end())
            m_driverInfo.m_mapTaskGroups.erase(itCurTaskGrp);
    }
    mapDelTaskGrp.clear();
    return PK_SUCCESS;
}

int CMainTask::OnStart()
{
    int nStatus = PK_SUCCESS;

    nStatus = RMAPI_Init();
    if (nStatus != PK_SUCCESS)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR,"��ʼ��RMAPIʧ��!");
    }

	int nRet = PKCommunicate_Init();
	if (nRet != 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKCommunicate_Init return %d", nRet);

    nStatus = LoadConfig(m_driverInfo);
    if (nStatus != PK_SUCCESS)
    {
        //��¼��־����ȡ�����ļ�ʧ��
        g_logger.LogMessage(PK_LOGLEVEL_ERROR," fail to read config file, file not exist, lack attribute or attribute is empty!");
        m_driverInfo.UpdateDriverStatus(-100,"configfile error", NULL);
        return -1;
    }
    g_logger.LogMessage(PK_LOGLEVEL_INFO,"success to read config file!");

    InitQueWithNodeServer(); // ��ʼ����NodeServerͨ�ŵĶ���

	Py_InitDriver(&m_driverInfo); // ����python �����ĳ�ʼ�������� �������ĸ�ֵ����Ҫ��Copy2OutDriverInfo֮ǰ����

	// ΪPKDRIVER��PKDEVICE��ֵ
	m_driverInfo.Copy2OutDriverInfo(); // ����ȡ����������Ϣ����һ��,���㴫��C++����

	nStatus = m_driverInfo.Start();
    if(nStatus != PK_SUCCESS)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR,"��������ʧ��:%d!", nStatus);
    }

    return PK_SUCCESS;
}

// ������ֹͣʱ
int CMainTask::OnStop()
{
	m_driverInfo.Stop();

    // �����ȷ���ʼ�����̼�ͨ��
    int nRet = UnInitProcQues();
    if(nRet != PK_SUCCESS)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR,"InitShmConn failed,ret:%d��", nRet);
        return -1;
    }

	nRet = PKCommunicate_UnInit();
	if (nRet != 0)
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "PKCommunicate_UnInit return %d", nRet);

    return nRet;
}

int CMainTask::ReadTagsInfoFromDb(CPKEviewDbApi &PKEviewDbApi, string strDeviceId, vector<CDrvTag *> &vecTags)
{
	vector< vector<string> > vecRows;
	char szSqlTag[2048] = { 0 };

	// ��ʹ����Ͷ���ģʽ��ʹ�ô�ͳ��ʽ
	sprintf(szSqlTag, "select TAG.name as name,TAG.address,TAG.pollrateMS,TAG.datatype,TAG.datalen,TAG.byteorder,TAG.description,TAG.param from t_device_tag TAG,t_device_list DEV where TAG.device_id=DEV.id and DEV.id = %s", strDeviceId.c_str());
	string strError; 
	int nRet = PKEviewDbApi.SQLExecute(szSqlTag, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�������豸(ID:%s)��ѯ����TAG��ʧ��(�豸ģʽ��, ����ֵ:%d SQL:%s, error:%s", strDeviceId.c_str(), nRet, szSqlTag, strError.c_str());
	}
	else
	{
		for (int iTag = 0; iTag < vecRows.size(); iTag++)
		{
			vector<string> &row = vecRows[iTag];

			int iTagCol = 0;
			CDrvTag *pTag = new CDrvTag();
			pTag->m_strTagName = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
			pTag->m_strAddress = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
			pTag->m_strPollRateMS = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
			pTag->m_strDataType = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
			pTag->m_strDataLen = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
			pTag->m_strByteOrder = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
			pTag->m_strDesc = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
			pTag->m_strParam = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
			vecTags.push_back(pTag);
			row.clear();
		}
		vecRows.clear();
	}

	char szSqlClassProp[2048] = { 0 };
	// ��ȡ��̳е����������.�������Խ�����ȡһ�������������豸�ı���ֵ��
	sprintf(szSqlClassProp, "select OBJ.name as obj_name,CLSPROP.name as prop_name,CLSPROP.address,CLSPROP.pollrate,CLSPROP.datatype,CLSPROP.datalen,CLSPROP.byteorder,CLSPROP.description,CLSPROP.param \
		from t_class_prop CLSPROP, t_class_list CLS, t_object_list OBJ \
	where CLSPROP.class_id = CLS.id and CLS.id = OBJ.class_id AND (CLSPROP.type_id = 0 or CLSPROP.type_id is null or CLSPROP.type_id='') and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') and OBJ.device_id = %s", strDeviceId.c_str());
	nRet = PKEviewDbApi.SQLExecute(szSqlClassProp, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�������豸(ID:%s)��ѯ���ж�������Ե㣨����ģʽ��ʧ��, ����ֵ:%d SQL:%s, error:%s", strDeviceId.c_str(), nRet, szSqlClassProp, strError.c_str());
		// return nRet;
	}

	map<string, CDrvTag*> mapName2DrvTag;
	for (int iTag = 0; iTag < vecRows.size(); iTag++)
	{
		vector<string> &row = vecRows[iTag];

		int iTagCol = 0;
		CDrvTag *pTag = new CDrvTag();
		string strObjName = NULLASSTRING(row[iTagCol].c_str());
		iTagCol++;
		string strPropName = NULLASSTRING(row[iTagCol].c_str());
		iTagCol++;
		pTag->m_strTagName = strObjName + "." + strPropName;

		pTag->m_strAddress = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
		pTag->m_strPollRateMS = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
		pTag->m_strDataType = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
		pTag->m_strDataLen = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
		pTag->m_strByteOrder = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
		pTag->m_strDesc = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;
		pTag->m_strParam = NULLASSTRING(row[iTagCol].c_str()); iTagCol++;

		vecTags.push_back(pTag);
		mapName2DrvTag[pTag->m_strTagName] = pTag;
		row.clear();
	}
	vecRows.clear();

	char szSqlObjProp[2048] = { 0 };
	// ��ȡ�����Լ�������
	sprintf(szSqlObjProp, "select OBJ.name as object_name,CLSPROP.name as prop_name,OBJPROP.object_prop_field_name,OBJPROP.object_prop_field_value \
		from t_object_prop OBJPROP, t_class_prop CLSPROP, t_class_list CLS, t_object_list OBJ \
		where CLSPROP.class_id = CLS.id and CLS.id = OBJ.class_id and OBJPROP.object_id = OBJ.id and OBJPROP.class_prop_id = CLSPROP.id and (OBJ.enable is NULL or OBJ.enable<>0 or OBJ.enable='') and OBJ.device_id = % s", strDeviceId.c_str());
	nRet = PKEviewDbApi.SQLExecute(szSqlObjProp, vecRows, &strError);
	if (nRet != 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "�������豸(ID:%s)��ѯ���ж�������ԣ�����ģʽ��ʧ��, ����ֵ:%d SQL:%s, error:%s", strDeviceId.c_str(), nRet, szSqlObjProp, strError.c_str());
		return nRet;
	}

	for (int iTag = 0; iTag < vecRows.size(); iTag++)
	{
		vector<string> &row = vecRows[iTag];

		int iTagCol = 0;
		string strObjName = NULLASSTRING(row[iTagCol].c_str());
		iTagCol++;
		string strPropName = NULLASSTRING(row[iTagCol].c_str());
		iTagCol++;
		string strTagName = strObjName + "." + strPropName;

		string strPropFieldKey = NULLASSTRING(row[iTagCol].c_str());
		iTagCol++;
		string strPropFieldValue = NULLASSTRING(row[iTagCol].c_str()); 
		iTagCol++;

		map<string, CDrvTag*>::iterator itTag = mapName2DrvTag.find(strTagName);
		if (itTag == mapName2DrvTag.end())
		{
			g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "����ģʽ���޸Ķ���(id:%s)����, ����:%s, ����:%s, �Ҳ������õĶ�������!", strDeviceId.c_str(), strObjName.c_str(), strPropName.c_str());
			continue;
		}

		// �޸���������Ӧ������ֵ
		CDrvTag *pTag = itTag->second;
		if (PKStringHelper::StriCmp(strPropFieldKey.c_str(), "address") == 0)
			pTag->m_strAddress = strPropFieldValue;
		row.clear();
	}
	vecRows.clear();
	mapName2DrvTag.clear();

	if (vecTags.size() <= 0)
	{
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "deviceId:%s has NO tags or Obj props, tagSQL:%s, classPropSQL:%s, objPropSQL:%s", strDeviceId.c_str(), szSqlTag, szSqlClassProp, szSqlObjProp);
	}
	return PK_SUCCESS;
}

// �������ݿ����������µ������ļ�
// szDrvCfgPath, i.e. d:/..../config/drivers/modbus.xml
int CMainTask::ConvertConfigFromDbToXml(char *szDriverName, char *szDrvXmlCfgPath)
{
     // ���������ļ�
    char *szBinPath = (char *)PKComm::GetBinPath(); // D:\Work\Gateway\bin\drivers\modbus\..\..\\config
    sprintf(szDrvXmlCfgPath, "%s%cdrivers%c%s%c%s.xml",szBinPath, ACE_DIRECTORY_SEPARATOR_CHAR, ACE_DIRECTORY_SEPARATOR_CHAR, g_strDrvName.c_str(), ACE_DIRECTORY_SEPARATOR_CHAR, g_strDrvName.c_str()); // config/gateway.db

    tinyxml2::XMLDocument newDoc;
	const char* strDeclaration = "xml version=\"1.0\" encoding=\"GB2312\" standalone=\"yes\"";
	tinyxml2::XMLDeclaration* declaration= newDoc.NewDeclaration(strDeclaration); // default:<?xml version="1.0" encoding="UTF-8"?>
	newDoc.InsertFirstChild(declaration);

    tinyxml2::XMLElement *elemDriver = newDoc.NewElement("driver");

    string strDriverId = "-1";
    char szSql[2048] = {0};
    long lQueryId = -1;
    int nRet = 0;
	CPKEviewDbApi PKEviewDbApi;
    nRet = PKEviewDbApi.Init();
    if (nRet != 0)
    {
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Init Database failed(��ʼ�����ݿ�ʧ��)!");
        newDoc.InsertEndChild(elemDriver);
        newDoc.SaveFile(szDrvXmlCfgPath); // ���������ļ�
		PKEviewDbApi.Exit();
        return -1;
    }

    vector< vector<string> > vecDriver;
    // ��ѯָ��������,δ���õ�����
	sprintf(szSql, "select id,name from t_device_driver where (enable is null or enable<>0) and name = '%s'", szDriverName); // id ��һ���������0.id==-3��ʾ�ڴ������id==-2��ʾģ�����
	string strError; 
	nRet = PKEviewDbApi.SQLExecute(szSql, vecDriver, &strError);
    if (nRet != 0)
    {
        PKEviewDbApi.Exit();
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Query Database failed:%s, error:%s", szSql, strError.c_str());
        newDoc.InsertEndChild(elemDriver);
        newDoc.SaveFile(szDrvXmlCfgPath); // ���������ļ�
        return -1;
    }

    if (vecDriver.size() <= 0)
    {
        PKEviewDbApi.Exit();
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "Cannot find driver config(Drvier:%s), SQL:%s!", szDriverName, szSql);
        newDoc.InsertEndChild(elemDriver);
        newDoc.SaveFile(szDrvXmlCfgPath); // ���������ļ�
        return -2;
    }

    if (vecDriver.size() > 1)
    {
        PKEviewDbApi.Exit();
        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "find more than 1 driver:%d config, drivername:%s, please CHECK, SQL:%s!", vecDriver.size(), szDriverName, szSql);
        newDoc.InsertEndChild(elemDriver);
        newDoc.SaveFile(szDrvXmlCfgPath); // ���������ļ�
        return -3;
    }

    int iColDrv = 0;
    strDriverId = NULLASSTRING(vecDriver[0][0].c_str());
    elemDriver->SetAttribute("name", szDriverName);

    // ��ȡ�豸��Ϣ��δ���õ��豸/����
    vector< vector<string> > vecDevice;
	sprintf(szSql, "select DEV.id,DEV.name,conntype,connparam,conntimeout,taskno,DEV.description,DEV.param1,DEV.param2,DEV.param3,DEV.param4 from t_device_list DEV, t_device_driver DRV \
				where DEV.driver_id = DRV.id and (DEV.enable is null or DEV.enable <>0) and (DRV.enable is null or DRV.enable<>0) and (DEV.simulate is null or DEV.simulate = '') and driver_id = %s", strDriverId.c_str());
    nRet = PKEviewDbApi.SQLExecute(szSql, vecDevice, &strError);
    if(nRet != 0)
    {
        PKEviewDbApi.Exit();
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "query devices of driver id:%s failed:%s, error:%s",  strDriverId.c_str(), szSql, strError.c_str());
        newDoc.InsertEndChild(elemDriver);
        newDoc.SaveFile(szDrvXmlCfgPath); // ���������ļ�
        return -1;
    }

    if(vecDevice.size() <= 0)
    {
        PKEviewDbApi.Exit();
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "devices count of Drvier:%s is ZERO(0), SQL:%s!", szDriverName, szSql);
        newDoc.InsertEndChild(elemDriver);
        newDoc.SaveFile(szDrvXmlCfgPath); // ���������ļ�
        return -1;
    }

    for(int iDev = 0; iDev < vecDevice.size(); iDev ++)
    {
        vector<string> &row = vecDevice[iDev];
        int iCol = 0;
        // device
        tinyxml2::XMLElement *elemDevice = newDoc.NewElement("device");
        string strDeviceId = NULLASSTRING(row[0].c_str());
        string strDeviceName = NULLASSTRING(row[1].c_str());
        iCol++;
        elemDevice->SetAttribute("name", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("conntype", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("connparam", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("conntimeout", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("taskno", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("description", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("param1", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("param2", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("param3", NULLASSTRING(row[iCol].c_str()));
        iCol++;
        elemDevice->SetAttribute("param4", NULLASSTRING(row[iCol].c_str()));
        iCol++;

        // ���ҵ����豸����������tag��
		vector<CDrvTag *> vecTags;
		nRet = ReadTagsInfoFromDb(PKEviewDbApi, strDeviceId, vecTags);
        if(nRet != 0)
        {
            elemDriver->InsertEndChild(elemDevice); // �����豸
            g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "driver:%s, device:(ID:%s,Name:%s), query all tags failed, return:%d SQL:%s!", strDriverId.c_str(), strDeviceId.c_str(),strDeviceName.c_str(), nRet, szSql);
            continue;
        }

        if(vecTags.size() <= 0)
        {
            elemDriver->InsertEndChild(elemDevice); // �����豸
            g_logger.LogMessage(PK_LOGLEVEL_ERROR, "driver:%s, device:(ID:%s,Name:%s), tag count is ZERO(0)!", szDriverName,  strDeviceId.c_str(), strDeviceName.c_str());
			vecTags.clear();
            continue;
        }

        g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "driver:%s, device:(ID:%s,Name:%s), query return tag count:%d", szDriverName,  strDeviceId.c_str(), strDeviceName.c_str(), vecTags.size());
        for(int iTag = 0; iTag < vecTags.size(); iTag ++)
        {
            CDrvTag *pTag = vecTags[iTag];
            tinyxml2::XMLElement *elemTag = newDoc.NewElement("tag");
			elemTag->SetAttribute("name", pTag->m_strTagName.c_str());
			elemTag->SetAttribute("address", pTag->m_strAddress.c_str());
			elemTag->SetAttribute("pollrate", pTag->m_strPollRateMS.c_str());
			elemTag->SetAttribute("datatype", pTag->m_strDataType.c_str());
			elemTag->SetAttribute("datalen", pTag->m_strDataLen.c_str());
			elemTag->SetAttribute("byteorder", pTag->m_strByteOrder.c_str());
			elemTag->SetAttribute("description", pTag->m_strDesc.c_str());
			elemTag->SetAttribute("param", pTag->m_strParam.c_str());
			delete pTag;
            elemDevice->InsertEndChild(elemTag);
        }
        vecTags.clear();

        elemDriver->InsertEndChild(elemDevice);
    }
    vecDevice.clear();

    newDoc.InsertEndChild(elemDriver);
    // ���������ļ�
    newDoc.SaveFile(szDrvXmlCfgPath);
    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "generate driver config xml file success:%s", szDrvXmlCfgPath);

    PKEviewDbApi.Exit();

    return 0;
}

/*
<?xml version="1.0" encoding="GB2312" ?>
<Driver name="SJBroadcastDriver" param1="" param2="" param3="">
<Device name="SJBroadCastDev1" taskid="1" conntype="tcpclient" connectParam="ip=127.0.0.1/192.168.1.1;port=102;multiLink=1;" cyclerate="1000" param1="" param2="" param3="" description="">
<DataItem devAddress="����_1" param1="" param2="" param3="" description="" />
<DataItem devAddress="����_2" param1="" param2="" param3="" description="" />
</Device>
</Driver>
*/
int CMainTask::LoadConfig(CDriver &driver)
{
	int nAutoTaskNo = 0;	// ���δ�����豸��taskno����taskno������֤taskno��ͬ���Ա㿪��Ϊ���߳����Ч�ʡ�������վ�յ�����Ϊδ���̵߳����ٶȺ������Ҳ����ױ����֡�
    m_nTagNum = 0;
    m_nDeviceNum = 0;
    int nStatus = PK_SUCCESS;
    // �γ������ļ�����·��
    char szDrvXmlConfigPath[PK_SHORTFILENAME_MAXLEN] = {0};
    // szConfigFile Ϊxml�ļ�·��
    ConvertConfigFromDbToXml((char *)g_strDrvName.c_str(), szDrvXmlConfigPath);

    // ���������ļ�xml��ʽ
    tinyxml2::XMLElement* pElmChild = NULL;
    tinyxml2::XMLDocument doc;
	int nRet = doc.LoadFile(szDrvXmlConfigPath);
    if(nRet!= tinyxml2::XML_SUCCESS)
    {
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Load Driver Config File %s failed, errCode=%d!", szDrvXmlConfigPath, nRet);
        return EC_ICV_DA_CONFIG_FILE_INVALID;
    }

    g_logger.LogMessage(PK_LOGLEVEL_INFO, "Load Config File %s Successful!", szDrvXmlConfigPath);

    // 2.Get Root Element.
    tinyxml2::XMLElement* pElmRoot = doc.RootElement();
    CHECK_NULL_RETURN_ERR(pElmRoot);
    driver.LoadParamsFromXml(pElmRoot);
    tinyxml2::XMLElement* pNodeDevice = pElmRoot->FirstChildElement(XML_ELEMENTNODE_DEVICE);
    // Ϊ�˼�������Ƿ��ظ�
    while(pNodeDevice != NULL)
    {
        string strDeviceName = NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_NAME));
        string strTaskID =  NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_TASK));
        string strTaskGrpName = TaskGroup_NAME_PREFIX;
		if (strTaskID.empty()) // ���δ����taskno��������һ��������taskno����֤��ͬ�豸�����ڲ�ͬ���߳��У�
		{
			nAutoTaskNo++;
			char szAutoTaskNo[32] = { 0 };
			sprintf(szAutoTaskNo, "_auto_%d", nAutoTaskNo);
			strTaskID = szAutoTaskNo;
		}
        strTaskGrpName += strTaskID;

        CTaskGroup *pTaskGroup = AssureGetTaskGroup(&driver, strTaskGrpName);

        LoadDeviceInfo(&driver, pTaskGroup, pNodeDevice);
        pNodeDevice = pNodeDevice->NextSiblingElement(XML_ELEMENTNODE_DEVICE);
    }

    if(driver.m_mapTaskGroups.empty())
    {
        g_logger.LogMessage(PK_LOGLEVEL_WARN, "No TaskGroup configured");
    }

    // ɾ��û���豸���ݿ���豸�������飨��Ϊ�豸��ȱʡ�Ὠ��һ��������TaskGroup�����ÿ�����ݿ鶼����������ź��豸������������ˣ�
    map<string, CTaskGroup*>::iterator itTaskGroup = driver.m_mapTaskGroups.begin();
    for(;itTaskGroup != driver.m_mapTaskGroups.end(); )
    {
        CTaskGroup *pTaskGrp = itTaskGroup->second;
        map<string, CDevice*>::iterator itDevice = pTaskGrp->m_mapDevices.begin();
        for(;itDevice != pTaskGrp->m_mapDevices.end(); )
        {
            CDevice *pDevice = itDevice->second;

            // ����豸��û���κ����ݿ飬��ɾ���豸
            if(pDevice->m_vecAllTag.size() < 0)
            {
                g_logger.LogMessage(PK_LOGLEVEL_ERROR, "device %s has no tags configed, delete this device", pDevice->m_strName.c_str());
                SAFE_DELETE(pDevice);
                pTaskGrp->m_mapDevices.erase(itDevice++);
            }
            else
            {
                g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "device %s has %d tags configed", pDevice->m_strName.c_str(), pDevice->m_vecAllTag.size());
                itDevice ++;
            }
        }

        // �����������û���κ��豸��ɾ���豸��
        if(pTaskGrp->m_mapDevices.size() <= 0)
        {
            //g_logger.LogMessage(PK_LOGLEVEL_WARN, "(LoadConfig)delete TaskGroup name: %s,device num:%d", pTaskGrp->m_drvObjBase.m_strName.c_str(), pTaskGrp->m_mapDevices.size());
            SAFE_DELETE(pTaskGrp);
            driver.m_mapTaskGroups.erase(itTaskGroup ++);
        }
        else
            itTaskGroup++;
    }

    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "driver:%s, taskgrp count:%d, devnum:%d, tag:%d",
        this->m_driverInfo.m_strName.c_str(), driver.m_mapTaskGroups.size(), m_nDeviceNum, m_nTagNum);

    return PK_SUCCESS;
}

// ����ĳ���豸��Ϣ��pDevice
int CMainTask::LoadDeviceInfo(CDriver *pDriver, CTaskGroup* pTaskGrp, tinyxml2::XMLElement* pNodeDevice)
{
    string strDeviceName = NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_NAME));
    // ����Ƿ��ظ�,����ظ���ȡ����ǰ���Ǹ�
    CDevice *pDevice = AssureGetDevice(pDriver, pTaskGrp, strDeviceName, NULL);
    if(!pDevice)
    {
        g_logger.LogMessage(PK_LOGLEVEL_WARN, "��������ʱ: �����豸��(%s) ���豸�� %s��δ�ҵ��������豸!",
            (char *)strDeviceName.c_str(), pTaskGrp->m_drvObjBase.m_strName.c_str());
        return -1;
    }

    pDevice->LoadParamsFromXml(pNodeDevice);	// ��xml�ж�ȡȱʡ�����ļ�
    pDevice->m_strConnType = NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_CONNTYPE));
    pDevice->m_strConnParam = NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_CONNPARAM));

    //const char *szCycleRate = NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_CYCLERATE));
    //if(szCycleRate)
    //	pDevice->m_nPollRate = ::atoi(szCycleRate);
    const char *szConnTimeout = NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_CONNTIMEOUT));
    if(szConnTimeout)
        pDevice->m_nConnTimeOut = ::atoi(szConnTimeout);
    if(pDevice->m_nConnTimeOut <= 0)
        pDevice->m_nConnTimeOut = 3000;

    const char *szRecvTimeout = NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_RECVTIMEOUT));
    if(szRecvTimeout)
        pDevice->m_nRecvTimeOut = ::atoi(szRecvTimeout);
    if(pDevice->m_nRecvTimeOut <= 0)
        pDevice->m_nRecvTimeOut = 500;
    pDevice->m_strTaskID = NULLASSTRING(pNodeDevice->Attribute(XML_ELEMENTNODE_TASK));
	
    LoadTagsOfDevice(pDriver, pDevice, pNodeDevice);
    pTaskGrp->m_mapDevices[pDevice->m_strName] = pDevice;
    return PK_SUCCESS;
}

/**
 *  ��ȡ���ݿ�����.
 *
 *  @param  -[in,out]  CDevice*  pDevice: [comment]
 *  @param  -[in]  tinyxml2::XMLElement*  node: [comment]
 *
 *  @version     07/14/2008  shijunpu  Initial Version.
 *  @version	3/17/2013  shijunpu  ����豸û���������ݿ飬��ֱ�Ӵ����ݿ��ж�ȡ�㣬Ȼ������û��ӿڣ��������ݿ�.
 */
int CMainTask::LoadTagsOfDevice(CDriver *pDriver, CDevice* pDevice, tinyxml2::XMLElement* pNodeDevice)
{
    //ʹ�õ�ֱ�����ɿ�
    if(!pNodeDevice)
        return -1;

	int nInvalidTagCount = 0;
	int nIndex = 0;
    vector<PKTAG *> vecTags;
    tinyxml2::XMLElement* pNodeTag = pNodeDevice->FirstChildElement();
	while (pNodeTag)
	{
		//printf("No:%d--11,other\n", nIndex);
		string strTagName = NULLASSTRING(pNodeTag->Attribute("name")); // name
		string strDataType = NULLASSTRING(pNodeTag->Attribute("datatype")); // datatype
		string strByteOrder = NULLASSTRING(pNodeTag->Attribute("byteorder")); // �ֽ���ʽ
		string strDataLen = NULLASSTRING(pNodeTag->Attribute("datalen")); // �ַ����ĳ���
		string strAddrInDevice = NULLASSTRING(pNodeTag->Attribute("address")); // blockname:addrinblock
		string strIntv = NULLASSTRING(pNodeTag->Attribute("pollrate")); // in ms
		string strHWDataType = NULLASSTRING(pNodeTag->Attribute("devicedatatype")); // Ӳ����������
		string strParam = NULLASSTRING(pNodeTag->Attribute("param")); // ����
		nIndex++;
		//printf("No:%d----,other\n", nIndex);
		int nLenBits = 0;
		int nDataType = 0;
		PKMiscHelper::GetTagDataTypeAndLen(strDataType.c_str(), strDataLen.c_str(), &nDataType, &nLenBits);
		//printf("No:%d----222,other\n", nIndex);
		int nDataLen = ceil(nLenBits / 8.0f); // �ó��ֽڸ���
		if (TAG_DT_UNKNOWN == nDataType)
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "device:%s, tag:%s datatype:%s not support!", 
				pDevice->m_strName.c_str(), strTagName.c_str(), strDataType.c_str());
			nInvalidTagCount++;
			continue;
		}

		if (nDataLen <= 0) 
		{
			nInvalidTagCount++;
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "device:%s, tag:%s datatype:%s, calc datalen:%d <= 0", pDevice->m_strName.c_str(), strTagName.c_str(), strDataType.c_str(), nDataLen);
			continue;
		} 

		PKTAG *pTag = new PKTAG();
		PKTAG_Reset(pTag); 
		int nTagNameLen = strTagName.length();
		pTag->szName = new char[nTagNameLen + 1];
		memset(pTag->szName, 0, nTagNameLen + 1);
		PKStringHelper::Safe_StrNCpy(pTag->szName, strTagName.c_str(), nTagNameLen + 1);

		int nTagAddressLen = strAddrInDevice.length();
		pTag->szAddress = new char[nTagAddressLen + 1];
		memset(pTag->szAddress, 0, nTagAddressLen + 1);
		PKStringHelper::Safe_StrNCpy(pTag->szAddress, strAddrInDevice.c_str(), nTagAddressLen + 1);

		pTag->nDataType = nDataType;
		pTag->nDataLen = nDataLen; // �ó��ֽڸ���
		pTag->nLenBits = nLenBits;

		pTag->szData = new char[pTag->nDataLen + 1];
		memset(pTag->szData, 0, pTag->nDataLen + 1);

		//printf("No:%d----225,other\n", nIndex);
		int nParamLen = strParam.length() + 1;
		pTag->szParam = new char[nParamLen];
		memset(pTag->szParam, 0, nParamLen);
		PKStringHelper::Safe_StrNCpy(pTag->szParam, strParam.c_str(), nParamLen);

        pTag->nByteOrder = CByteOrder::GetSwapType(strByteOrder.c_str()); // �ַ�����blob���ȣ�����뷽ʽ
        pTag->nPollRate = ::atoi(strIntv.c_str());
        if(pTag->nPollRate <= 0)
            pTag->nPollRate = 1000 * 1; // ȱʡΪ3��

        vecTags.push_back(pTag);
        pNodeTag = pNodeTag->NextSiblingElement();
		//printf("No:%d--,other\n", nIndex);
    }
	//printf("%d,%d\n", vecTags.size(), nInvalidTagCount);
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "device:%s, read %d valid tag, %d tags are invalid from Configfile!", pDevice->m_strName.c_str(), vecTags.size(), nInvalidTagCount);
	// ������е�tag��
    for(int iTag = 0; iTag < pDevice->m_vecAllTag.size(); iTag ++)
    {
        delete pDevice->m_vecAllTag[iTag];
		PKTAG_Free(pDevice->m_vecAllTag[iTag]);
        pDevice->m_vecAllTag[iTag] = NULL;
    }
	pDevice->m_vecAllTag.clear();

    // ��tag�㿽�����豸�µ�����
    // ȱʡ���豸����״̬����
	char szDefDevConnStatusTagName[256] = { 0 };
	sprintf(szDefDevConnStatusTagName, "device.%s.connstatus", pDevice->m_strName.c_str());
	char szDefDevEnableConnectTagName[256] = { 0 };
	sprintf(szDefDevEnableConnectTagName, "device.%s.enableconnect", pDevice->m_strName.c_str());

    bool bHaveDevConnStatusTag = false; // ȱʡ���豸����״̬�����Ƿ��Ѿ���ռ��
	bool bHaveDevEnableConnectTag = false; // ȱʡ���豸����״̬�����Ƿ��Ѿ���ռ��
	for (vector<PKTAG *>::iterator itTag = vecTags.begin(); itTag != vecTags.end(); itTag++)
    {
        PKTAG *pTag = *itTag;
		if (strcmp(szDefDevConnStatusTagName, pTag->szName) == 0)  // ȱʡ���豸����״̬�����Ѿ���ռ��
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�豸:%s, ϵͳ����״̬��:%s, ���ֹ����øõ�!", pDevice->m_strName.c_str(), pTag->szName);
			vecTags.erase(itTag);
			itTag--;
			pDevice->m_pTagConnStatus = pTag;
		}
		if (strcmp(szDefDevEnableConnectTagName, pTag->szName) == 0) // ȱʡ���豸����״̬�����Ѿ���ռ��
		{
			g_logger.LogMessage(PK_LOGLEVEL_ERROR, "�豸:%s, ϵͳ����״̬��:%s, ���ֹ����øõ�!", pDevice->m_strName.c_str(), pTag->szName);
			pDevice->m_pTagEnableConnect = pTag;
			vecTags.erase(itTag);
			itTag--;
		}
    }

    // ���û�����ñ�ʾ�豸����״̬��tag�㣬��ô�Զ�����һ���õ�,�����ӵ�m_vecAllTag�У��������ӵ�״̬��������ȥ
    // ��ʱ��Ϊû���ڱ������������øñ������ʶ�Ҳ�޷����ñ���
    // ������û���ֹ������豸����״̬�ĵ㣬�������ǻ�Ϊÿ���豸�Զ�����һ���豸��.connstatus�ĵ�!
    // if(pDevice->m_vecTagConnStatus.size() <= 0)
	if (!pDevice->m_pTagConnStatus) // ȱʡ���豸����״̬tag���������ǵ�ַ��δ��ռ��,û������ȱʡ�����ӵ�
	{
		PKTAG *pTag = new PKTAG();
		PKTAG_Reset(pTag);
		pTag->szName = new char[PK_NAME_MAXLEN];
		pTag->szAddress = new char[PK_NAME_MAXLEN];
		pTag->szParam = new char[PK_PARAM_MAXLEN];
		pTag->szData = new char[PK_NAME_MAXLEN];
		memset(pTag->szName, 0, PK_NAME_MAXLEN);
		memset(pTag->szAddress, 0, PK_NAME_MAXLEN);
		memset(pTag->szParam, 0, PK_PARAM_MAXLEN);
		memset(pTag->szData, 0, PK_NAME_MAXLEN);

		strcpy(pTag->szName, szDefDevConnStatusTagName); // %s.connstatus
		PKStringHelper::Safe_StrNCpy(pTag->szAddress, "connstatus", PK_NAME_MAXLEN);
		pTag->nDataType = TAG_DT_INT16;
		pTag->nLenBits = sizeof(unsigned short) * 8;
		pTag->nDataLen = sizeof(unsigned short);
		//vecTags.push_back(pTag);  ����㲻���ӵ��豸��ȥ-------------
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "drivter:%s, device:%s, auto add tag address:connstatus, to represent device connect status,tagname::%s",
			this->m_driverInfo.m_strName.c_str(), pDevice->m_strName.c_str(), pTag->szName);
		pDevice->m_pTagConnStatus = pTag;
	}

	if (!pDevice->m_pTagEnableConnect)
	{
		PKTAG *pTagEnableConn = new PKTAG();
		PKTAG_Reset(pTagEnableConn);
		pTagEnableConn->szName = new char[PK_NAME_MAXLEN];
		pTagEnableConn->szAddress = new char[PK_NAME_MAXLEN];
		pTagEnableConn->szParam = new char[PK_PARAM_MAXLEN];
		pTagEnableConn->szData = new char[PK_NAME_MAXLEN];
		memset(pTagEnableConn->szName, 0, PK_NAME_MAXLEN);
		memset(pTagEnableConn->szAddress, 0, PK_NAME_MAXLEN);
		memset(pTagEnableConn->szParam, 0, PK_PARAM_MAXLEN);
		memset(pTagEnableConn->szData, 0, PK_NAME_MAXLEN);

		pDevice->m_pTagEnableConnect = pTagEnableConn;
		strcpy(pTagEnableConn->szName, szDefDevEnableConnectTagName); // %s.enableconnect
		PKStringHelper::Safe_StrNCpy(pTagEnableConn->szAddress, "enableconnect", PK_NAME_MAXLEN);
		pTagEnableConn->nDataType = TAG_DT_INT16;
		pTagEnableConn->nLenBits = sizeof(unsigned short) * 8;
		pTagEnableConn->nDataLen = sizeof(unsigned short);
		Drv_SetTagData_Text(pTagEnableConn, "1", 0, 0, 0); // ȱʡΪ��������
		//vecTags.push_back(pTagEnableConn);  ����㲻���ӵ��豸��ȥ-------------
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "driver:%s, device:%s, auto add tagaddress:enableconnect to represent enable connect to device mode, tagname:%s",
			this->m_driverInfo.m_strName.c_str(), pDevice->m_strName.c_str(), pTagEnableConn->szName);
		pDevice->m_pTagEnableConnect = pTagEnableConn;
    }
	pDevice->SetEnableConnect(true); // ȱʡ�����������ӵģ�����ֵ��Ϊ0

	for (vector<PKTAG *>::iterator itTag = vecTags.begin(); itTag != vecTags.end(); itTag++)
    {
        PKTAG *pTag = *itTag;

        pDevice->m_vecAllTag.push_back(pTag);
        pDevice->m_mapName2Tags[pTag->szName] = pTag;

        // �õ����ݵ�ַ��tag�������map���Ա���ݵ�ַ���ٲ���
        vector<PKTAG *> *pVecTagOfAddr = NULL;
        map<string, vector<PKTAG *> *>::iterator itMapT = pDevice->m_mapAddr2TagVec.find(pTag->szAddress);
        if(itMapT == pDevice->m_mapAddr2TagVec.end())
        {
            pVecTagOfAddr = new vector<PKTAG *>();
            pDevice->m_mapAddr2TagVec[pTag->szAddress] = pVecTagOfAddr;
        }
        else
            pVecTagOfAddr = itMapT->second;

        pVecTagOfAddr->push_back(pTag); // �õ�ַ��Ӧ�������ж���һ��tag
    }
    vecTags.clear();

    g_logger.LogMessage(PK_LOGLEVEL_NOTICE, "drv:%s,dev:%s,tagcount:%d", this->m_driverInfo.m_strName.c_str(), pDevice->m_strName.c_str(), pDevice->m_vecAllTag.size());
    m_nTagNum += pDevice->m_vecAllTag.size();
    m_nDeviceNum ++;

    return PK_SUCCESS;
}

int CMainTask::LoadDevicesOfGroup(CDriver *pDriver, CTaskGroup* pTaskGrp, tinyxml2::XMLElement* pNodeGrpDevice)
{
    tinyxml2::XMLElement* pNodeDevice = pNodeGrpDevice->FirstChildElement(XML_ELEMENTNODE_DEVICE);
    while(pNodeDevice)
    {
        LoadDeviceInfo(pDriver, pTaskGrp, pNodeDevice);
        pNodeDevice = pNodeGrpDevice->NextSiblingElement(XML_ELEMENTNODE_DEVICE);
    }

    return PK_SUCCESS;
}

// ȷ����TaskGroup�ں�TaskID��ͬ���豸���У����û���豸�飬�������豸�顣
// ��ȷ���豸�����и��豸�����û�������ɸ��豸��������򿽱�pDevice��Ϣ�������ɵ��豸
CTaskGroup * CMainTask::AssureGetTaskGroup(CDriver *pDriver, string strTaskGrpName)
{
    CTaskGroup *pTaskGroup = NULL;
    map<string, CTaskGroup*>::iterator itTaskGrp = pDriver->m_mapTaskGroups.find(strTaskGrpName);
    if(itTaskGrp != pDriver->m_mapTaskGroups.end()) // δ�ҵ����豸��
        return itTaskGrp->second;

    pTaskGroup = new CTaskGroup(strTaskGrpName.c_str(), "");
    pTaskGroup->m_pMainTask = this;
    pDriver->m_mapTaskGroups[strTaskGrpName] = pTaskGroup;

    return pTaskGroup;
}

// ȷ����TaskGroup�ں�TaskID��ͬ���豸���У����û���豸�飬�������豸�顣
// ��ȷ���豸�����и��豸�����û�������ɸ��豸��������򿽱�pDevice��Ϣ�������ɵ��豸
// ���û���豸�����½�һ����pDevice������ͬ���豸���������ֱ�ӷ����ҵ����豸
CDevice * CMainTask::AssureGetDevice(CDriver *pDriver, CTaskGroup *pTaskGroup, string strDeviceName, CDevice *pDevice)
{
    CDevice *pDeviceRet = NULL;
    map<string, CDevice*>::iterator itDevice = pTaskGroup->m_mapDevices.find(strDeviceName);
    if(itDevice != pTaskGroup->m_mapDevices.end()) // �ҵ��豸
    {
        pDeviceRet = itDevice->second;
        return pDeviceRet;
    }

     // δ�ҵ��豸
    pDeviceRet = new CDevice(pTaskGroup, strDeviceName.c_str());

    pTaskGroup->m_mapDevices[strDeviceName] = pDeviceRet;
    if(pDevice)
        *pDeviceRet = *pDevice;
    return pDeviceRet;
}

int CMainTask::Start()
{
    return activate(THR_NEW_LWP | THR_JOINABLE |THR_INHERIT_SCHED);
}

void CMainTask::Stop()
{
    m_bStop = true;
    int nWaitResult = wait();

    ACE_Time_Value tvSleep;
    tvSleep.msec(100);
    while (!m_bStopped)
        ACE_OS::sleep(tvSleep);
}

// �ֹ�����һ����������
void CMainTask::TriggerRefreshConfig()
{
    m_bManualRefreshConfig = true;
}

int CMainTask::InitQueWithNodeServer()
{
	unsigned int nTagCount = 0;
	unsigned int nTagDataSize = 0;
	unsigned int nTagMaxDataSize = 0;
	int nRet = CalcDriverTagDataSize(&nTagCount, &nTagDataSize, &nTagMaxDataSize);
    unsigned int nQueBufSize = PROCESSQUEUE_PROCESSMGR_CONTROL_AREASIZE; // ��������������������tag��100���Ϳ�����
    //	if (nQueBufSize < PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE_MIN)
    //		nQueBufSize = PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE_MIN; // 250 ��4K�������

    //	if (nQueBufSize > PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE_MAX)
    //		nQueBufSize = PROCESSQUEUE_CONTROL_TO_DRIVER_AREASIZE_MAX; // 250 ��4K�������

	// �������տ�������Ĺ������
	g_pQueCtrlFromNodeServer = new CProcessQueue((char *)g_strDrvName.c_str());
	g_pQueCtrlFromNodeServer->Open(true, nQueBufSize); // �������տ�������Ĺ������
	if (nRet != PK_SUCCESS)
        g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Create Sharedmemory Queue For Driver(%s) To RecvControl failed, ret:%d, QueueBuffSize:%d", g_strDrvName.c_str(), nRet, nQueBufSize);
	else
        g_logger.LogMessage(PK_LOGLEVEL_INFO, "Create Sharedmemory Queue For Driver(%s) To RecvControl Success, QueueBuffSize:%d", g_strDrvName.c_str(), nQueBufSize);

    // ��ȡ�������ݸ�NodeServer�Ĺ������
    g_pQueData2NodeServer = new CProcessQueue(MODULENAME_LOCALSERVER);
	nRet = g_pQueData2NodeServer->Open(); // ���۳ɲ��ɹ�������Ҫ����Ϊÿ��EnQueue��DeQueue��Ҫ��鲢�򿪣�
	if(nRet == PK_SUCCESS)
		g_logger.LogMessage(PK_LOGLEVEL_INFO, "Driver:%s, Open Sharedmemory Queue To NodeServer Success", (char *)g_strDrvName.c_str());
	else
		g_logger.LogMessage(PK_LOGLEVEL_ERROR, "Driver:%s, Open Sharedmemory Queue To NodeServer Fail, RetCode:%d", (char *)g_strDrvName.c_str(), nRet);

    return 0;
}

int CMainTask::UnInitProcQues()
{
    if(g_pQueData2NodeServer)
        delete g_pQueData2NodeServer;
    g_pQueData2NodeServer = NULL;

    if(g_pQueCtrlFromNodeServer)
        delete g_pQueCtrlFromNodeServer;
    g_pQueCtrlFromNodeServer = NULL;
    return PK_SUCCESS;
}

// ��������tag��Ĵ�С
int CMainTask::CalcDriverTagDataSize(unsigned int *pnTagCount, unsigned int *pnTagDataSize, unsigned int *pnTagMaxDataSize)
{
	*pnTagDataSize = 0;
	*pnTagCount = 0;
	int nInvalidTagCount = 0;
	*pnTagMaxDataSize = 0;

	// �����豸�飬�����豸
	for (map<string, CTaskGroup*>::iterator iter = m_driverInfo.m_mapTaskGroups.begin(); iter != m_driverInfo.m_mapTaskGroups.end(); iter++)
	{
		CTaskGroup *pTmpTaskGroup = iter->second;
		
		for (map<string, CDevice*>::iterator iterDevice = pTmpTaskGroup->m_mapDevices.begin(); iterDevice != pTmpTaskGroup->m_mapDevices.end(); iterDevice++)
		{
			CDevice *pDeviceTmp = iterDevice->second;
			for (vector<PKTAG *>::iterator itTag = pDeviceTmp->m_vecAllTag.begin(); itTag != pDeviceTmp->m_vecAllTag.end(); itTag++)
			{
				PKTAG *pTag = *itTag;
				if (pTag->nDataLen <= 0)
				{
					nInvalidTagCount++;
					continue;
				}
				(*pnTagCount)++;
				(*pnTagDataSize) += pTag->nDataLen;
				if (*pnTagMaxDataSize < pTag->nDataLen)
					*pnTagMaxDataSize = pTag->nDataLen;
			}
		}
	}
	
	g_logger.LogMessage(PK_LOGLEVEL_INFO, "CalcDriverTagDataSize, driver:%s, ValidTagCount:%d, DriverTagSize:%d, InvalidTagCount:%d",
		m_driverInfo.m_strName.c_str(), *pnTagCount, *pnTagDataSize, nInvalidTagCount);

	return 0;
}
