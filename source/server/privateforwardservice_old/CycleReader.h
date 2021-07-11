/**************************************************************
 *  Filename:    DataBlock.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 数据块信息实体类.
 *
 *  @author:     lijingjing
 *  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _DATABLOCK_H_
#define _DATABLOCK_H_

#include <ace/Thread_Mutex.h>
#include <ace/Guard_T.h>
#include <ace/Event_Handler.h>
#include "common/Quality.h"
#include <list>
#include "common/CommHelper.h" 
#include "../forwardaccessor/ForwardAccessor.h"
#include "ClientSockHandler.h"
class CMainTask;

#define	 GATEWAY_ID_LEN					8	
#define  PROJECT_CODE_LENGTH			8

#define		COMMAND_ID_REGISTER_GROUP				0x41
#define		COMMAND_ID_UNREGISTER_ALL				0x42
#define		COMMAND_ID_WRTITE_GROUP					0x43
#define		COMMAND_ID_CONTROL						0x44
 
class CCycleReader: public ACE_Event_Handler
{
public:
	CCycleReader(CMainTask *pTask);
	virtual ~CCycleReader();
	
	void StartTimer();
	void StopTimer();

	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data 
	int RegisterAllGroups();
	int RegisterAGroup(TAGGROUP *pTagGroup);
	int UnRegisterAllGroups();
	int UnRegisterAGroup(TAGGROUP *pTagGroup);
	int ForwardAGroup(TAGGROUP *pTagGroup);
	long SendToServer(char * szRegisterBuf, unsigned short nBufLen );
	long OnRegisterGroupResponse(unsigned short uLocalGrpId, unsigned short uServerGrpId);
	long OnUnRegisterAllGroupResponse(unsigned short uServerGrpId);
	long OnWriteGroupResponse(unsigned short uServerGrpId, unsigned char nStatus);
	long RegisterToServer();
	long OnRegisterToServerResponse(unsigned char cRetCode, time_t uServerGrpId);
	int GetGroupsNum();
public:
	int					m_nPollRate;			// 扫描周期，单位ms
	int					m_nPollPhase;			// 扫描相位，单位ms
	CMainTask			*m_pMainTask;
	vector<TAGGROUP *>	m_vecTagGroup;			// TAG点分组存储以便
	char				m_szGatewayId[GATEWAY_ID_LEN + 1];
	char				m_szProjectCode[PROJECT_CODE_LENGTH + 1];
	unsigned char		m_nTransId;

	CClientSockHandler	m_sockHandler; 
	// 注册网关的协议不需要返回服务器代码，因为服务端靠连接号区分了，以后的数据通过这个连接就过去了，不需要通过网关号区分
	// unsigned int		m_nServerHandle;		// 服务端通过网关分配的网关句柄区分不同的网关
};

#endif  // _DATABLOCK_H_
