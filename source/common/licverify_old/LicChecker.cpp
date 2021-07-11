/**************************************************************
 *  Filename:    LicChecker.cpp
 *  Copyright:   Shanghai Peak InfoTech Co., Ltd.
 *
 *  Description: Implementation Of LicChecker.
 *
 *  @author:     chenzhiquan
 *  @version     10/29/2008  chenzhiquan  Initial Version
 *  @version	2/26/2013  baoyuansong  �����ж��Ƿ���Ӳ�����֤ѡ��ӿ�.
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
	if(lRet == 0) // ��Ч���֤������ʱ���֤�����ɵõ�����
	{
		m_bLicValid = true;
		char szExipreDate[32] = {0};
		pkGetLicValue(m_hLicense, LM_KEY_EXPIRE_DATE, szExipreDate, sizeof(szExipreDate));
		string strExpireDateTime = szExipreDate; // yyyy-mm-dd
		strExpireDateTime += " 23:59:59"; //yyyy-mm-dd hh:mm:ss
		m_tmExpire = PKTimeHelper::String2Time((char *)strExpireDateTime.c_str());
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "�������֤�ɹ�, Ϊ��Ч���֤������ʱ�䣺%s", szExipreDate);
	}
	else
	{
		m_bLicValid = false;
		time(&m_tmExpire);
		m_tmExpire += CV_LICENSE_TEMPORARY_VALIDATE;
		PKLog.LogMessage(PK_LOGLEVEL_NOTICE, "�޺Ϸ����֤,2Сʱ����");
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
 *  @version	2/26/2013  baoyuansong  ��������Ӳ�����֤ѡ��.
 */
bool CLicChecker::IsCurTimeExpired()
{
	time_t tmNow;
	time(&tmNow);
	if(m_tmExpire - tmNow >= 0) //����Ч�ڷ�Χ��
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
	printf("--------------------�ڵ����֤��Ϣ------------------ \n");
	printf("\t���֤���ͣ�\t%s\n", m_bLicValid? "��Ч":"��Ч");
	printf("\t�����Ч�ڣ�\t%s\n", szExpireTime);
	printf("------------------------------------------------- \n");
}
