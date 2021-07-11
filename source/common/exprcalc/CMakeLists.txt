cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (exprcalc)

#Setting Source Files
SET(SRCS ${SRCS} lexer.cpp parser.cpp ExprCalc.cpp)
SET_SOURCE_FILES_PROPERTIES(${SRCS} COMPILE_FLAGS "-D_CRT_NONSTDC_NO_DEPRECATE")

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME exprcalc)
#Setting library type used when build a library
SET(LIB_TYPE STATIC)

############FOR_MODIFIY_END#########################
INCLUDE(../../CMakeCommonLib)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
ENDIF(MSVC)
