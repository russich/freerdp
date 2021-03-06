/*
   FreeRDP: A Remote Desktop Protocol client.
   RAIL(TS RemoteApp) Virtual Channel Unit Tests

   Copyright 2011 Vic Lee
   Copyright 2011 Roman Barabanov <romanbarabanov@gmail.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/channels/channels.h>
#include <freerdp/utils/event.h>
#include <freerdp/utils/hexdump.h>
#include <freerdp/utils/memory.h>
#include <freerdp/utils/rail.h>
#include <freerdp/rail.h>


#include "test_rail.h"

#define HCF_HIGHCONTRASTON  0x00000001
#define HCF_AVAILABLE       0x00000002
#define HCF_HOTKEYACTIVE    0x00000004
#define HCF_CONFIRMHOTKEY   0x00000008
#define HCF_HOTKEYSOUND     0x00000010
#define HCF_INDICATOR       0x00000020
#define HCF_HOTKEYAVAILABLE 0x00000040


int init_rail_suite(void)
{
	freerdp_channels_global_init();
	return 0;
}

int clean_rail_suite(void)
{
	freerdp_channels_global_uninit();
	return 0;
}

int add_rail_suite(void)
{
	add_test_suite(rail);

	add_test_function(rail_plugin);

	return 0;
}


static uint8 client_handshake[] =
{
0x05, 0x00, 0x08, 0x00, 0xb0, 0x1d, 0x00, 0x00
};

static uint8 client_info_pdu[] =
{
0x0b, 0x00, 0x08, 0x00, 0x01, 0x00, 0x00, 0x00
};

// Flags: TS_RAIL_EXEC_FLAG_EXPAND_ARGUMENTS
// ExeOrFile : ||iexplore
// WorkingDir: f:\windows\system32
// Arguments: www.bing.com

static uint8 client_execute_pdu[] =
{
0x01,0x00,0x5e,0x00,0x08,0x00,0x14,0x00,0x26,0x00,0x18,0x00,0x7c,0x00,
0x7c,0x00,0x69,0x00,0x65,0x00,0x78,0x00,0x70,0x00,0x6c,0x00,0x6f,0x00,
0x72,0x00,0x65,0x00,0x66,0x00,0x3a,0x00,0x5c,0x00,0x77,0x00,0x69,0x00,
0x6e,0x00,0x64,0x00,0x6f,0x00,0x77,0x00,0x73,0x00,0x5c,0x00,0x73,0x00,
0x79,0x00,0x73,0x00,0x74,0x00,0x65,0x00,0x6d,0x00,0x33,0x00,0x32,0x00,
0x77,0x00,0x77,0x00,0x77,0x00,0x2e,0x00,0x62,0x00,0x69,0x00,0x6e,0x00,
0x67,0x00,0x2e,0x00,0x63,0x00,0x6f,0x00,0x6d,0x00
};

static uint8 client_activate_pdu[] =
{
0x02,0x00,
0x09,0x00,
0x8e,0x00,0x07,0x00,
0x01
};



static uint8 client_sysparam_highcontrast_pdu[] =
{
0x03,0x00,
0x12,0x00,
0x43,0x00,0x00,0x00, // SPI_SETHIGHCONTRAST
0x7e,0x00,0x00,0x00, // HCF_AVAILABLE | HCF_HOTKEYACTIVE | HCF_CONFIRMHOTKEY
                     // HCF_HOTKEYSOUND | HCF_INDICATOR | HCF_HOTKEYAVAILABLE
0x02,0x00,0x00,0x00, // Minimum length 2
0x00,0x00 // Unicode String
};


static uint8 client_sysparam_taskbarpos_pdu[] =
{
0x03,0x00,
0x10,0x00,
0x00,0xf0,0x00,0x00, // RAIL_SPI_TASKBARPOS
0x00,0x00, // 0
0x9a,0x03, // 0x039a
0x90,0x06, // 0x0690
0xc2,0x03  // 0x03c2
};

static uint8 client_sysparam_mousebuttonswap_pdu[] =
{
0x03,0x00,
0x09,0x00,
0x21,0x00,0x00,0x00, // SPI_SETMOUSEBUTTONSWAP
0x00 // false
};


static uint8 client_sysparam_keyboardpref_pdu[] =
{
0x03,0x00,
0x09,0x00,
0x45,0x00,0x00,0x00, // SPI_SETKEYBOARDPREF
0x00 // false
};


static uint8 client_sysparam_dragfullwindow_pdu[] =
{
0x03,0x00,
0x09,0x00,
0x25,0x00,0x00,0x00, // SPI_SETDRAGFULLWINDOWS
0x01 // true
};


static uint8 client_sysparam_keyboardcues_pdu[] =
{
0x03,0x00,
0x09,0x00,
0x0b,0x10,0x00,0x00, //SPI_SETKEYBOARDCUES
0x00 // false
};

static uint8 client_sysparam_setworkarea_pdu[] =
{
0x03,0x00,
0x10,0x00,
0x2f,0x00,0x00,0x00, //SPI_SETWORKAREA
0x00,0x00, // 0
0x00,0x00, // 0
0x90,0x06, // 0x0690
0x9a,0x03  // 0x039a
};

static uint8 client_syscommand_pdu[] =
{
0x04,0x00,
0x0a,0x00,
0x52,0x00,0x02,0x00,
0x20,0xf0
};

static uint8 client_notify_pdu[] =
{
0x06,0x00,
0x10,0x00,
0xaa,0x01,0x02,0x00,
0x02,0x00,0x00,0x00,
0x04,0x02,0x00,0x00
};

static uint8 client_windowmove_pdu[] =
{
0x08,0x00,
0x10,0x00,
0x20,0x00,0x02,0x00,
0x09,0x03,
0x00,0x01,
0xdb,0x05,
0x88,0x01
};

static uint8 client_system_menu_pdu[] =
{
0x0c,0x00,
0x0c,0x00,
0x22,0x01,0x09,0x00,
0xa4,0xff,
0x4a,0x02
};

static uint8 client_langbar_pdu[] =
{
0x0D,0x00,0x08,0x00,0x01,0x00,0x00,0x00
};

static uint8 client_get_app_id_req_pdu[] =
{
0x0E,0x00,0x08,0x00,0x52,0x00,0x02,0x00
};

static uint8 server_handshake[] =
{
	0x05, 0x00, 0x08, 0x00, 0xb0, 0x1d, 0x00, 0x00
};

static uint8 server_exec_result_pdu[] =
{
0x80,0x00,0x24,0x00,0x08,0x00,0x03,0x00,0x15,0x00,0x00,0x00,0x00,0x00,
0x14,0x00,0x7c,0x00,0x7c,0x00,0x57,0x00,0x72,0x00,0x6f,0x00,0x6e,0x00,
0x67,0x00,0x41,0x00,0x70,0x00,0x70,0x00
};

static uint8 server_exec_result_exe_or_file[] =
{
0x7c,0x00,0x7c,0x00,0x57,0x00,0x72,0x00,0x6f,0x00,0x6e,0x00,
0x67,0x00,0x41,0x00,0x70,0x00,0x70,0x00
};

static uint8 server_sysparam1_pdu[] =
{
0x03,0x00,
0x09,0x00,
0x77,0x00,0x00,0x00,
0x00
};

static uint8 server_sysparam2_pdu[] =
{
0x03,0x00,
0x09,0x00,
0x11,0x00,0x00,0x00,
0x00
};

static uint8 server_localmovesize_start_pdu[] =
{
0x09,0x00,0x10,0x00,0x8e,0x00,0x07,0x00,0x01,0x00,0x09,0x00,0x7e,0x01,
0x0a,0x00
};

static uint8 server_localmovesize_stop_pdu[] =
{
0x09,0x00,0x10,0x00,0x8e,0x00,0x07,0x00,0x00,0x00,0x09,0x00,0xa6,0x00,
0x44,0x00
};

static uint8 server_minmaxinfo_pdu[] =
{
0x0a,0x00,0x18,0x00,0x8e,0x00,0x07,0x00,0x08,0x04,0xd6,0x02,0x00,0x00,
0x00,0x00,0x70,0x00,0x1b,0x00,0x0c,0x04,0x0c,0x03
};

static uint8 server_langbar_pdu[] =
{
0x0D,0x00,0x08,0x00,0x01,0x00,0x00,0x00
};


static uint8 server_app_get_resp_pdu[] =
{
0x0F,0x00,0x08,0x02,0x52,0x00,0x02,0x00,0x6d,0x00,0x69,0x00,0x63,0x00,
0x72,0x00,0x6f,0x00,0x73,0x00,0x6f,0x00,0x66,0x00,0x74,0x00,0x2e,0x00,
0x77,0x00,0x69,0x00,0x6e,0x6f,0x00,0x77,0x00,0x73,0x00,0x2e,0x00,0x6e,
0x00,0x6f,0x00,0x74,0x00,0x65,0x00,0x70,0x00,0x61,0x00,0x64,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00
};

static uint8 server_app_get_resp_app_id[] =
{
0x6d,0x00,0x69,0x00,0x63,0x00,0x72,0x00,0x6f,0x00,0x73,0x00,0x6f,0x00,
0x66,0x00,0x74,0x00,0x2e,0x00,0x77,0x00,0x69,0x00,0x6e,0x6f,0x00,0x77,
0x00,0x73,0x00,0x2e,0x00,0x6e,0x00,0x6f,0x00,0x74,0x00,0x65,0x00,0x70,
0x00,0x61,0x00,0x64,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,

0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00
};


#define EMULATE_SERVER_SEND_CHANNEL_DATA(inst, byte_array) \
	emulate_server_send_channel_data(inst, byte_array, RAIL_ARRAY_SIZE(byte_array))

#define STREAM_EQUAL_TO_DUMP(stream, dump) \
	(stream_equal_dump((stream)->data, (stream)->size, dump, RAIL_ARRAY_SIZE(dump)))

#define UNICODE_STRING_EQUAL_TO_DUMP(ustring, dump) \
	(stream_equal_dump((ustring)->string, (ustring)->length, dump, RAIL_ARRAY_SIZE(dump)))

typedef struct
{
	RAIL_HANDSHAKE_ORDER handshake;
	RAIL_CLIENT_STATUS_ORDER client_status;
	RAIL_EXEC_ORDER exec;
	RAIL_EXEC_RESULT_ORDER exec_result;
	RAIL_SYSPARAM_ORDER sysparam;
	RAIL_ACTIVATE_ORDER activate;
	RAIL_SYSMENU_ORDER sysmenu;
	RAIL_SYSCOMMAND_ORDER syscommand;
	RAIL_NOTIFY_EVENT_ORDER notify_event;
	RAIL_MINMAXINFO_ORDER minmaxinfo;
	RAIL_LOCALMOVESIZE_ORDER localmovesize;
	RAIL_WINDOW_MOVE_ORDER window_move;
	RAIL_LANGBAR_INFO_ORDER langbar_info;
	RAIL_GET_APPID_REQ_ORDER get_appid_req;
	RAIL_GET_APPID_RESP_ORDER get_appid_resp;

} RAIL_ORDERS;

typedef struct
{
	RAIL_ORDERS order_info;
	uint32 event_type;
}
RAIL_EVENT;

typedef struct
{
	rdpChannels* chan_man;
	freerdp*    instance;
	int         th_count;
	int         th_to_finish;

	RAIL_ORDERS out_rail_orders;

	RAIL_EVENT  in_events[20];
	size_t      in_events_number;

	STREAM      in_streams[20];
	size_t      in_streams_number;

	RDP_PLUGIN_DATA plugin_data;


} thread_param;

static thread_param* global_thread_params = NULL;

//-----------------------------------------------------------------------------
int stream_equal_dump(void * dataS, size_t sizeS, void * data, size_t size)
{
	size_t i;
	if (sizeS != size)
	{
		printf("----------------- stream_equal_dump -----------------\n");
		printf("Stream and dump have different length (%d != %d)\n",
			(int) sizeS, (int) size);
		printf("Stream hexdump:\n");
		freerdp_hexdump(dataS, sizeS);

		printf("Dump hexdump:\n");
		freerdp_hexdump(data, size);

		printf("----------------- stream_equal_dump -----------------\n");
		return 0;
	}


	for (i=0; i < size; i++)
	{
		if (((uint8*)dataS)[i] != ((uint8*)data)[i])
		{
			printf("----------------- stream_equal_dump -----------------\n");
			printf("Stream and dump have different content from %d offset.\n", (int) i);
			printf("Stream hexdump:\n");
			freerdp_hexdump(dataS, sizeS);

			printf("Dump hexdump:\n");
			freerdp_hexdump(data, size);
			printf("----------------- stream_equal_dump -----------------\n");
			return 0;
		}
	}

	return 1;
}
//-----------------------------------------------------------------------------
static void test_on_free_rail_client_event(RDP_EVENT* event)
{
	if (event->event_class == RDP_EVENT_CLASS_RAIL)
	{
		rail_free_cloned_order(event->event_type, event->user_data);
	}
}
//-----------------------------------------------------------------------------
static void send_ui_event2plugin(
	rdpChannels* chan_man,
	uint16 event_type,
	void * data
	)
{
	RDP_EVENT* out_event = NULL;
	void * payload = NULL;

	payload = rail_clone_order(event_type, data);
	if (payload != NULL)
	{
		out_event = freerdp_event_new(RDP_EVENT_CLASS_RAIL, event_type,
				test_on_free_rail_client_event, payload);
		freerdp_channels_send_event(chan_man, out_event);
	}
}
//-----------------------------------------------------------------------------
static void emulate_server_send_channel_data(
	freerdp* instance,
	void* data,
	size_t size
	)
{
	static int counter = 0;
	counter++;

	printf("Emulate server packet (%d packet):\n", counter);
	freerdp_hexdump(data, size);

	freerdp_channels_data(instance, 0, (char*)data, size,
			CHANNEL_FLAG_FIRST | CHANNEL_FLAG_LAST, size);
	usleep(10*1000);
}
static void save_dump(void* data, size_t size)
{
	thread_param * p = global_thread_params;
	if (p->in_streams_number < RAIL_ARRAY_SIZE(p->in_streams))
	{
		STREAM* s = &p->in_streams[p->in_streams_number];
		s->data = malloc(size);
		s->size = size;

		memcpy(s->data, data, size);
		p->in_streams_number++;
	}
}
//-----------------------------------------------------------------------------
static int emulate_client_send_channel_data(
	freerdp* freerdp, int channelId, uint8* data, int size
	)
{
	static int counter = 0;
	counter++;

	printf("Client send to server (%d packet):\n", counter);
	freerdp_hexdump(data, size);

	// add to global dumps list
	save_dump(data, size);

	return 0;
}
//-----------------------------------------------------------------------------
void save_event(RDP_EVENT* event, RAIL_EVENT* rail_event)
{
	rail_event->event_type = event->event_type;

	switch (event->event_type)
	{
		case RDP_EVENT_TYPE_RAIL_CHANNEL_GET_SYSPARAMS:
			printf("UI receive Get Sysparams Event\n");
			break;

		case RDP_EVENT_TYPE_RAIL_CHANNEL_EXEC_RESULTS:
			{
				RAIL_EXEC_RESULT_ORDER* exec_result = (RAIL_EXEC_RESULT_ORDER*)event->user_data;
				printf("UI receive Exec Results Event\n");
				memcpy(&rail_event->order_info.exec_result, event->user_data,
					sizeof(RAIL_EXEC_RESULT_ORDER));

				rail_unicode_string_alloc(&rail_event->order_info.exec_result.exeOrFile,
						exec_result->exeOrFile.length);

				memcpy(rail_event->order_info.exec_result.exeOrFile.string,
					exec_result->exeOrFile.string,
					exec_result->exeOrFile.length);
			}
			break;

		case RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_SYSPARAM:
			printf("UI receive Server Sysparam Event\n");
			memcpy(&rail_event->order_info.sysparam, event->user_data,
				sizeof(RAIL_SYSPARAM_ORDER));
			break;

		case RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_MINMAXINFO:
			printf("UI receive Server Minmax Info Event\n");
			memcpy(&rail_event->order_info.minmaxinfo, event->user_data,
				sizeof(RAIL_MINMAXINFO_ORDER));
			break;

		case RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_LOCALMOVESIZE:
			printf("UI receive Server Local Movesize Event\n");
			memcpy(&rail_event->order_info.localmovesize, event->user_data,
				sizeof(RAIL_LOCALMOVESIZE_ORDER));
			break;

		case RDP_EVENT_TYPE_RAIL_CHANNEL_APPID_RESP:
			printf("UI receive AppId Response Event\n");
			memcpy(&rail_event->order_info.get_appid_resp, event->user_data,
				sizeof(RAIL_GET_APPID_RESP_ORDER));

			rail_event->order_info.get_appid_resp.applicationId.string =
				&rail_event->order_info.get_appid_resp.applicationIdBuffer[0];
			break;

		case RDP_EVENT_TYPE_RAIL_CHANNEL_LANGBARINFO:
			printf("UI receive Language Info Event\n");
			memcpy(&rail_event->order_info.langbar_info, event->user_data,
				sizeof(RAIL_LANGBAR_INFO_ORDER));
			break;
	}
}
//-----------------------------------------------------------------------------
static void process_events_and_channel_data_from_plugin(thread_param* param)
{
	RDP_EVENT* event;

	param->th_count++;
	while (param->th_to_finish == 0)
	{
		freerdp_channels_check_fds(param->chan_man, param->instance);

		while (1)
		{
			event = freerdp_channels_pop_event(param->chan_man);
			if (event == NULL) break;

			static int counter = 0;
			counter++;

			printf("UI receive event. (type=%d counter=%d).\n",
					event->event_type,
					counter);

			// add to global event list
			if (param->in_events_number < RAIL_ARRAY_SIZE(param->in_events))
			{
				save_event(event, &param->in_events[param->in_events_number]);
				param->in_events_number++;
			}
			freerdp_event_free(event);
		}

		usleep(1000);
	}
	param->th_count--;
}
//-----------------------------------------------------------------------------
void* thread_func(void* param)
{
	thread_param* th_param = (thread_param*)param;
	process_events_and_channel_data_from_plugin(th_param);
	pthread_detach(pthread_self());

	return NULL;
}
//-----------------------------------------------------------------------------
void test_rail_plugin(void)
{
	thread_param param;
	pthread_t thread;

	rdpChannels* chan_man;
	rdpSettings settings = { 0 };
	freerdp s_inst = { 0 };
	freerdp* inst = &s_inst;
	size_t sn = 0;
	size_t en = 0;
	STREAM* ss = NULL;
	RAIL_EVENT* ee = NULL;

	printf("\n");

	settings.hostname = "testhost";
	inst->settings = &settings;
	inst->SendChannelData = emulate_client_send_channel_data;

	chan_man = freerdp_channels_new();

	freerdp_channels_load_plugin(chan_man, &settings, "../channels/rail/rail.so", NULL);
	freerdp_channels_pre_connect(chan_man, inst);
	freerdp_channels_post_connect(chan_man, inst);

	memset(&param, 0, sizeof(param));

	param.chan_man = chan_man;
	param.instance = inst;
	param.th_count = 0;
	param.th_to_finish = 0;

	global_thread_params = &param;

	pthread_create(&thread, 0, thread_func, &param);

	// 1. Emulate server handshake binary
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_handshake);
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_exec_result_pdu);
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_sysparam1_pdu);
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_sysparam2_pdu);
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_localmovesize_start_pdu);
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_localmovesize_stop_pdu);
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_minmaxinfo_pdu);
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_langbar_pdu);
	EMULATE_SERVER_SEND_CHANNEL_DATA(inst, server_app_get_resp_pdu);

	// 2. Send UI events

	param.out_rail_orders.sysparam.params = 0;

	param.out_rail_orders.sysparam.params |= SPI_MASK_SET_HIGH_CONTRAST;
	param.out_rail_orders.sysparam.highContrast.flags = 0x7e;
	param.out_rail_orders.sysparam.highContrast.colorScheme.length = 0;
	param.out_rail_orders.sysparam.highContrast.colorScheme.string = NULL;

	param.out_rail_orders.sysparam.params |= SPI_MASK_TASKBAR_POS;
	param.out_rail_orders.sysparam.taskbarPos.left = 0;
	param.out_rail_orders.sysparam.taskbarPos.top = 0x039a;
	param.out_rail_orders.sysparam.taskbarPos.right = 0x0690;
	param.out_rail_orders.sysparam.taskbarPos.bottom = 0x03c2;

	param.out_rail_orders.sysparam.params |= SPI_MASK_SET_MOUSE_BUTTON_SWAP;
	param.out_rail_orders.sysparam.mouseButtonSwap = false;

	param.out_rail_orders.sysparam.params |= SPI_MASK_SET_KEYBOARD_PREF;
	param.out_rail_orders.sysparam.keyboardPref = false;

	param.out_rail_orders.sysparam.params |= SPI_MASK_SET_DRAG_FULL_WINDOWS;
	param.out_rail_orders.sysparam.dragFullWindows = true;

	param.out_rail_orders.sysparam.params |= SPI_MASK_SET_KEYBOARD_CUES;
	param.out_rail_orders.sysparam.keyboardCues = false;

	param.out_rail_orders.sysparam.params |= SPI_MASK_SET_WORK_AREA;
	param.out_rail_orders.sysparam.workArea.left = 0;
	param.out_rail_orders.sysparam.workArea.top = 0;
	param.out_rail_orders.sysparam.workArea.right = 0x0690;
	param.out_rail_orders.sysparam.workArea.bottom = 0x039a;

	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_SET_SYSPARAMS,
		&param.out_rail_orders.sysparam);

	param.plugin_data.size = sizeof(RDP_PLUGIN_DATA);
	param.plugin_data.data[0] = "||iexplore";
	param.plugin_data.data[1] = "f:\\windows\\system32";
	param.plugin_data.data[2] = "www.bing.com";
	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_EXEC_REMOTE_APP,
		&param.plugin_data);

	param.out_rail_orders.activate.enabled = true;
	param.out_rail_orders.activate.windowId = 0x0007008e;
	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_ACTIVATE,
		&param.out_rail_orders.activate);

	param.out_rail_orders.syscommand.windowId = 0x00020052;
	param.out_rail_orders.syscommand.command  = 0xf020;
	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_SYSCOMMAND,
		&param.out_rail_orders.syscommand);

	param.out_rail_orders.notify_event.windowId = 0x000201aa;
	param.out_rail_orders.notify_event.notifyIconId = 0x02;
	param.out_rail_orders.notify_event.message = 0x0204;
	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_NOTIFY_EVENT,
		&param.out_rail_orders.notify_event);


	param.out_rail_orders.window_move.windowId = 0x00020020;
	param.out_rail_orders.window_move.left = 0x0309;
	param.out_rail_orders.window_move.top = 0x0100;
	param.out_rail_orders.window_move.right = 0x05db;
	param.out_rail_orders.window_move.bottom = 0x0188;
	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_WINDOW_MOVE,
		&param.out_rail_orders.window_move);

	param.out_rail_orders.sysmenu.windowId = 0x00090122;
	param.out_rail_orders.sysmenu.left = 0xffa4; // TODO: possible negative values?
	param.out_rail_orders.sysmenu.top = 0x024a;
	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_SYSMENU,
		&param.out_rail_orders.sysmenu);

	param.out_rail_orders.langbar_info.languageBarStatus = 0x00000001;
	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_LANGBARINFO,
		&param.out_rail_orders.langbar_info);

	param.out_rail_orders.get_appid_req.windowId = 0x00020052;
	send_ui_event2plugin(chan_man, RDP_EVENT_TYPE_RAIL_CLIENT_APPID_REQ,
		&param.out_rail_orders.get_appid_req);

	// Waiting for possible events or data
	sleep(1);

	// Finishing thread and wait for it
	param.th_to_finish = 1;
	while (param.th_count > 0)
	{
		usleep(1000);
	}

	// We need to collected all events and data dumps and then to.
	// create CU_ASSERT series here!
	sn = param.in_streams_number;
	en = param.in_events_number;
	ss = &param.in_streams[0];
	ee = &param.in_events[0];

	CU_ASSERT(sn > 0 && STREAM_EQUAL_TO_DUMP(&ss[ 0], client_handshake));
	CU_ASSERT(sn > 1 && STREAM_EQUAL_TO_DUMP(&ss[ 1], client_info_pdu));
	CU_ASSERT(sn > 2 && STREAM_EQUAL_TO_DUMP(&ss[ 2], client_sysparam_highcontrast_pdu));
	CU_ASSERT(sn > 3 && STREAM_EQUAL_TO_DUMP(&ss[ 3], client_sysparam_taskbarpos_pdu));
	CU_ASSERT(sn > 4 && STREAM_EQUAL_TO_DUMP(&ss[ 4], client_sysparam_mousebuttonswap_pdu));
	CU_ASSERT(sn > 5 && STREAM_EQUAL_TO_DUMP(&ss[ 5], client_sysparam_keyboardpref_pdu));
	CU_ASSERT(sn > 6 && STREAM_EQUAL_TO_DUMP(&ss[ 6], client_sysparam_dragfullwindow_pdu));
	CU_ASSERT(sn > 7 && STREAM_EQUAL_TO_DUMP(&ss[ 7], client_sysparam_keyboardcues_pdu));
	CU_ASSERT(sn > 8 && STREAM_EQUAL_TO_DUMP(&ss[ 8], client_sysparam_setworkarea_pdu));
	CU_ASSERT(sn > 9 && STREAM_EQUAL_TO_DUMP(&ss[ 9], client_execute_pdu));
	CU_ASSERT(sn >10 && STREAM_EQUAL_TO_DUMP(&ss[10], client_activate_pdu));
	CU_ASSERT(sn >11 && STREAM_EQUAL_TO_DUMP(&ss[11], client_syscommand_pdu));
	CU_ASSERT(sn >12 && STREAM_EQUAL_TO_DUMP(&ss[12], client_notify_pdu));
	CU_ASSERT(sn >13 && STREAM_EQUAL_TO_DUMP(&ss[13], client_windowmove_pdu));
	CU_ASSERT(sn >14 && STREAM_EQUAL_TO_DUMP(&ss[14], client_system_menu_pdu));
	CU_ASSERT(sn >15 && STREAM_EQUAL_TO_DUMP(&ss[15], client_langbar_pdu));
	CU_ASSERT(sn >16 && STREAM_EQUAL_TO_DUMP(&ss[16], client_get_app_id_req_pdu));

	CU_ASSERT(en >  0 && ee[ 0].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_GET_SYSPARAMS);
	CU_ASSERT(en >  1 &&
		ee[ 1].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_EXEC_RESULTS &&
		ee[ 1].order_info.exec_result.flags == 0x08 &&
		ee[ 1].order_info.exec_result.execResult == 0x03 &&
		UNICODE_STRING_EQUAL_TO_DUMP(
			&ee[ 1].order_info.exec_result.exeOrFile,
			server_exec_result_exe_or_file)
		);
	CU_ASSERT(en >  2 &&
		ee[ 2].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_SYSPARAM &&
		ee[ 2].order_info.sysparam.setScreenSaveSecure == false
		);

	CU_ASSERT(en >  3 &&
		ee[ 3].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_SYSPARAM &&
		ee[ 3].order_info.sysparam.setScreenSaveActive == false
		);

	CU_ASSERT(en >  4 &&
		ee[ 4].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_LOCALMOVESIZE &&
		ee[ 4].order_info.localmovesize.windowId == 0x0007008e &&
		ee[ 4].order_info.localmovesize.isMoveSizeStart == true &&
		ee[ 4].order_info.localmovesize.moveSizeType == RAIL_WMSZ_MOVE &&
		ee[ 4].order_info.localmovesize.posX == 0x017e &&
		ee[ 4].order_info.localmovesize.posY == 0x000a
		);

	CU_ASSERT(en >  5 &&
		ee[ 5].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_LOCALMOVESIZE &&
		ee[ 5].order_info.localmovesize.windowId == 0x0007008e &&
		ee[ 5].order_info.localmovesize.isMoveSizeStart == false &&
		ee[ 5].order_info.localmovesize.moveSizeType == RAIL_WMSZ_MOVE &&
		ee[ 5].order_info.localmovesize.posX == 0x00a6 &&
		ee[ 5].order_info.localmovesize.posY == 0x0044
		);

	CU_ASSERT(en >  6 &&
		ee[ 6].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_SERVER_MINMAXINFO &&
		ee[ 6].order_info.minmaxinfo.windowId == 0x0007008e &&
		ee[ 6].order_info.minmaxinfo.maxWidth == 0x0408 &&
		ee[ 6].order_info.minmaxinfo.maxHeight == 0x02d6 &&
		ee[ 6].order_info.minmaxinfo.maxPosX ==  0x0000 &&
		ee[ 6].order_info.minmaxinfo.maxPosY ==  0x0000 &&
		ee[ 6].order_info.minmaxinfo.minTrackWidth ==  0x0070 &&
		ee[ 6].order_info.minmaxinfo.minTrackHeight ==  0x001b &&
		ee[ 6].order_info.minmaxinfo.maxTrackWidth ==  0x040c &&
		ee[ 6].order_info.minmaxinfo.maxTrackHeight ==  0x030c
		);

	CU_ASSERT(en >  7 &&
		ee[ 7].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_LANGBARINFO &&
		ee[ 7].order_info.langbar_info.languageBarStatus == TF_SFT_SHOWNORMAL
		);

	CU_ASSERT(en >  8 &&
		ee[ 8].event_type == RDP_EVENT_TYPE_RAIL_CHANNEL_APPID_RESP &&
		ee[ 8].order_info.get_appid_resp.windowId == 0x00020052 &&
		UNICODE_STRING_EQUAL_TO_DUMP(
			&ee[ 8].order_info.get_appid_resp.applicationId,
			server_app_get_resp_app_id
			)
		);

	freerdp_channels_close(chan_man, inst);
	freerdp_channels_free(chan_man);
}




