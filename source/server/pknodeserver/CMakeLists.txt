cmake_minimum_required(VERSION 2.6)

PROJECT (pknodeserver)

INCLUDE_DIRECTORIES(../../common/exprcalc/include)
############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
#SET(SRCS ${SRCS} PKNodeServer.cpp MainTask.cpp CtrlProcTask.cpp DataProcTask.cpp HisDataTask.cpp TagExprCalc.cpp VarCalculator.cpp EAProcessor.cpp ByteOrder.cpp NodeCommonDef.cpp)
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pknodeserver)

#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)
SET(LINK_LIBS pkce tinyxml2 pkserverbase pkdrvdatasync pklog pkcomm eviewcomm rmapi jsoncpp shmqueue exprcalc pkmemdbapi pkeviewdbapi pklic pkdbapi cryptlib pkcrashdump)# )

IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread util dl boost_system boost_thread boost_regex)
ELSE(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} psapi exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonExec)
