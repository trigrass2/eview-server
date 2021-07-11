cmake_minimum_required(VERSION 2.6)

PROJECT (procstatus2memdb)

IF(CMAKE_SYSTEM MATCHES "AIX*")
	ADD_DEFINITIONS(-D__ABS_LONG__)
ENDIF(CMAKE_SYSTEM MATCHES "AIX*")

IF(UNIX) #隐藏所有的除了特别声明的符号
    IF(CMAKE_COMPILER_IS_GNUCC)
         SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(UNIX)

#Setting The Target Description Shown in the File Properties Dialog
SET(TARGET_DESCRIPTION "进程状态转存到内存数据库")
SET(TARGET_VERSION 1.0.0)

#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME procstatus2memdb)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

SET(LINK_LIBS pkce pklog pkcomm jsoncpp pkmemdbapi)
#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

IF(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} pthread util)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

SET(SPECOUTDIR /pkservermgr_plugin)
INCLUDE(../../CMakeCommonLib)