/**************************************************************
 *  Filename:    DataBlock.h
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: ���ݿ���Ϣʵ����.
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

#define  INVALID_BLOCK_NUMBER	-1		// DIT�����ݿ鴴�����ɹ�ʱ�����ݿ��
#define	 GATEWAY_ID_LEN			8	
#define	 PROJECT_CODE_LENGTH    8

#define		COMMAND_ID_REGISTER_GROUP				1
#define		COMMAND_ID_UNREGISTER_ALL				2
#define		COMMAND_ID_WRTITE_GROUP					3

class CCycleReader: public ACE_Event_Handler
{
public:
	CCycleReader(CMainTask *pTask);
	virtual ~CCycleReader();
	
	void StartTimer();
	void StopTimer();

	long RegisterToServer();

	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);	// collect data 
public:
	int					m_nPollRate;			// ɨ�����ڣ���λms
	int					m_nPollPhase;			// ɨ����λ����λms
	CMainTask			*m_pMainTask;
	vector<TAGGROUP *>	m_vecTagGroup;			// TAG�����洢�Ա�
	char				m_szGatewayId[GATEWAY_ID_LEN + 1];
	char				m_szProjectCode[PROJECT_CODE_LENGTH + 1];
	unsigned char		m_nTransId;

	CClientSockHandler	m_sockHandler; 
};

#endif  // _DATABLOCK_H_
