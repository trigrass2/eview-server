#pragma once
#include "drvframework.h"
#include "PythonHelper.h"

class CTaskGroup;
class CRedisProxy;
class CDriver: public CDrvObjectBase
{
public:
	CDriver(void);
	~CDriver(void);
	void Copy2OutDriverInfo();
	static int UpdateDriverStatus(int nStatus, char *szErrTip, char *pszTime);
	static int UpdateDriverOnlineConfigTime(int nStatus, char *szErrTip, char *pszTime);
	static int UpdateDriverStartTime();
	int UpdateDriverDeviceNum();
	int GetDeviceNum();
	static void ResetTagsValue(string g_strDrvName);

	int Start();
	int Stop();
	//�����豸���ѯ�ַ������ҵ�datablock
	int AddTaskGrps(map<string, CTaskGroup*> mapAddTaskGrp);

protected:
	int InitDriverShmData();
	int ClearShmDataOfDriver();
	int PublishClearDrvShm2NodeServer(int nActionType, const char *szContent, int nContentLen);

public:
	std::map<std::string, CTaskGroup *> m_mapTaskGroups;	// ���ݿ��б�
	bool m_bAutoDeleted;
	std::string m_strType;
	PKDRIVER *m_pOutDriverInfo;	// ���ھ���������������Ϣ, ����C++����ʹ��
	PyObject *m_pyDriverInfo;	// ���ھ���������������Ϣ������Python����ʹ��
};

