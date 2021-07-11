cmake_minimum_required(VERSION 2.6)

PROJECT (pkeviewdbapitest)

INCLUDE(../../CMakeCommon)
############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} main.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkeviewdbapitest)

SET(LINK_LIBS pkeviewdbapi pkcomm pklog pkce)
IF(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} pthread)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)


INCLUDE(../../CMakeCommonExec)
