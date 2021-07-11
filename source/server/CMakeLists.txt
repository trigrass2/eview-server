cmake_minimum_required(VERSION 2.6)
PROJECT(server)
SUBDIRS(pkmodserver link pknodeserver pkHostService pkmqttforwardserver pkdbtransferserver pkdbtoeviewserver pkuppernodeserver pksync2uppernodeserver)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	SUBDIRS(pkhisdataserver )
ELSE()

ENDIF()
