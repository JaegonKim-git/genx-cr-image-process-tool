#pragma once
#include "IPredixCB.h"

#define SDK_FILE_NAME		_T("GenXCR_SDK.dll")
#ifndef WM_REFRESH_LIST
#define WM_REFRESH_LIST	(WM_USER + 1013)
#endif

static const  char*                       button_scanner_msgs[] =
{
"Disconnected",
"Waiting",
"Ready",
"Scanning",
"Transferring",
"Busy",
"System",
"Sleeped",//wjlee : 20220508 ver. Add
"None",//wjlee : Add
};

typedef enum  LANGUAGE
{
	el_ENG,
	el_KOR,
	el_dummy = 0xFFFFFFFF,
} LANGUAGE;

typedef enum  SCAN_MODE
{
	Mode_SDK,
	Mode_TWAIN
} SCAN_MODE;

//wjlee : 20220605 ver. modifyed
//typedef enum        IMAGE_RESOLUTION : unsigned char
//{
//	eir_SR = 0,
//	eir_HR = 1,
//} IMAGE_RESOLUTION;
typedef enum        IMAGE_RESOLUTION : unsigned char
//typedef enum        IMAGE_RESOLUTION : long
{
#if 0
	//20220525
	eir_SR = 0,
	eir_HR = 1,
#else
	eir_HiRes = 0,
	eir_StRes = 1,

#endif
} IMAGE_RESOLUTION;
//end

//wjlee : 20220508 ver. Add
typedef struct _touchdata_
{
	int	calibrated_checksum;
	unsigned short touchleft;
	unsigned short touchright;
	unsigned short touchtop;
	unsigned short touchbottom;
} stTouchData;
//end

typedef struct _calibrationdata_
{// 32bit 64bit에서 문제없도록 8의배수가 되게 하자 
	unsigned short sigs[4];
	double  data[5];
	unsigned short sige[4];

}stcalibration;

typedef enum NC_COMMAND
{
	nc_set_register = 1,
	nc_scan_start = 2,
	nc_scan_stop = 3,
	nc_BLDC_onoff = 13,
	//nc_SM_dir = 16,	//wjlee : 20220913 ver. delete
	//nc_SM_onoff = 17,	//wjlee : 20220913 ver. delete
	//nc_eraser_onoff = 22,	//wjlee : 20220913 ver. delete
	nc_request_image = 61,
	//nc_request_log = 63,	//wjlee : 20220913 ver. delete
	nc_request_hardware_info = 72,
	nc_LCD_onoff = 238,
	nc_touch_onoff = 239,//wjlee : 20220508 ver. modifyed - nc_find_belt_start = 239,
	nc_change_2_ftver = 240,//wjlee : 20220508 ver. modifyed - nc_move_attach = 240,
	nc_change_2_upver = 241,//wjlee : 20220508 ver. modifyed - nc_updated_failed = 241,
	//nc_display_temper = 244,	//wjlee : 20220913 ver. delete
	nc_bias_off = 245,
	nc_bias_on = 246,
	nc_test_command4 = 250,
	nc_unknown = 255
} NC_COMMAND;

#pragma pack(push, 1)
//wjlee : 20220802 ver. modify
/*typedef     union   HARDWARE_SETTING
{
	struct {
		unsigned short  Signature[4];           //2
		char            Name[20];               //7

		char            Vendor[40];
		char            Model[40];              //27

		char            UDI[51];
		char            reserved1[1];           //40

		//unsigned char   HardWare_version[4];//wjlee : 20220623 ver. delete
		unsigned char   multilang_version[4];//20220620	//wjlee : 20220623 ver. add
		unsigned char   Firmware_version1[4];
		unsigned char   FPGA_version1[4];
		unsigned char   UI_version1[4];
		unsigned char   Firmware_version2[4];
		unsigned char   FPGA_version2[4];
		unsigned char   UI_version2[4];         //47

		unsigned char   ipaddr[4];
		unsigned char   netmask[4];
		unsigned char   gateway[4];
		unsigned char   MAC_ADDR[6];
		unsigned char   reserved2[2];           //52

		LANGUAGE        language;
		unsigned short  screen_saver_time;//wjlee : 20220508 ver. modifyed
		unsigned short	sleep_time;
		IMAGE_RESOLUTION image_resolution;// 1byte -> 4byte 변경
		unsigned char	reserved9[3];// 3byte
		int             useDHCP;                //56

		unsigned int    BLDC_speed;
		unsigned int    BLDC_shift_index;

		unsigned int    SM_move_mc_value;
		unsigned int    SM_scan_mc_value;

		unsigned int    buzzer_freq;//SM_init_stop_pos;//20210928
		unsigned int    buzzer_time;//size_0_offset;//20210928
		unsigned int    stage_entry_time;//gs_length_th_size_0;//20210928
		unsigned int    erase_entry_time;//gs_length_th_size_3;//20210928


		unsigned int    eraser_on_time;

		unsigned int    SM_speed;
		unsigned int    MPPC1_bias;
		unsigned int    MPPC2_bias;

		unsigned int    LCD_type;
		unsigned int    BLDC_type;
		unsigned int    laser_type;
		unsigned int	LaserOnTime;
		unsigned int    SM_type;
		unsigned int    belt_type;
		unsigned int    MPPC_type;
		unsigned int    eraser_type;
		unsigned int	reserved_type1;// reserved_type1;//gate_sensor_type 77
		int             reserved_type2;          //78 
		unsigned int	erase_speed;//gate_sensor_sensitivity_1
		unsigned int    MPPC3_bias;
		unsigned int    legacy_value2;//MPPC4_bias
		unsigned int 	laser_power;//wjlee : 20220508 ver. modifyed - legacy_value3;//gate_sensor_sensitivity_2;
		unsigned int    sleep_mode;
		int             temp_sensor;
		int				current_push_image;
		int             reserved3[31 - (sizeof(stcalibration) / sizeof(int))];          //20210506     
		stcalibration   stcali;// int 4개 분량 // 20210509 64bit머신에서 문제될수있어서 순서 주의하고  새로 추가할때마다 잘 확인할 것
		char			host_name[12];//20200624
		char			device_name[12];//20200624
		unsigned int	working_mode;
		unsigned int	bl_updated;
		unsigned int    gui_updated;	//read only
		unsigned int    fpga_updated;	//read only
		unsigned int    fw_updated;	//read only
		TRAY_CB_SCANNER_STATE	state;	////read only
	};
	unsigned int params[128];
	bool operator==(HARDWARE_SETTING b) {
		for (int i = 0; i < sizeof(HARDWARE_SETTING) / sizeof(int); i++)
			if (this->params[i] != b.params[i])return false;
		return true;
	}
	HARDWARE_SETTING& operator=(const HARDWARE_SETTING& r)
	{
		memcpy(this, &r, sizeof(HARDWARE_SETTING));
		return *this;
	}
}HARDWARE_SETTING;*/
typedef     union   HARDWARE_SETTING
{
	struct {
		unsigned short  Signature[4];           //2
		char            Name[20];               //7

		char            Vendor[40];
		char            Model[40];              //27

		char            UDI[51];
		char            reserved1[1];           //40

		//unsigned char   HardWare_version[4];
		unsigned char   multilang_version[4];//20220620
		unsigned char   Firmware_version1[4];
		unsigned char   FPGA_version1[4];
		unsigned char   UI_version1[4];
		unsigned char   Firmware_version2[4];
		unsigned char   FPGA_version2[4];
		unsigned char   UI_version2[4];         //47

		unsigned char   ipaddr[4];
		unsigned char   netmask[4];
		unsigned char   gateway[4];
		unsigned char   MAC_ADDR[6];
		unsigned char   reserved2[2];           //52

		LANGUAGE        language;
		unsigned short  screen_saver_time;
		unsigned short	sleep_time;
		IMAGE_RESOLUTION image_resolution;// 1byte -> 4byte 변경
		unsigned char	reserved9[3];// 3byte
		int             useDHCP;                //56

		unsigned int    BLDC_speed;
		unsigned int    BLDC_shift_index;

		//unsigned int    SM_move_mc_value;
		//unsigned int    SM_scan_mc_value;
		//unsigned int	reservedA[2];
		unsigned int    scan_entry_time_1;//gs_length_th_size_0;//20210928//scan_entry_time
		unsigned int    scan_entry_time_2;//gs_length_th_size_0;//20210928//scan_entry_time

		unsigned int    buzzer_freq;//SM_init_stop_pos;//20210928
		unsigned int    buzzer_time;//size_0_offset;//20210928
		unsigned int    scan_entry_time_0;//gs_length_th_size_0;//20210928//scan_entry_time
		unsigned int    erase_entry_time;//gs_length_th_size_3;//20210928


		//unsigned int    eraser_on_time;
		//unsigned int	reservedB[1];
		unsigned int    scan_entry_time_3;//gs_length_th_size_0;//20210928//scan_entry_time
		unsigned int    SM_speed;
		unsigned int    MPPC1_bias;
		unsigned int    MPPC2_bias;

		unsigned int    LCD_type;
		unsigned int    BLDC_type;
		unsigned int    laser_type;
		unsigned int	LaserOnTime;
		unsigned int    SM_type;
		unsigned int    belt_type;
		unsigned int    MPPC_type;
		unsigned int    eraser_type;
		unsigned int	reserved_type1;// reserved_type1;//gate_sensor_type 77
		int             reserved_type2;          //78 
		unsigned int	erase_speed;//gate_sensor_sensitivity_1
		unsigned int    MPPC3_bias;
		unsigned int    legacy_value2;//MPPC4_bias
		unsigned int 	laser_power;// legacy_value3;//gate_sensor_sensitivity_2;
		unsigned int    sleep_mode;
		int             temp_sensor;
		int				current_push_image;
		unsigned int	scan_count;
		unsigned int	selected_ip_size;
		int				start_speed;//20220718
		int				normal_speed;//20220718
		int				sm_step_val;//20220718
		int				sm_step_dur;//20220718
		int				size0_positioning_time;//20220718
		int				size1_positioning_time;//20220718
		int				size2_positioning_time;//20220718
		int				size3_positioning_time;//20220718
//		int				ip_positioning_time;//20220718
		int				ip_eject_time;//20220718
		int				parking_length;//20220718
		int				backward_length;//20220718
		int				max_speed;//20220731

//		int             reserved3[31 - ((sizeof(stcalibration)/sizeof(int)) + 1 +(sizeof(stTouchData) / sizeof(int))/*scan_count*/ + 1 + 12)];          //20210506     
		//int             reserved3[31 - (14 + 1 + 3 + 1 + 11)];//20220715
		stTouchData		sttouch;
		stcalibration   stcali;// int 4개 분량 // 20210509 64bit머신에서 문제될수있어서 순서 주의하고  새로 추가할때마다 잘 확인할 것
		char			host_name[12];//20200624
		char			device_name[12];//20200624
		unsigned int	working_mode;
		unsigned int	bl_updated;
		unsigned int    gui_updated;	//read only
		unsigned int    fpga_updated;	//read only
		unsigned int    fw_updated;	//read only
		TRAY_CB_SCANNER_STATE	state;	////read only
	};
	unsigned int params[128];
	bool operator==(HARDWARE_SETTING b) {
		for (int i = 0; i < sizeof(HARDWARE_SETTING) / sizeof(int); i++)
			if (this->params[i] != b.params[i])return false;
		return true;
	}
	HARDWARE_SETTING& operator=(const HARDWARE_SETTING& r)
	{
		memcpy(this, &r, sizeof(HARDWARE_SETTING));
		return *this;
	}
}HARDWARE_SETTING;
//end
#pragma pack(pop)

// IP Size ( 0, 1, 2, 3 )
typedef enum        IP_SIZE : long
{
	eis_None = -1,
	eis_IP0 = 0,
	eis_IP1 = 1,
	eis_IP2 = 2,
	eis_IP3 = 3,
	eis_IPEND = 0x7FFFFFFF /* 컴파일러 관계없이 4바이트로 강제하기 위해 추가됨*///wjlee : 20211129 ver. added
} IP_SIZE;

// Image List
//wjlee : 20211129 ver. modifyed
//#define MAX_IMAGE_SIZE 500
//#define MAX_NAME_SIZE 50
#define MAX_IMAGE_COUNT 500
#define MAX_NAME_LENGTH ( 128 -  ( 8 + 1 + 4+ 4))  /*20211028  메모리 넣고 빼기 좋게 구조체를 128바이트로 만듬  */
typedef struct IMAGE_LIST
{
	//wjlee : 20211129 ver. modifyed
	//	unsigned int cur_image_num;                     //lastest image index
	//	double image_date[MAX_IMAGE_SIZE];              //image date
	//	IMAGE_RESOLUTION image_mode[MAX_IMAGE_SIZE];    //image resolution
	//	char name[MAX_IMAGE_SIZE][MAX_NAME_SIZE];       //patient name
	//	unsigned int name_index[MAX_IMAGE_SIZE];        //patient id
	//	IP_SIZE ip_size[MAX_IMAGE_SIZE];                //IP size
	unsigned int cur_image_num;                     //lastest image index
	double image_date[MAX_IMAGE_COUNT];              //image date
	IMAGE_RESOLUTION image_mode[MAX_IMAGE_COUNT];    //image resolution
	char name[MAX_IMAGE_COUNT][MAX_NAME_LENGTH];       //patient name
	unsigned int name_index[MAX_IMAGE_COUNT];        //patient id
	IP_SIZE ip_size[MAX_IMAGE_COUNT];                //IP size
}IMAGE_LIST;

//wjlee : 20211129 ver. added
#pragma pack(push, 1)
typedef struct stIMAGE_INFO
{
	double image_date;//8
	unsigned int name_index;//4
	IP_SIZE ip_size;//4
	IMAGE_RESOLUTION image_resolution;//1
	char name[MAX_NAME_LENGTH];//
} stIMAGE_INFO;
#pragma pack(pop)

//wjlee : 20211129 ver. added
typedef struct stIMAGE_INFO_LIST
{
	unsigned int cur_image_num;
	unsigned int dummy[128 - 1/*cur_image_num*/];
	stIMAGE_INFO  imageinfos[MAX_IMAGE_COUNT];
}stIMAGE_INFO_LIST;

//////////////////////////////////////////////////////////////////////////////////////////
//Interface fn
//////////////////////////////////////////////////////////////////////////////////////////

//Callback
typedef void(cdecl* PsSetCallback)(CTrayCallback *pCB);
static PsSetCallback PsSetCallbackFn;

//Ini
typedef char*(cdecl* PsGetValuefromIni)(char* szSection, char* szKey, STRING_CASE_TYPE type);
static PsGetValuefromIni PsGetValuefromIniFn;
typedef bool(cdecl* PsSetValuetoIni)(char* szSection, char* szKey, char* szValue);
static PsSetValuetoIni PsSetValuetoIniFn;

//Tray control
//typedef void(cdecl* PsReadytoClose)(VOID);
//static PsReadytoClose PsReadytoCloseFn;
typedef int(cdecl* PsGetEthernetInfo)(std::vector<ST_AdapterInfo*>& vAdapterInfoss);
static PsGetEthernetInfo PsGetEthernetInfoFn;
typedef bool(cdecl* PsSetHardwareInfo)(HARDWARE_SETTING setting);
static PsSetHardwareInfo PsSetHardwareInfoFn;
typedef bool(cdecl* PsRequestHardwareInfo)(VOID);
static PsRequestHardwareInfo PsRequestHardwareInfoFn;
typedef int(cdecl* PsGetPostProcessType)(VOID);
static PsGetPostProcessType PsGetPostProcessTypeFn;
typedef bool(cdecl* PsSetPostProcessType)(int nProcessType);
static PsSetPostProcessType PsSetPostProcessTypeFn;


typedef void(cdecl* PsCalibrationInit)(VOID);
static PsCalibrationInit PsCalibrationInitFn;
typedef void(cdecl* PsSetCalibrationArea)(int nValidWidth, int nValidHeight, bool bUpdateValidAreatoImage);
static PsSetCalibrationArea PsSetCalibrationAreaFn;
typedef bool(cdecl* PsCheckScanImageFile)(char* szFilename);
static PsCheckScanImageFile PsCheckScanImageFileFn;
typedef CALIBRATION_INFO*(cdecl* PsCheckImageShift)(char* szFilename);
static PsCheckImageShift PsCheckImageShiftFn;
typedef void(cdecl* PsApplyCalibrationFile)(int nImageTotalCount);
static PsApplyCalibrationFile PsApplyCalibrationFileFn;
typedef char*(cdecl* PsCalcCalibrationData)(int& nImageTotalCount, bool bDefinedArea);
static PsCalcCalibrationData PsCalcCalibrationDataFn;
typedef bool(cdecl* PsSaveCalibrationData)(VOID);
static PsSaveCalibrationData PsSaveCalibrationDataFn;
typedef int(cdecl* PsCalibrationDataSavetoScanner)(VOID);
static PsCalibrationDataSavetoScanner PsCalibrationDataSavetoScannerFn;
typedef void(cdecl* PsCalibrationDataReset)(VOID);
static PsCalibrationDataReset PsCalibrationDataResetFn;
typedef bool(cdecl* PsSendPatienNameByTray)(const char * name);
static PsSendPatienNameByTray PsSendPatienNameByTrayFn;
typedef void(cdecl* PsSetSaveFileName)(const char * name);
static PsSetSaveFileName PsSetSaveFileNameFn;
typedef void(cdecl* PsSetScanModeTray)(int nMod);
static PsSetScanModeTray PsSetScanModeTrayFn;



//Scan control
typedef bool(cdecl* PsConnectScanner)(VOID);
static PsConnectScanner PsConnectScannerFn;
typedef bool(cdecl* PsDisConnectScanner)(VOID);
static PsDisConnectScanner PsDisConnectScannerFn;
typedef bool(cdecl* PsGetLastImage)(VOID);
static PsGetLastImage PsGetLastImageFn;
typedef bool(cdecl* PsDeleteImageAll)(VOID);
static PsDeleteImageAll PsDeleteImageAllFn;
typedef bool(cdecl* PsScannerReset)(VOID);
static PsScannerReset PsScannerResetFn;
typedef bool(cdecl* PsFWUpdate)(int UpdateType, char* szPathName, char* szFileName);
static PsFWUpdate PsFWUpdateFn;
typedef bool(cdecl* PsRequestImageList)();
static PsRequestImageList PsRequestImageListFn;
typedef bool(cdecl* PsReleaseTray)(VOID);
static PsReleaseTray PsReleaseTrayFn;
typedef bool(cdecl* PsInitializeTray)(VOID);
static PsInitializeTray PsInitializeTrayFn;
typedef bool(cdecl* PsSendProtocolToDevice)(int command, int nShiftValue, int nSubCommand);
static PsSendProtocolToDevice PsSendProtocolToDeviceFn;

typedef long(cdecl* PsConnectScannerN2N)(char* DeviceIp);
static PsConnectScannerN2N PsConnectScannerN2NFn;
typedef long(cdecl* PsDisConnectScannerN2N)(VOID);
static PsDisConnectScannerN2N PsDisConnectScannerN2NFn;
//////////////////////////////////////////////////////////////////////////////////////////





//////////////////////////////////////////////////////////////////////////////////////////
static HINSTANCE m_hScanInst = NULL;
static bool LoadTrayFn()
{
	if (m_hScanInst == NULL)
		return false;

	PsSetCallbackFn = (PsSetCallback)GetProcAddress(m_hScanInst, "PsSetCallback");
	if (PsSetCallbackFn == NULL)
		return false;


	PsGetValuefromIniFn = (PsGetValuefromIni)GetProcAddress(m_hScanInst, "PsGetValuefromIni");
	if (PsGetValuefromIniFn == NULL)
		return false;
	PsSetValuetoIniFn = (PsSetValuetoIni)GetProcAddress(m_hScanInst, "PsSetValuetoIni");
	if (PsSetValuetoIniFn == NULL)
		return false;


	/*PsReadytoCloseFn = (PsReadytoClose)GetProcAddress(m_hScanInst, "PsReadytoClose");
	if (PsReadytoCloseFn == nullptr)
		return false;*/
	PsGetEthernetInfoFn = (PsGetEthernetInfo)GetProcAddress(m_hScanInst, "PsGetEthernetInfo");
	if (PsGetEthernetInfoFn == NULL)
		return false;
	PsSetHardwareInfoFn = (PsSetHardwareInfo)GetProcAddress(m_hScanInst, "PsSetHardwareInfo");
	if (PsSetHardwareInfoFn == NULL)
		return false;
	PsRequestHardwareInfoFn = (PsRequestHardwareInfo)GetProcAddress(m_hScanInst, "PsRequestHardwareInfo");
	if (PsRequestHardwareInfoFn == NULL)
		return false;
	PsGetPostProcessTypeFn = (PsGetPostProcessType)GetProcAddress(m_hScanInst, "PsGetPostProcessType");
	if (PsGetPostProcessTypeFn == NULL)
		return false;
	PsSetPostProcessTypeFn = (PsSetPostProcessType)GetProcAddress(m_hScanInst, "PsSetPostProcessType");
	if (PsSetPostProcessTypeFn == NULL)
		return false;

	PsCalibrationInitFn = (PsCalibrationInit)GetProcAddress(m_hScanInst, "PsCalibrationInit");
	if (PsCalibrationInitFn == NULL)
		return false;
	PsSetCalibrationAreaFn = (PsSetCalibrationArea)GetProcAddress(m_hScanInst, "PsSetCalibrationArea");
	if (PsSetCalibrationAreaFn == NULL)
		return false;
	PsCheckScanImageFileFn = (PsCheckScanImageFile)GetProcAddress(m_hScanInst, "PsCheckScanImageFile");
	if (PsCheckScanImageFileFn == NULL)
		return false;
	PsCheckImageShiftFn = (PsCheckImageShift)GetProcAddress(m_hScanInst, "PsCheckImageShift");
	if (PsCheckImageShiftFn == NULL)
		return false;
	PsApplyCalibrationFileFn = (PsApplyCalibrationFile)GetProcAddress(m_hScanInst, "PsApplyCalibrationFile");
	if (PsApplyCalibrationFileFn == NULL)
		return false;
	PsCalcCalibrationDataFn = (PsCalcCalibrationData)GetProcAddress(m_hScanInst, "PsCalcCalibrationData");
	if (PsCalcCalibrationDataFn == NULL)
		return false;
	PsSaveCalibrationDataFn = (PsSaveCalibrationData)GetProcAddress(m_hScanInst, "PsSaveCalibrationData");
	if (PsSaveCalibrationDataFn == NULL)
		return false;
	PsCalibrationDataSavetoScannerFn = (PsCalibrationDataSavetoScanner)GetProcAddress(m_hScanInst, "PsCalibrationDataSavetoScanner");
	if (PsCalibrationDataSavetoScannerFn == NULL)
		return false;
	PsCalibrationDataResetFn = (PsCalibrationDataReset)GetProcAddress(m_hScanInst, "PsCalibrationDataReset");
	if (PsCalibrationDataResetFn == NULL)
		return false;
	PsSendPatienNameByTrayFn = (PsSendPatienNameByTray)GetProcAddress(m_hScanInst, "PsSendPatienNameByTray");
	if (PsSendPatienNameByTrayFn == NULL)
		return false;
	PsSetSaveFileNameFn = (PsSetSaveFileName)GetProcAddress(m_hScanInst, "PsSetSaveFileName");
	if (PsSetSaveFileNameFn == NULL)
		return false;
	PsRequestImageListFn = (PsRequestImageList)GetProcAddress(m_hScanInst, "PsRequestImageList");
	if (PsRequestImageListFn == NULL)
		return false;
	PsSetScanModeTrayFn = (PsSetScanModeTray)GetProcAddress(m_hScanInst, "PsSetScanModeTray");
	if (PsSetScanModeTrayFn == NULL)
		return false;
	
	

	PsConnectScannerFn = (PsConnectScanner)GetProcAddress(m_hScanInst, "PsConnectScanner");
	if (PsConnectScannerFn == NULL)
		return false;
	PsDisConnectScannerFn = (PsDisConnectScanner)GetProcAddress(m_hScanInst, "PsDisConnectScanner");
	if (PsDisConnectScannerFn == NULL)
		return false;
	PsGetLastImageFn = (PsGetLastImage)GetProcAddress(m_hScanInst, "PsGetLastImage");
	if (PsGetLastImageFn == NULL)
		return false;
	PsDeleteImageAllFn = (PsDeleteImageAll)GetProcAddress(m_hScanInst, "PsDeleteImageAll");
	if (PsDeleteImageAllFn == NULL)
		return false;
	PsScannerResetFn = (PsScannerReset)GetProcAddress(m_hScanInst, "PsScannerReset");
	if (PsScannerResetFn == NULL)
		return false;
	PsFWUpdateFn = (PsFWUpdate)GetProcAddress(m_hScanInst, "PsFWUpdate");
	if (PsFWUpdateFn == NULL)
		return false;
	PsReleaseTrayFn = (PsReleaseTray)GetProcAddress(m_hScanInst, "PsReleaseTray");
	if (PsReleaseTrayFn == NULL)
		return false;
	PsInitializeTrayFn = (PsInitializeTray)GetProcAddress(m_hScanInst, "PsInitializeTray");
	if (PsInitializeTrayFn == NULL)
		return false;

	PsSendProtocolToDeviceFn = (PsSendProtocolToDevice)GetProcAddress(m_hScanInst, "PsSendProtocolToDevice");
	if (PsSendProtocolToDeviceFn == NULL)
		return false;

	PsConnectScannerN2NFn = (PsConnectScannerN2N)GetProcAddress(m_hScanInst, "PsConnectScannerN2N");
	if (PsConnectScannerN2NFn == NULL)
		return false;
	PsDisConnectScannerN2NFn = (PsDisConnectScannerN2N)GetProcAddress(m_hScanInst, "PsDisConnectScannerN2N");
	if (PsDisConnectScannerN2NFn == NULL)
		return false;

	return true;
}

class CTrayCB : public CTrayCallback
{
public:
	CTrayCB() {}
	~CTrayCB() {}

public:
	virtual void ScannerState(TRAY_CB_SCANNER_STATE	state);
	virtual void RefreshHardwareInfo(unsigned char *data);
	virtual void ScanProgressState(int nPos);
	virtual void ReceiveImageList(unsigned char* data);
	virtual void ReceiveImage(const wchar_t * file, int nWidth, int nHeight, unsigned short * img16);
	virtual void ReceiveRelease(int nState);
	virtual void PostProcessState(int nState);
	virtual void ReceiveScannerList(void* DeviceList);
};

