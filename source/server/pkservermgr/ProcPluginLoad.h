#ifndef CPROCPLUGINLOAD_HXX
#define CPROCPLUGINLOAD_HXX

#include "ace/DLL.h"
#include "common/eviewdefine.h"
#include <list>

typedef int (*FBeginFunc)(char *pBufferErrorInfo, int nBufferLen);
typedef int (*FEndFunc)(char *pBufferErrorInfo, int nBufferLen);

typedef struct _PLUGIN_INFO
{
	ACE_DLL hPlugin;
	FBeginFunc fBeginFunc;
	FEndFunc fEndFunc;
	_PLUGIN_INFO():hPlugin(true)
	{
		fBeginFunc = NULL;
		fEndFunc = NULL;
	}
}PLUGIN_INFO,*PPLUGIN_INFO;

class CProcPluginLoad
{
public:
	CProcPluginLoad();
	~CProcPluginLoad(void);
	void Load(const char *pICVExePath);
	void UnLoad();
protected:
	void Initialize(const char *pICVExePath);
private:
	char m_szError[PK_LONGFILENAME_MAXLEN];
	std::list<PLUGIN_INFO *> m_listLoadedPlugin;
};

#endif

