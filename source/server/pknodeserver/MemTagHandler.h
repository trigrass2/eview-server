/**************************************************************
*  Filename:    DataBlock.h
*  Copyright:   Shanghai Peak InfoTech Co., Ltd.
*
*  Description: 数据块信息实体类.
*
*  @author:     lijingjing
*  @version     05/28/2008  lijingjing  Initial Version
**************************************************************/

#ifndef _MEM_TAG_HANDLER_H_
#define _MEM_TAG_HANDLER_H_

#include <ace/Thread_Mutex.h>
#include <ace/Guard_T.h>
#include <list>
#include <set>

class CTagProcessTask;
class CMemTagHandler : public ACE_Event_Handler
{
public:
	CTagProcessTask *m_pTimerTask;

public:
	CMemTagHandler();
	virtual ~CMemTagHandler();
	void StartTimer();
	void StopTimer();
	virtual int handle_timeout(const ACE_Time_Value &current_time, const void *act);

public:
	int WriteTag(CDataTag *pTag, char *szTagData, int nTagDataLen);
	int InitTags();
};

#endif  // _DATABLOCK_H_
