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
	CPKServerBase (bool bVerifyLic, const char *szLicIdToVerify = 0);// szLicIdToVerify:����б�����֤������ID�����ơ�ΪNULL����ַ������ʾ����Ҫ��֤
	virtual ~CPKServerBase();
	// ����ʵ�ֵĺ���
	virtual int OnStart(int argc, char* args[]) = 0;
	virtual int OnStop() = 0;

	// ����ѡ��ʵ�ֵ�����
	virtual void OnRefresh(); // ֪ͨˢ��
	virtual void PrintStartUpScreen();
	virtual void PrintHelpScreen(); // һ�㲻��Ҫ����
	virtual bool ProcessAppCmd(char c); // �������q����������һ�㲻��Ҫ����
	 
public:	
	int	Main(int argc, char* args[]); // ��main�������õķ���

protected:
	char	m_szProcName[256];
	char	m_szLicIdToVerify[256]; // ����б�����֤������ID�����ơ�ΪNULL����ַ������ʾ����Ҫ��֤
private:
	CPKServerBaseImpl * m_pServerBaseImpl;
};

#endif//_PEAK_SERVER_BASE_H_
