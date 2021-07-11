#ifndef _GETTIMEOFDAY_TCNT_H_
#define _GETTIMEOFDAY_TCNT_H_

#include <ace/OS_NS_sys_time.h>
#include <ace/High_Res_Timer.h>
namespace CVLibHead
{
//#ifdef _WIN32
static ACE_Time_Value gettimeofday_tickcnt()
{
	return ACE_High_Res_Timer::gettimeofday_hr(); // 避免修改系统时间造成不良影响
}
	/*
	static ACE_UINT32 nLastTickCount = 0;
	static ACE_UINT32 nTickCountSwitch = 0;

	ACE_UINT64 nTickCount64;
		
	ACE_UINT32 nTickCount = GetTickCount();

	if (nLastTickCount > nTickCount)
	{
		nTickCountSwitch ++;
	}
	nLastTickCount = nTickCount;
	nTickCount64 = nTickCountSwitch; 

	nTickCount64 = nTickCount64 << 32;

	nTickCount64 += nTickCount;
	return ACE_Time_Value( (ACE_UINT32)(nTickCount64/1000), (ACE_UINT32)(nTickCount64%1000 * 1000));
	//return tv;
}
#else
inline ACE_Time_Value gettimeofday_tickcnt()
{
	return ACE_OS::gettimeofday();
}
*/
//#endif//_WIN32

} // namespace CVLibHead
	
#endif//_GETTIMEOFDAY_TCNT_H_
