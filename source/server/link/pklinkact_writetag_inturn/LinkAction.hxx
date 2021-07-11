/**************************************************************
 *  Filename:    pdbaction.hxx
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: �������ݿ���ģ��.
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
	time_t tmHoldTime; //ռ��ʱ��ʱ��
	int nHoldSeconds;  // ռ�е�����
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

//�������ƶ���ģ������
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
	//����ִ�����ʼ������
	int	InitAction();
	//����������
    int	ExecuteAction(char *szActionId, char *szTagNameList, char *szTagValue, char *szHoldTime, char *szActionParam4, char* nPriority, char* szJsonEventParams, char *szResult, int nResultBufLen);

	//��Դ�ͷź���
	int	ExitAction();
};


#endif // !defined(AFX_PDBACTION_HXX__6199A96D_43BA_45A2_8BD1_73528155F701__INCLUDED_)
