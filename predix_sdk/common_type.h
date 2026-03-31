// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "cruxcan_sdk.h"
#include "cruxcan_defines.h"
#include "crx_pipe_class.h"
#include <cstdarg>
#include <vector>


#define MYPLOGDG(...)		UNREFERENCED_PARAMETER(__VA_ARGS__)
#define MYPLOGDN(...)		UNREFERENCED_PARAMETER(__VA_ARGS__)
#define MYPLOGMA(...)		UNREFERENCED_PARAMETER(__VA_ARGS__)
#define MYPLOGMW(...)		UNREFERENCED_PARAMETER(__VA_ARGS__)
#define MYPLOGDGI(...)		UNREFERENCED_PARAMETER(__VA_ARGS__)

#define MYLOGOLDA(...)
#define MYLOGOLDW(...)

#define USE_UDP_CRC_CHECK		(1)
#define BLOCK_SIZE				(512)
#define HARDWARE_SETTING_SIZE	BLOCK_SIZE	//(BLOCK_SIZE * 8)	// kiwa72(2023.01.21 18h) - New Calibration Data
#define UDP_BLOCK_SIZE			BLOCK_SIZE
#define TCP_COMMAND_SIZE		BLOCK_SIZE
#define MAX_PACKETLEN			(1024 * 1024 * 16)

#define WM_CRX_AUTO_CLOSE		(WM_USER + 638)


enum enum_obl_code
{
	eoc_cruxell = 0,
	eoc_genoray = 13,
	eoc_null = 255
};

enum enum_save_image_step
{
	SaveImageNone = 0,
	SaveImageCorrectionStep,
	SaveImageProcessingStep,
	SaveBothProcessingStep
};

// 2024-04-22. jg kim. 영상보정, 영상처리에 관련된 리턴값 정의
enum en_Error_Cal_Coefficients
{
	Cal_DefaultCoefficients = 0,// 에러메시지
								// 캘리브레이션 파라미터가 유효하지 않아 기본값을 사용합니다. 당사에 연락하여 서비스 담당자의 지시에 따르십시오. 
								// The calibration parameters are invalid and default values are used. Contact GENORAY and follow the instructions provided.
	Cal_ValidCoefficients = 1	// 유효한 값
};

enum en_Error_Img_Correction
{
	IC_NotCorrected = 0,		// 영상보정을 하지 않았음.
	IC_Success = 1				// 정상처리
};

enum en_Error_Img_Processing
{
	IP_NotProcessed = 0,		// 영상처리를 하지 않았음.
	IP_Success = 1				// 정상처리
};

enum en_Error_Door
{
	HW_DoorNotClosed = 0,		// 에러메시지
								// PSP 투입구 커버가 닫히지 않았습니다. 당사에 연락하여 서비스 담당자의 지시에 따르십시오. 
								// The PSP inlet cover is not closed. Contact GENORAY and follow the instructions provided.
	HW_DoorClosed = 1			// 정상상태
};

enum en_Error_ScanMode
{
	HW_InvalidScanMode = 0,		// 에러메시지
								// 스캔 정보를 수신하지 못했습니다. 당사에 연락하여 서비스 담당자의 지시에 따르십시오.
								// No scan information was received. Contact GENORAY and follow the instructions provided.
	HW_ValidScanMode = 1		// 정상 스캔모드
};

 // 2024-06-13. jg kim. 시스템 에러 및 Notification 처리를 위함
enum en_Error_System
{
	Error_FLAG_0 = 0,	// SD 카드 초기화에 실패했습니다.
	Error_FLAG_1,		// 메모리 초기화 실패했습니다.
	Error_FLAG_2,		// 센서 초기화 실패했습니다.
	Error_FLAG_3,		// LCD 초기화에 실패했습니다.
	Error_FLAG_4,		// 터치 스크린 초기화에 실패했습니다.
	Error_FLAG_5,		// IR 센서 초기화에 실패했습니다.
	Error_FLAG_6,		// 이더넷 초기화에 실패했습니다.
	Error_FLAG_7,		// GUI 로드에 실패했습니다.
	Error_FLAG_8,		// GUI 초기화에 실패했습니다.
	Error_FLAG_9,		// 펌웨어 로딩에 실패했습니다.
	Error_FLAG_10,		// 스캔 모듈이 활성화되지 않았습니다.
	Error_FLAG_11,		// 펌웨어 확인 필요
	Error_FLAG_12,		// 파일을 열지 못했습니다.
	Error_FLAG_13,		// 파일을 닫지 못했습니다.
	Error_FLAG_14,		// FPGA 이미지 정보를 로드하지 못했습니다.
	Error_FLAG_15,		// timeout Tray Home Position
	Error_FLAG_16,		// timeout Tray Extra Position
	Error_FLAG_17,		// timeout Tray Backward Position
	Error_FLAG_18,		// timeout BLDC Stabilization
	Error_FLAG_19,		// timeout FPGA Scan Ready
	Error_FLAG_20,		// timeout Scan Entering
	Error_FLAG_21,		// timeout Scanning Read FIFO
	Error_FLAG_22,		// timeout Erase Entry Complete
	Error_FLAG_23,		// timeout IP Erase Complete
	Error_FLAG_24,		// timeout Wakeup Go Parking
	Error_FLAG_25,		// timeout Wakeup Parked
	Error_FLAG_26,		// timeout Wakeup UnParking
	Error_FLAG_27,		// fail FW Downloading
	Error_FLAG_Resetted = 65535 // Initial state. No error state
};

// bit flag : g_nNotification
#define No_Image		0x01		// 저장된 영상이 없습니다.
#define Low_XRAY		0x02		// X선 에너지 부족
#define DOOR_OPEN		0x04		// PSP 투입구 커버가 닫히지 않았습니다.
#define Notification_Resetted 0

// 2024-04-24. jg kim. PortView에 해상도 정보를 전달하기 위해 추가
// IMAGE_RESOLUTION::eir_HiRes 값!
enum en_ImageResolution
{
	// 2024-05-02. jg kim. cruxcan_sdk.h에 정의된 IMAGE_RESOLUTION과 순서가 맞도록 변경
	IR_HighResolution = 0,		// 고해상도. 25 um (0.025 mm)
	IR_LowResolution = 1		// 저해상도. 50 um (0.050 mm)
};

void dpf(const char* format, ...);
void dpc(const char* format, ...);


AFX_EXT_API void check_working_path();


enum REGISTER_MAP
{
	ADD_BOARD_VER				= 0x01,
	ADD_MAJOR_LOGIC_VER			= 0x02,
	ADD_MINOR_LOGIC_VER			= 0x03,
	ADD_FIFO_CLR				= 0x04,
	ADD_FIFO_USEDW				= 0x05,
	ADD_FIFO_TEST_SPEED			= 0x06,
	ADD_FIFO_TEST_RUN			= 0x07,
	ADD_SM_RESET				= 0x08,
	ADD_BLDC_RUN				= 0x09,
	ADD_BLDC_SPEED				= 0x0A,
	ADD_BLDC_RPM				= 0x0B,
	ADD_LASER_PWR_ON			= 0x0C,
	ADD_LD_ONOFF_ENABLE			= 0x0D,
	ADD_MPPC_CH0_BIAS_VALUE		= 0x0E,
	ADD_MPPC_CH1_BIAS_VALUE		= 0x0F,
	ADD_MPPC_CH2_BIAS_VALUE		= 0x10,
	ADD_MPPC_CH0_ON				= 0x11,	// 20220530 shutdonw -> on renaming
	ADD_MPPC_CH1_ON				= 0x12,
	ADD_MPPC_CH2_ON				= 0x13,
	ADD_SCAN_RUN				= 0x14,
	ADD_BUZZER_ON				= 0x15,
	//ADD_BUZZER_FREQ			= 0x16,	// not use 20220517
	ADD_BUZZER_TIME				= 0x17,
	ADD_SHIFT_INDEX				= 0x18,
	ADD_IS_SS_MODE				= 0x19,	// if 1 then  HR mode else   SR mode
	//ADD_SM_SCAN_SPD			= 0x1A,
	ADD_SCAN_SPEED				= 0x1A,
	ADD_MOVE_MC_VALUE			= 0x1B,
	ADD_SCAN_MC_VALUE			= 0x1C,
	//ADD_ST_ENTRY_TIME			= 0x1D,
	ADD_SCAN_ENTRY_TIME			= 0x1D,
	ADD_ST_ERASE_TIME			= 0x1E,
	//ADD_ST_ENTRY_ERASE_TIME	= 0x1F,
	//ADD_ST_ERASE_ENTRY_TIME	= 0x1F,
	ADD_ERASE_ENTRY_TIME		= 0x1F,
	ADD_TEMP_H_COUNT			= 0x20,
	ADD_TEMP_L_COUNT			= 0x21,
	ADD_DLY_PIXEL_VAL			= 0x22,
	ADD_SCAN_READY				= 0x23,
	ADD_SM_MOTOR_RUN			= 0x24,
	ADD_S_LIMIT_INT				= 0x25,
	ADD_S_HOME_INT				= 0x26,
	ADD_TCP_TRANSFER_DONE		= 0x27,
	ADD_BLDC_STABLE				= 0x28,
	ADD_ERASE_SPEED				= 0x29,
	ADD_STAGE_VALUE				= 0x2A,
	ADD_SLEEP_MODE				= 0x2B,
	ADD_LASER_POWER_WRITE		= 0x30,
	ADD_LASER_POWER_READ		= 0x31,
	ADD_MANUAL_SPEED			= 0x40,
	ADD_MANUAL_LENGTH			= 0x41,
	ADD_MANUAL_DIR				= 0x42,
	ADD_MANUAL_RUN				= 0x43,
	ADD_START_SPEED				= 0x44,
	ADD_NORMAL_SPEED			= 0x45,
	ADD_SM_STEP_VAL				= 0x46,
	ADD_SM_STEP_DUR				= 0x47,
	ADD_IP_POSITIONING_TIME		= 0x48,
	ADD_EJECT_TIME				= 0x49,
	ADD_PARKING_LENGTH			= 0x4A,
	ADD_BACKWARD_LENGTH			= 0x4B,
};


// Update: 20220905
typedef enum NETWORK_COMMAND
{
	nc_none							= 0,	// 예약값

	nc_set_register					= 1,	// 지정한 레지스터 어드레스에 값을 전달하기 위한
	nc_scan_start					= 2,	// 스캔 시작 명령
	nc_scan_stop					= 3,	// 스캔 중지 명령 - 현재 지원하지 않음
	nc_BLDC_onoff					= 13,	// BLDC  on/off
	nc_scan_image_size				= 31,	// 스캔시작시 or SW에서 이미지 요청시 이미지 정보 보냄
	nc_scan_image_send_fail			= 32,	// SW에서 요청한 이미지를 보내지 못하는 경우
	nc_image_list					= 33,	// SW에서 요청한 이미지 리스트에 대한 회신

	nc_scan_image_send_cancel		= 34,	// 개발용 명령 현재 - 사용하지 않음
	nc_scan_image_send_timeout		= 35,	// 개발용 명령 현재 - 사용하지 않음

	nc_image_list_with_name			= 36,	// SW에서 요청한 이미지 리스트에 대한 회신

	nc_ip_extraction_info			= 37,	// kiwa72(2023.01.21 18h) - ip 추출 위치 및 크기 정보 (startY, ipSizeX, ipSizeY) 미사용

	nc_bootloader_update			= 39,	// bootloader 바이너라 파일 전송시작 명령	kiwa72(2023.03.28 00h) - 추가
	nc_firmware_update				= 41,	// FW 업데이트용 바이너라 파일 전송시작 명령
	nc_fpga_update					= 42,	// FPGA 업데이트용 바이너라 파일 전송시작 명령
	nc_gui_update					= 43,	// GUI 업데이트용 바이너라 파일 전송시작 명령
	
	nc_firmware_update_only_save	= 44,	// FW 업데이트 테스트용 명령
	nc_fpga_update_only_save		= 45,	// FPGA 업데이트 테스트용 명령
	nc_gui_update_only_save			= 46,	// GUI 업데이트 테스트용 명령
	
	nc_request_image				= 61,	// SW에서 이미지를 요청하는 명령
	nc_request_image_list			= 62,	// SW에서 이미지리스트를 요청하는 명령 legacy
	nc_request_image_list_with_name	= 65,	// SW에서 이미지리스트를 요청하는 명령 최신
	
	nc_send_patient_name			= 66,	// SW 내부에서 사용함
	nc_tray_busy					= 67,	// SW단이 영상처리 등을 하여 FW단의 요청을 거부할때 사용

	nc_hardware_info				= 71,	// "최신 하드웨어 설정정보 전송 SW의 diagnostics화면에서 수정된 내용을 FW로  보내거나 SW의 요청으로 펌웨어서 보내줌"
	nc_request_hardware_info		= 72,	// SW에서 FW로 하드웨어 설정 정보 요청함
	nc_new_calib_hardware_info		= 73,	// kiwa72(2023.04.21 09h) - SW ->> FW | HARDWARE_SETTING[512] + 제노레이 요청 New Calibration Data[3584] = Total 4096 Byte
	nc_request_hardware_info2		= 74,	// kiwa72(2023.08.05 11h) - FW -> SW | HARDWARE_SETTING[512] + New_Calibration_Data[3584] = Total 4096 Byte

	nc_delete_all_image				= 81,	// FW에 저장된  raw영상들과 이미지리스트 삭제 요청

	nc_sleep_scanner				= 83,	// 개발용 명령 현재 사용하지 않음
	nc_resume_scanner				= 84,	// 개발용 명령 현재 사용하지 않음

	nc_pc_sleep						= 85,
	nc_text_update					= 86,	// 다국어파일 업데이트용 명령

	nc_HR_SR_mode_state				= 90, 	// 231212 added by JH		0: eir_HiRes,		1: eir_StRes
	nc_X_ray_Low_Pwer_state			= 91,	// 231212 added by JH		0: Normal, 			1: bXray_Low_Power
	nc_systemError					= 92,	// 240613 added by JH		내용은 data 체크할 것
	nc_Notification					= 93,	// 240613 added by JH		내용은 data 체크할 것


	// kiwa72(2023.05.01 11h) - NtoN 명령 추가
	nc_NtoN_UDP_Step01				= 111,	//  FW -> PC : 각 장비별 정보를 전달

	nc_n2n_device					= 123,	// n2n 환경에서 predix_cr이 predix_center로 브로드케스팅
	nc_n2n_center					= 124,	// n2n 환경에서 predix_center가 predix_cr로 브로드캐스팅

	nc_sdk_disconnected				= 125,	// SW 종료 알림 명령
	nc_threshold					= 127,	// 예약값(127 미만 SW FW간  사용, 128이상은 SW 내부 혹은 SW/FW 대상  특수명령)

	nc_tray							= 128,	// n2n 환경에서 swrk center로 보내는 명령혹은 데이터

	nc_pcname						= 129,	// SW가 실행중인 PC의 이름을 전송
	nc_scanner_state				= 130,	// SW 내부에서 현재 스캐너의 상태를 전송함
	nc_tray_progress				= 131,	// SW 내부에서 데이터 전송률 전달
	nc_tray_data					= 132,	// SW 내부에서 데이터 전송시
	nc_image_done					= 133,	// SW 내부에서  영상처리 완료 전달
	nc_tray_file_path				= 135,	// SW 내부에서 대용량 데이터 전송시 현재 사용하지 않음
	nc_scan_image_size_prep			= 136,	// SW 내부에서 영상처리 완료 상태 및 영상의 최종 크기(바이트) 전달
	nc_connect_scanner				= 148,	// n2모드에서 스캐너 연결 명령 SW 내부에서 사용함

	nc_tray_cancel					= 149,	// 연결취소 요청

	nc_chunk_saved					= 236,	// "SW에서  GUI 리소스와 같이 대용량 데이터  업로드시 일정 크기 단위로 전송하고  FW단에서 microSD에 기록할때까지 대기함,  FW단에서 microSD 저장이 완료되면 SW단에 알려서 업로드를 재개함"7
	nc_preupdate_check				= 237,	// SW단에서 FW에게  펌웨어 업데이트를 진행할지 문의함

	nc_LCD_onoff					= 238,	// 테스트용 LCD on/off 요청
	nc_touch_onoff					= 239,	// 테스트용 터치 on/off 요청

	nc_change_2_ftver				= 240,	// 펌웨어 업데이트 테스트용 임시명령1 - 재부팅시 공장 펌웨어로 부팅함
	nc_change_2_upver				= 241,	// 펌웨어 업데이트 테스트용 임시명령2 - 재부팅시 최신 펌웨어로 부팅함

	nc_bias_off						= 245,	// 테스트용 mppc bias off 명령
	nc_bias_on						= 246,	// 테스트용 mppc bias on 명령

	nc_binary_saved					= 247,	// FW에서 수신한 바이너리를 microSD에 저장완료 상태 전달
	nc_binary_update				= 248,	// microSD에 바이너리 전달 요청

	nc_test_command5				= 249,	//! kiwa72(2023.10.23 13h) - TCP 연결 상태 알림 처리

	nc_command_screen_capture = 250,	// 2023-11-24. jg kim. 개발용 테스트 명령4 에서 screen capture 명령으로 변경
										// 공정툴에서만 유지
	nc_current_time = 251,			// 2025-05-02. jg kim.  장비의 현재 시간을 요청하는 명령어 추가
	nc_system_reset					= 254,	// 펌웨어 소프트 리셋 명령

	nc_unknown						= 255	// 예약값

} NETWORK_COMMAND;


typedef enum LANGUAGE
{
	el_ENG,
	el_KOR,
	el_dummy = 0xFFFFFFFF,
} LANGUAGE;


typedef enum AUTO_SLEEP
{
	es_no_sleep,
	es_10min,
	es_30min,
	es_dummy = 0xFFFFFFFF,
} AUTO_SLEEP;


typedef enum NET_STATUS
{
	enet_none,
	enet_firmware_update,
	enet_FPGA_update,
	enet_ui_image_update,

} NET_STATUS;


typedef union UDIStruct
{
	char bytes[51];
	struct
	{
		char delim0[4];//"(01)"
		union
		{
			char gtin[14];
			struct
			{
				char const0[4];//"0880"
				char company[6];
				char type[4];
			};
		};
		char delim1[4];//"(10)"
		char promonth[4];//
		char packmonth[2];//
		char delim2[4];//"(11)"
		char model[3];
		char obl[3];
		char region;
		char perform;
		char delim3[4];//"(21)"
		char revision;
		char pn[5];
	};

} UDIStruct;


//! kiwa72(2023.04.26 14h) - 터치 보정 구조체 사용
typedef struct _touchdata_
{
	int	calibrated_checksum;
	unsigned short touchleft;
	unsigned short touchright;
	unsigned short touchtop;
	unsigned short touchbottom;

} stTouchData;


typedef struct _calibrationdata_
{
	// 32bit 64bit에서 문제없도록 8의배수가 되게 하자 

	unsigned short sigs[4];
	double  data[5];	
	unsigned short sige[4];

} stcalibration;

// 2024-06-25. jg kim. 공정툴에서 사용
typedef struct HardwareParameters
{
	HardwareParameters()
	{
		fMainClockFrequency = 100;//MHz. FPGA clock
		fMega = 1000000;
		fIdlerDiameter = 25;// mm unit
		fPI = 3.14159265358979323846;
		fPulsesPerRotation = 200;// puls
		fReductionRatio = 40;// 40 : 1
		fPulseDivision = 16;//
		fStepMotorPulsScale = 2;//
		fConstant = (fMainClockFrequency * fMega * fIdlerDiameter * fPI)/(fPulsesPerRotation * fReductionRatio * fPulseDivision * fStepMotorPulsScale);
		fHeightIP[0] = 31; // mm
		fHeightIP[1] = 40; // mm
		fHeightIP[2] = 41; // mm
		fHeightIP[3] = 54; // mm
		fWidthShade = 21; // mm
		fDistanceToScanLaser = 50.5; // mm. home_init sensor(photo sensor) to scan laser
		fDistanceToEraseLed = 12.15; // mm
		fDistanceToEject = 4.75; // mm // 2025-06-23. jg kim. 기구사양 확인 후 수정.
		fDistanceToBackward = 33; // mm
		fNormalSpeed=10.742; // mm / sec
		fScanSpeed=6; // mm / sec
		fEraseSpeed=8; // mm / sec
		fStartSpeed= 1.992; // mm / sec
		fMaxSpeed = 12.783; // mm / sec
		fStep=1000; // increase or decrease pulses per iteration
		fDuration=500; // * 1e-8 sec
		fScale=1;
	}
	double fMainClockFrequency;   // 100MHz. FPGA clock
	double fMega;
	double fIdlerDiameter;     // 25 mm unit
	double fPI;
	double fPulsesPerRotation;    // 200 puls
	double fReductionRatio;       // 40 : 1
	double fPulseDivision;         // 16
	double fStepMotorPulsScale;   // 2
	double fConstant;
	double fHeightIP[4];
	double fWidthShade;
	double fDistanceToScanLaser;
	double fDistanceToEraseLed; // Scan Stop후 역방향 진행시 거리
	double fDistanceToEject;
	double fDistanceToBackward;

	double fNormalSpeed;
	double fScanSpeed;
	double fEraseSpeed;
	double fStartSpeed;
	double fMaxSpeed;

	double fStep;
	double fDuration;
	double fScale;

}HwParam;

// kiwa72(2023.01.21 18h) - 제노레이 요청 New Calibration Data 추가 - 3240(Byte)
#pragma pack(push, 1)
typedef union __NewCalibrationData
{
	struct
	{
		unsigned short sigs[4]; // 2023.11.22 jg kim. sigs, sige 추가
		unsigned short data1[10];
		float data2[50];
		unsigned short sige[4];
	};	// 236 Byte

	unsigned char dParam[BLOCK_SIZE];	// 512 Byte (512 * 7)

	__NewCalibrationData()
	{
		memset(dParam, 0x0, sizeof(dParam));
	}

} NewCalibrationData;
#pragma pack(pop)


// 구조체 크기 1바이트 정렬시킴
#pragma pack(push, 1)
// kiwa72(2023.03.22 14h) - 불필요한 항목 제외, touch cal 설정값 추가
typedef union __HARDWARE_SETTING
{
	struct
	{
		unsigned short		Signature[4];		// 정상 데이터 체크용 SOF

		char				Name[20];			// 브랜드명
		char				Vendor[40];			// 제조사명
		char				Model[40];			// 제품모델명
		char				UDI[51];			// 제품 UDI

		char				reserved0[1];		// 예약 - 4바이트정렬 더미

		unsigned char		multilang_version[4];	// 다국어 파일 버전
		unsigned char		Firmware_version1[4];	// 펌웨어 버전(공장 초기화 버전)	kiwa72(2023.03.28 00h) - 
		unsigned char		FPGA_version1[4];		// FPGA 버전 내부 비교용	kiwa72(2023.03.28 00h) - 

		unsigned char		Bootloader_version2[4];	// 부트로더 버전	kiwa72(2023.03.28 00h) - 추가
		unsigned char		Firmware_version2[4];	// 펨웨어 버전(현재 버전)
		unsigned char		FPGA_version2[4];		// FPGA 버전	kiwa72(2023.03.28 00h) - 
		unsigned int		GUI_version2;			// GUI 버전이 없어 수신된 CRC32 값을 값는다.

		unsigned char		ipaddr[4];			// ip address
		unsigned char		netmask[4];			// sunet mask
		unsigned char		gateway[4];			// gateway
		unsigned char		MAC_ADDR[6];		// mac address

		unsigned char		reserved2[2];		// 예약 - 4바이트 정렬 더미

		LANGUAGE			language;			// 언어
		unsigned short		screen_saver_time;	// 스크린세이버시간
		unsigned short		sleep_time;			// 슬립시간
		IMAGE_RESOLUTION	image_resolution;	// 스캔 해상도	

		unsigned char		reserved3[3];		// 예약 - 4바이트 정렬 더미

		int					useDHCP;			// DHCP 사용여부
		unsigned int		BLDC_speed;			// BLDC 스피드값
		unsigned int		BLDC_shift_index;	// BLDC shit index 값: raw영상 치우침 정도
		unsigned int		scan_entry_time_1;	// size 1 ip 스캔 엔트리 타임
		unsigned int		scan_entry_time_2;	// size 2 ip 스캔 엔트리 타임

		unsigned int		reserved4[2];		// 예약

		unsigned int		scan_entry_time_0;	// size 0 ip 스캔 엔트리 타임
		unsigned int		erase_entry_time;	// 이레이저 엔트리 타임
		unsigned int		scan_entry_time_3;	// size 3 ip 스캔 엔트리 타임
		unsigned int		SM_speed;			// 스텝모터 스피드
		unsigned int		MPPC1_bias;			// MPPC 1 bias
		unsigned int		MPPC2_bias;			// MPPC 2 bias

		//		uint32_t	reserved5[3]	;	//	사용하지 않음
		unsigned int		eject_speed;	// 2025-07-03. jg kim. 기구요청으로 추가. FW_0.8.0.2B5 부터 동작
		unsigned int		reserved5[2];		//	예약

		unsigned int		LaserOnTime;		// 레이저 활성화 시간

		unsigned int		reserved6[6];		// 예약 - 스텝모터 타입, HW 따라 다른 동작이 필요한 경우를 상정한 예약값

		unsigned int		erase_speed;		// 이레이저 속도
		unsigned int		MPPC3_bias;			// MPPC 3 bias
		unsigned int		Scan_StartY;		// kiwa72(2023.01.21 18h) - 선택된 IP Scan StartY
		unsigned int		laser_power;		// 레이저 파워

#if (1)
		unsigned int		reserved7[3];		// 예약
#else
		//! kiwa72(2023.04.26 14h) - 터치 보정 구조체 사용으로 제거
		//// kiwa72(2023.04.19 13h) - 위치 변경
		//unsigned short	tsCal_Saved;
		//unsigned short	tsCal_X_org;
		//unsigned short	tsCal_X_limit;
		//unsigned short	tsCal_Y_org;
		//unsigned short	tsCal_Y_limit;

		unsigned short		reserved9[1];		// 예약 - 2바이트 정렬 더미
#endif

		unsigned int		scan_count;			// 누적 스캔 횟수
		unsigned int		selected_ip_size;	// 선택된 ip size
		int					start_speed;		// 스캔 시작 스피드
		int					normal_speed;		// 일반 스피드
		int					sm_step_val;		// 스텝모더 value
		int					sm_step_dur;		// 스텝모터 duration

		int					size0_positioning_time;	// size0 포지션 타임
		int					size1_positioning_time;	// size1 포지션 타임
		int					size2_positioning_time;	// size2 포지션 타임
		int					size3_positioning_time;	// size3 포지션 타임

		int					ip_eject_time;		// ip 배출 시간
		int					parking_length;		// 파킹 길이

#if (DEF_BACKWARD_OFFSET_LENGTH)
		// kiwa72(2022.12.06 00h) - Backward Offset Length
		unsigned int		Backward_Offset_Length;
#else
		unsigned int		reserved8[1];		// 사용하지 않음
#endif

		int					max_speed;			// 최대속도


		stTouchData			sttouch;			// 터치 calibration 값

		stcalibration		stcali;				// 이미지 Calibration 값

		char				host_name[12];		// n2n용  연결 PC 이름
		char				device_name[12];	// n2n용 스캐너 이름

		unsigned int		working_mode;		// update 체크 모드(예약값)

		unsigned int		bl_updated;			// 부트로드 update 상태값
		unsigned int		gui_updated;		// gui update 상태값
		unsigned int		fpga_updated;		// fpga 업데이트 상태값
		unsigned int		fw_updated;			// fw 업데이트 상태값

		SCANNER_STATE		state;				// 현재 스캐너 상태

#if (0)
		NewCalibrationData	NewCalib;
#endif
	};

	// kiwa72(2023.01.21 18h) - New Calibration Data
#if (0)
	unsigned int	 params[BLOCK_SIZE / 4];
#else
	unsigned int	 params[HARDWARE_SETTING_SIZE / 4];
#endif

	bool operator == (__HARDWARE_SETTING b)
	{
		for (int i = 0; i < sizeof(__HARDWARE_SETTING) / sizeof(int); i++)
		{
			if (this->params[i] != b.params[i])
			{
				return false;
			}
		}

		return true;
	}

	__HARDWARE_SETTING& operator = (const __HARDWARE_SETTING& r)
	{
		memcpy(this, &r, sizeof(__HARDWARE_SETTING));
		return *this;
	}

} HARDWARE_SETTING;
#pragma pack(pop)


//-------------------------------------------------------------------
// kiwa72(2023.04.21 09h) - 제노레이 요청 New Calibration Data
extern NewCalibrationData	g_tNewCalibrationData;
//-------------------------------------------------------------------


// HARDWARE_SETTING 제외하고 기본값 사용하도록 되돌림
typedef enum UPDATE_STATUS
{
	eupdate_factory = 1,
	eupdate_new,
	eupdate_under_sdcard,
	eupdate_under_flash,
	eupdate_under_confirm_ethernet,
	eupdate_under_confirm_wait1,
	eupdate_under_confirm_wait2,

} UPDATE_STATUS;


#ifndef define___crx_n2n_device__
#define define___crx_n2n_device__
// device는 최대 20개 
typedef union CRX_N2N_PROTOCOL
{
	struct
	{
		unsigned short	SignatureF[4];	// 2 * 4	- 0xFFFD, 1, 0xFFFB, command
		unsigned int	index;			// 4
		unsigned short	curpage;		// 2
		unsigned short	totalpage;		// 2
		unsigned char	version[4];		// 1 * 4
		int				device_count;	// 4
		CRX_N2N_DEVICE	devices[20];	// 24 * 20
		unsigned short	SignatureE[4];	// 2		- 0xFFFC 1 0xFFFE 0xFFFF
	};

	unsigned int params[128];	// 4 * 128 = 512 Byte

	int operator == (CRX_N2N_PROTOCOL &b)
	{
		for (int i = 0; i < sizeof(CRX_N2N_PROTOCOL) / sizeof(int); i++)
		{
			if (this->params[i] != b.params[i])
				return 0;
		}
		return 1;
	}

	CRX_N2N_PROTOCOL& operator=(const CRX_N2N_PROTOCOL &r)
	{
		memcpy(this, &r, sizeof(CRX_N2N_PROTOCOL));
		return *this;
	}

	void init(NETWORK_COMMAND nc)
	{
		memset(this, 0, sizeof(CRX_N2N_PROTOCOL));
		
		SignatureF[0] = 0xFFFD;	// 65533
		SignatureF[1] = 1;		// 1
		SignatureF[2] = 0xFFFB;	// 65531
		SignatureF[3] = (USHORT)nc;		// 124

		SignatureE[0] = 0xFFFC;	// 65532
		SignatureE[1] = 1;		// 1
		SignatureE[2] = 0xFFFE;	// 65534
		SignatureE[3] = 0xFFFF;	// 65535
	}

	void init()
	{
		init(nc_n2n_center);
	}

	void init_devices()
	{
		device_count = 0;
		memset(devices, 0, sizeof(CRX_N2N_DEVICE) * 20);
	}

} CRX_N2N_PROTOCOL;

extern CRX_N2N_PROTOCOL crx_n2n_protocol;
extern int is_valid_crx_n2n_protocol(CRX_N2N_PROTOCOL * b);
#endif


extern const HARDWARE_SETTING default_setting;
extern const HARDWARE_SETTING none_setting;
extern HARDWARE_SETTING current_setting;

extern stpvIMAGE_INFO_LIST current_pvIMAGE_INFO_list;
extern HWND main_hwnd;
extern CALLBACK_SET* callback_set1;
extern CALLBACK_SET2* callback_set2;
extern CALLBACK_SET3* callback_set3;
extern wchar_t image_path[1024];

extern unsigned char	validtcppacketsdk[TCP_COMMAND_SIZE];
extern unsigned char	validtcppacketdevice[TCP_COMMAND_SIZE];


#ifndef MY_QUEUE_SIZE
#define MY_QUEUE_SIZE (UDP_BLOCK_SIZE * 3)
#endif


typedef struct  __stmyqueue__
{
	unsigned char * qdata;
	int qhead;
	int qtail;
	int qcapacity;
	int qdatasize;
	int qfreesize;	//initial value  = qcapacity - 1;//1칸은  front와 rear가 겹쳐서  0인지  full인지 구분하지 못하는 사태를 막기 위해 안 채움

} stmyqueue;


extern stmyqueue  udpqueue;
extern stmyqueue  tcpqueuesdk;
extern stmyqueue  tcpqueuedevice;
extern unsigned char udp_q_data[MY_QUEUE_SIZE + 4];			// 1칸은 front와 rear가 겹쳐서  0인지  full인지 구분하지 못하는 사태를 막기 위해 안 채움 //데이터정렬하기 좋게 4바이트를 넣음 실제론 1바이트 필요
extern unsigned char tcp_q_data_sdk[MY_QUEUE_SIZE + 4];		// 1칸은 front와 rear가 겹쳐서  0인지  full인지 구분하지 못하는 사태를 막기 위해 안 채움 //데이터정렬하기 좋게 4바이트를 넣음 실제론 1바이트 필요
extern unsigned char tcp_q_data_device[MY_QUEUE_SIZE + 4];	// 1칸은 front와 rear가 겹쳐서  0인지  full인지 구분하지 못하는 사태를 막기 위해 안 채움 //데이터정렬하기 좋게 4바이트를 넣음 실제론 1바이트 필요
extern unsigned char crxtags[6];


#ifndef MAX_HOST_COUNT
//reserved
#define MAX_HOST_COUNT 32
#endif


#ifndef MYPLOG_MACRO
#define MYPLOG_MACRO

#define MYLOGOLDA(...)
#define MYLOGOLDW(...)
#endif


#define SDCARD_TRANSFER_SIZE	(512 * 30)
