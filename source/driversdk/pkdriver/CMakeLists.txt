cmake_minimum_required(VERSION 2.6)

PROJECT (pkdriver)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} main.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkdriver)
#Setting library type used when build a library
#SET(LIB_TYPE STATIC)
SET(LINK_LIBS)

IF(WIN32)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")
    SET(LINK_LIBS ${LINK_LIBS} )
ELSE()
    SET(LINK_LIBS ${LINK_LIBS} pkce pthread pkdrvcmn)
    ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)
    IF(HPUX)
            SET(LINK_LIBS ${LINK_LIBS} pthread)
    ENDIF(HPUX)

ENDIF()


#set(CompilerFlags
#        CMAKE_CXX_FLAGS
#        CMAKE_CXX_FLAGS_DEBUG
#        CMAKE_CXX_FLAGS_RELEASE
#        CMAKE_C_FLAGS
#        CMAKE_C_FLAGS_DEBUG
#        CMAKE_C_FLAGS_RELEASE
#        )
#foreach(CompilerFlag ${CompilerFlags})
#  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
#endforeach()

#SET(SPECOUTDIR /drivers/modbustcp)
INCLUDE(../../CMakeCommonExec)

#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MT")

