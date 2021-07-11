cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (serialwrite)

IF(CMAKE_SYSTEM MATCHES "AIX*")
	ADD_DEFINITIONS(-D__ABS_LONG__)
ENDIF(CMAKE_SYSTEM MATCHES "AIX*")

#Setting The Target Description Shown in the File Properties Dialog
SET(TARGET_DESCRIPTION "serialreader")
SET(TARGET_VERSION 1.0.0)

#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )
#SET(SRCS ${SRCS} ClientManageTask.cpp ConfigLoader.cpp CRService.cpp MainTasksManager.cpp NetWrapper.cpp TimerTask.cpp QueryHandleTask.cpp ReqFetchTask.cpp ExecutionTask.cpp TriggerTask.cpp ActionTask.cpp ActionTaskManager.cpp TriggerTaskManager.cpp CRSEvent.cpp ShrdMemoryReadTask.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME serialwrite)
#Setting library type used when build a library
#SET(LIB_TYPE STATIC)

SET(LINK_LIBS pkce)
#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
	IF(CMAKE_SYSTEM MATCHES "Linux")
                SET(LINK_LIBS ${LINK_LIBS} )
	ENDIF(CMAKE_SYSTEM MATCHES "Linux")
endif( CMAKE_SIZEOF_VOID_P EQUAL 4 )

IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread)

	IF(CMAKE_SYSTEM MATCHES "SunOS.*")
		SET(LINK_LIBS ${LINK_LIBS} socket)
	ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
	
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

#Setting Executable and Library Output Path

#Setting Include Directorys

#Setting Link Directorys
############FOR_MODIFIY_END#########################
INCLUDE(../../../CMakeCommonExec)

IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )	
ENDIF(MSVC)
