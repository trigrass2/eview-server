
#ifndef _PORT_H
#define _PORT_H

#include <assert.h>

#define	INLINE
#define PR_BEGIN_EXTERN_C			extern "C" {
#define	PR_END_EXTERN_C				}

#ifdef __cplusplus
PR_BEGIN_EXTERN_C
#endif
/* ----------------------- Defines ------------------------------------------*/

#define ENTER_CRITICAL_SECTION( )
#define EXIT_CRITICAL_SECTION( )
#define MB_PORT_HAS_CLOSE	1
#ifndef TRUE
#define TRUE            1
#endif
#ifndef FALSE
#define FALSE           0
#endif
/* ----------------------- Type definitions ---------------------------------*/
//typedef int     SOCKET;

#define SOCKET_ERROR (-1)
//#define INVALID_SOCKET (~0)
typedef int    BOOL;
typedef unsigned char UCHAR;
typedef unsigned char BYTE;
typedef char    CHAR;
typedef unsigned short USHORT;
typedef short   SHORT;

typedef unsigned long ULONG;
typedef long    LONG;
typedef enum
{
	MB_LOG_DEBUG,
		MB_LOG_INFO,
		MB_LOG_WARN,
		MB_LOG_ERROR
} eMBPortLogLevel;

/* ----------------------- Function prototypes ------------------------------*/

void            TcpvMBPortLog( eMBPortLogLevel eLevel, const CHAR * szModule, const CHAR * szFmt,
							  ... );

#ifdef __cplusplus
PR_END_EXTERN_C
#endif

#endif	//_PORT_H
