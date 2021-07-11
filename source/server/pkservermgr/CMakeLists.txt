cmake_minimum_required(VERSION 2.6)

PROJECT (pkservermgr)

INCLUDE_DIRECTORIES(../../../include/cryptopp)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} pkProcess.cpp ProcessMngr.cpp main.cpp ProcPluginLoad.cpp MainTask.cpp main_NTService.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkservermgr)
#Setting library type used when build a library
#SET(LIB_TYPE STATIC)
#SET(LIB_TYPE SHARED)

#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)
#ADD_DEFINITIONS(-DCRYPTOPP_DLL_ONLY)
IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread dl boost_thread boost_system)
ELSE(UNIX)
    SET(SRCS ${SRCS} EnumProc.cpp)
    SET(LINK_LIBS ${LINK_LIBS} psapi exceptionreport dbghelp)
ENDIF(UNIX)
SET(LINK_LIBS  ${LINK_LIBS} pklog pkcomm shmqueue tinyxml rmapi jsoncpp pkce  pkmemdbapi pkdbapi pkeviewdbapi pklic cryptlib pkprocesscontroller)

INCLUDE(../../CMakeCommonExec)
