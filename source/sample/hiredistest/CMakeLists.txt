cmake_minimum_required(VERSION 2.6)

PROJECT (hiredistest)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} RedisTestor.cpp Task1.cpp Task2.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME hiredistest)
#Setting library type used when build a library
#SET(LIB_TYPE STATIC)
SET(LINK_LIBS pkce pkserverbase pklog pkcomm tinyxml redisproxy shmqueue)
IF(UNIX)
    SET(LINK_LIBS ${LINK_LIBS} )
	IF(HPUX)
		SET(LINK_LIBS ${LINK_LIBS} pthread)
	ENDIF(HPUX)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonExec)
