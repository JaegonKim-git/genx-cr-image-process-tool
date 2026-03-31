// Added by Makeit, on 2023/07/13.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "rect.h"
#include "common.h"

#include "../predix_sdk/CiniFile.h"
#include "../predix_sdk/constants.h"
#include "../gnPostProcessing/inline_imgProc.h"
#include "CTicker.h"


// forward declarations
class CTCPClientSock;
class CTCPsc;
class CUDPsc;
class crx_pipe_class;


enum enum_obl_code;
enum NETWORK_COMMAND;


// for callback functions
typedef void(*PREDIX_NORMAL_CALLBACK)(void);
typedef void(*PREDIX_ONEINT_CALLBACK)(int);
typedef bool(*PREDIX_do_something)(unsigned char* data, int size, NETWORK_COMMAND nc, void* Socket);
typedef CString(*PREDIX_GetSaveFilename)(void);
typedef void(*PREDIX_LPCWSTR)(LPCWSTR);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// predix_sdk(cruxcan_sdk) 에서 사용하기 위한 global data 들을 관리/저장하기 위한 singleton 클래스.
// 실제 목적은 extern으로 선언하여 여기저기에서 사용하고 있는 변수들을 정리하고 없애기 위함임.
class AFX_EXT_CLASS global_data
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public static methods
public:
	// Create a singleton instance
	static global_data* InitInstance(void);

	// Get singleton instance
	static global_data* GetInstance(void);

	// Release instance (프로그램 종료 전에 한번만 호출해주세요)
	static void ReleaseInstance(void);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public methods
public:
	// return human readable text of the scanner state.
	static const char* GetScannerMessage(int nState);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public members
public:
	enum_obl_code g_obl_code;
	HANDLE hCloseMutex;
	CDialog* g_pTtray_Dlg;
	CINIFile* g_piniFile = nullptr;
	CINIFile* g_plogFile = nullptr;
	CUDPsc* g_pUDPsc;
	CTicker* g_pTicker;
	CDialog* g_pUpdateDialog;

	//! kiwa72(2023.04.23 11h) - 스캐너 장비가 접속한 클라이언트 소켓
	CTCPClientSock* g_pDeviceSocket;

	CTCPsc* g_TCP_Server;
	unsigned short* last_image_data;
	BOOL bAcquire_after_scan;
	BOOL bAuto_pp;
	// 2026-01-12. jg kim. MTF 측정 모드, Wobble 보정 결과
	BOOL bProcMTF;
	int nCorrectWobbleResult;
	//2026 - 01 - 13. jg kim.장비에서 넘겨주는 반사판 영역의 Laser On / Off x 좌표 및, 세로 줄 범위(시작 / 끝 x 좌표)
	int g_iLeftLaserOnPos;
	int g_iLeftLaserOffPos;
	int g_iRightLaserOnPos;
	int g_iRightLaserOffPos;
	int g_iLeftLineStartPos;
	int g_iLeftLineEndPos;
	int g_iRightLineStartPos;
	int g_iRightLineEndPos;
	// 2024-08-26. jg kim. cal image 획득 모드 추가(공정툴에서 사용)
	BOOL bAcquireCalImage;
	int g_iCalImgCount;
	// 2026-02-27. jg kim. ImageList에서 요청한 영상은 영상처리 없이 raw만 저장
	BOOL bImageListMode;
	int g_inetwork_mode;
	CString single_target;
	CString g_strudps_org;	//! kiwa72(2024.08.17 15h) - 브로드케스팅 IP 셋팅
	CString g_strudps[32];
	CString g_strMask[32];
	int g_adaptercount;
	int g_selected_adapter_index;
	int g_use_ini_calibration_coefs;
	// 2026.03.31 jg kim. CalibrationDlg의 가로/세로 유효영역을 ini 파일에서 입력 받도록 변경
	int g_iVaidWidth;
	int g_iValidHeight;
	float g_fcoefs[(DEGREE_COUNT + 1)];//2023.11.22 jg kim.
	NewCalibrationData g_NewCalibrationData;
	// 2024-08-15. jg kim. 각 모드/해상도별 파라미터 추가
	IMG_PROCESS_PARAM g_mceIppi_HARD_HR; // 2024-02-29. jg kim. paramMceIppi가 IMG_PROCESS_PARAM보다 나중에 만들었고 내용상 동일하기 때문에 IMG_PROCESS_PARAM을 사용하도록 수정
	IMG_PROCESS_PARAM g_mceIppi_HARD_SR;
	IMG_PROCESS_PARAM g_mceIppi_MEDIUM_HR; // 2025-05-30. jg kim. MEDIUM 모드 구현
	IMG_PROCESS_PARAM g_mceIppi_MEDIUM_SR;
	IMG_PROCESS_PARAM g_mceIppi_SOFT_HR;
	IMG_PROCESS_PARAM g_mceIppi_SOFT_SR;
	enum_save_image_step g_save_image_step; // 2024-02-27. jg kim. 인증 문서상 필요에 의해 추가
	// 2024-04-22. jg kim.  PortView에서 사용할 수 있도록 영상보정, 영상처리에 관련된 리턴값 저장
	en_Error_Cal_Coefficients g_en_CalCoefficients;
	en_Error_Img_Correction g_en_ImgCorrection;
	en_Error_Img_Processing g_en_ImgProcessing;
	en_Error_Door g_en_Door;
	en_Error_ScanMode g_enScanMode;
	//<공정툴 기본값> 2024-06-03 jg kim. 공정툴 기본값 추가
	int g_nMeasuredScanSpeed;
	// 2026 - 01 - 14 jg kim. 장비에 저장하는 것으로 변경
	//double g_fBackwaredOffset;
	//double g_fPositiningOffset; // 2025-06-17 jg kim. Positioning offet (for Size 0~2) 추가
	//int g_nEntryOffsetPixel2; // pixel unit. Size0 ~ Size2
	//int g_nEntryOffsetPixel3; // pixel unit. Size3
	 // 2024-06-13. jg kim. 시스템 에러 및 Notification 검출 및 전달 위함
	en_Error_System g_enSystemError;
	int g_nNotification;		// notification bit flag
	//en_ImageResolution g_enImgResolution; // 2024-04-24. jg kim. PortView에 해상도 정보를 저달하기 위해 추가
	int g_is_image_loaded;
	CString g_strmyIPs[32];
	stpvIMAGE_INFO_LIST current_pvIMAGE_INFO_list;
	HARDWARE_SETTING current_setting;
	int is_binary_saved;				// kiwa72(2022.11.26 23h) - 자료형 변경: BOOL -> int
	TEX img_tex;
	CStringA n2n_request_ip;
	CPtrList g_ptrClientSocketList;		//! 접속한 스캐너 소켓 리스트
	DWORD start_tick;
	DWORD end_tick;
	int kernel_size;
	CString g_sConnectDeviceName;		//! kiwa72(2023.04.23 11h) - 기존에 접속한 스캐너 정보(장비 이름, IP주소)
	CString g_sConnectDeviceAddr;
	float scanning_progress_tray;
	int nLastScannedRes;
	unsigned int broadcast_message;		// it was in common_type.cpp
	MAP<raw_t> map_origin;
	MAP<raw_t> map_raw;
	MAP<geo_t> map_geo;
	//MAP<raw_t> map_last_prep;
	MAP<fin_t> map_fin;
	MAP<raw_t> map_img16buffer;
	crx_pipe_class crx_pipe;
	int g_ipostprocess;
	int g_ipp_type;
	int g_iblur_radius;
	int g_iedge_enhance;
	int g_ibrightness_offset;			// 20210319 7000 -> 0
	int last_image_width;
	int last_image_height;
	int g_nConnected_ScannerCnt;		//! 접속한 스캐너의 개수

	// ROI settings from ini file
	int g_nROI_x;
	int g_nROI_y;
	int g_nROI_w;
	int g_nROI_h;

	PREDIX_NORMAL_CALLBACK self_close;					// 이전 cruxcan_trayDlg.cpp의 void self_close() 함수 대체용.
	PREDIX_ONEINT_CALLBACK notify_status_changed;		// 이전 cruxcan_trayDlg.cpp의 void ChangeTrayIcon(int) 함수 대체용.
	PREDIX_do_something do_something;					// bool do_something(...) 대체.
	PREDIX_ONEINT_CALLBACK set_progressbar_position;	// void set_progressbar_position(int) 대체.
	PREDIX_ONEINT_CALLBACK notify_progress_changed;		// TODO: Makeit - set_progressbar_position 와 함께 리팩토링 대상임.
	PREDIX_NORMAL_CALLBACK check_iniFile;				// void check_iniFile() 대체.
	PREDIX_NORMAL_CALLBACK clear_imagectrl;				// void clear_imagectrl() 대체.
	PREDIX_NORMAL_CALLBACK command_clear_stage;			// void ClearStage() 대체.
	PREDIX_NORMAL_CALLBACK command_refresh_image;		// void g_refreshIamge() 대체.
	PREDIX_NORMAL_CALLBACK close_window_via_traydlg;
	PREDIX_GetSaveFilename GetSaveFilenameFromUI;
	PREDIX_LPCWSTR command_LoadTiffToStage;


	//! kiwa72(2023.08.30 11h) - 버튼 선택: [Connect], [Reset Connect]
	BOOL g_bSel_Btn_Connect;
	BOOL g_bSel_Btn_ResetConnect;
	//!...
	
	//! kiwa72(2023.08.30 11h) - 현재 접속중인 장비 정보
	CRX_N2N_DEVICE g_CurrentConnectionDevice;
	//!...


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private methods
private:
	// initialize internal members
	void Initialize(void);

	// release internal members
	void ReleaseData(void);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private members
private:


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private constructor and destructor
private:
	// constructor
	global_data();

	// destructor
	~global_data();
};

#define GetPdxData()		global_data::GetInstance()


const char* button_scanner_msgs[];
