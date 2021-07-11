cmake_minimum_required(VERSION 2.6)
#Setting Project Name
PROJECT (pkeviewdbapi)

SET(TARGET_DESCRIPTION "eview数据库访问接口")
SET(TARGET_VERSION 1.0.0)

#Setting Source Files, including .cpp,.c,.cxx
AUX_SOURCE_DIRECTORY(. SRC_LIST)
SET(SRCS ${SRCS} ${SRC_LIST} )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkeviewdbapi)
#Setting library type used when build a library
SET(LIB_TYPE STATIC)

INCLUDE(../../CMakeCommonLib)

IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
ENDIF(MSVC)