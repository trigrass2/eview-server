#ifndef LIC_VERIFY_NO_CV_LICENSE

#include "common/HostIDGetter.h"
//#include <ace/Basic_Types.h>
#include "cryptopp/cpu.h"
using namespace CryptoPP;

#define CV_LICENSE_SEG_SEPRATER		";"
#if defined(_AIX) || defined(__hpux) || defined(_hpux) || defined(hpux)
bool CpuId(int input, unsigned int *output)
{
	//TODO:
	return false;
}
#endif

string CHostIDGetter::GetHostID()
{
	stringstream ss;
	ss << std::hex;
	unsigned int nRegister[4];
	for (unsigned int i = 0; i < 3; i++)
	{
		if(CpuId(i, nRegister))
		{
			ss << i << CV_LICENSE_SEG_SEPRATER
				<< nRegister[0] << CV_LICENSE_SEG_SEPRATER << (i != 1 ? nRegister[1] : nRegister[1] & 0x00FFFFFF) << CV_LICENSE_SEG_SEPRATER 
			   << nRegister[2] << CV_LICENSE_SEG_SEPRATER << nRegister[3] << CV_LICENSE_SEG_SEPRATER;
		}
	}
	return ss.str();
}

#endif