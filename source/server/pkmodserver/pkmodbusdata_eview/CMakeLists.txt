cmake_minimum_required(VERSION 2.6)
PROJECT (pkmodbusdata_eview)

#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkmodbusdata)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)
SET(LINK_LIBS pkce pklog pkcomm pkdata jsoncpp)


IF(HPUX)
	SET(LINK_LIBS ${LINK_LIBS} pthread)
ENDIF(HPUX)

INCLUDE(../../../CMakeCommonLib)
