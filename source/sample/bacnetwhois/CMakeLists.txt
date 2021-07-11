cmake_minimum_required(VERSION 2.6)

PROJECT (bacnetwhois)

############FOR_MODIFIY_BEGIN#######################
#Setting Source Files
SET(SRCS ${SRCS} main.cpp ../../drivers/autogroup/AutoGroup_ObjDev.cpp
../../drivers/bacnetip/stack/abort.cpp ../../drivers/bacnetip/stack/address.cpp  ../../drivers/bacnetip/stack/alarm_ack.cpp
../../drivers/bacnetip/stack/apdu.cpp ../../drivers/bacnetip/stack/arf.cpp  ../../drivers/bacnetip/stack/awf.cpp
../../drivers/bacnetip/stack/bacaddr.cpp  ../../drivers/bacnetip/stack/bacapp.cpp  ../../drivers/bacnetip/stack/bacdcode.cpp
../../drivers/bacnetip/stack/bacdevobjpropref.cpp ../../drivers/bacnetip/stack/bacerror.cpp ../../drivers/bacnetip/stack/bacint.cpp
../../drivers/bacnetip/stack/bacprop.cpp ../../drivers/bacnetip/stack/bacpropstates.cpp ../../drivers/bacnetip/stack/bacreal.cpp
../../drivers/bacnetip/stack/bacstr.cpp ../../drivers/bacnetip/stack/bactext.cpp ../../drivers/bacnetip/stack/bigend.cpp
../../drivers/bacnetip/stack/bip.cpp ../../drivers/bacnetip/stack/bvlc.cpp ../../drivers/bacnetip/stack/cov.cpp
../../drivers/bacnetip/stack/crc.cpp ../../drivers/bacnetip/stack/datalink.cpp ../../drivers/bacnetip/stack/datetime.cpp
../../drivers/bacnetip/stack/dcc.cpp ../../drivers/bacnetip/stack/debug.cpp ../../drivers/bacnetip/stack/event.cpp
../../drivers/bacnetip/stack/fifo.cpp ../../drivers/bacnetip/stack/filename.cpp ../../drivers/bacnetip/stack/getevent.cpp
../../drivers/bacnetip/stack/get_alarm_sum.cpp ../../drivers/bacnetip/stack/iam.cpp ../../drivers/bacnetip/stack/ihave.cpp
../../drivers/bacnetip/stack/indtext.cpp ../../drivers/bacnetip/stack/key.cpp ../../drivers/bacnetip/stack/keylist.cpp
../../drivers/bacnetip/stack/lso.cpp ../../drivers/bacnetip/stack/memcopy.cpp
../../drivers/bacnetip/stack/mstp.cpp ../../drivers/bacnetip/stack/mstptext.cpp ../../drivers/bacnetip/stack/npdu.cpp
../../drivers/bacnetip/stack/proplist.cpp ../../drivers/bacnetip/stack/ptransfer.cpp ../../drivers/bacnetip/stack/rd.cpp
../../drivers/bacnetip/stack/readrange.cpp ../../drivers/bacnetip/stack/reject.cpp ../../drivers/bacnetip/stack/ringbuf.cpp
../../drivers/bacnetip/stack/rp.cpp ../../drivers/bacnetip/stack/rpm.cpp ../../drivers/bacnetip/stack/sbuf.cpp
../../drivers/bacnetip/stack/timestamp.cpp ../../drivers/bacnetip/stack/timesync.cpp ../../drivers/bacnetip/stack/tsm.cpp
../../drivers/bacnetip/stack/version.cpp ../../drivers/bacnetip/stack/whohas.cpp ../../drivers/bacnetip/stack/whois.cpp
../../drivers/bacnetip/stack/wp.cpp ../../drivers/bacnetip/stack/wpm.cpp 

../../drivers/bacnetip/object/ai.cpp ../../drivers/bacnetip/object/ao.cpp ../../drivers/bacnetip/object/av.cpp ../../drivers/bacnetip/object/bacfile.cpp ../../drivers/bacnetip/object/ ../../drivers/bacnetip/object/bi.cpp
../../drivers/bacnetip/object/bo.cpp ../../drivers/bacnetip/object/bv.cpp ../../drivers/bacnetip/object/csv.cpp ../../drivers/bacnetip/object/ ../../drivers/bacnetip/object/device.cpp ../../drivers/bacnetip/object/lc.cpp
../../drivers/bacnetip/object/lo.cpp ../../drivers/bacnetip/object/lsp.cpp ../../drivers/bacnetip/object/ ../../drivers/bacnetip/object/ms-input.cpp ../../drivers/bacnetip/object/mso.cpp ../../drivers/bacnetip/object/ ../../drivers/bacnetip/object/msv.cpp
../../drivers/bacnetip/object/trendlog.cpp


../../drivers/bacnetip/handler/dlenv.cpp             ../../drivers/bacnetip/handler/h_alarm_ack.cpp       ../../drivers/bacnetip/handler/h_arf.cpp
../../drivers/bacnetip/handler/h_arf_a.cpp           ../../drivers/bacnetip/handler/h_awf.cpp             ../../drivers/bacnetip/handler/h_ccov.cpp
../../drivers/bacnetip/handler/h_cov.cpp             ../../drivers/bacnetip/handler/h_dcc.cpp             ../../drivers/bacnetip/handler/h_gas_a.cpp
../../drivers/bacnetip/handler/h_getevent.cpp        ../../drivers/bacnetip/handler/h_get_alarm_sum.cpp   ../../drivers/bacnetip/handler/h_iam.cpp
../../drivers/bacnetip/handler/h_ihave.cpp           ../../drivers/bacnetip/handler/h_lso.cpp             ../../drivers/bacnetip/handler/h_npdu.cpp
../../drivers/bacnetip/handler/h_pt.cpp              ../../drivers/bacnetip/handler/h_pt_a.cpp            ../../drivers/bacnetip/handler/h_rd.cpp
../../drivers/bacnetip/handler/h_rp.cpp              ../../drivers/bacnetip/handler/h_rpm.cpp
../../drivers/bacnetip/handler/h_rpm_a.cpp           ../../drivers/bacnetip/handler/h_rp_a.cpp            ../../drivers/bacnetip/handler/h_rr.cpp
../../drivers/bacnetip/handler/h_rr_a.cpp            ../../drivers/bacnetip/handler/h_ts.cpp              ../../drivers/bacnetip/handler/h_ucov.cpp
../../drivers/bacnetip/handler/h_upt.cpp             ../../drivers/bacnetip/handler/h_whohas.cpp          ../../drivers/bacnetip/handler/h_whois.cpp
../../drivers/bacnetip/handler/h_wp.cpp              ../../drivers/bacnetip/handler/h_wpm.cpp             ../../drivers/bacnetip/handler/noserv.cpp
../../drivers/bacnetip/handler/objects.cpp           ../../drivers/bacnetip/handler/s_ack_alarm.cpp       ../../drivers/bacnetip/handler/s_arfs.cpp
../../drivers/bacnetip/handler/s_awfs.cpp            ../../drivers/bacnetip/handler/s_cevent.cpp          ../../drivers/bacnetip/handler/s_cov.cpp
../../drivers/bacnetip/handler/s_dcc.cpp             ../../drivers/bacnetip/handler/s_iam.cpp             ../../drivers/bacnetip/handler/s_ihave.cpp
../../drivers/bacnetip/handler/s_lso.cpp             ../../drivers/bacnetip/handler/s_ptransfer.cpp       ../../drivers/bacnetip/handler/s_rd.cpp
../../drivers/bacnetip/handler/s_readrange.cpp       ../../drivers/bacnetip/handler/s_router.cpp          ../../drivers/bacnetip/handler/s_rp.cpp
../../drivers/bacnetip/handler/s_rpm.cpp             ../../drivers/bacnetip/handler/s_ts.cpp              ../../drivers/bacnetip/handler/s_uevent.cpp
../../drivers/bacnetip/handler/s_upt.cpp             ../../drivers/bacnetip/handler/s_whohas.cpp          ../../drivers/bacnetip/handler/s_whois.cpp
../../drivers/bacnetip/handler/s_wp.cpp              ../../drivers/bacnetip/handler/s_wpm.cpp             ../../drivers/bacnetip/handler/txbuf.cpp
)

IF(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    SET(SRCS ${SRCS} ../../drivers/bacnetip/ports/win32/bip-init.cpp)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    SET(SRCS ${SRCS} ../../drivers/bacnetip/ports/linux/bip-init.cpp)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES Windows)

aux_source_directory(. bacnet-../../tools/bacnetwhois/src)

#Setting Target Name (executable file name | library name)
SET(TARGET_NAME bacnetwhois)
#Setting library type used when build a library
SET(LIB_TYPE STATIC)

IF(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    SET(LINK_LIBS pkdrvcmn ws2_32 iphlpapi.lib)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    SET(LINK_LIBS pkdrvcmn)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES Windows)

INCLUDE_DIRECTORIES(../../drivers/bacnetip/include)
IF(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    INCLUDE_DIRECTORIES(../..drivers/bacnetip/ports/win32)
ELSE(${CMAKE_SYSTEM_NAME} MATCHES Windows)
    INCLUDE_DIRECTORIES(../..drivers/bacnetip/ports/linux)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES Windows)

SET(SPECOUTDIR /)

INCLUDE(../../CMakeCommonExec)