cmake_minimum_required(VERSION 2.6)

PROJECT (pkuppernodeserver)

INCLUDE_DIRECTORIES(../../common/exprcalc/include)
############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkuppernodeserver)

#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)
SET(LINK_LIBS pkce pkserverbase pkdrvdatasync pklog pkcomm eviewcomm rmapi jsoncpp shmqueue pkmemdbapi pklic pkdbapi cryptlib mosquitto pkcrashdump)

IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread util dl boost_system boost_thread boost_regex)
ELSE(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} psapi exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonExec)
