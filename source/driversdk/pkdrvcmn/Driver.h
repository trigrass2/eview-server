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
	//根据设备块查询字符串查找到datablock
	int AddTaskGrps(map<string, CTaskGroup*> mapAddTaskGrp);

protected:
	int InitDriverShmData();
	int ClearShmDataOfDriver();
	int PublishClearDrvShm2NodeServer(int nActionType, const char *szContent, int nContentLen);

public:
	std::map<std::string, CTaskGroup *> m_mapTaskGroups;	// 数据块列表
	bool m_bAutoDeleted;
	std::string m_strType;
	PKDRIVER *m_pOutDriverInfo;	// 用于具体驱动的配置信息, 传给C++驱动使用
	PyObject *m_pyDriverInfo;	// 用于具体驱动的配置信息，传给Python驱动使用
};

