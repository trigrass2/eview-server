cmake_minimum_required(VERSION 2.6)

PROJECT (pydriver)

ADD_DEFINITIONS(-DDRV_SUPPORT_PYTHON)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_LIST_DIR}/../../../include/python2.7)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} DrvHelperPy.cpp)
#SET(SRCS ${SRCS} ../pkdrvcmn/UserTimer.cpp ../pkdrvcmn/Device.cpp pythonerr.cpp )

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pydriver)
#Setting library type used when build a library
SET(LIB_TYPE SHARED)

#set_target_properties(${target} PROPERTIES SUFFIX "byd")
#set_target_properties(${target} PROPERTIES PREFIX "")
#set_target_properties(${target} PROPERTIES OUTPUT_NAME "xxxxxx")

#set_target_properties(${target}
#        PROPERTIES
#        OUTPUT_NAME xxxxxx         # 重名命生成的库名称libxmath.lib
#        SOVERSION ${major_version}      # 设置函数API版本
#        VERSION ${version}              # 设置动态库版本
#        )

#set(CMAKE_DEBUG_POSTFIX "pyd") no use
#set(CMAKE_RELEASE_POSTFIX "pyd") 

SET(LINK_LIBS pkce pkcomm pkdrvcmn pklog)

IF(UNIX)
        SET(LINK_LIBS ${LINK_LIBS} pthread  util dl m c  pkmemdbapi)#boost_system python2.7
ELSE(UNIX)
	SET(LINK_LIBS ${LINK_LIBS} exceptionreport dbghelp)
ENDIF(UNIX)

INCLUDE(../../CMakeCommonLib)
