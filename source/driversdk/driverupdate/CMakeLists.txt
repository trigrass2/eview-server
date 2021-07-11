cmake_minimum_required(VERSION 2.6)

PROJECT (driverupdate)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST})

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME driverupdate)
#Setting library type used when build a library

SET(LINK_LIBS pkce pklog pkcomm tinyxml pkserverbase shmqueue cppsqlite forwardaccessor)

IF(UNIX)
	IF(CMAKE_SYSTEM MATCHES "Linux")
		#SET(LINK_LIBS ${LINK_LIBS} boost_system boost_thread)
		if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
			#SET(LINK_LIBS ${LINK_LIBS} SentinelKeys32)
		endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	ENDIF(CMAKE_SYSTEM MATCHES "Linux")
	IF(HPUX)
		SET(LINK_LIBS ${LINK_LIBS} pthread)
	ENDIF(HPUX)
	IF(CMAKE_SYSTEM MATCHES "SunOS.*")
		SET(LINK_LIBS ${LINK_LIBS} socket)
	ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonExec)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )	
ENDIF(MSVC)
