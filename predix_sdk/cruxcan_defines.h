// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once


//----------------common---------------------------
#define DBGMSG(a)		OutputDebugStringA(a "\n");

//----------------viewer---------------------------
#define WM_MAIN_TRAY	(WM_USER + 6782)

//----------------preprocess,geoprocess---------------------------
#define RAW_IMAGE_MAX_SIZE		((scan_width + line_info_length) * scan_height)
#define SCAN_IMAGE_MAX_SIZE		(scan_width * scan_height)

//----------------network---------------------------
#define MIN_TCP_SOCKET_BUFFER	(1 * 1024)
#define MAX_TCP_SOCKET_BUFFER	MAX_PACKETLEN


//#ifndef SAFE_DELETE_ARRAY
//#define SAFE_DELETE_ARRAY(a)	{ if (a) { delete [] a; a = NULL; } }
//#define SAFE_DELETE(a)			{ if (a) { delete a; a = NULL; } }
//#endif


#define PREDIX_UDP_PORT			(5552)
#define PREDIX_TCP_PORT			(5552)
//#define N2N_CENTER_DEVICE_PORT		(5554)

#define CRX_1000_PORT					(5552)
#define N2N_CENTER_RECEIVER_PORT		(5554)
#define N2N_SDK_RECEIVER_PORT			(5555)	//
#define N2N_CENTER_DUMMY_PORT			(5557)
#define N2N_TRAY_INTRANET_CHECK_PORT	(5558)
#define N2N_TRAY_RECEIVER_PORT			(5559)	//

#define TCP_PACKET_MIN_SIZE		(512)

#define MIN_STDEVICE_SIZE		(64)

#define UDP_PACKET_SIZE			(512)

#define TRAY_IP_ADDR			_T("127.0.0.1")

// ping check limist
#define DEFAULT_PING_LIMIT_COUNT	(2)	//! kiwa72(2023.10.17 13h) – 값 변경 1 ->> 5

//----------------tray---------------------------
#ifndef __SCANNNER_MSGS__
#define __SCANNNER_MSGS__
#endif
