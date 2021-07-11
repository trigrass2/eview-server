/**************************************************************
 *  Filename:    pdbaction.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: 过程数据控制模块.
 *
 *  @author:     caichunlei
 *  @version     08/22/2008  caichunlei  Initial Version
**************************************************************/
#if !defined(AFX_PDBACTION_HXX__6199A96D_43BA_45A2_8BD1_73528155F701__INCLUDED_)
#define AFX_PDBACTION_HXX__6199A96D_43BA_45A2_8BD1_73528155F701__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "pkdata/pkdata.h"
#include <string>
#include <vector>
#include <algorithm>
#include <map>
using namespace std;

typedef struct _LinkTag
{
	string strTagName;
	bool bHold;
	time_t tmHoldTime; //占有时的时间
	int nHoldSeconds;  // 占有的秒数
	int nPriority;
	_LinkTag()
	{
		strTagName = "";
		bHold = false;
		tmHoldTime = 0;
		nPriority = 0;
		nHoldSeconds = 10;
	}
}LinkTag;

typedef struct _LinkTagGroup
{
	string strTagsMd5;
	vector<LinkTag*> vecTags;
	_LinkTagGroup()
	{
		strTagsMd5 = "";
		vecTags.clear();
	}
}LinkTagGroup;

//变量控制动作模块名称
class CLinkAction
{
public:
	CLinkAction();
	virtual ~CLinkAction();

public:
	LinkTagGroup * GetTagGroup(char *szTagNameList);
	map<string, LinkTagGroup *> m_mapMd52TagGroup;

public:
	PKHANDLE m_hPKData;
	//动作执行类初始化函数
	int	InitAction();
	//动作处理函数
    int	ExecuteAction(char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldTime, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen);

	//资源释放函数
	int	ExitAction();
};


#endif // !defined(AFX_PDBACTION_HXX__6199A96D_43BA_45A2_8BD1_73528155F701__INCLUDED_)
