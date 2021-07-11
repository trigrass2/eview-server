cmake_minimum_required(VERSION 2.6)

PROJECT (pkdrvdatasync)

INCLUDE_DIRECTORIES(../iodatasync/include)
############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} IODataSync_Imp.cpp IODataProcessor.cpp)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME pkdrvdatasync)
#Setting library type used when build a library
SET(LIB_TYPE STATIC)

#SET(LINK_LIBS pkce netqueue pklog pkcomm tinyxml)
#ADD_DEFINITIONS(-DACE_AS_STATIC_LIBS)

############FOR_MODIFIY_END#########################
INCLUDE(../../../CMakeCommonLib)
