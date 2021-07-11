/**************************************************************
 *  Filename:    DataBlock.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 数据块信息实体类.
 *
 * 确实是指向CDrvObjectBase内存
**************************************************************/

#include "CycleReader.h" 
#include "MainTask.h"
#include "common/pklog.h"
#include "SystemConfig.h"

extern CPKLog PKLog;
//extern PFN_ReadAllTagsInfo	g_pfnReadAllTagsInfo;
//extern PFN_ReadAllTagsData	g_pfnReadAllTagsData;
//extern PFN_ReleaseAllTagsInfo	g_pfnReleaseAllTagsInfo;


/**
 *  构造函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCycleReader::CCycleReader(CMainTask *pTask)
{
	 m_pMainTask = pTask;
	 m_sockHandler.m_pCycleReader = this;
	 m_sockHandler.m_pMainTask = m_pMainTask;
	 m_nPollRate = 0;
	 m_nTransId = 0;
	 memset(m_szGatewayId, 0, sizeof(m_szGatewayId));
}

/**
 *  析构函数.
 *
 *
 *  @version     05/30/2008  shijunpu  Initial Version.
 */
CCycleReader::~CCycleReader()
{	
 
}

void CCycleReader::StartTimer()
{
	//// 读取所有的tag点信息
	//if(g_pfnReadAllTagsInfo)
	//	g_pfnReadAllTagsInfo(&m_vecTagGroup);
	//else
	//{
	//	PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ReadAllTagsInfo is null!");
	//}

	// 开始重连服务器
	if(m_sockHandler.TryConnectToServer() == 0)
	{
		RegisterToServer();
	}
	// 注册所有的组
	//RegisterAllGroups();
	

	if(m_nPollRate > 0)
	{
		ACE_Time_Value	tvPollRate;		// 扫描周期，单位ms
		tvPollRate.msec(m_nPollRate);
		ACE_Time_Value tvPollPhase;
		tvPollPhase.msec(m_nPollPhase);
		tvPollPhase += ACE_Time_Value::zero;
		m_pMainTask->m_pReactor->schedule_timer((ACE_Event_Handler *)(CCycleReader *)this, NULL, tvPollPhase, tvPollRate);
	}	
}

void CCycleReader::StopTimer()
{
	m_pMainTask->m_pReactor->cancel_timer((ACE_Event_Handler *)(CCycleReader *)this);
	// 如果在这里释放tag点信息，可能出现释放完后handle_timeout还会被调用的情况，因此就不释放了
	//UnRegisterAllGroups();
}

// 每次定时器到来时，去读取一次所有要转发的点，然后调用socket发送接口发送出去
int CCycleReader::handle_timeout( const ACE_Time_Value &current_time, const void *act /*= 0*/ )
{
	//// 将所有tag点数据先读取到tag内存中
	//if(g_pfnReadAllTagsData)
	//	g_pfnReadAllTagsData(&m_vecTagGroup);

	time_t tmNow;
	time(&tmNow);

	// 定时连接服务
	if(m_sockHandler.TryConnectToServer() == 0)
	{
		RegisterToServer();
	}

	//// 组织包转发给接受服务
	//vector<TAGGROUP *>::iterator itGroup = m_vecTagGroup.begin();
	//for(; itGroup != m_vecTagGroup.end(); itGroup ++)
	//{
	//	TAGGROUP *pGroup = (TAGGROUP *)*itGroup;

	//	// 判断是否服务器3次没有应答，需要重新注册
	//	long lInterval = tmNow - pGroup->tmLastResponse;
	//	if(labs(lInterval) > 60 * 3) // 大于3分钟没有从服务端收到应答则认为需要重连
	//	{
	//		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Group does not receive response from server > 3 minutes! Reconnect!");
	//		pGroup->uServerGrpHandle = -1;
	//	}

	//	//注册或者转发一个组的数据
	//	if(pGroup->uServerGrpHandle < 0) // 未注册或者需要重连
	//	{
	//		RegisterAGroup((TAGGROUP *) *itGroup);
	//	}
	//	else
	//	{
	//		ForwardAGroup(pGroup);
	//	}
	//	
	//}
	return 0;
}

long CCycleReader::RegisterToServer()
{
	// 发送标识来源的包 到 服务
	char szSendBuf[MAX_SENDBUF_LEN];
	memset(szSendBuf, 0x00, sizeof(szSendBuf));

	char *pSendBuf = szSendBuf;

	// 开始 1B
	*pSendBuf = PACK_HEADER_FLAG;
	pSendBuf++;


	unsigned int nCodeLen = SYSTEM_CONFIG->m_strProjectCode.length();

	if (nCodeLen > PROJECT_CODE_LENGTH)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "项目代号超长 %d 长度，只能有8位长度", nCodeLen);
		return -1;
	}

	unsigned int nNameLen = SYSTEM_CONFIG->m_strGatewayName.length();
	if (nNameLen > GATEWAY_ID_LEN)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "网关代号超长 %d 长度，只能有8位长度", nNameLen);
		return -1;
	}
	// 长度 2B
	unsigned short nSendLen = PROJECT_CODE_LENGTH + GATEWAY_ID_LEN + 2;
	memcpy(pSendBuf, & nSendLen, sizeof(unsigned short));
	pSendBuf ++;
	pSendBuf ++;

	// 事务ID
	*pSendBuf = 01;
	pSendBuf ++;

	// 命令ID
	*pSendBuf = COMMAND_NOTIFY_SOURCE;
	pSendBuf ++;

	// 来源
	*pSendBuf = CONN_FROM_GATEWAY;
	pSendBuf ++;

	// 数据区数据
	memcpy(pSendBuf, SYSTEM_CONFIG->m_strProjectCode.c_str(), nCodeLen);
	memcpy(m_szProjectCode, SYSTEM_CONFIG->m_strProjectCode.c_str(), nCodeLen);
	pSendBuf += PROJECT_CODE_LENGTH; 

	memcpy(pSendBuf, SYSTEM_CONFIG->m_strGatewayName.c_str(), nNameLen);
	memcpy(m_szGatewayId, SYSTEM_CONFIG->m_strGatewayName.c_str(), nNameLen);
	pSendBuf += GATEWAY_ID_LEN; 

	// 结束标记
	*pSendBuf = PACK_FOOTER_FLAG;
	pSendBuf ++;

	long lRet = m_sockHandler.SendDataToServer((char *)szSendBuf, pSendBuf - szSendBuf);

	if (lRet > 0 )
	{
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"注册网关[%s]", SYSTEM_CONFIG->m_strGatewayName.c_str());
		return 0;
	}
	else
	{
		return -1;
	}
}
