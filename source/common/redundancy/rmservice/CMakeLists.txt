cmake_minimum_required(VERSION 2.6)

PROJECT (rmservice)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} RMPeerCommunicator.cpp RMConfigLoader.cpp RMService.cpp RMStatusCtrl.cpp ShareMemHelper.cpp RMSvcMain.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME rmservice)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

SET(LINK_LIBS ACE pklog pkcomm cppsqlite tinyxml pkserverbase shmqueue hiredis)

IF(UNIX)
	if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
		IF(CMAKE_SYSTEM MATCHES "Linux")
			SET(LINK_LIBS ${LINK_LIBS} SentinelKeys32)
		ENDIF(CMAKE_SYSTEM MATCHES "Linux")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	SET(LINK_LIBS ${LINK_LIBS} rt)
	IF(CMAKE_SYSTEM MATCHES "SunOS.*")
		SET(LINK_LIBS ${LINK_LIBS}  socket)
	ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
	IF(HPUX)
		SET(LINK_LIBS ${LINK_LIBS} pthread iconv)
	ENDIF(HPUX)
ENDIF(UNIX)

#MESSAGE(${LINK_LIBS})

#SET(CMAKE_CFG_INTDIR "")

#Setting Executable and Library Output Path
#SET(EXECUTABLE_OUTPUT_PATH ../../../${EXE_DIR})
#SET(LIBRARY_OUTPUT_PATH ../../../${LIB_DIR})

#Setting Include Directorys
#INCLUDE_DIRECTORIES(../../../Include)

#Setting Link Directorys
#LINK_DIRECTORIES(../../../${LIB_DIR})
############FOR_MODIFIY_END#########################

INCLUDE(../../../CMakeCommonExec)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )	
ENDIF(MSVC)
	