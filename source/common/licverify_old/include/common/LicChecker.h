#ifndef _LIC_VERIFY_LIC_CHECKER_H_
#define _LIC_VERIFY_LIC_CHECKER_H_

#include "common/eviewdefine.h"
#include "pklic/license.h"
#include <string>
#include <map>
using namespace std;
//#include "HostIDGetter.h"

//////////////////////////////////////////////////////////////////////////
// CLicChecker Class Declaration
//////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
#pragma comment(lib, "Iphlpapi.lib")
#endif//WIN32

#define CV_LICENSE_TEMPORARY_VALIDATE		(60*60*2) // 2 hr temporary (60 * 60 * 2) 


class CLicChecker
{
public:
	bool m_bLicValid;	// 许可证是否有效
	time_t		m_tmExpire;	// 过期时间

	CLicChecker();
	// Get License Information
	void PrintLicenseInfo();

	long GetTimeValid(HighResTime &timeValid);
	void CheckLicenseInfo();

	bool IsCurTimeExpired();

private:
	LM_HANDLE m_hLicense;

#ifndef LIC_VERIFY_NO_CV_LICENSE
	//CHostIDGetter m_hostIDGetter;
#endif
};

#endif//_LIC_VERIFY_LIC_CHECKER_H_
