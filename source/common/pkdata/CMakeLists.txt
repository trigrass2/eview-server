cmake_minimum_required(VERSION 2.6)

PROJECT (pkdata)

IF(UNIX) #隐藏所有的除了特别声明的符号
    IF(CMAKE_COMPILER_IS_GNUCC)
         SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(UNIX)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} pkdata.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkdata)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

SET(LINK_LIBS pkce pklog pkcomm jsoncpp pkmemdbapi pkcrashdump)
#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

IF(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} pthread util)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonLib)