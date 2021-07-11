cmake_minimum_required(VERSION 2.6)

############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (pkdbapi)

SET(TARGET_DESCRIPTION "凌峰数据库访问接口")
SET(TARGET_VERSION 5.0.0)

#Setting Source Files, including .cpp,.c,.cxx
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkdbapi)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

SET(LINK_LIBS sqlapi pkce pklog pkcomm)
IF(MSVC)
SET(LINK_LIBS ${LINK_LIBS} version)
ENDIF()

#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

INCLUDE(../../CMakeCommonLib)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
ENDIF(MSVC)
