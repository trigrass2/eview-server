/************************************************************************
 *	Filename:		CVDRIVERCOMMON.CPP
 *	Copyright:		Shanghai Peak InfoTech Co., Ltd.
 *
 *	Description:	$(Desp) .
 *
 *	@author:		shijunpu
 *	@version		??????	shijunpu	Initial Version
 *  @version	9/17/2012  baoyuansong  ��������Ԫ�ش�С�ӿ� .
*************************************************************************/
#include "lua.hpp"
#include "TaskGroup.h"
#include "Device.h"
#include "MainTask.h"
#include "driversdk/pkdrvcmn.h"
#include "pklog/pklog.h"
#include "common/RMAPI.h"
#ifdef WIN32
#ifndef NO_EXCEPTION_ATTACHER
#include "common/RegularDllExceptionAttacher.h"
#pragma comment(lib, "ExceptionReport.lib")
#pragma comment(lib, "dbghelp.lib")
// global member to capture exception so as to analyze 
static CRegularDllExceptionAttacher	g_RegDllExceptAttacher;
#endif//#ifndef NO_EXCEPTION_ATTACHER
#endif

lua_State *g_lua_state = NULL;
//��Lua���õ�Cע�ắ����
static int add2(lua_State* L)
{
	//���ջ�еĲ����Ƿ�Ϸ���1��ʾLua����ʱ�ĵ�һ������(������)���������ơ�
	//���Lua�����ڵ���ʱ���ݵĲ�����Ϊnumber���ú�����������ֹ�����ִ�С�
	double op1 = luaL_checknumber(L,1);
	double op2 = luaL_checknumber(L,2);
	//�������Ľ��ѹ��ջ�С�����ж������ֵ��������������ѹ��ջ�С�
	lua_pushnumber(L,op1 + op2);
	//����ֵ������ʾ��C�����ķ���ֵ��������ѹ��ջ�еķ���ֵ������
	return 1;
}

int drv_GetDriverInfo(lua_State *lua_state)
{
	// DRVHANDLE hDriver
	PKDRIVER *pDriver = NULL;
	//lua_pushnumber(lua_state,pDriver);
	return 1;
}

int drv_GetDeviceInfo(lua_State *lua_state)
{
	//HDEVICE hDevice
	// CVDEVICE *pDevice = NULL;
	//lua_pushnumber(lua_state,pDevice);
	return 1;
}

// DRVHANDLE h, int iIndex, int lUserData
// void
int drv_SetUserData(lua_State *lua_state)
{
	return 1;
}

// DRVHANDLE h, int iIndex
// int
int drv_GetUserData(lua_State *lua_state)
{
	return 0;
}

// DRVHANDLE hDrv, int iIndex
// void *
int drv_GetUserDataPtr(lua_State *lua_state)
{
	return 0;
}

// DRVHANDLE hDrv, int iIndex, void *pUserData
// void
int drv_SetUserDataPtr(lua_State *lua_state)
{
	return 0;
}

// ���ط��͵��ֽڸ���
// DRVHANDLE hDevice, char *szBuffer, long lBufLen, long lTimeOutMS 
// int
int drv_SendToDevice(lua_State *lua_state)
{
	return 0;
}

// �����յ����ֽڸ���
// DRVHANDLE hDevice, char *szBuff, long lBufLen, long lTimeOutMS
// int
int drv_RecvFromDevice(lua_State *lua_stat)
{
	return 0;
}

// ����N�ֽڣ������յ����ֽڸ���
// DRVHANDLE hDevice, char *szBuffer, long lBufLen, long lTimeOutMS
// int
int drv_Recv_NFromDevice(lua_State *lua_state)
{
	return 0;
}

// DRVHANDLE hDevice, long lTimeOutMS
// int
int drv_ReConnectDevice(lua_State *lua_state)
{
	return 0;
}

//DRVHANDLE hDevice
// void 
int drv_ClearRecvBuffer(lua_State *lua_state)
{
	return 0;
}

// int nLogLevel, const char *fmt,...
// void
int drv_LogMessage(lua_State *lua_state)
{
	return 0;
}

// DRVTIME *pDrvTime
// void 
int drv_GetCurrentTime(lua_State *lua_state)	// ��ȡ��ǰʱ��
{
	return 0;
}

// DRVHANDLE hDevice, unsigned short usPort 
// int
int drv_SetTcpClientLocalPort(lua_State *lua_state)
{	
	return 0;
}

// DRVHANDLE hDevice, int *pnTagNum
// DRVTAG *
int drv_GetTags(lua_State *lua_state)
{
	return 0;
}

// �������ݿ�����ݡ�״̬��ʱ���, ���µ�redis��ȥ
// DRVHANDLE hDevice, TAGDATA *pTagsData, int nTagDataNum, long lStatus, DRVTIME* ptvTimeStamp
// long
int drv_UpdateTagsData(lua_State *lua_state)
{
	return 0;
}
// ��ȡ���ݿ���Ϣ�����ڿ��豸���а�λ����ʱ���Ȼ�ȡtag���Ӧ�Ŀ��ֵ��Ȼ���tag������λ����
// DRVHANDLE hDevice, TAGDATA *pTagData, long *plStatus, DRVTIME* ptvTimeStamp
// long
int drv_GetTagData(lua_State *lua_state)
{
	return 0;
}

//DRVHANDLE hDevice, DRVTIMER *timerInfo
//long
int drv_CreateTimer(lua_State *lua_state)
{
	return 0;
}

//DRVHANDLE hDevice, int *pnTimerNum
//DRVTIMER *
int drv_GetTimers(lua_State *lua_state)
{
	return 0;
}

//DRVHANDLE hDevice, DRVHANDLE hTimer
//long
int drv_DestroyTimer(lua_State *lua_stat)
{
	return 0;
}


// �ڲ�����lua����
double lua_add(double x, double y)
{
	double z;
	lua_getglobal(g_lua_state, "add");    // ��ȡlua����f
	lua_pushnumber(g_lua_state, x);    // ѹ�����x��y
	lua_pushnumber(g_lua_state, y);

	if(lua_pcall(g_lua_state, 2, 1, 0) != 0)
		printf("error running function 'f': %s", lua_tostring(g_lua_state, -1));

	if(!lua_isnumber(g_lua_state, -1))
		printf( "function 'f' must return a number");
	z = lua_tonumber(g_lua_state, -1);
	lua_pop(g_lua_state, 1);
	return z;
}

long InitLua()
{
	g_lua_state = luaL_newstate();
	//2.���ô�ע���Lua��׼�⣬������Ǹ����Lua�ű��õ�
	//��Ϊ����������ֻ����Lua�ű��������hello world������ֻ���������Ϳ�����
	static const luaL_Reg lualibs[] =
	{
		{ "base", luaopen_base },
		{ NULL, NULL}
	};

	//3.ע��Lua��׼�Ⲣ���ջ
	const luaL_Reg *lib = lualibs;
	for(; lib->func != NULL; lib++)
	{
		luaL_requiref(g_lua_state, lib->name, lib->func, 1);
		lua_pop(g_lua_state, 1);
	}

	//��ָ���ĺ���ע��ΪLua��ȫ�ֺ������������е�һ���ַ�������ΪLua����
	//�ڵ���C����ʱʹ�õ�ȫ�ֺ��������ڶ�������Ϊʵ��C������ָ�롣
	lua_register(g_lua_state, "add2", add2);
	lua_register(g_lua_state, "drv_GetDriverInfo", drv_GetDriverInfo);
	lua_register(g_lua_state, "drv_GetDeviceInfo", drv_GetDeviceInfo);
	lua_register(g_lua_state, "drv_SetUserData", drv_SetUserData);
	lua_register(g_lua_state, "drv_GetUserData", drv_GetUserData);
	lua_register(g_lua_state, "drv_SendToDevice", drv_SendToDevice);
	lua_register(g_lua_state, "drv_RecvFromDevice", drv_RecvFromDevice);
	lua_register(g_lua_state, "drv_ReConnectDevice", drv_ReConnectDevice);
	lua_register(g_lua_state, "drv_ClearRecvBuffer", drv_ClearRecvBuffer);
	lua_register(g_lua_state, "drv_LogMessage", drv_LogMessage);
	lua_register(g_lua_state, "drv_GetTags", drv_GetTags);
	lua_register(g_lua_state, "drv_UpdateTagsData", drv_UpdateTagsData);
	lua_register(g_lua_state, "drv_GetTagData", drv_GetTagData);
	lua_register(g_lua_state, "drv_CreateTimer", drv_CreateTimer);
	lua_register(g_lua_state, "drv_GetTimers", drv_GetTimers);
	lua_register(g_lua_state, "drv_DestroyTimer", drv_DestroyTimer);

	//lua_register(g_lua_state, "drv_SetUserDataPtr", drv_SetUserDataPtr);
	//lua_register(g_lua_state, "drv_GetUserDataPtr", drv_GetUserDataPtr);
	//lua_register(g_lua_state, "drv_GetCurrentTime", drv_GetCurrentTime);
	//lua_register(g_lua_state, "drv_SetTcpClientLocalPort", drv_SetTcpClientLocalPort);
	// ��һ�ַ���
	/*
	lua_pushcfunction( lua_state, getSum );
	lua_setglobal( lua_state, "getSum" );
	lua_pushcfunction( lua_state, mycontact );
	lua_setglobal( lua_state, "mycontact" );*/


	//4������hello.lua�ű�

	const char* testfunc = "a=200 b=1000 print(\"ʯͷaaʯͷ\") print(add2(1.0,2.0)) function add ( x, y ) return x + y + a+b end";
	if (luaL_dostring(g_lua_state,testfunc))
	    printf("Failed to invoke.\n");

	double ret = lua_add(100,200);
	printf("call Lua func: add,result:%f",ret);
	//luaL_dofile(lua_state, "d:\\PrivateWork\\GateWay\\executable\\hello.lua");

	//int result = luaAdd(lua_state,4,1);
	//printf("result:%d\n",result);
	return 0;
}

long UnInitLua()
{
	//5. �ر�Lua�����
	lua_close(g_lua_state);
	return 0;
}