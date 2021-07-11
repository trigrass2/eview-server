cmake_minimum_required(VERSION 2.6)

PROJECT(common)

#include(CheckCXXCompilerFlag)
#CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
#CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

SUBDIRS(eviewcomm pkiobuffer redundancy pkdata exprcalc pkeviewdbapi servermgrcfggen servermgrprocstatus2memdb pydata)

IF(CMAKE_SYSTEM MATCHES "Linux")
ENDIF()
