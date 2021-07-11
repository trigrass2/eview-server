/************************************************************************
 *	Filename:		PROCPLUGINLOAD.CPP
 *	Copyright:		Shanghai Peak InfoTech Co., Ltd.
 *
 *	Description:	�������в�� .
 *
 *	@author:		shijunpu
 *	@version		6/3/2013	shijunpu	Initial Version
*************************************************************************/
#include "ProcPluginLoad.h"
#include "pkcomm/pkcomm.h"
#include "ace/OS_NS_stdio.h"
#include "ace/Default_Constants.h"
#include "common/eviewdefine.h"
#include <string>

#define PROCESSMGR_PLUGINS_NAME                     "procmgr_plugins"

CProcPluginLoad::CProcPluginLoad()
{
	memset(m_szError, 0, sizeof(m_szError));
}

CProcPluginLoad::~CProcPluginLoad(void)
{
}

/**
 *  $(Desp) �������в����ִ��begin����.
 *  $(Detail) .
 *
 *  @return		void.
 *
 *  @version	6/3/2013  shijunpu  Initial Version.
 */
void CProcPluginLoad::Load(const char *pICVExePath)
{
	Initialize(pICVExePath);
	for (std::list<PLUGIN_INFO *>::iterator iter = m_listLoadedPlugin.begin(); \
		iter != m_listLoadedPlugin.end(); ++iter)
	{
		if ((*iter)->fBeginFunc != NULL)
			(*iter)->fBeginFunc(m_szError, PK_LONGFILENAME_MAXLEN);
	}
}

/**
 *  $(Desp) ִ�����в��end����.
 *  $(Detail) .
 *
 *  @return		void.
 *
 *  @version	6/3/2013  shijunpu  Initial Version.
 */
void CProcPluginLoad::UnLoad()
{
	for (std::list<PLUGIN_INFO *>::iterator iter = m_listLoadedPlugin.begin(); \
		iter != m_listLoadedPlugin.end(); ++iter)
	{
		if ((*iter)->fEndFunc != NULL)
			(*iter)->fEndFunc(m_szError, PK_LONGFILENAME_MAXLEN);
		delete *iter;
	}
	m_listLoadedPlugin.clear();
}

/**
 *  $(Desp) �������в��.
 *  $(Detail) .
 *
 *  @param		-[in,out]  const char * pICVExePath : [comment]
 *  @return		void.
 *
 *  @version	6/3/2013  shijunpu  Initial Version.
 */
void CProcPluginLoad::Initialize(const char *pICVExePath)
{
	if (NULL == pICVExePath)
		return;

	std::string strProcMngrPlugins = std::string(pICVExePath) + ACE_DIRECTORY_SEPARATOR_STR + PROCESSMGR_PLUGINS_NAME;
	if(!PKFileHelper::IsDirectoryExist(strProcMngrPlugins.c_str())) // ���Ŀ¼�����ھͲ���Ҫ������
		return;

	std::list<std::string> listFiles;
	PKFileHelper::ListFilesInDir(strProcMngrPlugins.c_str(), listFiles);
	for (list<string>::iterator itFileName = listFiles.begin(); itFileName != listFiles.end(); itFileName ++)
	{
		std::string strPluginName = strProcMngrPlugins + ACE_DIRECTORY_SEPARATOR_STR + *itFileName;
		PLUGIN_INFO *pPluginInfo = new PLUGIN_INFO;
		if (0 == pPluginInfo->hPlugin.open(strPluginName.c_str()))
		{
			pPluginInfo->fBeginFunc = (FBeginFunc)pPluginInfo->hPlugin.symbol("BeginFunc");
			pPluginInfo->fEndFunc = (FEndFunc)pPluginInfo->hPlugin.symbol("EndFunc");
		}
		m_listLoadedPlugin.push_back(pPluginInfo);
	}
	listFiles.clear();
}
