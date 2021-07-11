cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (PKSockClientTest)

IF(CMAKE_SYSTEM MATCHES "AIX*")
	ADD_DEFINITIONS(-D__ABS_LONG__)
ENDIF(CMAKE_SYSTEM MATCHES "AIX*")

#Setting The Target Description Shown in the File Properties Dialog
SET(TARGET_DESCRIPTION "SockClient≤‚ ‘")
SET(TARGET_VERSION 1.0.0)

#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME PKSockClientTest)

SET(LINK_LIBS pkce pksockclient)

IF(CMAKE_SYSTEM MATCHES "Linux")
	SET(LINK_LIBS ${LINK_LIBS} rt pthread)
ENDIF(CMAKE_SYSTEM MATCHES "Linux")


IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread)
	
	IF(CMAKE_SYSTEM MATCHES "SunOS.*")
                SET(LINK_LIBS ${LINK_LIBS} socket)
	ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
	
ELSE()
        SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF()

#Setting Executable and Library Output Path

#Setting Include Directorys

#Setting Link Directorys
############FOR_MODIFIY_END#########################
INCLUDE(../../CMakeCommonExec)

IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )	
ENDIF(MSVC)
