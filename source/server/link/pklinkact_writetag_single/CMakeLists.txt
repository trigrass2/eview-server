cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (pklinkact_writetag_single)

IF(UNIX) #隐藏所有的除了特别声明的符号
    IF(CMAKE_COMPILER_IS_GNUCC)
         SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    ENDIF(CMAKE_COMPILER_IS_GNUCC)
ENDIF(UNIX)

#Setting Source Files, including .cpp,.c,.cxx
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pklinkact_writetag_single)
#Setting library type used when build a library
#STATIC/SHARED
SET(LIB_TYPE SHARED)

SET(LINK_LIBS pkce rmapi pklog pkcomm pkdata)
#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

IF(HPUX)
        SET(LINK_LIBS ${LINK_LIBS} pthread)
ENDIF(HPUX)

############FOR_MODIFIY_END#########################

INCLUDE(../../../CMakeCommonLib)
