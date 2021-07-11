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
#include "GWProtoDef.h"

extern CPKLog PKLog;
extern PFN_Initialize		g_pfnInitialize;
extern PFN_ReadAllTagsData	g_pfnReadAllTagsData;
extern PFN_UnInitialize		g_pfnUnInitialize;


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
	// 读取所有的tag点信息
	if(g_pfnInitialize)
		g_pfnInitialize(&m_vecTagGroup);
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "ReadAllTagsInfo is null!");
	}

	// 开始重连服务器
	// 定时尝试注册到服务器，在注册中会尝试重连
	long lRetConn = this->m_sockHandler.TryConnectToServer();
	if(lRetConn != 0)
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterToServer failed, retcode:%d, will continue...", lRetConn);
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "RegisterToServer success");

	// 注册所有的组
	long lRetRegister = RegisterAllGroups();
	if(lRetConn != 0)
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterAllGroups failed(%d 个失败), retcode:%d, will continue...",  -1 * lRetRegister);
	else
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "RegisterAllGroups success");

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
	UnRegisterAllGroups();
}

// 每次定时器到来时，去读取一次所有要转发的点，然后调用socket发送接口发送出去
int CCycleReader::handle_timeout( const ACE_Time_Value &current_time, const void *act /*= 0*/ )
{
	// 定时连接服务
	if(m_sockHandler.TryConnectToServer() == 0)
	{
		if (RegisterToServer() != 0)
		{
			return -1;
		}
		
	}

	// 将所有tag点数据先读取到tag内存中
	if(g_pfnReadAllTagsData)
		g_pfnReadAllTagsData(&m_vecTagGroup);

	time_t tmNow;
	time(&tmNow);

	// 组织包转发给接受服务
	vector<TAGGROUP *>::iterator itGroup = m_vecTagGroup.begin();
	for(; itGroup != m_vecTagGroup.end(); itGroup ++)
	{
		TAGGROUP *pGroup = (TAGGROUP *)*itGroup;

		// 判断是否服务器3次没有应答，需要重新注册
		long lInterval = tmNow - pGroup->tmLastResponse;
		if(labs(lInterval) > m_nPollRate /1000 * 3) // 大于3分钟没有从服务端收到应答则重新注销该组
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "Group does not receive response from server > 3 minutes! Set ServerGrpHandle==0!");
			pGroup->uServerGrpHandle = 0;
		}

		//注册或者转发一个组的数据
		if(pGroup->uServerGrpHandle <= 0) // 未注册或者需要重连
		{
			long lInterval = tmNow - pGroup->tmLastRequest;
			if(labs(lInterval) > m_nPollRate /1000 * 3) // 定时间隔3次没有发送成功则重连
			{
				RegisterAGroup((TAGGROUP *) *itGroup);
			}
		}
		else
		{
			ForwardAGroup(pGroup);
		}
		
	}
	return 0;
}

// 注册一个tag组
// 格式：网关ID+组ID+时间+对象个数+对象ID+tag名称1长度+tag名称1+tag数据类型+tag2名称长度+.....
int CCycleReader::ForwardAGroup(TAGGROUP *pTagGroup)
{
	char szRegisterBuf[MAX_SENDBUF_LEN] = {0}; // 一个包的最大长度为1024
	char *pBuf = szRegisterBuf;
	// 命令ID
	*pBuf ++ = COMMAND_ID_WRTITE_GROUP;
	// 服务端返回的组的ID
	memcpy(pBuf, &pTagGroup->uServerGrpHandle, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);

	// 时间戳
	time_t nowTime;
	time(&nowTime);
	memcpy(pBuf, &nowTime, sizeof(time_t));
	pBuf += sizeof(time_t);

	// 对象个数2B+对象ID 2B+tag质量1B+值
	// 对象个数
	unsigned short nDeviceNum = 1;
	memcpy(pBuf, &nDeviceNum, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);

	for (int i = 0 ; i < nDeviceNum; i++)
	{
		// 对象ID
		unsigned short nDeviceId = 1;
		memcpy(pBuf, &nDeviceId, sizeof(unsigned short));
		pBuf += sizeof(unsigned short);
		vector<CVTAG *>::iterator itTag = pTagGroup->vecTag.begin();
		for(; itTag != pTagGroup->vecTag.end(); itTag ++)
		{
			CVTAG *pTag = (CVTAG *)*itTag;
			// tag质量
			*pBuf ++ = (unsigned char )pTag->uQuality;
			// tag值内容
			memcpy(pBuf, pTag->rawData.pszData, pTag->rawData.nDataSize);
			pBuf += pTag->rawData.nDataSize;
		}
	}

	//// tag点的个数
	//unsigned short nTagNum = pTagGroup->vecTag.size();
	//memcpy(pBuf, &nTagNum, sizeof(unsigned short));
	//pBuf += sizeof(unsigned short);

	//// tag1质量（1Byte）+tag1值（根据数据类型而定）.....
	//vector<CVTAG *>::iterator itTag = pTagGroup->vecTag.begin();
	//for(; itTag != pTagGroup->vecTag.end(); itTag ++)
	//{
	//	CVTAG *pTag = (CVTAG *)*itTag;
	//	// tag质量
	//	*pBuf ++ = (unsigned char )pTag->uQuality;
	//	// tag值内容
	//	memcpy(pBuf, pTag->rawData.pszData, pTag->rawData.nDataSize);
	//	pBuf += pTag->rawData.nDataSize;
	//}

	long lRet = SendToServer(szRegisterBuf, pBuf - szRegisterBuf);
	return 0;
}

int CCycleReader::GetGroupsNum()
{
	return m_vecTagGroup.size();
}

// 注册所有的tag点的组
int CCycleReader::RegisterAllGroups()
{
	long lRetFail = 0;
	vector<TAGGROUP *>::iterator itGroup = m_vecTagGroup.begin();  
	for(; itGroup != m_vecTagGroup.end(); itGroup ++)
	{
		long lRet = RegisterAGroup((TAGGROUP *) *itGroup);
		// 注册所有的组
		if(lRet != 0)
		{
			PKLog.LogMessage(PK_LOGLEVEL_ERROR, "RegisterAGroup() failed, retcode:%d, will continue...", lRet);
			lRetFail --;
		}
		else
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "RegisterAGroup() success");
	}
	return lRetFail;
}

// 注册一个tag组
int CCycleReader::RegisterAGroup(TAGGROUP *pTagGroup)
{
	char szRegisterBuf[MAX_SENDBUF_LEN] = {0}; // 一个包的最大长度为1024
	char *pBuf = szRegisterBuf;
	// 命令ID
	*pBuf ++ = COMMAND_ID_REGISTER_GROUP;
	//// 网关的ID
	//memcpy(pBuf, m_szGatewayId, GATEWAY_ID_LEN);
	//pBuf += GATEWAY_ID_LEN;

	// 本地组的ID
	memcpy(pBuf, &pTagGroup->uLocalGrpId, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);

	// 设备对象的个数
	unsigned short nDeviceNum = 1;
	memcpy(pBuf, &nDeviceNum, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);

	//接下来处理每个对象中的参数（tag点）
	// hjhj 这里对象个数，以及metername 都需要根据配置文件变动。
	for (int i = 0 ; i < nDeviceNum; i++)
	{
		// 对象的ID ，由本地维护
		unsigned short nDeviceId = 1;
		memcpy(pBuf, &nDeviceId, sizeof(unsigned short));
		pBuf += sizeof(unsigned short);

		string metername = "Meter1";
		// 设备对象的名称长度
		unsigned short nDeviceNameLen = metername.length();
		memcpy(pBuf, &nDeviceNameLen, sizeof(unsigned short));
		pBuf += sizeof(unsigned short);

		// 对象的名称
		memcpy(pBuf, metername.c_str(), nDeviceNameLen);
		pBuf += nDeviceNameLen;
		// tag点的个数
		unsigned short nTagNum = pTagGroup->vecTag.size();
		memcpy(pBuf, &nTagNum, sizeof(unsigned short));
		pBuf += sizeof(unsigned short);

		// tag名称1长度+tag名称1+tag数据类型+tag2名称长度+.....
		vector<CVTAG *>::iterator itTag = pTagGroup->vecTag.begin();
		for(; itTag != pTagGroup->vecTag.end(); itTag ++)
		{
			CVTAG *pTag = (CVTAG *)*itTag;
			// tag名称
			unsigned short nTagNameLen = pTag->strTagName.length();
			memcpy(pBuf, &nTagNameLen, sizeof(unsigned short));
			pBuf += sizeof(unsigned short);
			// tag名称内容
			memcpy(pBuf, pTag->strTagName.c_str(), nTagNameLen);
			pBuf += nTagNameLen;
			// tag数据类型
			*pBuf ++ = pTag->nDataType;
		}

		pTagGroup->lRequestWriteCount++;
		time(&pTagGroup->tmLastRequest);
	}

	long lRet = SendToServer(szRegisterBuf, pBuf - szRegisterBuf);
	return lRet;
}

// 注册所有的tag点的组
int CCycleReader::UnRegisterAllGroups()
{
	char szRegisterBuf[MAX_SENDBUF_LEN] = {0}; // 一个包的最大长度为1024
	char *pBuf = szRegisterBuf;
	// 命令ID
	*pBuf ++ = COMMAND_ID_UNREGISTER_ALL;
	// 网关的ID
	memcpy(pBuf, m_szGatewayId, GATEWAY_ID_LEN);
	pBuf += GATEWAY_ID_LEN;
	// 服务端返回的组的ID
	unsigned short lServerGrpHandle = 0; // 0表示注册所有的组
	memcpy(pBuf, &lServerGrpHandle, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);
	
	long lRet = SendToServer(szRegisterBuf, pBuf - szRegisterBuf);
	return lRet;
}

// 注册一个tag组
int CCycleReader::UnRegisterAGroup(TAGGROUP *pTagGroup)
{
	return 0;
}

// 发送给服务器，并处理断开重连
long CCycleReader::SendToServer(char * szPackBodyBuf, unsigned short nPackLen )
{
	// 组织缓冲区
	unsigned char szPackBuf[MAX_SENDBUF_LEN] = {0}; // 一个包的最大长度为1024
	unsigned char *pBuf = szPackBuf;
	*pBuf ++ = 0xAB;

	nPackLen += 1; // 事务号，因此需要+1
	memcpy(pBuf, &nPackLen, sizeof(unsigned short));
	pBuf += sizeof(unsigned short);
	// 事务号ID
	*pBuf++ = ++m_nTransId;

	memcpy(pBuf, szPackBodyBuf, nPackLen);
	pBuf += nPackLen;

	*pBuf ++ = 0xED;
	// 发送数据
	long lRet = m_sockHandler.SendDataToServer((char *)szPackBuf, pBuf - szPackBuf);
	return lRet;
}

long CCycleReader::OnRegisterGroupResponse(unsigned short uLocalGrpId, unsigned short uServerGrpId)
{
	bool bFound = false;
	vector<TAGGROUP *>::iterator itGrp = m_vecTagGroup.begin();
	for(;itGrp != m_vecTagGroup.end(); itGrp ++)
	{
		TAGGROUP *pGrp = (TAGGROUP *)*itGrp;
		if(pGrp->uLocalGrpId == uLocalGrpId)
		{
			pGrp->uServerGrpHandle = uServerGrpId;
			PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "组ID: %d 注册返回服务端号码: %d", uLocalGrpId, uServerGrpId);
			bFound = true;
			// 更新服务端的响应时间
			time(&pGrp->tmLastResponse);
			break;
		}
	}


	if(!bFound)
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR, "组ID: %d 服务端注册返回号码: %d未找到客户端ID", uLocalGrpId, uServerGrpId);
		return -1;
	}
	return 0;
}

long CCycleReader::OnUnRegisterAllGroupResponse(unsigned short uServerGrpId)
{
	bool bFound = false;
	vector<TAGGROUP *>::iterator itGrp = m_vecTagGroup.begin();
	for(;itGrp != m_vecTagGroup.end(); itGrp ++)
	{
		TAGGROUP *pGrp = (TAGGROUP *)*itGrp;
		pGrp->uServerGrpHandle = 0; 
		pGrp->tmLastRequest = pGrp->tmLastResponse = 0;
		pGrp->lRequestWriteCount = pGrp->lSucessWriteCount = 0;
	}

	PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "服务端注销所有组调用返回");
 
	return 0;
}

long CCycleReader::OnWriteGroupResponse(unsigned short uServerGrpId, unsigned char nStatus )
{
	bool bFound = false;
	vector<TAGGROUP *>::iterator itGrp = m_vecTagGroup.begin();
	for(;itGrp != m_vecTagGroup.end(); itGrp ++)
	{
		TAGGROUP *pGrp = (TAGGROUP *)*itGrp;
		if(	pGrp->uServerGrpHandle == uServerGrpId)
		{
			pGrp->lSucessWriteCount ++;
			time(&pGrp->tmLastResponse);

			if(nStatus == 0)
				PKLog.LogMessage(PK_LOGLEVEL_INFO, "接收到服务端(服务端组:%d)已正确接收数据的应答", pGrp->uServerGrpHandle);
			else
			{
				PKLog.LogMessage(PK_LOGLEVEL_ERROR, "接收到服务端(服务端组:%d)错误接收数据的应答, 返回状态:%d, 将重新注册该组", pGrp->uServerGrpHandle, nStatus);
				RegisterAGroup(pGrp);
			}

			return 0;
		}
	}


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
	*pSendBuf = PROTOCOL_GW_TO_SERVER_REGISTERGW;
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
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE,"发送注册网关[%s]请求成功", SYSTEM_CONFIG->m_strGatewayName.c_str());
		return 0;
	}
	else
	{
		PKLog.LogMessage(PK_LOGLEVEL_ERROR,"发送注册网关[%s]请求失败", SYSTEM_CONFIG->m_strGatewayName.c_str());
		return -1;
	}
}

long CCycleReader::OnRegisterToServerResponse(unsigned char cRetCode, time_t uServerGrpId)
{
	return 0;
}

