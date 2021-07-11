cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (pkiobuffer)

INCLUDE(../../CMakeCommon)

#Setting Source Files
SET(SRCS ${SRCS} pkInputBuffer.cpp pkOutputBuffer.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkiobuffer)
#Setting library type used when build a library
SET(LIB_TYPE STATIC)

#SET(LINK_LIBS pkce)
#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

############FOR_MODIFIY_END#########################

INCLUDE(../../CMakeCommonLib)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
ENDIF(MSVC)
