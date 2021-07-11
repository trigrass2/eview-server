// RegularDllExceptionAttacher.h: interface for the RegularDllExceptionAttacher class.
//
//////////////////////////////////////////////////////////////////////
#pragma once

#ifdef _WIN32
#	ifdef pkcrashdump_EXPORTS
#		define PKEXCEPTIONREPORT_API __declspec(dllexport)
#	else
#		define PKEXCEPTIONREPORT_API __declspec(dllimport)
#	endif
#else//_WIN32
#	define PKEXCEPTIONREPORT_API  __attribute__ ((visibility ("default")))
#endif//_WIN32


class PKEXCEPTIONREPORT_API CPKCrashDump
{
public:
	CPKCrashDump();	// �Ƿ񵯳��쳣�Ի���0��ʾ��������1��ʾ����
	virtual ~CPKCrashDump();
};

static CPKCrashDump g_crashDump;
