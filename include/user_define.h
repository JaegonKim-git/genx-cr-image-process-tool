#pragma once

#define PANORAMA				0
#define CEPHALO					1
#define ORAL_XRAY				2
#define ORAL_VIDEO				3
#define DIGITAL_CAMERA			4
#define CBCT					5

#define DEVICE_COUNT			6

static CString Tab_Modality_String[] = {
	_T("Panorama"),
	_T("Cephalo"),
	_T("OralXray"),
	_T("OralVideo"),
	_T("DigitalCamera"),
	_T("CBCT")
};

#define MAX_COUNT_EQUIP			CBCT+1

////#define ANIMAL

#define NULL_COMPARE			6
#define NULL_PRINT				7
#define _SEL_IMAGE_TRI_SIZE		15	

#define THUMNAIL_LEFT			9100
#define THUMNAIL_RIGHT			9101

#define TOOLS_TAB_BTN_WIDTH		51
#define TOOLS_TAB_BTN_HEIGHT	66

#define MINIMIZE				1001
#define EXIT					1002
#define FULLSCREEN				1003 //KKK-090717-ADD: _USE_FULLSCREEN

#define MODIFY_STUDY_INFO		2001
#define CLOSE_STUDY				2002

#define NULL_MODE				3001
#define ZOOM					3002
#define FIT_WINDOW				3003
#define MAGIC_GLASS				3004
#define PAN						3005
#define RESET					3006
#define DELETE_IMAGE			3007
#define DRAW_OVERLAY			3008
#define SAVE_STUDY				3009

//#define CW						4001
//#define CCW						4002
#define MIRROR_VERTICAL			4003
#define MIRROR_HORIZONTAL		4004
#define INVERT					4005

#define ANNO_DISTANCE			5001
#define ELLIPSE					5002
#define ANNO_TEXT				5003
#define POLYGON					5004
#define ANGLE					5005
#define BOX						5006
#define ARROW					5007
#define CALIBRATION				5008
#define DELETE_ANNO				5009
#define PALETTE_ANNO			5010
#define FREE_PEN				5011
#define LR_MARK					5012
#define SCREEN_KEYBOARD			5013
#define STAMP					5014

#define IMPLANT_DB				6001
#define IMPLANT_RUBBER_INTERNAL	6002
#define IMPLANT_RUBBER_EXTERNAL	6003
#define IMPLANT_RUBBER_SIMPLE	6004

#define REGISTER			            7001
#define ARCHIVE				            7002
#define ACQUISITION		            	7003
#define ACQUISITION_IMPORT_FILE			8001
#define ACQUISITION_TWAIN_DEVICE		8004
#define ACQUISITION_SETTING				8002
#define ACQUISITION_LIVE				8010
#define ACQUISITION_CAPTURE				8011
#define ACQUISITION_REQUEST_OP			8711
#define ACQUISITION_SAVE	    8003


#define COPY_IMAGE			    9001
#define NERVE				    9002 //KKK-NERVE
#define PROFILE				    9003 //KKK-PROFILE
#define ANATOMIC			    9004 //JACK-ANATOMIC

#define CONVERT_EXPORT		    7004
#define SEND				    7005
#define PRINTER				    7006
#define DESIGN_FILM_MOUNT	    7007

#define RETURN_MAEIN_MENU	    7008
#define RESLICE_VOLUME          7009 //KKK-REV1: ResliceVolume

#define BUTTON_HEIGHT			30
#define CTRL_HEIGHT				25
#define BUTTON_WIDTH			100
#define OFFSET_FRAME			5
#define TOOL_HEIGHT				114 // kyt
#define TOOL_TAB_NOBE_HEIGHT	28.
#define MAIN_CTRL_HEIGHT		124 // kyt
#define MAN_COMMAND_WIDTH		600
#define TOOL_TAB_WIDTH			520
#define STUDY_TAB_HEIGHT		55
#define SIZE_TAB_EQUIP			55
#define SIZE_DENTAL_MENU		45
#define ROUNT_FRAME				5


//#define RGB_FRAME_BORDER		RGB(39,39,39)


#define FILE_EXCEPTION_COUNT	10
#define FILE_EXCEPTION_DELAY	150  

//#define _ODBC_ _T("DSN=triana;UID=sa;PWD=triana;WSID=JACK_NOTEBOOK;DATABASE=db_triana;")
//#define _ODBC_ //"DSN=Pig;DBQ=D:\\Parameters\\pig.mdb;DriverId=25;FIL=MS Access;MaxBufferSize=2048;PageTimeout=5;PWD=pig;UID=admin;"
#define _ODBC_ _T("DSN=triana;UID=sa;PWD=triana;DATABASE=db_triana;")//Network=DBMSSOCN;

/*KKK-090525-ADD: OEM
#define FILE_TRIANA_INFO		_T("Triana.tri")
#define FILE_TRIANA_DB_BACKUP	_T("triana_db.bak")
*/
#define FILE_STAMP_INFO			_T("Stamp.inf")
#define FILE_DENTIST_INFO		_T("Dentist.inf")
#define FILE_CATEGORY_INFO		_T("Category.inf")
#define FILE_IMPLANT_INFO		_T("Implant.inf")
#ifndef FILE_TRANS_LANGUAGE		// TransLanguage.h 내의 정의와 동일해야 함.
#define FILE_TRANS_LANGUAGE		_T("PortView_trans.inf")
#endif

#define FILE_DB_STUDY_INFO		_T("DbStudyInfo.inf")

#ifdef _LYNX
#define TITLE_CTOP   "ORION_OP"
#else
#define TITLE_CTOP   "NOVA_OP"
#endif


#define PATH_TEMPLATE			_T("\\Film_Mount")
#define PATH_PANORAMA			_T("\\PANORAMA")
#define PATH_CEPHALO			_T("\\CEPHALO")
#define PATH_ORAL_XRAY			_T("\\ORAL_XRAY")
#define PATH_ORAL_VIDEO			_T("\\ORAL_VIDEO")
#define PATH_DIGITAL_CAMERA		_T("\\DIGITAL_CAMERA")
#define PATH_CBCT				_T("\\CT")
#define PATH_TEMP				_T("\\temp")
#define PATH_IMPLANT			_T("\\Implant")
#define PATH_SQL				_T("\\SQL")
#define PATH_ACCESS				_T("\\ACCESS")	//KYT
#define PATH_ETC				_T("\\ETC")
#define CT_CAPTURED_FOLDER		_T("\\CapturedImage")
#define CT_SLICEDATA_FOLDER		_T("\\SliceData")



#define SETTING_TAB_HEIGHT      46
#define SETTING_TAB_WIDTH       206


#define MESSAGE_DLG_OK          1
#define MESSAGE_DLG_CANCEL      2
#define MESSAGE_DLG_CLOSE       3



#define MODE_DENTAL_FRMAE_NORMAL	0
#define MODE_DENTAL_FRMAE_COMPARE	1
#define MODE_DENTAL_FRMAE_PRINT		2

//_DENTAL_FRAME_INFO_
#define STUDY_MODE				0
#define AQUISITION_MODE			1
#define NOVA_MODE				2	

#define MAX_COUNT_OPEN_STUDY    6

#define EXP_PANORAMA			1
#define EXP_OTHO_PAN			2
#define EXP_TMJ					3	
#define EXP_SINUS				4
#define EXP_PADIATRIC			5
#define EXP_CEP_PA				6
#define EXP_CEP_LAT				7
#define EXP_CEP_CAPUS			8


#define EXP_IX					100
#define EXP_IV					101
#define EXP_DC					102

#define INSTRUCT_CODE_NO		0
#define INSTRUCT_CODE_WAIT		1
#define INSTRUCT_CODE_PUSH_BUTTON		2
#define INSTRUCT_CODE_PRESSED_BUTTON	3
#define INSTRUCT_CODE_RELEASE_BUTTON	4
#define INSTRUCT_CODE_PSRE_BUTTON		5

#define MSACCESS				0	
#define MSSQL					1	

// 
#define DLG_CONNECTDB_RETRY		1
#define DLG_CONNECTDB_EXIT		2
#define DLG_CONNECTDB_LOCALDB	3

#define TIMERID_CLOSE_DEVICE				0x1234
#define TIMERID_POLLING_GAMEPORT			0x1235
#define TIMERID_POLLING_CHECK_TEMPER_PANCEP	0x1236
#define TIMERID_POLLING_CHECK_USB_SENSOR    0x1237
#define TIMERID_POLLING_MD760				0x1238		
#define TIMERID_POLLING_WINUS				0x1239		
#define TIMERID_SCANNER_CONNECT_IPADDR	    0x1288	

#define TIMERID_REMAIN_STANDBY_DURATION		0x1291
#define TIMERID_REMAIN_SUSPEND_DELAY		0x1292

#define TIMERID_GENXCR_UPDATE_IMAGE_LIST	0x1295

#define TIMERID_IRAY_STATE					0x1296
#define TIMERID_IRAY_DETECT					0x1297
#define TIMERID_TWAIN_STATE					0x1298

#define EXPORT_FORMAT_JPEG					0
#define EXPORT_FORMAT_BMP					1
#define EXPORT_FORMAT_PNG					2
#define EXPORT_FORMAT_DICOM					3

// _OPTION_OVERLAY_ 의 변수 크기와 동일해야 한다.
#define MAX_PREFIX_LENGTH					40

#define OFFSETX 				50//10
#define OFFSETY					260
#define MNT_WIDTH				130
#define	MNT_HEIGHT				130

#define THUMB_WND_HEIGHT		150
#define MENU_HEIGHT				130
//#define RATIO					0.7//5
#define TMB_WIDTH				136
#define	TMB_HEIGHT				130
#define TMB_BTN_WIDTH			24
#define TMB_BTN_HEIGHT			40 
#define RATIO					5

enum BUTTON_STATUS
{
	STATUS_NOTFOUND,
	STATUS_NOTREADY,
	STATUS_READY,
	STATUS_PROCESSING,
	STATUS_ORALVIDEO_LIVE,
	STATUS_ORALVIDEO_STOP,
	STATUS_SENSOR_PROCESS_CALIBRATION,
	STATUS_CONNECTING,
	STATUS_DISCONNECTING,
	STATUS_DISCONNECTED, // 2025-03-13. jg kim. PortView 코드 반영
	STATUS_SLEEP,
};
enum INFO_TYPE
{
	TYPE_IMPORT,
	TYPE_SDK,
	TYPE_TWAIN,
};
#define	DEFAULT_FILM_RESOLUTION	300
#define	DEFAULT_FILMSIZE_ID		_T("8INX10IN")		// _T("14INX17IN")
#define	DEFAULT_FILMSIZE_W		14
#define	DEFAULT_FILMSIZE_H		17

#define	DATA_TRANSFER					(WM_USER+0x1F28)	
#define	DS_TRANSFER				0x7777
#define DATA_TRANSFER_THEIA				(WM_USER+0x5000)
#define UPDATE_PATIENT_FROM_THEIA		(WM_USER+0x5001)
#define UPDATE_PATIENT_FROM_THEIA_ADDR	(WM_USER+0x5002)


#define FILTER_UNSHARPMASK		1001
#define FILTER_SMOOTH			1002
#define FILTER_EDGE_ENHANCE		1003
#define FILTER_SHARP			1004
#define FILTER_EMBOSS			1005
#define FILTER_INVERT			1006
#define	FILTER_NONE				1007

#define SHARP_SOFT			0
#define SHARP_MEDIUM		1
#define SHARP_HARD			2

#define BTN_WIDTH			51
#define BTN_HEIGHT			70
#define BTN_HEIGHT_POPUP	60		//51
#define PATIENT_WIDTH		130		//170
#define PATIENT_HEIGHT		84

#define SENSOR_WIDTH		100
//#define SENSOR_HEIGHT		76
#define DEVICE_BTN_WIDTH    48
#define DEVICE_BTN_HEIGHT   32

#define TREE_DLG_HEIGHT     160
#define TREE_DLG_WIDTH      190
#define MAX_MENU_BUTTON	    24
#define MAX_MENU_SEPARATOR	26
#define FREQUENTLY_MENU		21
#define INFREQUENTLY_MENU	21
#define BUTTON_OFFSET_X		3
#define SEPARATOR_WIDTH		10
#define SEPARATOR_HEIGHT	48
#define MENU_TEXT_WIDTH		59
#define MENU_LEFT_WIDTH		246
#define CLOCK_WIDTH			50
#define CLOCK_HEIGHT        40
#define	THUMB_TREE_WIDTH	170

#define MOUNT_PREVIEW		0
//wjlee : modify - 20220914
//#define MOUNT_FMX			1 // FMX
#define MOUNT_FMX18			1 // FMX 18
//end
#define	MOUNT_ACQ			2
#define MOUNT_FULL_BASIC	3
#define MOUNT_MILK			4
#define MOUNT_1X1			5
#define MOUNT_2X1			6
#define MOUNT_3X1			7
#define MOUNT_2X2			8
//wjlee : add - 20220914
#define MOUNT_FMX22			9 // FMX 22
#define MOUNT_FMX32			10 // FMX 32
//end

#define LAYOUT_MENU_TOP		0
#define LAYOUT_MENU_LEFT	1

#define TREE_SELECTEDMOUNT  0
#define	TREE_FULL_MOUTH		1 //TREE_ALLMOUNT
#define	TREE_ALLMOUNT		2
#define BUTTON_MENU			0
#define BUTTON_NORMAL		1
#define BUTTON_SUB			2
#define ENABLE_BTN_OK		0
#define ENABLE_BTN_CANCEL	1
#define ENABLE_BTN_NONE		2

#define COMPARE_SAVEMODE_FILE			0
#define COMPARE_SAVEMODE_CLIPBOARD		1

#define TOOTH_FDI			0
#define TOOTH_PALMER		1
#define	TOOTH_UNIVERSAL		2
#define	TOOTH_ANIMAL	    4

#define STANDALONE			0				// 그냥 독립적으로 단독 실행된 경우인듯.
#define	TRIANA_CALL			1				// Triana 에서 호출되어 실행된 경우인듯.
#define THIRD_PARTY			2				// 타사 제품에서 호출되어 실행되는 경우인듯.
#define TWAIN_APP			3				// TWAIN 장비로 인식되어 PortView가 획득/전송 가능한 영상들을 다른 프로그램에 전달하는 기능. (portview.ds 로 실행된다)
#define LOCALDB				4

#define VIEW_MODE_FULLMOUNT 0
#define VIEW_MODE_ACQ		1

#define CONVERT_DCM_SEND    0
#define CONVERT_DCM_PRINT   1
#define CONVERT_DCM_BURN    3
#define CONVERT_DCM_EXPORT  4

#define FMX_MOVE_MANUAL		0		
#define FMX_MOVE_AUTO		1		
#define FMX_MOVE_RESET      2

#define FILE_TYPE_RAW		0
#define FILE_TYPE_TIF		1

#define GIX_ACQ_NONE			0
#define GIX_ACQ_START			1
#define GIX_ACQ_RETRY_WAIT		2
#define GIX_ACQ_RETRY_OK		3
#define GIX_ACQ_RETRY_CANCEL	4

#define GIX_DISCONNECTED		0
#define GIX_CONNECTED			1
#define GIX_DISCONNECTING		2
#define GIX_CONNECTING			3

#define ID_MENU1_CONNECT			20000
#define ID_MENU1_IMAGE_LIST			20001
#define ID_MENU1_CALIBRATION		20002
#define ID_MENU1_RESET				20003
#define ID_MENU1_SCANNER_CONTROL	20004
#define ID_MENU1_ACQ				20005
#define ID_MENU1_SELECT_DETECT		20006
#define ID_MENU1_GET_LASTIMAGE		20007
#define ID_MENU1_DISCONNECT			20008

#define ID_MENU2_HPK				20100
#define ID_MENU2_E2V				20101
#define ID_MENU2_IRAY				20102
#define ID_MENU2_FIRECR				20103
#define	ID_MENU2_GENXCR				20104
// 2025-03-13. jg kim. PortView 코드 반영
#define	ID_MENU2_HNDY				20105

#define	ID_MENU3_HNDY1				30105
#define	ID_MENU3_HNDY2				30106
#define	ID_MENU3_HNDY3				30107
#define	ID_MENU3_HNDY4				30108
#define	ID_MENU3_HNDY5				30109

// for modality administration mode  // 2024-10-14. jg kim. PortView쪽 수정사항 반영
#define ID_MENU_ADMIN_FWUPDATE		21001
#define ID_MENU_ADMIN_CALIB			21002
#define ID_MENU_ADMIN_DIAG			21003
#define ID_MENU_ADMIN_TEST			21004
#define ID_MENU_ADMIN_EX1			21005		// [reserved]
#define ID_MENU_ADMIN_EX2			21006
#define ID_MENU_ADMIN_EX3			21007

#define ID_MENU3_DETECTOR			30000

#define OX_MANUFACTURE_TOSHIBA		0
#define OX_MANUFACTURE_SUNI			1
#define OX_MANUFACTURE_SCHICK		2
#define OX_MANUFACTURE_SCANX		3 //KYT-SCANX
#define OX_MANUFACTURE_DIGIXIO		4 //KKK-110725-ADD: _USE_DIGIXIO_IOSENSOR
#define OX_MANUFACTURE_E2V			5
#define OX_MANUFACTURE_FIRECR       6
#define OX_MANUFACTURE_HPK			7
#define OX_MANUFACTURE_NULL         8
#define OX_MANUFACTURE_VATECH		9 ////jggang 2019.0806
#define OX_MANUFACTURE_GENXCR		10
#define OX_MANUFACTURE_IRAY			11
// 2025-03-13. jg kim. PortView 코드 반영
#define OX_MANUFACTURE_HNDY			12