cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (pklinkact_sendsms)

#Setting Source Files, including .cpp,.c,.cxx
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pklinkact_sendsms)
#Setting library type used when build a library
#STATIC/SHARED
SET(LIB_TYPE SHARED)

SET(LINK_LIBS pkce rmapi pklog pkcomm tinyxml pkdata)
#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

IF(HPUX)
        SET(LINK_LIBS ${LINK_LIBS} pthread)
ENDIF(HPUX)

############FOR_MODIFIY_END#########################

INCLUDE(../../../CMakeCommonLib)
