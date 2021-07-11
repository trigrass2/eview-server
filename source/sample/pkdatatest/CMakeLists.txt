cmake_minimum_required(VERSION 2.6)

PROJECT (pkdatatest)

INCLUDE(../../CMakeCommon)
############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} main.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkdatatest)

SET(LINK_LIBS pkdata)
IF(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} pthread)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)


INCLUDE(../../CMakeCommonExec)
