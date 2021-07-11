cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (pksync2uppernodeserver)

IF(CMAKE_SYSTEM MATCHES "AIX*")
	ADD_DEFINITIONS(-D__ABS_LONG__)
ENDIF(CMAKE_SYSTEM MATCHES "AIX*")

#Setting The Target Description Shown in the File Properties Dialog
SET(TARGET_DESCRIPTION "同步到上级节点程序")
SET(TARGET_VERSION 1.0.0)

#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pksync2uppernodeserver)
#Setting library type used when build a library
#SET(LIB_TYPE STATIC)

SET(LINK_LIBS pkce pklog pkcomm rmapi pkserverbase jsoncpp redisproxy shmqueue mosquitto pkdbapi pkmemdbapi pkdata pkcrashdump)
if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	IF(CMAKE_SYSTEM MATCHES "Linux")
                SET(LINK_LIBS ${LINK_LIBS} pthread )
	ENDIF(CMAKE_SYSTEM MATCHES "Linux")
endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )

IF(UNIX)
	IF(HPUX)
        	SET(LINK_LIBS ${LINK_LIBS} pthread)
	ENDIF(HPUX)
	
	IF(CMAKE_SYSTEM MATCHES "SunOS.*")
		SET(LINK_LIBS ${LINK_LIBS} socket)
	ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
        SET(LINK_LIBS ${LINK_LIBS} pthread) #CentOS7.0 needs too!
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

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
