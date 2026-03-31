#include "pch.h"
#include "common_type.h"
#include "scanner.h"
#include "global_data.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



#define MINIMUM_LOG		(1)



void dpf(const char* format, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 511, format, args);
	std::string dbgmsg = "[CRX/XXX/TRY/INF]";
	dbgmsg.append(buffer).append("\n");
	OutputDebugStringA(dbgmsg.c_str());
	//do something with the error
	va_end(args);
}


void dpc(const char* format, ...)
{
	char buffer[512];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, 511, format, args);
	std::string dbgmsg = "[CRX/XXX/TRY/CBS]";
	dbgmsg.append(buffer);// .append("\n");
	OutputDebugStringA(dbgmsg.c_str());
	//do something with the error
	va_end(args);
}


const HARDWARE_SETTING none_setting =
{
	{
		{ 65533/* 0xFFFD */, 1, 65531/* 0xFFFB */, 71/* nc_hardware_info */ },
		"PREDIX SCAN",
		"genoray",
		"PREDIX SCAN",
		"(01)00000000000000(11)000000(10)00000000(21)000000",
		{ 0 },			// 예약

		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },	// 예약
		{ 0, 0, 0, 0 },	// 예약

		{ 0, 0, 0, 0 },	// 예약
		{ 0, 0, 0, 0 },	// 펨웨어 버전(현재 버전)
		{ 0, 0, 0, 0 },	// 예약
		0,	// GUI CRC32 값

		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
		{ 0, 0 },
		el_ENG,
		0,
		0,
		eir_HiRes,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		{ 0, },
	}
};


const HARDWARE_SETTING default_setting =
{
	{
		{ 65533/* 0xFFFD */, 1, 65531/* 0xFFFB */, 71/* nc_hardware_info */ },
		"PREDIX CR",
		"genoray",
		"PREDIX CR",
		"(01)0880XXXXXX021X(11)XXXXXX(10)XXXXXXXX(21)000000",
		{0},		// 예약

		{1,0,0,0},
		{1,0,0,0},	// 예약
		{1,0,0,0},	// 예약

		{1,0,0,0},
		{1,0,0,0},
		{1,0,0,0},	// 예약
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
		0, 0, 0,	// 사용하지 않음
#else
		//// kiwa72(2023.03.22 14h) - touch cal 기본값
		//5,	// tsCal_Saved
		//4,	// tsCal_X_org
		//3,	// tsCal_X_limit
		//2,	// tsCal_Y_org
		//1,	// tsCal_Y_limit
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

		{0, 0, 0, 0, 0},
		{{0, 0, 0, 0}, {0.0, 0.0, 0.0, 0.0, 0.0}, {0, 0, 0, 0}},

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


//IMAGE_LIST current_image_list;
TCHAR working_path[1024];


CRX_N2N_PROTOCOL crx_n2n_protocol =
{
	65533, 1, 65531, 124,	// SOF - 123: device, 124: center
	0,				// index
	1,				// curpage
	1,				// totalpage
	{1 ,0, 0, 0},	// version
	0,				// device_count
	{
		// (CRX_N2N_DEVICE)devices[20];
		{{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none},
		{{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none},
		{{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none},
		{{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none}, {{0},{0},{0},ss_none},
	},
	{65532, 1, 65534, 65535}	// SOE
};


int is_valid_crx_n2n_protocol(CRX_N2N_PROTOCOL* b)
{
	if (b->SignatureF[0] != 65533	||	// 0xFFFD 
		b->SignatureF[1] != 1		||	// 0x0001
		b->SignatureF[2] != 65531		// 0xFFFB
	)
	{
		return (-1);
	}

	if (b->SignatureE[0] != 65532	||	// 0xFFFC
		b->SignatureE[1] != 1		||	// 0x0001
		b->SignatureE[2] != 65534	||	// 0xFFFE
		b->SignatureE[3] != 65535		// 0xFFFF
	)
	{
		return (-2);
	}

	return (0);
}


//-----------------------------------------------------------------------------


void check_working_path()
{
	TCHAR szSpecialPath[1024] = { 0 };
	SHGetSpecialFolderPath(NULL, szSpecialPath, CSIDL_COMMON_DOCUMENTS, FALSE);	// make working directory
	_stprintf_s(working_path, _T("%s\\cruxcan\\"), szSpecialPath);
	CreateDirectory(working_path, NULL);
}


//void init_mylogger(char * filename)
//{
//	OutputDebugStringA(filename);
//#if MINIMUM_LOG
//	static plog::RollingFileAppender<plog::MySimpleFormatterS> fileAppender(filename, 1024 * 1024 , 10);//소스파일및 라인 기록안함
//#else
//	static plog::RollingFileAppender<plog::MySimpleFormatterF> fileAppender(filename, 1024 * 1024, 10);//소스파일및 라인 기록함
//#endif
//	static plog::DebugOutputAppender<plog::MySimpleFormatterS> debugOutputAppender;
//	plog::init(plog::debug, &debugOutputAppender).addAppender(&fileAppender);
//
//	MYPLOGDG("\n");
//	MYPLOGDG("--------------------------------------------------------");
//	MYPLOGDG("-------------------    tray start    -------------------");
//	MYPLOGDG("--------------------------------------------------------");
//}
//
//void close_mylogger()
//{
//	MYPLOGDG("--------------------------------------------------------");
//	MYPLOGDG("-------------------    tray end    ---------------------");
//	MYPLOGDG("--------------------------------------------------------");
//}
