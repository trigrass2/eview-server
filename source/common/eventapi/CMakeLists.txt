cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (eventapi)

#Setting Source Files
SET(SRCS ${SRCS} EventAPI.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME eventapi)
#Setting library type used when build a library
#STATIC/SHARED
SET(LIB_TYPE SHARED)

SET(LINK_LIBS ACE rmapi pklog pkcomm tinyxml)

IF(HPUX)
        SET(LINK_LIBS ${LINK_LIBS} pthread)
ENDIF(HPUX)

############FOR_MODIFIY_END#########################

INCLUDE(../../CMakeCommonLib)
