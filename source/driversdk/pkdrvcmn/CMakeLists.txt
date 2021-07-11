cmake_minimum_required(VERSION 2.6)

PROJECT (pkdrvcmn)

IF(UNIX) #隐藏所有的除了特别声明的符号
    IF(CMAKE_COMPILER_IS_GNUCC)
         SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(UNIX)

ADD_DEFINITIONS(-DDRV_SUPPORT_PYTHON)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR}/../../../include)
############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} dll_main.cpp MainTask.cpp)
SET(SRCS ${SRCS} UserTimer.cpp TaskGroup.cpp Driver.cpp Device.cpp)
SET(SRCS ${SRCS} drvframework.cpp )
SET(SRCS ${SRCS} pkDriverCommon.cpp)
SET(SRCS ${SRCS} pythonerr.cpp PythonHelper.cpp)
SET(SRCS ${SRCS} globalObj.cpp ByteOrder.cpp)
#SET(SRCS ${SRCS} TcpServerHandler.cpp TcpClientHandler.cpp UdpServerHandler.cpp SerialPort.cpp)
 
#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkdrvcmn)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

SET(LINK_LIBS ${LINK_LIBS} pkce tinyxml2 pkdrvdatasync rmapi pklog pkcomm eviewcomm jsoncpp pkmemdbapi shmqueue pkprocesscontroller pkcommunicate pkcrashdump)
SET(LINK_LIBS ${LINK_LIBS} pkeviewdbapi pkdbapi )

IF(UNIX)
    SET(LINK_LIBS ${LINK_LIBS} pthread)# boost_system)
    SET(LINK_LIBS ${LINK_LIBS} util dl m c python2.7) #Python must be included in LI
    #message("pytho2.7 lib include")
ELSE(UNIX)
	#SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp python27)
    SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonLib)
