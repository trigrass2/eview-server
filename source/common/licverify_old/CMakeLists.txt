cmake_minimum_required(VERSION 2.6)
############FOR_MODIFIY_BEGIN#######################
#Setting Project Name
PROJECT (licverify)

#Setting Source Files
SET(SRCS ${SRCS} LicChecker.cpp)

ADD_DEFINITIONS(-DLIC_VERIFY_NO_CV_LICENSE)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME licverify)
#Setting library type used when build a library
SET(LIB_TYPE STATIC)

############FOR_MODIFIY_END#########################
INCLUDE(../../CMakeCommonLib)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
ENDIF(MSVC)
