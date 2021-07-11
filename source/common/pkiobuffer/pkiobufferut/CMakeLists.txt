cmake_minimum_required(VERSION 2.6)
PROJECT (pkiobufferut)

SET(SRCS TestCVComBufferUT.cpp TestCVInputBuffer.cpp CVComBuffer.cpp)

SET_SOURCE_FILES_PROPERTIES(${SRCS} COMPILE_FLAGS -DCPPUNIT_FRIEND_CLASS)

SET(TARGET_NAME pkiobufferut)

SET(LIB_TYPE STATIC)

SET(LINK_LIBS ace cppunit pkcpput pkiobuffer)

SET(SPECOUTDIR /unittest)
INCLUDE(../../../CMakeCommonExec)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
ENDIF(MSVC)
