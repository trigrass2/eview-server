/**
 *  @file        license.h
 *  ���֤��֤�ͻ��˶����ļ�
 *  @version     10/27/2009  fanguozhu  0.1
 *  @version     07/30/2010  wenyan		1.2.0
 *  @version     09/07/2010  wangjian	1.2.2
 */
#ifndef __LICENSE_H__
#define __LICENSE_H__

// ���֤״̬����Ч���֤���޹���ʱ�䣬2Сʱ������ʽ���֤��һ�νϳ�ʱ�䣩����ʱ���֤��һ��Ϊ1���£�����ʽ��ͬ��
#define	PK_LIC_STATUS_EXIST_VALID				0		// ���֤��Ч���Ϸ�
#define	PK_LIC_STATUS_NOEXIST					1		// ���֤������
#define PK_LIC_STATUS_EXIST_FILE_OPENFAILED		2		// �ļ���ʽ�Ƿ�������xml��ʽ, ����ȱ�ٱ���Ľڵ�
#define	PK_LIC_STATUS_EXIST_EXPIRED				3		// ���ڵ�����
#define	PK_LIC_STATUS_EXIST_UNDUE				4		// ���ڵ�δ����
#define	PK_LIC_STATUS_EXIST_HOSTID_NOT_MATCH	5		// ���ڵ�����ID��ƥ��
#define	PK_LIC_STATUS_EXIST_SIGNATURE_NOTMATCH	6		// ���ڵ�ǩ����ƥ��

//fixed attr
#define	PK_LIC_KEY_LICENSE						"license"		/**<  -ǩ���ַ���.  */
#define PK_LIC_KEY_CUSTOMER						"customer"	/**<  -��Ȩ�ͻ�������.  */
#define PK_LIC_KEY_PROJECT						"project"	/**<  -��Ȩ��Ŀ������.  */
#define PK_LIC_KEY_EXPIRE_DATE					"expiredate"	/**<  -����ʱ��.  */
#define	PK_LIC_KEY_START_DATE					"startdate"		/**<  -��ʼʱ��.  */
#define PK_LIC_KEY_OS							"os"			/**<  -��Ȩ����ϵͳ.  */
#define PK_LIC_KEY_IO_NUMBER					"ionum"		/**<  -��Ʒ�汾.  */
#define PK_LIC_KEY_CLIENTS_NUMBER				"clientnum"		/**<  -��Ʒ�汾.  */
#define PK_LIC_KEY_DISTRIBUTOR					"distributor"	/**<  -��Ȩ��λ.  */
#define PK_LIC_KEY_PRODUCT_NAME					"productname"	/**<  -��Ʒ����.  */
#define PK_LIC_KEY_PRODUCT_CODE					"productcode"	/**<  -���֤��Ʒ����.  */
#define PK_LIC_KEY_PRODUCT_VERSION				"productversion"	/**<  -��Ʒ�汾.  */

#define PK_LIC_KEY_HOST_ID						"hostid"	/**<  -������ʶ(MAC��ַ).  */
#define	PK_LIC_KEY_SIGN_DATE					"signdate"		/**<  -ǩ��ʱ��.  */
#define	PK_LIC_KEY_SIGNATURE					"signature"		/**<  -ǩ���ַ���.  */

#define PK_LIC_ERRCODE_OK						0		/**<  -�ɹ���־.  */
#define PK_LIC_ERRCODE_BASE						80000				/**<  -������뷶Χ��ʼ.  */
#define PK_LIC_ERRCODE_KEY_NOT_FOUND			PK_LIC_ERRCODE_BASE + 1	/**<  -��Ϣ�����.  */
#define PK_LIC_ERRCODE_SIGNATURE_NOT_EXIST		PK_LIC_ERRCODE_BASE + 2	// ǩ���ֶβ�����
#define PK_LIC_ERRCODE_SIGNATURE_LEN_ERR		PK_LIC_ERRCODE_BASE + 3	// ǩ���ֶγ��Ȳ��ԣ�����80
#define PK_LIC_ERRCODE_SIGNATURE_NOTMATCH		PK_LIC_ERRCODE_BASE + 4	// ǩ����ƥ��
#define PK_LIC_ERRCODE_VERIFY_PROCESS_EXCEPT	PK_LIC_ERRCODE_BASE + 5	// ��֤�����쳣
#define PK_LIC_ERRCODE_LICFILE_NOTEXIST			PK_LIC_ERRCODE_BASE + 6	// ����ļ�������
#define PK_LIC_ERRCODE_LICFILE_INVALIDPARAM		PK_LIC_ERRCODE_BASE + 7	// ��������Ƿ�
#define PK_LIC_ERRCODE_LACK_NODE				PK_LIC_ERRCODE_BASE + 8	// ȱ�ٽڵ�
#define PK_LIC_ERRCODE_HANDLE_ISNULL			PK_LIC_ERRCODE_BASE + 9	// ������Ϊ��
#define PK_LIC_ERRCODE_HOSTID_NOT_MATCH			PK_LIC_ERRCODE_BASE + 10	// ����ID��ƥ��
#define PK_LIC_ERRCODE_LICFILE_OPENFAILED		PK_LIC_ERRCODE_BASE + 11	// ������ļ�ʧ��

typedef void *PKLIC_HANDLE;

/**
 *  �����֤�ļ�������֤�Ƿ�Ϸ�.
 *  @param[in] file [���֤�ļ���·��]
 *  @param[in] license_code [��Ʒ�Ĵ���]
 *  @param[out] lm_handle [���ص����֤���]
 *  @return
 *		- ==0 �ɹ�
 *		- !=0 �����쳣
 */
int pkOpenLic(char *szLiencseFilePath, PKLIC_HANDLE *lm_handle, int *pnLicStatus, char *szErrTip, int nErrTipSize);

/**
 *  �ر����֤���ͷ����֤�����Դ.
 *  @param[in] lm_handle [�򿪵����֤���]
 *  @return
 *		- ==0 �ɹ�
 *		- !=0 �����쳣
 */
int pkCloseLic(PKLIC_HANDLE lm_handle);

/**
 *  ��ȡ���֤��Ϣ���ֵ.
 *  @param[in] lm_handle [�򿪵����֤���]
 *  @param[in] key [��Ϣ������]
 *  @param[out] value [���ص���Ϣ���ֵ]
 *  @param[in] size [��Ϣ��ֵ�Ļ���������]
 *  @return
 *		- ==0 �ɹ�
 *		- !=0 �����쳣
 */
int pkGetLicValue(PKLIC_HANDLE lm_handle, const char *key, char *value, int size);

#endif /* __LICENSE_H__ */
