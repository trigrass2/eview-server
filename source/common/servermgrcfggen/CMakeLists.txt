cmake_minimum_required(VERSION 2.6)

PROJECT (pkservermgr_eview_cfggen)

IF(CMAKE_SYSTEM MATCHES "AIX*")
	ADD_DEFINITIONS(-D__ABS_LONG__)
ENDIF(CMAKE_SYSTEM MATCHES "AIX*")

IF(UNIX) #隐藏所有的除了特别声明的符号
    IF(CMAKE_COMPILER_IS_GNUCC)
         SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(UNIX)

#Setting The Target Description Shown in the File Properties Dialog
SET(TARGET_DESCRIPTION "产生eview进程管理器所需要的配置文件")
SET(TARGET_VERSION 1.0.0)

#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkservermgr_eview_cfggen)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

SET(LINK_LIBS pkce pklog pkcomm pkdbapi pkeviewdbapi eviewcomm)
#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread util boost_thread boost_system boost_filesystem pkmemdbapi)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

SET(SPECOUTDIR /pkservermgr_plugin)
INCLUDE(../../CMakeCommonLib)
