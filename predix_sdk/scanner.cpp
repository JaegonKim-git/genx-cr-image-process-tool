// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "tiff_image.h"
//#include "bitmapex.h"
#include "common.h"
#include "postprocess.h"
#include "global_data.h"
#include "scanner.h"
#include "../include/logger.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


int scan_width = 1530;
int scan_height = geo_y;

int geo_width = 1406;
int geo_height = geo_y;

int line_info_length = 6;

#define RAW_IMAGE_MAX_SIZE ((scan_width + line_info_length) * scan_height)
#define SCAN_IMAGE_MAX_SIZE (scan_width * scan_height)

int new_scanner = 0;
int scan_running = 0;
int scanner_connected = 0;
int image_list_done = 0;
IP_SIZE ip_size_s = eis_None;
IMAGE_RESOLUTION image_speed_mode_s = eir_HiRes;
SCANNER_STATE g_Scanner_State_s = ss_disconnected;
int image_scanner_progress_s = 0;

unsigned int* line_data;
unsigned int* bldc_data1;
unsigned int* bldc_data2;
unsigned int line_count;

CRUXCAN_IMAGE* cruxcan_image;

extern bool is_image_received_at_twain;


void scan_data_init(void)
{
	auto pPdxData = GetPdxData();
	ASSERT(pPdxData);

	// 코드가 이상하게 되어 있어서 수정.
	if ( pPdxData->last_image_data == nullptr )
	{
		pPdxData->last_image_data = new unsigned short [RAW_IMAGE_MAX_SIZE];
		ASSERT(pPdxData->last_image_data);
	}
	::ZeroMemory(pPdxData->last_image_data, sizeof(USHORT) * RAW_IMAGE_MAX_SIZE);
}

void scan_data_destroy(void)
{
	SAFE_DELETE_ARRAY(GetPdxData()->last_image_data);
}

void set_new_scanner(int num)
{
	new_scanner = num;
}

int get_new_scanner(void)
{
	return new_scanner;
}

void set_scan_running(int num)
{
	scan_running = num;
}

int get_scan_running(void)
{
	return scan_running;
}

void set_scanner_connected(int num)
{
	scanner_connected = num;
}

int get_scanner_connected(void)
{
	return scanner_connected;
}

void set_ip_size(IP_SIZE is)
{
	ip_size_s = is;

	if (ip_size_s == 0)
	{
		GetPdxData()->last_image_width = 880;
		GetPdxData()->last_image_height = 1240;
	}
	else if (ip_size_s == 1)
	{
		GetPdxData()->last_image_width = 960;
		GetPdxData()->last_image_height = 1600;
	}
	else if (ip_size_s == 2)
	{
		GetPdxData()->last_image_width = 1240;
		GetPdxData()->last_image_height = 1640;
	}
	else
	{
		GetPdxData()->last_image_width = 1080;
		GetPdxData()->last_image_height = 2160;
	}

	if (get_image_speed_mode())
	{
		GetPdxData()->last_image_width = GetPdxData()->last_image_width / 2;
		GetPdxData()->last_image_height = GetPdxData()->last_image_height / 2;
	}
}

IP_SIZE get_ip_size(void)
{
	return ip_size_s;
}

void set_image_speed_mode(IMAGE_RESOLUTION ir)
{
	image_speed_mode_s = ir;
}

IMAGE_RESOLUTION get_image_speed_mode(void)
{
	return image_speed_mode_s;
}

void set_image_ip_size(IP_SIZE ip)
{
	set_ip_size(ip);
}

IP_SIZE get_image_ip_size(void)
{
	return get_ip_size();
}

void set_scanner_progress_s(int pct)
{
	image_scanner_progress_s = pct;
}

int get_scanner_progress_s(void)
{
	return image_scanner_progress_s;
}

//-------------------------------------------------------------------

void set_scanner_state_s_only(SCANNER_STATE ss)
{
	if ( ss >= 0 && ss <= ss_system )
	{
		KLOG3A("SCANNER_STATE ss = %d [%s]", ss, button_scanner_msgs[ss]);
	}

	if (ss > ss_system || ss < ss_disconnected)
	{
		KLOG3A("ss is abnormal value2 !!!!");
		writelog("SCANNER_STATE: ERROR - Abnormal state value\n", "predix_network.log");
	}

	g_Scanner_State_s = ss;
}

extern void num2arr(const unsigned long num, unsigned char* arr);
extern bool send_protocol_to_local(NETWORK_COMMAND nc, unsigned char* data, int size);

void send_state_to_host(SCANNER_STATE ss)
{
	unsigned char temp[4];
	num2arr(ss, temp);

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	KLOG3A("nc_scanner_state = %d | %s", ss, pGlobalData->GetScannerMessage(ss));

	//! kiwa72(2023.08.30 11h) - 장비 연결되면 버튼 선택상태 초기화
	GetPdxData()->g_bSel_Btn_Connect = FALSE;
	GetPdxData()->g_bSel_Btn_ResetConnect = FALSE;
	//!...

	//! kiwa72(2023.06.16 11h) - 'SW'의 스캐너 상태를 장치에 전송함.
	send_protocol_to_local(nc_scanner_state, temp, 4);
}

void set_scanner_state_s(SCANNER_STATE ss)
{
	if ( ss >= 0 && ss <= ss_system )
	{
		KLOG3A("SCANNER_STATE ss = %d [%s]", ss, button_scanner_msgs[ss]);
	}

	if (ss == ss_disconnected)
	{
		unsigned char temp[4];
		num2arr(ss, temp);

		//! kiwa72(2023.06.16 11h) - 'SW'의 스캐너에 연결 취소 요청
		send_protocol_to_local(nc_tray_cancel, temp, 4);
		Sleep(2 * 1000);
	}
	else
	{
		send_state_to_host(ss);
		Sleep(500);
	}

	g_Scanner_State_s = ss;
}

SCANNER_STATE get_scanner_state_s(void)
{
	return g_Scanner_State_s;
}

/**
 * @brief	is scanner busy
 * @details	현재 상태 스캐너 확인
 * @param	None
 * @return	enum _SCANNER_STATE_
 * @author	kiwa72
 * @date	kiwa72(2023.06.24 11h)
 */
int is_scanner_busy(void)
{
	switch (g_Scanner_State_s)
	{
	default:
	case ss_disconnected:
	case ss_warmup:
		return -2;

	case ss_ready:
	case ss_sleep:
		return 0;

	case ss_scan:
	case ss_erasing:	//! kiwa72(2023.06.24 11h) - 신규 추가: 스캔 후 ip 삭제 중
	case ss_preview:	//! kiwa72(2023.06.24 11h) - 신규 추가: 스캔 후 바로보기 화면 상태
	case ss_transfer:
	case ss_busy:
	case ss_system:
		return 1;
	}
}
