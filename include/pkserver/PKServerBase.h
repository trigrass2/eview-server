#ifndef _PEAK_SERVER_BASE_H_
#define _PEAK_SERVER_BASE_H_

#ifdef _WIN32
#ifdef pkserverbase_EXPORTS
#define  PK_SERVERBASE __declspec(dllexport)
#else
#define  PK_SERVERBASE __declspec(dllimport)
#endif
#else //_WIN32
#define  PK_SERVERBASE  __attribute__ ((visibility ("default")))
#endif //_WIN32

class CPKServerBaseImpl;
class PK_SERVERBASE CPKServerBase
{
public:
	CPKServerBase (bool bVerifyLic, const char *szLicIdToVerify = 0);// szLicIdToVerify:许可中必须验证包含的ID后名称。为NULL或空字符串则表示不需要验证
	virtual ~CPKServerBase();
	// 必须实现的函数
	virtual int OnStart(int argc, char* args[]) = 0;
	virtual int OnStop() = 0;

	// 可以选择实现的命令
	virtual void OnRefresh(); // 通知刷新
	virtual void PrintStartUpScreen();
	virtual void PrintHelpScreen(); // 一般不需要重载
	virtual bool ProcessAppCmd(char c); // 处理除了q外的其他命令。一般不需要重载
	 
public:	
	int	Main(int argc, char* args[]); // 被main函数调用的方法

protected:
	char	m_szProcName[256];
	char	m_szLicIdToVerify[256]; // 许可中必须验证包含的ID后名称。为NULL或空字符串则表示不需要验证
private:
	CPKServerBaseImpl * m_pServerBaseImpl;
};

#endif//_PEAK_SERVER_BASE_H_
