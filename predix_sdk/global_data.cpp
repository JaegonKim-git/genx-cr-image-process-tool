// Added by Makeit, on 2023/07/13.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "cruxcan_sdk.h"
#include "common_type.h"
#include "CTCPsc.h"
#include "CUDPsc.h"
#include "global_data.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class related global data

static global_data* g_pPredixGlobalData = nullptr;

const char* button_scanner_msgs[] =
{
	"Disconnected",		// 0
	"Waiting",			// 1
	"Ready",			// 2
	"Scanning",			// 3

	"Erasing",			// 4	//! kiwa72(2023.06.24 11h) - 신규 추가: 스캔 후 ip 삭제 중
	"Preview",			// 5	//! kiwa72(2023.06.24 11h) - 신규 추가: 스캔 후 바로보기 화면 상태

	"Transferring",		// 6
	"Busy",				// 7
	"System",			// 8
	"Sleeped",			// 9
};

const HARDWARE_SETTING def_hw_setting =
{
	{
		{ 65533/* 0xFFFD */, 1, 65531/* 0xFFFB */, 71/* nc_hardware_info */ },
		"PREDIX CR",
		"genoray",
		"PREDIX CR",
		"(01)0880XXXXXX021X(11)XXXXXX(10)XXXXXXXX(21)000000",
		{0},

		{1,0,0,0},
		{1,0,0,0},
		{1,0,0,0},

		{1,0,0,0},
		{1,0,0,0},
		{1,0,0,0},
		0,	// GUI CRC32 값

		{192,168,0,171},
		{255,255,255,0},
		{192,168,0,1},
		{0x05, 0x00, 0x00, 0xEF, 0xFF, 0xFF},
		{0,0},
		el_ENG /* el_ENG */,
		1005,
		1010,
		eir_HiRes /* eir_HiRes */,
		{0,0,0},
		0,
		1388890,
		8200,
		452000000,
		452000000,
		0,
		4000000,
		465000000,
		400000000,
		440000000,
		5199,
		36000,
		36000,
		1,
		1,
		1,
		150,
		1,
		1,
		1,
		1,
		0,
		0,
		3750,
		36000,	//	MPPC 3 bias
		0,	// kiwa72(2023.01.21 18h) - 선택된 IP Scan StartY
		0,	//	레이저 파워

#if (1)
		0, 0, 1,	// 사용하지 않음
#else
		//! kiwa72(2023.04.26 14h) - 터치 보정 구조체 적용으로 사용안함
		//// kiwa72(2023.03.22 14h) - touch cal 기본값
		//1,	// tsCal_Saved
		//2,	// tsCal_X_org
		//3,	// tsCal_X_limit
		//4,	// tsCal_Y_org
		//5,	// tsCal_Y_limit
		//{0},	// 예약 - 4바이트 정렬 더미
#endif

		0,	//	누적 스캔 횟수
		2,	//	선택된 ip size

		15000,	//	스캔 시작스피드
		4000,
		1000,
		500,

		10000000,	//	size0 포지션 타임
		30000000,
		30000000,
		50000000,

		80000000,
		500000000,

#if (DEF_BACKWARD_OFFSET_LENGTH)
		0x60000000,
#else
		0,
#endif

		2500,	// max_speed	최대속도

		{0,0,0,0,0},
		{{0,0,0,0}, {0,0,0,0,0}, {0,0,0,0}},
		"",
		"",
		0,

		0,
		0,
		0,
		0,

		ss_disconnected,

#if (0)
		{{0,}, {0,}, {0,}},	// kiwa72(2023.01.21 18h) - New Calibration Data
#endif
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
global_data::global_data()
	: g_obl_code(enum_obl_code::eoc_cruxell)
	, hCloseMutex(nullptr)
	, g_pTtray_Dlg(nullptr)
	, self_close(nullptr)
	, g_piniFile(nullptr)
	, g_plogFile(nullptr)
	, g_TCP_Server(nullptr)
	, g_pUDPsc(nullptr)
	, g_pTicker(nullptr)
	, g_pUpdateDialog(nullptr)
	, g_pDeviceSocket(nullptr)
	, last_image_data(nullptr)
	, bAcquire_after_scan(TRUE)
	, bAuto_pp(FALSE)
	, nCorrectWobbleResult(-1)	// 2026-03-10. jg kim. Wobble 보정 결과
	, bAcquireCalImage(FALSE)	// 2024-08-26. jg kim. cal image 획득 모드
	, g_iCalImgCount(0)
	, bImageListMode(FALSE)	// 2026-02-27. jg kim. ImageList에서 요청한 영상 모드
	, g_inetwork_mode(0)
	, g_strudps{ L"255.255.255.255", }
	, g_adaptercount(1)
	, g_selected_adapter_index(0)
	, g_use_ini_calibration_coefs(0)
	, g_fcoefs{ 0., }
	, g_en_CalCoefficients(Cal_ValidCoefficients)
	, g_en_Door(HW_DoorClosed)
	, g_enScanMode(HW_ValidScanMode)
	, g_nNotification(Notification_Resetted)
	, g_is_image_loaded(0)
	, g_strmyIPs{ L"0.0.0.0", }
	, current_setting(def_hw_setting)
	, is_binary_saved(0)
	, start_tick(0)
	, end_tick(0)
	, kernel_size(64)
	, scanning_progress_tray(0.f)
	, nLastScannedRes(eir_HiRes)
	, broadcast_message(0)
	, map_raw{ { raw_x, raw_y }, { 33, 33 } }
	, map_origin{ { raw_x, raw_y }, { 33, 33 } }
	, map_geo{ { geo_x, geo_y }, { 33, 33 } }
	//, map_last_prep{{ raw_x, raw_y }, { 33, 33 }}
	, map_fin{ { geo_x, geo_y }, { 33, 33 } }
	, map_img16buffer{ { geo_x, geo_y }, { 33, 33 } }
	, g_ipostprocess(0)
	, g_ipp_type(0)
	, g_iblur_radius(2)
	, g_iedge_enhance(1)
	, g_ibrightness_offset(0)	// 20210319 7000 -> 0
	, last_image_width(1240)
	, last_image_height(2160)
	, g_nConnected_ScannerCnt(0)
{
#if FOR_GENORAY_COMMON
	g_obl_code = enum_obl_code::eoc_genoray;
#define LOG_FOR_GENORAY	1
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
global_data::~global_data()
{
	g_nConnected_ScannerCnt = 0;

	ASSERT(g_TCP_Server == nullptr);
	ASSERT(g_pUDPsc == nullptr);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Create a singleton instance
global_data* global_data::InitInstance(void)
{
	if (g_pPredixGlobalData == nullptr)
	{
		ASSERT((INT_PTR)g_pPredixGlobalData != 0xDDDDDDDD);
		g_pPredixGlobalData = new global_data;
		g_pPredixGlobalData->Initialize();
	}
	ASSERT(g_pPredixGlobalData);

	return g_pPredixGlobalData;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get singleton instance
global_data* global_data::GetInstance(void)
{
	if ( g_pPredixGlobalData == nullptr || (INT_PTR)g_pPredixGlobalData == 0xDDDDDDDD )
	{
		AfxMessageBox( _T("[Developer error] global_data is not initialized!"), MB_ICONERROR );
		throw nullptr;
	}
	ASSERT(g_pPredixGlobalData);

	return g_pPredixGlobalData;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Release instance (프로그램 종료 전에 한번만 호출해주세요)
void global_data::ReleaseInstance(void)
{
	if ( g_pPredixGlobalData )
	{
		ASSERT((INT_PTR)g_pPredixGlobalData != 0xDDDDDDDD);

		// 클래스 내부에서 생성한 메모리들 먼저 한번 정리 해주고..
		g_pPredixGlobalData->ReleaseData();

		// 사용 완료.
		SAFE_DELETE(g_pPredixGlobalData);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// return human readable text of the scanner state.
const char* global_data::GetScannerMessage(int nState)
{
	// enum과 문자열들 정리 부탁 드립니다. 이곳에서 비정상 종료됩니다. (button_scanner_msgs 에 저장되는 내용과 enum이 맞지 않습니다)
	int nMsg = 0;
	if ( nState == ss_sleep )
	{
		nMsg = 9;	// "Sleeped"
	}
	// button_scanner_msgs 정의 범위를 벗어나는지 확인.
	else if ( nState < SCANNER_STATE::ss_disconnected || nState > SCANNER_STATE::ss_system )
	{
		nMsg = 7;	// Unknown state -> Busy로 표시.
	}
	//! kiwa72(2023.10.01 06h) - 상태 정보 저장
	else
	{
		nMsg = nState;
	}
	//!...

	return button_scanner_msgs[nMsg];
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// initialize internal members
void global_data::Initialize(void)
{
	::ZeroMemory(&current_pvIMAGE_INFO_list, sizeof(current_pvIMAGE_INFO_list));
	current_pvIMAGE_INFO_list.imageinfos[0].ip_size = IP_SIZE::eis_None;

	if (g_TCP_Server == nullptr)
	{
		g_TCP_Server = new CTCPsc;
		ASSERT(g_TCP_Server);
	}
	else
	{
		// 두번호출했나?
		ASSERT(0);
		ASSERT((INT_PTR)g_TCP_Server != 0xDDDDDDDD);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// release internal members
void global_data::ReleaseData(void)
{
	g_nConnected_ScannerCnt = 0;

	SAFE_DELETE(g_pUDPsc);
	SAFE_DELETE(g_TCP_Server);
}
