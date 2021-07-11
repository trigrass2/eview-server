/**
 *  @file        license.h
 *  许可证验证客户端定义文件
 *  @version     10/27/2009  fanguozhu  0.1
 *  @version     07/30/2010  wenyan		1.2.0
 *  @version     09/07/2010  wangjian	1.2.2
 */
#ifndef __LICENSE_H__
#define __LICENSE_H__

// 许可证状态，无效许可证（无过期时间，2小时）、正式许可证（一段较长时间）、临时许可证（一般为1个月，和正式相同）
#define	PK_LIC_STATUS_EXIST_VALID				0		// 许可证有效、合法
#define	PK_LIC_STATUS_NOEXIST					1		// 许可证不存在
#define PK_LIC_STATUS_EXIST_FILE_OPENFAILED		2		// 文件格式非法，不是xml格式, 或者缺少必须的节点
#define	PK_LIC_STATUS_EXIST_EXPIRED				3		// 存在但过期
#define	PK_LIC_STATUS_EXIST_UNDUE				4		// 存在但未到期
#define	PK_LIC_STATUS_EXIST_HOSTID_NOT_MATCH	5		// 存在但主机ID不匹配
#define	PK_LIC_STATUS_EXIST_SIGNATURE_NOTMATCH	6		// 存在但签名不匹配

//fixed attr
#define	PK_LIC_KEY_LICENSE						"license"		/**<  -签名字符串.  */
#define PK_LIC_KEY_CUSTOMER						"customer"	/**<  -授权客户的名称.  */
#define PK_LIC_KEY_PROJECT						"project"	/**<  -授权项目的名称.  */
#define PK_LIC_KEY_EXPIRE_DATE					"expiredate"	/**<  -过期时间.  */
#define	PK_LIC_KEY_START_DATE					"startdate"		/**<  -起始时间.  */
#define PK_LIC_KEY_OS							"os"			/**<  -授权操作系统.  */
#define PK_LIC_KEY_IO_NUMBER					"ionum"		/**<  -产品版本.  */
#define PK_LIC_KEY_CLIENTS_NUMBER				"clientnum"		/**<  -产品版本.  */
#define PK_LIC_KEY_DISTRIBUTOR					"distributor"	/**<  -授权单位.  */
#define PK_LIC_KEY_PRODUCT_NAME					"productname"	/**<  -产品名称.  */
#define PK_LIC_KEY_PRODUCT_CODE					"productcode"	/**<  -许可证产品代码.  */
#define PK_LIC_KEY_PRODUCT_VERSION				"productversion"	/**<  -产品版本.  */

#define PK_LIC_KEY_HOST_ID						"hostid"	/**<  -主机标识(MAC地址).  */
#define	PK_LIC_KEY_SIGN_DATE					"signdate"		/**<  -签发时间.  */
#define	PK_LIC_KEY_SIGNATURE					"signature"		/**<  -签名字符串.  */

#define PK_LIC_ERRCODE_OK						0		/**<  -成功标志.  */
#define PK_LIC_ERRCODE_BASE						80000				/**<  -错误代码范围起始.  */
#define PK_LIC_ERRCODE_KEY_NOT_FOUND			PK_LIC_ERRCODE_BASE + 1	/**<  -信息项不存在.  */
#define PK_LIC_ERRCODE_SIGNATURE_NOT_EXIST		PK_LIC_ERRCODE_BASE + 2	// 签名字段不存在
#define PK_LIC_ERRCODE_SIGNATURE_LEN_ERR		PK_LIC_ERRCODE_BASE + 3	// 签名字段长度不对，不是80
#define PK_LIC_ERRCODE_SIGNATURE_NOTMATCH		PK_LIC_ERRCODE_BASE + 4	// 签名不匹配
#define PK_LIC_ERRCODE_VERIFY_PROCESS_EXCEPT	PK_LIC_ERRCODE_BASE + 5	// 验证过程异常
#define PK_LIC_ERRCODE_LICFILE_NOTEXIST			PK_LIC_ERRCODE_BASE + 6	// 许可文件不存在
#define PK_LIC_ERRCODE_LICFILE_INVALIDPARAM		PK_LIC_ERRCODE_BASE + 7	// 传入参数非法
#define PK_LIC_ERRCODE_LACK_NODE				PK_LIC_ERRCODE_BASE + 8	// 缺少节点
#define PK_LIC_ERRCODE_HANDLE_ISNULL			PK_LIC_ERRCODE_BASE + 9	// 传入句柄为空
#define PK_LIC_ERRCODE_HOSTID_NOT_MATCH			PK_LIC_ERRCODE_BASE + 10	// 主机ID不匹配
#define PK_LIC_ERRCODE_LICFILE_OPENFAILED		PK_LIC_ERRCODE_BASE + 11	// 打开许可文件失败

typedef void *PKLIC_HANDLE;

/**
 *  打开许可证文件，并验证是否合法.
 *  @param[in] file [许可证文件的路径]
 *  @param[in] license_code [产品的代码]
 *  @param[out] lm_handle [返回的许可证句柄]
 *  @return
 *		- ==0 成功
 *		- !=0 出现异常
 */
int pkOpenLic(char *szLiencseFilePath, PKLIC_HANDLE *lm_handle, int *pnLicStatus, char *szErrTip, int nErrTipSize);

/**
 *  关闭许可证，释放许可证句柄资源.
 *  @param[in] lm_handle [打开的许可证句柄]
 *  @return
 *		- ==0 成功
 *		- !=0 出现异常
 */
int pkCloseLic(PKLIC_HANDLE lm_handle);

/**
 *  获取许可证信息项的值.
 *  @param[in] lm_handle [打开的许可证句柄]
 *  @param[in] key [信息项名称]
 *  @param[out] value [返回的信息项的值]
 *  @param[in] size [信息项值的缓冲区长度]
 *  @return
 *		- ==0 成功
 *		- !=0 出现异常
 */
int pkGetLicValue(PKLIC_HANDLE lm_handle, const char *key, char *value, int size);

#endif /* __LICENSE_H__ */
