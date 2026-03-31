#pragma once


// Application 에서 사용하는 구조체이므로 이곳에서 주석처리함.
//typedef struct ST_AdapterInfo
//{
//	std::string ip;
//	std::string desc;
//	std::string mac;
//} ST_AdapterInfo;

typedef enum _TRAY_CB_SCANNER_STATE
{
	cb_disconnected = 0,
	cb_waiting,// changed from ss_warmup
	cb_ready,
	cb_scan,
	cb_transfer,
	cb_busy,
	cb_system,
	cb_connected,	//wjlee : 20220623 ver. add
	cb_sleep = 0x7FFFFFFE,//wjlee : 20220523 ver. Add
	cb_none = 0x7FFFFFFF//wjlee : 20220523 ver. Add
} TRAY_CB_SCANNER_STATE;

typedef enum _STRING_CASE_TYPE
{
	STRING_CASE_NONE = 0,
	STRING_CASE_UPPER,
	STRING_CASE_LOWER,
} STRING_CASE_TYPE;

typedef enum _CALIBRATION_SHIFT_TYPE
{
	SHIFT_RIGHT = 0,
	SHIFT_LEFT,
} CALIBRATION_SHIFT_TYPE;

typedef struct _CALIBRATION_INFO
{
	CALIBRATION_SHIFT_TYPE ShiftType;
	int Avg;
	int Rotx;
	int Roty;
	int RotWidth;
	int RotHeight;
	float RotAngle;
	_CALIBRATION_INFO()
	{
		ShiftType = CALIBRATION_SHIFT_TYPE::SHIFT_RIGHT;
		Avg = 0;
		Rotx = 0;
		Roty = 0;
		RotWidth = 0;
		RotHeight = 0;
		RotAngle = 0;
	};
} CALIBRATION_INFO;

//wjlee : n2n 관련 UI,로직 분리 위해 추가
struct   N2N_DEVICE_INFO
{
	char   Name[12];
	char   device_ipaddr[32];
	char   host_ipaddr[32];
	TRAY_CB_SCANNER_STATE state;//4byte
};
//end

#ifdef PREDIXSCAN_IMPORTS
#define GENXCR_SDK __declspec(dllimport)
#else
#define GENXCR_SDK __declspec(dllexport)
#endif

//static CRITICAL_SECTION g_cs;
class GENXCR_SDK CTrayCallback
{
public:
	CTrayCallback()
	{ 
		//::InitializeCriticalSection(&g_cs);
		::InitializeCriticalSectionAndSpinCount(&g_cs, 2000);
	}
	~CTrayCallback()	{ ::DeleteCriticalSection(&g_cs); }

	CRITICAL_SECTION g_cs;

public:
	virtual void ScannerState(TRAY_CB_SCANNER_STATE	/*state*/) {}
	virtual void RefreshHardwareInfo(unsigned char* /*data*/) {}
	virtual void ScanProgressState(int /*nPos*/) {}
	virtual void ReceiveImageList(unsigned char* /*data*/) {}
	virtual void ReceiveImage(const wchar_t * /*file*/, int /*nWidth*/, int /*nHeight*/, unsigned short * /*img16*/) {}
	virtual void ReceiveRelease(int /*nState*/) {}
	virtual void PostProcessState(int /*nState*/) {}
	virtual void ReceiveScannerList(void* /*pDeviceList*/) {}
};