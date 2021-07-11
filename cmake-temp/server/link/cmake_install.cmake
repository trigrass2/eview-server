# Install script for directory: D:/work_xing/project/eview-server/source/server/link

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files/eview-server")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/work_xing/project/eview-server/cmake-temp/server/link/pklinkserver/cmake_install.cmake")
  include("D:/work_xing/project/eview-server/cmake-temp/server/link/pklinkact_poppage/cmake_install.cmake")
  include("D:/work_xing/project/eview-server/cmake-temp/server/link/pklinkact_writetag_single/cmake_install.cmake")
  include("D:/work_xing/project/eview-server/cmake-temp/server/link/pklinkact_writetag_inturn/cmake_install.cmake")
  include("D:/work_xing/project/eview-server/cmake-temp/server/link/pklinkact_writetag_multiple/cmake_install.cmake")
  include("D:/work_xing/project/eview-server/cmake-temp/server/link/pklinkevt_alarm/cmake_install.cmake")
  include("D:/work_xing/project/eview-server/cmake-temp/server/link/pkpyalarm/cmake_install.cmake")

endif()

