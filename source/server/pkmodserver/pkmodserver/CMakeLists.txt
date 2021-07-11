cmake_minimum_required(VERSION 2.6)

PROJECT (pkmodserver)

ADD_DEFINITIONS(-DDRV_SUPPORT_PYTHON)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR}/../../../../include/python2.7)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} ForwardService.cpp MainTask.cpp ClientSockHandler.cpp SystemConfig.cpp  SerialPort.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkmodserver)
#Setting library type used when build a library

#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)
SET(LINK_LIBS pkce pklog pkcomm eviewcomm pkmodbustcplib pkserverbase shmqueue pkdbapi pkeviewdbapi pklic cryptlib pkcrashdump)

IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread)
	IF(CMAKE_SYSTEM MATCHES "SunOS.*")
		SET(LINK_LIBS ${LINK_LIBS} socket)
	ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
ELSE(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../../CMakeCommonExec)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )	
ENDIF(MSVC)
