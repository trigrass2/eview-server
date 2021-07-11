cmake_minimum_required(VERSION 2.6)

PROJECT (pkmodbustcplib)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} functions/mbfunccoils.c functions/mbfuncdiag.c functions/mbfuncdisc.c functions/mbfuncholding.c functions/mbfuncinput.c functions/mbfuncother.c functions/mbutils.c tcp/mbtcp.c)
    
#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkmodbustcplib)
#Setting library type used when build a library
SET(LIB_TYPE STATIC)

SET(LINK_LIBS )

IF(UNIX)
	IF(HPUX)
		SET(LINK_LIBS ${LINK_LIBS} pthread)
	ENDIF(HPUX)
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../../CMakeCommonLib)
IF(MSVC)
	if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
		set_target_properties(${TARGET_NAME} PROPERTIES STATIC_LIBRARY_FLAGS "/machine:x64")
	endif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
ENDIF(MSVC)
