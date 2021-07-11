cmake_minimum_required(VERSION 2.6)

PROJECT (pkmemdbapi_test)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} pkmemdbapi_test.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkmemdbapi_test)
SET(LINK_LIBS pklog pkmemdbapi pkcomm tinyxml pkce jsoncpp)

IF(UNIX)
    SET(LINK_LIBS ${LINK_LIBS} )
	IF(HPUX)
		SET(LINK_LIBS ${LINK_LIBS} pthread)
	ENDIF(HPUX)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonExec)
