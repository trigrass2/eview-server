cmake_minimum_required(VERSION 2.6)

PROJECT (cpp_redis_test)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} cpp_redis_test.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME cpp_redis_test)
SET(LINK_LIBS pklog cpp_redis)

IF(UNIX)
    SET(LINK_LIBS ${LINK_LIBS} )
	IF(HPUX)
		SET(LINK_LIBS ${LINK_LIBS} pthread)
	ENDIF(HPUX)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonExec)
