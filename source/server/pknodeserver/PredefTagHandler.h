/**************************************************************
*  Filename:    DataBlock.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 数据块信息实体类.
*
*  @author:     lijingjing
*  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _PREDEFINE_TAG_HANDLER_H_
#define _PREDEFINE_TAG_HANDLER_H_

#include <ace/Thread_Mutex.h>
#include <ace/Guard_T.h>
#include <list>
#include <set>

#define ALARM_NAME_ALARM_COUNT					"alarmcount"
#define ALARM_NAME_ALARMING_COUNT				"alarmingcount"
#define ALARM_NAME_ALARM_UNCONFIRM_COUNT		"alarmunconfirmcount"
#define ALARM_NAME_ALARM_CONFIRMED_COUNT		"alarmconfirmcount"

#define TAGNAME_SYSTEM_ALARM					"system.alarm"

#define TAG_SYSTEM_CPU							"system.cpu"
#define TAG_SYSTEM_MEMORY						"system.memory"
#define TAG_SYSTEM_TIME							"system.time"

#define TAG_SYSTEM_TAG_COUNT					"system.tagcount"
#define TAG_SYSTEM_LICENSE_EXPIRETIME			"system.expiretime"				// 许可证到期时间


class CDataTag;
class CTagProcessTask;
class CPredefTagHandler : public ACE_Event_Handler
{
public:
	CTagProcessTask *m_pTagprocessTask;
public:
	CPredefTagHandler();
	virtual ~CPredefTagHandler();
	void StartTimer();
	void StopTimer();
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);

public:
	int WriteTag(CDataTag *pTag, char *szTagData, int nTagDataLen);
	int InitTags();
};

#endif  // _DATABLOCK_H_
