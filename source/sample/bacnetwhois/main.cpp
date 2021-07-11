/**************************************************************************
*
* Copyright (C) 2006 Steve Karg <skarg@users.sourceforge.net>
*
* Permission is hereby granted, free of charge, to any person obtaining
* a copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*
*********************************************************************/

/* command line tool that sends a BACnet service, and displays the reply */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>       /* for time */
#include <errno.h>
#include "bactext.h"
#include "iam.h"
#include "address.h"
#include "config.h"
#include "bacdef.h"
#include "npdu.h"
#include "apdu.h"
#include "device.h"
#include "datalink.h"
#include "bactext.h"
/* some demo stuff needed */
#include "filename.h"
#include "handlers.h"
#include "client.h"
#include "txbuf.h"
#if defined(BACDL_MSTP)
#include "rs485.h"
#endif
#include "dlenv.h"
#include "net.h"

/* buffer used for receive */
static uint8_t Rx_Buf[MAX_MPDU] = { 0 };

/* global variables used in this file */
static int32_t Target_Object_Instance_Min = -1;
static int32_t Target_Object_Instance_Max = -1;
static bool Error_Detected = false;

#define BAC_ADDRESS_MULT 1


void print_macaddr(uint8_t * addr,int len);

void my_i_am_handler(
    uint8_t * service_request,
    uint16_t service_len,
    BACNET_ADDRESS * src)
{
    int len = 0;
    uint32_t device_id = 0;
    unsigned max_apdu = 0;
    int segmentation = 0;
    uint16_t vendor_id = 0;

    (void) service_len;
    len =
        iam_decode_service_request(service_request, &device_id, &max_apdu,
        &segmentation, &vendor_id);
    fprintf(stderr, "\t\nReceived I-Am Request");
    if (len != -1) {
        fprintf(stderr, " from %lu, MAC = %d.%d.%d.%d.%d.%d\n",
            (unsigned long) device_id, src->mac[0], src->mac[1], src->mac[2],
            src->mac[3], src->mac[4], src->mac[5]);
		printf(";%-7s  %-20s %-5s %-20s %-4s\n", "Device", "MAC (hex)", "SNET",
			"SADR (hex)", "APDU");
		printf(";-------- -------------------- ----- -------------------- ----\n");
		printf(" %-7u ", device_id);
		print_macaddr(src->mac, src->mac_len);
		printf(" %-5hu ", src->net);
		if (src->net) {
			print_macaddr(src->adr, src->len);
		} else {
			print_macaddr(0, 1);
		}
		printf(" %-4hu ", max_apdu);
		printf("\n\t\n");

        //address_table_add(device_id, max_apdu, src);
    } else {
#if PRINT_ENABLED
        fprintf(stderr, ", but unable to decode it.\n");
#endif
    }

    return;
}

void MyAbortHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t abort_reason,
    bool server)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    (void) server;
    fprintf(stderr, "BACnet Abort: %s\r\n",
        bactext_abort_reason_name(abort_reason));
    Error_Detected = true;
}

void MyRejectHandler(
    BACNET_ADDRESS * src,
    uint8_t invoke_id,
    uint8_t reject_reason)
{
    /* FIXME: verify src and invoke id */
    (void) src;
    (void) invoke_id;
    fprintf(stderr, "BACnet Reject: %s\r\n",
        bactext_reject_reason_name(reject_reason));
    Error_Detected = true;
}

static void init_service_handlers(
    void)
{
    Device_Init(NULL);
    /* Note: this applications doesn't need to handle who-is
       it is confusing for the user! */
    /* set the handler for all the services we don't implement
       It is required to send the proper reject message... */
    apdu_set_unrecognized_service_handler_handler
        (handler_unrecognized_service);
    /* we must implement read property - it's required! */
    apdu_set_confirmed_handler(SERVICE_CONFIRMED_READ_PROPERTY,
        handler_read_property);
    /* handle the reply (request) coming back */
    apdu_set_unconfirmed_handler(SERVICE_UNCONFIRMED_I_AM, my_i_am_handler);
    /* handle any errors coming back */
    apdu_set_abort_handler(MyAbortHandler);
    apdu_set_reject_handler(MyRejectHandler);
}

void print_macaddr(
    uint8_t * addr,
    int len)
{
    int j = 0;

    while (j < len) {
        if (j != 0) {
            printf(":");
        }
        printf("%02X", addr[j]);
        j++;
    }
    while (j < MAX_MAC_LEN) {
        printf("   ");
        j++;
    }
}

int main(
    int argc,
    char *argv[])
{
    BACNET_ADDRESS src = {
        0
    };  /* address where message came from */
    uint16_t pdu_len = 0;
    unsigned timeout = 100;     /* milliseconds */
    time_t total_seconds = 0;
    time_t elapsed_seconds = 0;
    time_t last_seconds = 0;
    time_t current_seconds = 0;
    time_t timeout_seconds = 0;
    BACNET_ADDRESS dest;
    int argi;
	if(argc==1){
		printf("Please Enter The Local IP For Search Device!");
		return 0;
	}else if(argc > 2){
		printf("Only Accept The One IP ToSearch Device,Like:192.168.1.10!");
		return 0;
	}
    datalink_get_broadcast_address(&dest);
    /* setup my info */
    Device_Set_Object_Instance_Number(BACNET_MAX_INSTANCE);
    init_service_handlers();
    address_init();
    dlenv_init(argv[1]);
    atexit(datalink_cleanup);
	//bip_get_my_address(&dest);
    /* configure the timeout values 20s³¬Ê±*/
    last_seconds = time(NULL);
    timeout_seconds = 5;
    /* send the request */
    Send_WhoIs_To_Network(&dest, Target_Object_Instance_Min, Target_Object_Instance_Max);
	//Send_WhoIs_Local(Target_Object_Instance_Min,Target_Object_Instance_Max);
    /* loop forever */
    for (;;) {
        /* increment timer - exit if timed out */
        current_seconds = time(NULL);
        /* returns 0 bytes on timeout */
        pdu_len = datalink_receive(&src, &Rx_Buf[0], MAX_MPDU, timeout,NULL);
        /* process */
        if (pdu_len) {
            npdu_handler(&src, &Rx_Buf[0], pdu_len,NULL,NULL);
        }
        if (Error_Detected)
            break;
        /* increment timer - exit if timed out */
        elapsed_seconds = current_seconds - last_seconds;
        if (elapsed_seconds) {
#if defined(BACDL_BIP) && BBMD_ENABLED
            bvlc_maintenance_timer(elapsed_seconds);
#endif
        }
        total_seconds += elapsed_seconds;
        if (total_seconds > timeout_seconds){
			
           break;/* */
		}
        /* keep track of time for next check */
        last_seconds = current_seconds;
    }
   // print_address_cache();

    return 0;
}
