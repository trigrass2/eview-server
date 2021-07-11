cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (PKSockClientServer)

IF(CMAKE_SYSTEM MATCHES "AIX*")
	ADD_DEFINITIONS(-D__ABS_LONG__)
ENDIF(CMAKE_SYSTEM MATCHES "AIX*")

#Setting The Target Description Shown in the File Properties Dialog
SET(TARGET_DESCRIPTION "SockClient SockServer≤‚ ‘")
SET(TARGET_VERSION 1.0.0)

#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME PKSockClientServer)

SET(LINK_LIBS pkce pksockclient pksockserver pkserverbase pkcomm pklog)

IF(CMAKE_SYSTEM MATCHES "Linux")
	SET(LINK_LIBS ${LINK_LIBS} rt pthread)
ENDIF()


IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread)
	
	IF(CMAKE_SYSTEM MATCHES "SunOS.*")
                SET(LINK_LIBS ${LINK_LIBS} socket)
	ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
	
ELSE()
        SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF()

INCLUDE(../../CMakeCommonExec)

IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )	
ENDIF()
