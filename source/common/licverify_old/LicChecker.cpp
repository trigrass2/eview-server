/**************************************************************
 *  Filename:    LicChecker.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Implementation Of LicChecker.
 *
 *  @author:     chenzhiquan
 *  @version     10/29/2008  chenzhiquan  Initial Version
 *  @version	2/26/2013  baoyuansong  增加判断是否是硬件许可证选项接口.
**************************************************************/
#include "common/LicChecker.h"
#include "common/pkcomm.h"
#include "errcode/ErrCode_iCV_Common.hxx"
#include "pklog/pklog.h"
#include <ace/OS_NS_sys_time.h>
#include "common/TypeCast.h"
#include <ace/Default_Constants.h>
#include "common/pkGlobalHelper.h"

#include "pklic/license.h"


/**
 *  Constructor.
 *
 *
 *  @version     08/20/2008  chenzhiquan  Initial Version.
 */
CLicChecker::CLicChecker()
{
	long lRet = pkOpenLic("eview", &m_hLicense);
	if(lRet == 0) // 有效许可证（或临时许可证），可得到日期
	{
		m_bLicValid = true;
		char szExipreDate[32] = {0};
		pkGetLicValue(m_hLicense, LM_KEY_EXPIRE_DATE, szExipreDate, sizeof(szExipreDate));
		string strExpireDateTime = szExipreDate; // yyyy-mm-dd
		strExpireDateTime += " 23:59:59"; //yyyy-mm-dd hh:mm:ss
		m_tmExpire = PKTimeHelper::String2Time((char *)strExpireDateTime.c_str());
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "加载许可证成功, 为有效许可证，到期时间：%s", szExipreDate);
	}
	else
	{
		m_bLicValid = false;
		time(&m_tmExpire);
		m_tmExpire += CV_LICENSE_TEMPORARY_VALIDATE;
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "无合法许可证,2小时候到期");
	}
}

/**
 *  Check License Info.
 *  Load License File And Validate License
 *    Check Hardware Code
 *    Load License Info in License File.
 *
 *
 *  @version     08/20/2008  chenzhiquan  Initial Version.
 *  @version	2/26/2013  baoyuansong  增加设置硬件许可证选项.
 */
bool CLicChecker::IsCurTimeExpired()
{
	time_t tmNow;
	time(&tmNow);
	if(m_tmExpire - tmNow >= 0) //在有效期范围内
		return false;
	else
		return true;
}

void CLicChecker::PrintLicenseInfo()
{
	bool bExpired = IsCurTimeExpired();
	char szExpireTime[32] = {0};
	unsigned int nSeconds = m_tmExpire;
	PKTimeHelper::Time2String(szExpireTime, sizeof(szExpireTime), nSeconds);
	printf("--------------------节点许可证信息------------------ \n");
	printf("\t许可证类型：\t%s\n", m_bLicValid? "有效":"无效");
	printf("\t许可有效期：\t%s\n", szExpireTime);
	printf("------------------------------------------------- \n");
}
