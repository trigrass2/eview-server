cmake_minimum_required(VERSION 2.6)
PROJECT (rmserviceut)

SET(SRCS ${SRCS} ../RMPeerCommunicator.cpp 
					../RMConfigLoader.cpp 
					../RMStatusCtrl.cpp 
					../ShareMemHelper.cpp)
SET(SRCS ${SRCS} RMStatusCtrlTester.cpp RMUDPCommTester.cpp ShareMemHelperTester.cpp)

SET_SOURCE_FILES_PROPERTIES(${SRCS} COMPILE_FLAGS -DCPPUNIT_FRIEND_CLASS)

SET(TARGET_NAME RMServiceUT)

SET(LINK_LIBS ACE pklog pkcomm tinyxml cppunit cvcpput)

IF(UNIX)
	IF(CMAKE_SYSTEM MATCHES "SunOS.*")
		SET(LINK_LIBS ${LINK_LIBS}  socket)
	ENDIF(CMAKE_SYSTEM MATCHES "SunOS.*")
ENDIF(UNIX)


SET(SPECOUTDIR /unittest)

INCLUDE(../../../../CMakeCommonExec)

