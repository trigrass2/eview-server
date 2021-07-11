cmake_minimum_required(VERSION 2.6)
PROJECT (forwardaccessor)

#Setting Source Files
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} ForwardAccessor.def)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME forwardaccessor)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

SET(LINK_LIBS ACE pklog pkcomm tinyxml)
IF(HPUX)
	SET(LINK_LIBS ${LINK_LIBS} pthread)
ENDIF(HPUX)

INCLUDE(../../../CMakeCommonLib)
