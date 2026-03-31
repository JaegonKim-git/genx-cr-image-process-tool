// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "cruxcan_tray.h"
#include "AdminScannerInfo.h"
#include "afxdialogex.h"
#include "../predix_sdk/common.h"
#include "../predix_sdk/common_type.h"
#include "../predix_sdk/scanner.h"
#include "../predix_sdk/tray_network.h"
//#include "../predix_sdk/global_data.h" // 2024-08-23. jg kim. AdminScannerInfo.h로 이동
#include "../cruxcan_tray/resource.h"
#include "../gnParameterHandler/ParameterCalculatorInterface.h"
//#include "../gnHardwareInfo/HardwareInfo.h" // 2024-06-26. jg kim. MPPC bias 전압 계산을 위함.
#include "../gnHardwareInfo/HardwareInfoInterface.h"
#include "../gnPreProcessing/PreProcessingInterface.h" // 2025-05-07. jg kim.
#include "../include/inlineFunctions.h"
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
#include "../include/Logger.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define _TIMERID_HARDWARE_INFO		(4547)		// 2024-08.23 jg kim. -  parmeter dlg 띄우기 전 장비에서 설정값을 받아올 시간 확보를 위함.
#define _TIMERID_PARAMETER_EXTRACTION_INFO		(4548)	// 2024-11.12 jg kim. parameter extraction에 계산 시간이 많이 걸리게 되어 타이머 추가
float g_fAspectRatio = -1;
int g_nBldcShiftPixels = 65535;
int g_x;
int g_y;
int g_nBoundingWidth;
int g_nBoundingHeight;
bool g_bResultParameterExtraction = false;
bool g_bDoneParameterExtraction = false;
const char *LOG_FILE_NAME = "Image Processing Tool.log";

#if (DEF_BACKWARD_OFFSET_LENGTH)
// kiwa72(2023.03.22 14h) - 불필요한 항목 제외, touch cal 설정값 추가
static const int name_list_count = 39 + 6 + 4; 		// 2025-07-03. jg kim. Eject Speed 항목 추가
													// 2026-01-07. jg kim. Wobble 관련 파라미터 추가(Laser On/Off구간, 세로 줄 범위)
static const int user_input_list_count = 11 + 1; // 2024-07-15. jg kim. BLDC RPM 관련 항목 추가
												 // 2025-06-18. jg kim. Positioning offset 항목 추가
#endif

static bool is_readonly[name_list_count] =
{
	// kiwa72(2023.03.22 14h) - 불필요한 항목 제외, touch cal 설정값 추가
	0, 1,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
#if (DEF_BACKWARD_OFFSET_LENGTH)
	0,
#endif
	0, 0, 0, 0, 0,
};

static wchar_t user_input_list[user_input_list_count][100] =
{
	_T("Measured Scan Speed"),// 0
	_T("Backward offset (mm. Size 3)"),// 1
	_T("Positioning offset (mm. Size 0~2)"),// 2
	_T("Entry offset Size0~2(pixel)"),// 3
	_T("Entry offset Size3(pixel)"),// 4
	_T("MPPC1 Bias (V)"), // 5. 2024-06-26. jg kim. MPPC 관련 항목 추가
	_T("MPPC1 offset (V) "),//6
	_T("MPPC2 Bias (V)"),// 7
	_T("MPPC2 offset (V) "),// 8
	_T("MPPC3 Bias (V)"),// 9
	_T("MPPC3 offset (V) "),// 10
	_T("BLDC (RPM) "),//11
};

static wchar_t name_list[name_list_count][100] =
{
	// kiwa72(2023.03.22 14h) - 불필요한 항목 제외, touch cal 설정값 추가
	_T("Name"),							// 0

	_T("Firmware Version (Current)"),

	_T("IP Address"),
	_T("Subnet Mask"),
	_T("GateWay"),
	_T("MAC Address"),					// 5
	_T("Resolution"),

	_T("DHCP Mode"),
	_T("BLDC Speed"),
	_T("BLDC Shift Index"),
	_T("Laser Power"),					// 10
	_T("MPPC Bias1"),

	_T("MPPC Bias2"),
	_T("MPPC Bias3"),
	_T("Delay Pixel value"),
	_T("Start Speed"),					// 15
	_T("Scan Speed"),

	_T("Erase Speed"),
	_T("Normal Speed"),
	_T("SM Step Val"),
	_T("SM Step Dur"),					// 20
	_T("Scan Entry Time_0"),
	_T("Scan Entry Time_1"),
	_T("Scan Entry Time_2"),
	_T("Scan Entry Time_3"),

	_T("Erase Entry Time"),				// 25

	_T("Size0 Positioning Time"),
	_T("Size1 Positioning Time"),
	_T("Size2 Positioning Time"),
	_T("Size3 Positioning Time"),

	_T("IP Eject Time"),				// 30
	_T("IP Eject Speed"),
	_T("Parking Length"),
	_T("Max Speed"),
	// 2026-01-07. jg kim. Wobble 보정 관련 좌/우 Laser On/Off, 세로 줄 시작/끝 위치 
	_T("Left Laser On position (pixel unit)"),
	_T("Left Laser Off position (pixel unit)"),	// 35

	_T("Right Laser On position (pixel unit)"),
	_T("Right Laser Off position (pixel unit)"),
	_T("L-Vert. Line Start x (pixel unit)"),		// 38
	_T("L-Vert. Line End   x (pixel unit)"),
	_T("R-Vert. Line Start x (pixel unit)"),
	_T("R-Vert. Line End   x (pixel unit)"),

#if (DEF_BACKWARD_OFFSET_LENGTH)
	_T("Backward Offset Length"),//42
#endif

	_T("tsCal Saved"),					// 43
	_T("tsCal X org"),
	_T("tsCal X limit"),
	_T("tsCal Y org"),
	_T("tsCal Y limit"),				// 47
};


/* =========================================================
 * 1) uint16_t (unsigned short) bias fixed-point
 *    - raw: 0..65535
 *    - fixed_signed_range: -32768..+32767 (bias=32768)
 *    - real range: (-32768/scale) .. (32767/scale)
 * ========================================================= */

#define U16_BIAS   32768
#define U16_RAW_MIN 0u
#define U16_RAW_MAX 65535u

 /* =========================================================
  * 2) uint32_t (unsigned int) bias fixed-point
  *    - raw: 0..4294967295
  *    - fixed_signed_range: -2147483648..+2147483647 (bias=2^31)
  *    - real range: (-2147483648/scale) .. (2147483647/scale)
  * ========================================================= */

#define U32_BIAS        2147483648UL
#define U32_RAW_MIN     0UL
#define U32_RAW_MAX     4294967295UL

// CAdminScannerInfo 대화 상자입니다.

IMPLEMENT_DYNAMIC(CAdminScannerInfo, CDialogEx)
HWND admin_hwnd = nullptr;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CAdminScannerInfo::CAdminScannerInfo(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_ADMIN_SCANNER_INFO_DIALOG, pParent)
    , iSavedItem(0)
    , iSavedSubitem(0)
    , is_changed(false)
	, is_changed_user_input(false)
	, m_bAutoRefresh(FALSE)
	, m_bIsParameterDlg(FALSE) // 2024-08-23. jg kim. On timer 함수 호출시 어떤 
{
	iSavedItem = -1;
	iSavedSubitem = -1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CAdminScannerInfo::~CAdminScannerInfo()
{
	admin_hwnd = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2026-02-05. jg kim PostNcDestroy - 모달리스 다이얼로그 메모리 해제
void CAdminScannerInfo::PostNcDestroy()
{
	CDialogEx::PostNcDestroy();
	delete this;  // 동적으로 생성된 객체 삭제
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// data exchange
void CAdminScannerInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ADMIN_SCANNER_INFO_LIST, m_admin_scanner_info);
	DDX_Control(pDX, IDC_LIST_USER_INPUT, m_user_input);
	DDX_Check(pDX, IDC_CHECK_AUTO_REFRESH, m_bAutoRefresh);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
LRESULT CAdminScannerInfo::OnMsgRefreshList(WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	if (m_bAutoRefresh)
		update_current_to_list();

	return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// message map
BEGIN_MESSAGE_MAP(CAdminScannerInfo, CDialogEx)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ADMIN_SCANNER_INFO_LIST, &CAdminScannerInfo::OnLvnItemchangedAdminScannerInfoList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_USER_INPUT, &CAdminScannerInfo::OnLvnItemchangedListUserInput)
	ON_NOTIFY(NM_DBLCLK, IDC_ADMIN_SCANNER_INFO_LIST, &CAdminScannerInfo::OnNMDblclkAdminScannerInfoList)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_USER_INPUT, &CAdminScannerInfo::OnNMDblclkListUserInput)
	ON_NOTIFY(NM_CLICK, IDC_ADMIN_SCANNER_INFO_LIST, &CAdminScannerInfo::OnNMClickAdminScannerInfoList)
	ON_NOTIFY(NM_CLICK, IDC_LIST_USER_INPUT, &CAdminScannerInfo::OnNMClickListUserInput)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_ADMIN_SCANNER_INFO_LIST, &CAdminScannerInfo::OnNMCustomdrawAdminScannerInfoList)
	ON_EN_CHANGE(IDC_EDIT_MODIFY, &CAdminScannerInfo::OnEnChangeEditModify)
	ON_EN_CHANGE(IDC_EDIT_USER_INPUT, &CAdminScannerInfo::OnEnChangeEditUserInput)
	ON_BN_CLICKED(IDC_BUTTON_APPLY, &CAdminScannerInfo::OnBnClickedButton_Apply)
	ON_BN_CLICKED(IDC_BUTTON_SAVEPC, &CAdminScannerInfo::OnBnClickedButtonSavepc)
	ON_BN_CLICKED(IDC_BUTTON_LOADPC, &CAdminScannerInfo::OnBnClickedButtonLoadpc)
	ON_BN_CLICKED(IDC_BUTTON_LOAD, &CAdminScannerInfo::OnBnClickedButtonLoad)
	ON_BN_CLICKED(IDC_BUTTON_BACKUP, &CAdminScannerInfo::OnBnClickedButtonBackup)
	ON_BN_CLICKED(IDC_BUTTON_CALCULATE, &CAdminScannerInfo::OnBnClickedButtonCalculate)
	ON_MESSAGE(WM_REFRESH_LIST, &CAdminScannerInfo::OnMsgRefreshList)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT, &CAdminScannerInfo::OnBnClickedButtonDefault)
	ON_BN_CLICKED(IDC_BUTTON_BLDC_SCAN_SPEED, &CAdminScannerInfo::OnBnClickedButtonBldcScanSpeed)
	ON_BN_CLICKED(IDC_BUTTON_BACKWARD_OFFSET, &CAdminScannerInfo::OnBnClickedButtonBackwardOffset)
	ON_BN_CLICKED(IDC_BUTTON_ENTRY_OFFSET, &CAdminScannerInfo::OnBnClickedButtonEntryOffset)
	ON_BN_CLICKED(IDC_BUTTON_MPPC, &CAdminScannerInfo::OnBnClickedButtonMppc) // 2024-06-26. jg kim. MPPC 관련 항목 추가
	ON_BN_CLICKED(IDC_BUTTON_MPPC_DEFAULT, &CAdminScannerInfo::OnBnClickedButtonMppcDefault)
	ON_BN_CLICKED(IDC_BUTTON_BLDC, &CAdminScannerInfo::OnBnClickedButtonBldc)
	ON_WM_TIMER() // 2024-08-23. jg kim. Hardware 정보를 장비에서 받아오는데 시간이 필요해서 timer 사용
	ON_BN_CLICKED(IDC_BUTTON_POSITION_OFFSET, &CAdminScannerInfo::OnBnClickedButtonPositionOffset)
END_MESSAGE_MAP()

void CAdminScannerInfo::split_CString_number_to_each_part(CString number, int decimal_places, int & integer_part, int & decimal_part)
{
	UNREFERENCED_PARAMETER(decimal_places);
	int index = number.Find('-');
	CString str_decimal;
	CString str_integer;
	int sign = 1;
	if (number.Left(index + 1) == CString("-"))
	{
		sign = -1;
		number = number.Mid(index + 1); // - 부호가 있다는 것을 알았으니 뗀다.
	}
	index = number.Find('.');
	if (index == -1) // . 이 없는 경우
	{
		str_decimal = CString("0");
		str_integer = number;
	}
	else
	{
		str_decimal = number.Mid(index + 1);
		str_integer = number.Left(index);
	}
	integer_part = _tstoi(str_integer) * sign;
	decimal_part = _tstoi(str_decimal);

}

// CAdminScannerInfo 메시지 처리기입니다.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dialog initializing
BOOL CAdminScannerInfo::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	admin_hwnd = this->m_hWnd;

	RECT rt2;
	this->GetParent()->GetWindowRect(&rt2);

	SIZE sz;
	sz.cx = (rt2.right - rt2.left);
	sz.cy = (rt2.bottom - rt2.top);

	RECT rt;
	GetWindowRect(&rt);
	int width = rt.right - rt.left;
	int height = rt.bottom - rt.top;
	SetWindowPos(NULL, rt2.left + ((sz.cx - width) / 2), rt2.top + ((sz.cy - height) / 2), width, height, SWP_NOREPOSITION);

	iSavedItem = iSavedSubitem = -1;

	m_admin_scanner_info.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 175);
	m_admin_scanner_info.InsertColumn(1, _T("Data"), LVCFMT_LEFT);
	m_admin_scanner_info.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	m_admin_scanner_info.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// m_admin_scanner_info.sethe
	m_admin_scanner_info.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	for (int i = 0; i < name_list_count; i++)
	{
		m_admin_scanner_info.InsertItem(i, name_list[i]);
	}

	iSavedItemUserInput = iSavedSubitemUserInput = -1;
	m_user_input.InsertColumn(0, _T("Name"), LVCFMT_LEFT, 175);
	m_user_input.InsertColumn(1, _T("Data"), LVCFMT_LEFT);
	m_user_input.SetColumnWidth(1, LVSCW_AUTOSIZE_USEHEADER);
	m_user_input.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	// m_admin_scanner_info.sethe
	m_user_input.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	for (int i = 0; i < user_input_list_count; i++)
	{
		m_user_input.InsertItem(i, user_input_list[i]);
	}

	auto pGlobalData = GetPdxData();
	wchar_t c1_data[100] = { 0, };
	
	double packed = pGlobalData->current_setting.stcali.data[2];
	unsigned short nEncodedBackwardOffset, nEncodedPositioningOffset, nEncodedEntry2, nEncodedEntry3;
	unpackData(nEncodedBackwardOffset, nEncodedPositioningOffset, nEncodedEntry2, nEncodedEntry3, packed);

	double fBackwardOffset = decode_bias_u16(nEncodedBackwardOffset, 10);
	double fPositioningOffset = decode_bias_u16(nEncodedPositioningOffset, 10);
	double fEntry2 = (decode_bias_u16(nEncodedEntry2, 1));
	double fEntry3 = (decode_bias_u16(nEncodedEntry3, 1));

	char buf[1024];
	sprintf(buf, "BackwardOffset = %.1f\n", fBackwardOffset);
	writelog(buf, LOG_FILE_NAME);
	
	sprintf(buf, "PositioningOffset = %.1f\n", fPositioningOffset);
	writelog(buf, LOG_FILE_NAME);
	
	sprintf(buf, "Entry2 = %.0f\n", fEntry2);
	writelog(buf, LOG_FILE_NAME);
	
	sprintf(buf, "Entry3 = %.0f\n", fEntry3);
	writelog(buf, LOG_FILE_NAME);

	CString str;
	int count = 0;
	wsprintf(c1_data, L"%d", pGlobalData->current_setting.SM_speed);
	m_user_input.SetItemText(count++, 1, c1_data);
	str.Format(L"%.1f", fBackwardOffset); wcscpy_s(c1_data, str);
	m_user_input.SetItemText(count++, 1, c1_data); // 1
	str.Format(L"%.1f", fPositioningOffset); wcscpy_s(c1_data, str);
	m_user_input.SetItemText(count++, 1, c1_data); // 2

	str.Format(L"%.0f", fEntry2); wcscpy_s(c1_data, str);
	m_user_input.SetItemText(count++, 1, c1_data); // 3

	str.Format(L"%.0f", fEntry3); wcscpy_s(c1_data, str);
	m_user_input.SetItemText(count++, 1, c1_data); // 4
	
	//CHardwareInfo hi;  // 2024-06-26. jg kim. MPPC 관련 초기화.
	int volts[MPPC_COUNT] = { 0, }; // 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
	int j = 0;
	volts[j++] = HI_Mppc_Convert_DigitalValueToVoltage( int(pGlobalData->current_setting.MPPC1_bias));
	volts[j++] = HI_Mppc_Convert_DigitalValueToVoltage( int(pGlobalData->current_setting.MPPC2_bias));
	volts[j++] = HI_Mppc_Convert_DigitalValueToVoltage( int(pGlobalData->current_setting.MPPC3_bias));
	count = 5;
	//int decimal_place = 3;  // unused variable
	for (int i = 0; i < MPPC_COUNT; i++)
	{

		str.Format(L"%.2f", double(volts[i]) / 1000); wcscpy_s(c1_data, str);
		m_user_input.SetItemText(count + i * 2, 1, c1_data);

		str.Format(L"%.2f", double(0)); wcscpy_s(c1_data, str);
		m_user_input.SetItemText(count + i * 2 + 1, 1, c1_data);
	}
	count = 11;   // 2024-07-15. jg kim. BLDC 관련 초기화.
	double fBldcRPM = (double)std::llround(HI_getBldcRPM(pGlobalData->current_setting.BLDC_speed, MAIN_CLOCK_FREQUENCY)); // 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용

	str.Format(L"%.3f", fBldcRPM); wcscpy_s(c1_data, str);
	m_user_input.SetItemText(count, 1, c1_data);

	update_current_to_list();

	UpdateData(FALSE);

	is_changed = false;
	is_changed_user_input = false;

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::update_current_to_list(void)
{
	// 2024-08-23. jg kim. 변동사항이 있을 때만 업데이트 
	// 2024-11-18. jg kim. load from pc 버튼을 클릭하여 HardwareSetting을 불러오면 업데이트가 제대로 되지 않아 잠시 disable함.
	//if (m_HARDWARE_SETTING == GetPdxData()->current_setting)
	//{
	//	;
	//}
	//else
	{
		MYLOGOLDW(L">>>>CAdminScannerInfo::update_current_to_list\n");
		char buf[1024];
		wchar_t c1_data[100] = { 0, };
		int count = 0;

		MultiByteToWideChar(CP_ACP, 0, GetPdxData()->current_setting.Name, strlen(GetPdxData()->current_setting.Name) + 1, c1_data, 100);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		auto pGlobalData = GetPdxData();
		ASSERT(pGlobalData);
		// kiwa72(2023.03.22 14h) - 불필요한 항목 제외, touch cal 설정값 추가
		// 2026-02-02. jg kim. test version FW build verson 출력 추가 // 0.8.0.2B24 부터 동작
		if (pGlobalData->current_setting.reserved2[0])//test version FW
		{
			wsprintf(c1_data, L"%d.%d.%d.%dB%02d", pGlobalData->current_setting.Firmware_version2[0], pGlobalData->current_setting.Firmware_version2[1], pGlobalData->current_setting.Firmware_version2[2], pGlobalData->current_setting.Firmware_version2[3], pGlobalData->current_setting.reserved2[0]);
			sprintf(buf, "Firmware Version : %d.%d.%d.%dB%02d\n", pGlobalData->current_setting.Firmware_version2[0], pGlobalData->current_setting.Firmware_version2[1], pGlobalData->current_setting.Firmware_version2[2], pGlobalData->current_setting.Firmware_version2[3], pGlobalData->current_setting.reserved2[0]);
		}
		else // official version FW
		{
			wsprintf(c1_data, L"%d.%d.%d.%d", pGlobalData->current_setting.Firmware_version2[0], pGlobalData->current_setting.Firmware_version2[1], pGlobalData->current_setting.Firmware_version2[2], pGlobalData->current_setting.Firmware_version2[3]);
			sprintf(buf, "Firmware Version : %d.%d.%d.%d\n", pGlobalData->current_setting.Firmware_version2[0], pGlobalData->current_setting.Firmware_version2[1], pGlobalData->current_setting.Firmware_version2[2], pGlobalData->current_setting.Firmware_version2[3]);
		}
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d.%d.%d.%d", pGlobalData->current_setting.ipaddr[0], pGlobalData->current_setting.ipaddr[1], pGlobalData->current_setting.ipaddr[2], pGlobalData->current_setting.ipaddr[3]);
		sprintf(buf, "IP Address : %d.%d.%d.%d\n", pGlobalData->current_setting.ipaddr[0], pGlobalData->current_setting.ipaddr[1], pGlobalData->current_setting.ipaddr[2], pGlobalData->current_setting.ipaddr[3]);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d.%d.%d.%d", pGlobalData->current_setting.netmask[0], pGlobalData->current_setting.netmask[1], pGlobalData->current_setting.netmask[2], pGlobalData->current_setting.netmask[3]);
		sprintf(buf, "Subnet Mask : %d.%d.%d.%d\n", pGlobalData->current_setting.netmask[0], pGlobalData->current_setting.netmask[1], pGlobalData->current_setting.netmask[2], pGlobalData->current_setting.netmask[3]);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d.%d.%d.%d", pGlobalData->current_setting.gateway[0], pGlobalData->current_setting.gateway[1], pGlobalData->current_setting.gateway[2], pGlobalData->current_setting.gateway[3]);
		sprintf(buf, "GateWay : %d.%d.%d.%d\n", pGlobalData->current_setting.gateway[0], pGlobalData->current_setting.gateway[1], pGlobalData->current_setting.gateway[2], pGlobalData->current_setting.gateway[3]);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%02X:%02X:%02X:%02X:%02X:%02X", pGlobalData->current_setting.MAC_ADDR[0], pGlobalData->current_setting.MAC_ADDR[1], pGlobalData->current_setting.MAC_ADDR[2], pGlobalData->current_setting.MAC_ADDR[3], pGlobalData->current_setting.MAC_ADDR[4], pGlobalData->current_setting.MAC_ADDR[5]);
		sprintf(buf, "MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n", pGlobalData->current_setting.MAC_ADDR[0], pGlobalData->current_setting.MAC_ADDR[1], pGlobalData->current_setting.MAC_ADDR[2], pGlobalData->current_setting.MAC_ADDR[3], pGlobalData->current_setting.MAC_ADDR[4], pGlobalData->current_setting.MAC_ADDR[5]);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		sprintf(buf, "Scan mode = %d\n", pGlobalData->current_setting.selected_ip_size - 1000);  // 2024-08-13. jg kim.  로그 추가
		writelog(buf, LOG_FILE_NAME);
		if (pGlobalData->current_setting.image_resolution == eir_StRes)
		{
			wcscpy(c1_data, L"SR");
			sprintf(buf, "\t\t Warn. The scan resolution is not SR. Please change it HR.\n");  // 2024-08-13. jg kim. 작업자 실수에 경고
			writelog(buf, LOG_FILE_NAME);
			AfxMessageBox(L"스캔 해상도를 HR로 변경하세요.");
			sprintf(buf, "Resolution : SR\n");
			CAdminScannerInfo::EndDialog(1); // 2024-08-23. jg kim. 작업자 실수를 방지하기 위해 경고 띄운 후에는 dlg를 닫음 // IDOK
		}
		else
		{
			wcscpy(c1_data, L"HR");
			sprintf(buf, "Resolution : HR\n");
		}
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		if (pGlobalData->current_setting.useDHCP == 1)
		{
			wcscpy(c1_data, L"ON");
			sprintf(buf, "DHCP Mode : ON\n");
		}
		else
		{
			wcscpy(c1_data, L"OFF");
			sprintf(buf, "DHCP Mode : OFF\n");
		}
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.BLDC_speed);
		sprintf(buf, "BLDC_speed : %d\n", pGlobalData->current_setting.BLDC_speed);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.BLDC_shift_index);
		sprintf(buf, "BLDC_shift_index : %d\n", pGlobalData->current_setting.BLDC_shift_index);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.laser_power);
		sprintf(buf, "laser_power : %d\n", pGlobalData->current_setting.laser_power);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.MPPC1_bias);
		sprintf(buf, "MPPC1_bias : %d\n", pGlobalData->current_setting.MPPC1_bias);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.MPPC2_bias);
		sprintf(buf, "MPPC2_bias : %d\n", pGlobalData->current_setting.MPPC2_bias);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.MPPC3_bias);
		sprintf(buf, "MPPC3_bias : %d\n", pGlobalData->current_setting.MPPC3_bias);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.LaserOnTime);
		sprintf(buf, "LaserOnTime : %d\n", pGlobalData->current_setting.LaserOnTime);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.start_speed);
		sprintf(buf, "start_speed : %d\n", pGlobalData->current_setting.start_speed);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.SM_speed);
		sprintf(buf, "SM_speed : %d\n", pGlobalData->current_setting.SM_speed);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.erase_speed);
		sprintf(buf, "erase_speed : %d\n", pGlobalData->current_setting.erase_speed);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.normal_speed);
		sprintf(buf, "normal_speed : %d\n", pGlobalData->current_setting.normal_speed);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.sm_step_val);
		sprintf(buf, "sm_step_val : %d\n", pGlobalData->current_setting.sm_step_val);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.sm_step_dur);
		sprintf(buf, "sm_step_dur : %d\n", pGlobalData->current_setting.sm_step_dur);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.scan_entry_time_0);
		sprintf(buf, "scan_entry_time_0 : %d\n", pGlobalData->current_setting.scan_entry_time_0);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.scan_entry_time_1);
		sprintf(buf, "scan_entry_time_1 : %d\n", pGlobalData->current_setting.scan_entry_time_1);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.scan_entry_time_2);
		sprintf(buf, "scan_entry_time_2 : %d\n", pGlobalData->current_setting.scan_entry_time_2);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.scan_entry_time_3);
		sprintf(buf, "scan_entry_time_3 : %d\n", pGlobalData->current_setting.scan_entry_time_3);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.erase_entry_time);
		sprintf(buf, "erase_entry_time : %d\n", pGlobalData->current_setting.erase_entry_time);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.size0_positioning_time);
		sprintf(buf, "size0_positioning_time : %d\n", pGlobalData->current_setting.size0_positioning_time);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.size1_positioning_time);
		sprintf(buf, "size1_positioning_time : %d\n", pGlobalData->current_setting.size1_positioning_time);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.size2_positioning_time);
		sprintf(buf, "size2_positioning_time : %d\n", pGlobalData->current_setting.size2_positioning_time);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.size3_positioning_time);
		sprintf(buf, "size3_positioning_time : %d\n", pGlobalData->current_setting.size3_positioning_time);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.ip_eject_time);
		sprintf(buf, "ip_eject_time : %d\n", pGlobalData->current_setting.ip_eject_time);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		// 2025-07-03. jg kim. Eject speed 항목 추가
		wsprintf(c1_data, L"%d", pGlobalData->current_setting.eject_speed);
		sprintf(buf, "ip_eject_speed : %d\n", pGlobalData->current_setting.eject_speed);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.parking_length);
		sprintf(buf, "parking_length : %d\n", pGlobalData->current_setting.parking_length);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.max_speed);
		sprintf(buf, "max_speed : %d\n", pGlobalData->current_setting.max_speed);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		// Laser On/Off 구간 조절 0 ~ 5 // 2024-12-09. jg kim. 표시 디버깅
		// 2026-01-07. jg kim. Wobble 보정 관련 좌/우 Laser On/Off, 세로 줄 시작/끝 위치 
		unsigned short B1 = 0;
		unsigned short B2 = 0;
		unsigned short B3 = 0;
		unsigned short B4 = 0;
		unpackData(B1, B2, B3, B4, pGlobalData->current_setting.stcali.data[0]);

		count = 34;
		wsprintf(c1_data, L"%d", (int)B1);
		sprintf(buf, "L-Laser On position (pixel unit) : %d\n", B1);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", (int)B2);
		sprintf(buf, "L-Laser Off position (pixel unit) : %d\n", B2);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", (int)B3);
		sprintf(buf, "R-Laser On position (pixel unit) : %d\n", B3);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", (int)B4);
		sprintf(buf, "R-Laser Off position (pixel unit) : %d\n", B4);
		writelog(buf, LOG_FILE_NAME);

		unpackData(B1, B2, B3, B4, pGlobalData->current_setting.stcali.data[1]);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", (int)B1);
		sprintf(buf, "L-Vert. Line Start x (pixel unit) : %d\n", B1);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", (int)B2);
		sprintf(buf, "L-Vert. Line End x (pixel unit) : %d\n", B2);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", (int)B3);
		sprintf(buf, "R-Vert. Line Start x (pixel unit) : %d\n", B3);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", (int)B4);
		sprintf(buf, "R-Vert. Line End x (pixel unit) : %d\n", B4);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		count = 42;
#if (DEF_BACKWARD_OFFSET_LENGTH)
		wsprintf(c1_data, L"%d", pGlobalData->current_setting.Backward_Offset_Length);
		sprintf(buf, "Backward_Offset_Length : %d\n", pGlobalData->current_setting.Backward_Offset_Length);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);
#endif

		//! kiwa72(2023.04.26 14h) - 터치 보정 구조체 적용
		wsprintf(c1_data, L"%d", pGlobalData->current_setting.sttouch.calibrated_checksum);
		sprintf(buf, "calibrated_checksum : %d\n", pGlobalData->current_setting.sttouch.calibrated_checksum);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.sttouch.touchleft);
		sprintf(buf, "touchleft : %d\n", pGlobalData->current_setting.sttouch.touchleft);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.sttouch.touchright);
		sprintf(buf, "touchright : %d\n", pGlobalData->current_setting.sttouch.touchright);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.sttouch.touchtop);
		sprintf(buf, "touchtop : %d\n", pGlobalData->current_setting.sttouch.touchtop);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);

		wsprintf(c1_data, L"%d", pGlobalData->current_setting.sttouch.touchbottom);
		sprintf(buf, "touchbottom : %d\n", pGlobalData->current_setting.sttouch.touchbottom);
		writelog(buf, LOG_FILE_NAME);
		m_admin_scanner_info.SetItemText(count++, 1, c1_data);
		m_HARDWARE_SETTING = pGlobalData->current_setting; // 2024-08-23. jg kim. 현재 setting update
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
int CAdminScannerInfo::update_list_to_current(void)
{
	MYLOGOLDW(L">>>>CAdminScannerInfo::update_list_to_current\n");

	CString str;
	char str_ch[100];
	int invalid_num = -1;
	int result_int[6];
	int value;
    int count = 0;

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	str = m_admin_scanner_info.GetItemText(count++, 1);
	if (str.GetLength() >= sizeof(pGlobalData->current_setting.Name))
	{
		invalid_num = count - 1;//0_T("Name"),
	}
	else
	{
		int n_len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)str, -1, NULL, 0, NULL, NULL);
		WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)str, n_len, str_ch, 100, NULL, NULL);
		str_ch[n_len] = '\0';
		strcpy(pGlobalData->current_setting.Name, str_ch);
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	if (verify_4int(str, result_int))
	{
		pGlobalData->current_setting.Firmware_version2[0] = (UCHAR)result_int[0];
		pGlobalData->current_setting.Firmware_version2[1] = (UCHAR)result_int[1];
		pGlobalData->current_setting.Firmware_version2[2] = (UCHAR)result_int[2];
		pGlobalData->current_setting.Firmware_version2[3] = (UCHAR)result_int[3];
	}
	else
	{
		invalid_num = count - 1;//7_T("Firmware Version (Current)"),
	}

	// kiwa72(2023.03.22 14h) - 불필요한 항목 제외, touch cal 설정값 추가
	str = m_admin_scanner_info.GetItemText(count++, 1);
	if (verify_4int(str, result_int))
	{
		pGlobalData->current_setting.ipaddr[0] = (UCHAR)result_int[0];
		pGlobalData->current_setting.ipaddr[1] = (UCHAR)result_int[1];
		pGlobalData->current_setting.ipaddr[2] = (UCHAR)result_int[2];
		pGlobalData->current_setting.ipaddr[3] = (UCHAR)result_int[3];
	}
	else
	{
		invalid_num = count - 1;//10_T("IP Address"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	if (verify_4int(str, result_int))
	{
		pGlobalData->current_setting.netmask[0] = (UCHAR)result_int[0];
		pGlobalData->current_setting.netmask[1] = (UCHAR)result_int[1];
		pGlobalData->current_setting.netmask[2] = (UCHAR)result_int[2];
		pGlobalData->current_setting.netmask[3] = (UCHAR)result_int[3];
	}
	else
	{
		invalid_num = count - 1;//11_T("Subnet Mask"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	if (verify_4int(str, result_int))
	{
		pGlobalData->current_setting.gateway[0] = (UCHAR)result_int[0];
		pGlobalData->current_setting.gateway[1] = (UCHAR)result_int[1];
		pGlobalData->current_setting.gateway[2] = (UCHAR)result_int[2];
		pGlobalData->current_setting.gateway[3] = (UCHAR)result_int[3];
	}
	else
	{
		invalid_num = count - 1;//12_T("GateWay"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	if (verify_6hex(str, result_int))
	{
		pGlobalData->current_setting.MAC_ADDR[0] = (UCHAR)result_int[0];
		pGlobalData->current_setting.MAC_ADDR[1] = (UCHAR)result_int[1];
		pGlobalData->current_setting.MAC_ADDR[2] = (UCHAR)result_int[2];
		pGlobalData->current_setting.MAC_ADDR[3] = (UCHAR)result_int[3];
		pGlobalData->current_setting.MAC_ADDR[4] = (UCHAR)result_int[4];
		pGlobalData->current_setting.MAC_ADDR[5] = (UCHAR)result_int[5];
	}
	else
	{
		invalid_num = count - 1;//13_T("MAC Address"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1).MakeLower();
	if (str.Compare(L"HR") == 0 || str.Compare(L"hr") == 0 || str.Compare(L"Hr") == 0)
	{
		pGlobalData->current_setting.image_resolution = eir_HiRes;
	}
	else if (str.Compare(L"SR") == 0 || str.Compare(L"sr") == 0 || str.Compare(L"Sr") == 0)
	{
		pGlobalData->current_setting.image_resolution = eir_StRes;
	}
	else
	{
		invalid_num = count - 1;//15_T("Resolution"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	if(str.MakeLower() == L"on")
	{
		pGlobalData->current_setting.useDHCP = 1;
	}
	else if (str.MakeLower() == L"off")
	{
		pGlobalData->current_setting.useDHCP = 0;
	}
	else
	{
		invalid_num = count - 1;//16_T("DHCP Mode"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value <= 1000000000)
	{
		pGlobalData->current_setting.BLDC_speed = value;
	}
	else
	{
		invalid_num = count - 1;//17	_T("BLDC Speed"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value <= 100000)
	{
		pGlobalData->current_setting.BLDC_shift_index = value;
	}
	else
	{
		invalid_num = count - 1;//18	_T("BLDC Shift Index"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	pGlobalData->current_setting.laser_power = (value > 128) ? 128 : ((value < 0) ? 0 : value);// 30     _T("laser power"),

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value <= 65535)
	{
		pGlobalData->current_setting.MPPC1_bias = value;
	}
	else
	{
		invalid_num = count - 1;//27 _T("MPPC Bias1"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value <= 65535)
	{
		pGlobalData->current_setting.MPPC2_bias = value;
	}
	else
	{
		invalid_num = count - 1;//28 _T("MPPC Bias2"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value <= 65535)
	{
		pGlobalData->current_setting.MPPC3_bias = value;
	}
	else
	{
		invalid_num = count - 1;//29 _T("MPPC Bias3"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 50 && value <= 200)
	{
		pGlobalData->current_setting.LaserOnTime = value;
	}
	else
	{
		invalid_num = count - 1;//35 _T("Delay Pixel value"),//_T("Laser On Time(50~200)"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (1)
	{
		pGlobalData->current_setting.start_speed = value;
	}
	else
	{
		invalid_num = count - 1;
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value <= 100000)
	{
		pGlobalData->current_setting.SM_speed = value;
	}
	else
	{
		invalid_num = count - 1;//26 _T("Step Motor Speed"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value <= 100000)
	{
		pGlobalData->current_setting.erase_speed = value; //31 _T("Erase Speed"),
	}
	else
	{
		pGlobalData->current_setting.erase_speed = 7500; //31 _T("Erase Speed"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (1)
	{
		pGlobalData->current_setting.normal_speed = value;
	}
	else
	{
		invalid_num = count - 1;
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (1)
	{
		pGlobalData->current_setting.sm_step_val = value;
	}
	else
	{
		invalid_num = count - 1;
	}	

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (1)
	{
		pGlobalData->current_setting.sm_step_dur = value;
	}
	else
	{
		invalid_num = count - 1;
	}	

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value >= 100000000)
	{
		pGlobalData->current_setting.scan_entry_time_0 = value;
	}
	else
	{
		invalid_num = count - 1;//23 _T("Stage Entry Time0"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value >= 100000000)
	{
		pGlobalData->current_setting.scan_entry_time_1 = value;
	}
	else
	{
		invalid_num = count - 1;//23 _T("Stage Entry Time1"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value >= 100000000)
	{
		pGlobalData->current_setting.scan_entry_time_2 = value;
	}
	else
	{
		invalid_num = count - 1;//23 _T("Stage Entry Time2"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value >= 100000000)
	{
		pGlobalData->current_setting.scan_entry_time_3 = value;
	}
	else
	{
		invalid_num = count - 1;//23 _T("Stage Entry Time3"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (value >= 0 && value >= 100000000)
	{
		pGlobalData->current_setting.erase_entry_time = value;
	}
	else
	{
		invalid_num = count - 1;//24 _T("Erase Entry Time"),
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	//if (value >= 50 && value <= 200)
	if (1)
	{
		pGlobalData->current_setting.size0_positioning_time = value;//ip0 positoning time
	}
	else
	{
		invalid_num = count - 1;
	}	

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	//if (value >= 50 && value <= 200)
	if (1)
	{
		pGlobalData->current_setting.size1_positioning_time = value;//ip1 positoning time
	}
	else
	{
		invalid_num = count - 1;
	}	str = m_admin_scanner_info.GetItemText(count++, 1);

	value = _tstoi(str);
	//if (value >= 50 && value <= 200)
	if (1)
	{
		pGlobalData->current_setting.size2_positioning_time = value; // ip2 positoning time
	}
	else
	{
		invalid_num = count - 1;
	}	str = m_admin_scanner_info.GetItemText(count++, 1);

	value = _tstoi(str);
	//if (value >= 50 && value <= 200)
	if (1)
	{
		pGlobalData->current_setting.size3_positioning_time = value;//ip3 positoning time
	}
	else
	{
		invalid_num = count - 1;
	}	

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	//if (value >= 50 && value <= 200)
	if (1)
	{
		pGlobalData->current_setting.ip_eject_time = value;
	}
	else
	{
		invalid_num = count - 1;
	}	
	// 2025-07-03. jg kim. Eject speed 항목 추가
	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	if (1)
	{
		pGlobalData->current_setting.eject_speed = value;
	}
	else
	{
		invalid_num = count - 1;
	}

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	//if (value >= 50 && value <= 200)
	if (1)
	{
		pGlobalData->current_setting.parking_length = value;
	}
	else
	{
		invalid_num = count - 1;
	}	

	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	//if (value >= 50 && value <= 200)
	if (1)
	{
		pGlobalData->current_setting.max_speed = value;
	}
	else
	{
		invalid_num = count - 1;
	}


	pGlobalData->current_setting.stcali.sigs[0] = 65533, pGlobalData->current_setting.stcali.sigs[1] = 1, pGlobalData->current_setting.stcali.sigs[2] = 65531, pGlobalData->current_setting.stcali.sigs[3] = 65534;
	pGlobalData->current_setting.stcali.sige[0] = 65533, pGlobalData->current_setting.stcali.sige[1] = 1, pGlobalData->current_setting.stcali.sige[2] = 65531, pGlobalData->current_setting.stcali.sige[3] = 65533;
	
	m_user_input.GetItemText(1,1);
	
	// 2026-01-07. jg kim. Wobble 보정 관련 좌/우 Laser On/Off, 세로 줄 시작/끝 위치 
	count = 34;
	str = m_admin_scanner_info.GetItemText(count++, 1);
	unsigned short B1 = (unsigned short)_tstoi(str);
	str = m_admin_scanner_info.GetItemText(count++, 1);
	unsigned short B2 = (unsigned short)_tstoi(str);
	str = m_admin_scanner_info.GetItemText(count++, 1);
	unsigned short B3 = (unsigned short)_tstoi(str);
	str = m_admin_scanner_info.GetItemText(count++, 1);
	unsigned short B4 = (unsigned short)_tstoi(str);
	str = m_admin_scanner_info.GetItemText(count++, 1);
	unsigned short B5 = (unsigned short)_tstoi(str);
	str = m_admin_scanner_info.GetItemText(count++, 1);
	unsigned short B6 = (unsigned short)_tstoi(str);
	str = m_admin_scanner_info.GetItemText(count++, 1);
	unsigned short B7 = (unsigned short)_tstoi(str);
	str = m_admin_scanner_info.GetItemText(count++, 1);
	unsigned short B8 = (unsigned short)_tstoi(str);
	
	pGlobalData->current_setting.stcali.data[0] = packData(B1, B2, B3, B4);
	pGlobalData->current_setting.stcali.data[1] = packData(B5, B6, B7, B8);

	count = 0;
	str = m_user_input.GetItemText(count++, 1);
	str = m_user_input.GetItemText(count++, 1);//BackwaredOffset.
	double fBackwaredOffset = _tstof(str);
	uint16_t nEncodedBackwaredOffset = encode_bias_u16(fBackwaredOffset, 10);
	// 2025-06-18. jg kim. 영상에서 파라미터 추출하는 것을 하지 않도록 변경
	str = m_user_input.GetItemText(count++, 1);//PositioningOffset.
	double fPositioningOffset = _tstof(str);
	uint16_t nEncodedPositioningOffset = encode_bias_u16(fPositioningOffset, 10);// 소숫점 1자리까지
	str = m_user_input.GetItemText(count++, 1);
	double fEntryOffsetPixel2 = _tstof(str);
	uint16_t nEncodedEntryOffsetPixel2 = encode_bias_u16(fEntryOffsetPixel2, 1);//소숫점 사용 안함
	str = m_user_input.GetItemText(count++, 1);
	double fEntryOffsetPixel3 = _tstof(str);
	uint16_t nEncodedEntryOffsetPixel3 = encode_bias_u16(fEntryOffsetPixel3, 1);
	pGlobalData->current_setting.stcali.data[2] = packData(nEncodedBackwaredOffset, nEncodedPositioningOffset, nEncodedEntryOffsetPixel2, nEncodedEntryOffsetPixel3);

	count = 42;
#if (DEF_BACKWARD_OFFSET_LENGTH)
	str = m_admin_scanner_info.GetItemText(count++, 1);
	value = _tstoi(str);
	pGlobalData->current_setting.Backward_Offset_Length = value;	//49
#endif
	
	//! kiwa72(2023.04.26 14h) - 터치 보정 구조체 적용
	str = m_admin_scanner_info.GetItemText(count++, 1);
	double fvalue = _tstoi(str);
	pGlobalData->current_setting.sttouch.calibrated_checksum = (int)fvalue;	//50

	str = m_admin_scanner_info.GetItemText(count++, 1);
	fvalue = _tstoi(str);
	pGlobalData->current_setting.sttouch.touchleft = (USHORT)fvalue;		//51

	str = m_admin_scanner_info.GetItemText(count++, 1);
	fvalue = _tstoi(str);
	pGlobalData->current_setting.sttouch.touchright = (USHORT)fvalue;	//52

	str = m_admin_scanner_info.GetItemText(count++, 1);
	fvalue = _tstoi(str);
	pGlobalData->current_setting.sttouch.touchtop = (USHORT)fvalue;		//53

	str = m_admin_scanner_info.GetItemText(count++, 1);
	fvalue = _tstoi(str);
	pGlobalData->current_setting.sttouch.touchbottom = (USHORT)fvalue;	//54


	if (invalid_num != -1)
	{
		wchar_t notify_invaild[100];
		wsprintf(notify_invaild, L"\"%s\" field[%d] is invalid or the field size is over limit.", name_list[invalid_num], invalid_num);
		MessageBox(notify_invaild);
		
		//return false;
		return true;
	}

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::OnLvnItemchangedAdminScannerInfoList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	UNREFERENCED_PARAMETER(pNMLV);
	*pResult = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::OnNMDblclkAdminScannerInfoList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	iSavedItem = pNMItemActivate->iItem;
	iSavedSubitem = pNMItemActivate->iSubItem;

	if (pNMItemActivate->iItem != -1)
	{
		if (pNMItemActivate->iSubItem == 1 && is_readonly[pNMItemActivate->iItem] != 1)
		{
			CRect rect;
			m_admin_scanner_info.GetSubItemRect(pNMItemActivate->iItem, pNMItemActivate->iSubItem, LVIR_BOUNDS, rect);
			rect.left = rect.left + 5;
			rect.top = rect.top + 2;
			m_admin_scanner_info.ClientToScreen(rect);
			this->ScreenToClient(rect);

			GetDlgItem(IDC_EDIT_MODIFY)->SetWindowText(m_admin_scanner_info.GetItemText(pNMItemActivate->iItem, pNMItemActivate->iSubItem));
			GetDlgItem(IDC_EDIT_MODIFY)->SetWindowPos(NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
			GetDlgItem(IDC_EDIT_MODIFY)->SetFocus();
		}
	}

	*pResult = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::OnNMClickAdminScannerInfoList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	iSavedItem = iSavedSubitem = -1;
	GetDlgItem(IDC_EDIT_MODIFY)->SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);

	if (pNMItemActivate->iItem != -1)
	{
		iSavedItem = pNMItemActivate->iItem;
		iSavedSubitem = pNMItemActivate->iSubItem;
	}

	*pResult = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CAdminScannerInfo::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	if (pMsg->message == WM_KEYDOWN) {
		if (pMsg->wParam == VK_RETURN) {

			if (pMsg->hwnd == GetDlgItem(IDC_EDIT_MODIFY)->GetSafeHwnd())
			{
				if (iSavedItem != -1 && iSavedSubitem != -1)
				{
					CString str;
					GetDlgItemText(IDC_EDIT_MODIFY, str);
					m_admin_scanner_info.SetItemText(iSavedItem, iSavedSubitem, str);
				}

				GetDlgItem(IDC_EDIT_MODIFY)->SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
			}

			if (pMsg->hwnd == GetDlgItem(IDC_EDIT_USER_INPUT)->GetSafeHwnd())
			{
				if (iSavedItemUserInput != -1 && iSavedSubitemUserInput != -1)
				{
					CString str;
					GetDlgItemText(IDC_EDIT_USER_INPUT, str);
					m_user_input.SetItemText(iSavedItemUserInput, iSavedSubitemUserInput, str);  // 2024-08-13. jg kim. 왼쪽에서 값 입력하고 엔터키 누르면 오른쪽도 값이 같이 바뀌는 버그 수정
				}

				GetDlgItem(IDC_EDIT_USER_INPUT)->SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);
			}

			return TRUE;
		}

		if (pMsg->wParam == VK_ESCAPE)
		{
			MYLOGOLDW(L">>>>>>>>>>>>>>>>>>>>VK_ESCAPE\n");
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::OnEnChangeEditModify()
{
	if (iSavedItem != -1 && iSavedSubitem != -1)
	{
		CString str;
		GetDlgItemText(IDC_EDIT_MODIFY, str);
		m_admin_scanner_info.SetItemText(iSavedItem, iSavedSubitem, str);
	}
}

void CAdminScannerInfo::OnEnChangeEditUserInput()
{
	if (iSavedItemUserInput != -1 && iSavedSubitemUserInput != -1)
	{
		CString str;
		GetDlgItemText(IDC_EDIT_USER_INPUT, str);
		m_user_input.SetItemText(iSavedItemUserInput, iSavedSubitemUserInput, str);
	}
}

void CAdminScannerInfo::OnNMCustomdrawAdminScannerInfoList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	UNREFERENCED_PARAMETER(pNMCD);

	CString strProtocol, strRSTFIN;

	NMLVCUSTOMDRAW* pLVCD = (NMLVCUSTOMDRAW*)pNMHDR;

	*pResult = 0;

	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYSUBITEMDRAW;
	}
	else if ((CDDS_ITEMPREPAINT | CDDS_SUBITEM) == pLVCD->nmcd.dwDrawStage)
	{
		COLORREF crText = pLVCD->clrText, crBkgnd = pLVCD->clrTextBk;

		if (0 == pLVCD->iSubItem)
		{
			crText = RGB(0, 0, 0);
			crBkgnd = RGB(178, 204, 255);
		}
		else if (1 == pLVCD->iSubItem)
		{
			crText = is_readonly[pLVCD->nmcd.dwItemSpec] == 1 ? RGB(128, 128, 128) : RGB(0, 0, 0);
			crBkgnd = RGB(255, 255, 255);
		}
		pLVCD->clrText = crText;
		pLVCD->clrTextBk = crBkgnd;

		*pResult = CDRF_DODEFAULT;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
int CAdminScannerInfo::verify_4int(CString str, int *result)
{
	int count = 0;

	const int str_length = str.GetLength();
	for (int i = 0; i < str_length; i++)
	{
		if (str[i] == L'.')
		{
			count++;
		}
	}

	if (count == 3)
	{
		CString str_int[4];
		for (int i = 0; i < 4; i++)
		{
			AfxExtractSubString(str_int[i], str, i, _T('.'));
			result[i] = _tstoi(str_int[i]);
			if (result[i] < 0 || result[i] > 255)
				return false;
		}
		return true;
	}

	return false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
int CAdminScannerInfo::verify_6hex(CString str, int *result)
{
	int count = 0;

	const int str_length = str.GetLength();
	for (int i = 0; i < str_length; i++)
	{
		if (str[i] == L':')
		{
			count++;
		}
	}

	if (count == 5)
	{
		CString str_int[6];
		TCHAR *end = NULL;
		for (int i = 0; i < 6; i++)
		{
			AfxExtractSubString(str_int[i], str, i, _T(':'));
			if (str_int[i].GetLength() != 2)
				return false;

			result[i] = _tcstol(str_int[i], &end, 16);

			if (result[i] < 0 || result[i] > 255)
				return false;
		}

		return true;
	}

	return false;
}


//-------------------------------------------------------------------
// kiwa72(2023.04.21 09h) - 제노레이 요청 New Calibration Data
NewCalibrationData	g_tNewCalibrationData;
//-------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::OnBnClickedButton_Apply()
{
	if (update_list_to_current() == FALSE)
	{
		return;
	}

	if (is_scanner_busy() == 0)
	{
		// kiwa72(2023.04.21 09h) - HARDWARE_SETTING + NewCalibrationData 저장 버퍼 생성
		unsigned char* pSendData = NULL;
		// kiwa72(2023.04.21 09h) - 셈플 테스트 데이터 셋팅
		memset(g_tNewCalibrationData.data1, 0xFA, sizeof(g_tNewCalibrationData.data1));
		memset(g_tNewCalibrationData.data2, 0xFC, sizeof(g_tNewCalibrationData.data2));
		// 2023-11-22. jg kim.
		memcpy(&g_tNewCalibrationData, &GetPdxData()->g_NewCalibrationData, sizeof(NewCalibrationData));
		//-------------------------------------------------------------------

		// kiwa72(2023.04.21 09h) - nc_new_calib_hardware_info 전송
		pSendData = new unsigned char[sizeof(HARDWARE_SETTING) + sizeof(NewCalibrationData)];
		memset(pSendData, 0x0, sizeof(HARDWARE_SETTING) + sizeof(NewCalibrationData));

		char buf[1024];  // 2024-08-13. jg kim. 로그 추가

		double packed = GetPdxData()->current_setting.stcali.data[2];
		unsigned short nEncodedBackwardOffset, nEncodedPositioningOffset, nEncodedEntry2, nEncodedEntry3;
		unpackData(nEncodedBackwardOffset, nEncodedPositioningOffset, nEncodedEntry2, nEncodedEntry3, packed);

		double fBackwardOffset = decode_bias_u16(nEncodedBackwardOffset, 10);
		double fPositioningOffset = decode_bias_u16(nEncodedPositioningOffset, 10);
		double fEntry2 = (decode_bias_u16(nEncodedEntry2, 1));
		double fEntry3 = (decode_bias_u16(nEncodedEntry3, 1));

		sprintf(buf, "BackwardOffset = %.1f\n", fBackwardOffset);
		writelog(buf, LOG_FILE_NAME);
		sprintf(buf, "PositioningOffset = %.1f\n", fPositioningOffset);
		writelog(buf, LOG_FILE_NAME);
		sprintf(buf, "Entry2 = %.0f\n", fEntry2);
		writelog(buf, LOG_FILE_NAME);
		sprintf(buf, "Entry3 = %.0f\n", fEntry3);
		writelog(buf, LOG_FILE_NAME);

		int offset = 0;
		memcpy(pSendData + offset, &GetPdxData()->current_setting, sizeof(HARDWARE_SETTING));
		offset += sizeof(HARDWARE_SETTING);
		memcpy(pSendData + offset, &g_tNewCalibrationData,	sizeof(NewCalibrationData));

		BOOL send_ret = Send_FreeData_To_Device(nc_new_calib_hardware_info, pSendData, sizeof(HARDWARE_SETTING) + sizeof(NewCalibrationData));

		SAFE_DELETE_ARRAY(pSendData);

		if (send_ret == 0)
		{
			MessageBox(L"Send Success.");	// kiwa72(2023.01.21 18h) - 뭘 보고 Success 냐?
		}
		else
		{
			MessageBox(L"Send Failed.");
		}
	}
	else
	{
		MessageBox(L"Not Connected or scanner is not ready");
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::OnBnClickedButtonLoad()
{ // 2024-08-13. jg kim. 로그 추가
	char buf[1024];
	sprintf(buf, "");
	writelog(buf, LOG_FILE_NAME);
	// kiwa72(2023.08.05 11h) - SW -> FW | 하드웨어 설정 정보 + New Calibration Data 요청함
	send_protocol_to_device(nc_request_hardware_info, 0, 0);  // 2024-08-13. jg kim. 디버깅.

	// kiwa72(2022.12.06 00h) - 장비 하드웨어 정보 로딩 후 화면 갱신
	update_current_to_list();

	UpdateData(FALSE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::OnBnClickedButtonSavepc()
{
	if (update_list_to_current())
	{
		m_HARDWARE_SETTING = GetPdxData()->current_setting; // 2024-08-23. jg kim. 현재 setting update
		
		// 실행 파일 경로 얻기
		TCHAR szExePath[MAX_PATH];
		GetModuleFileName(NULL, szExePath, MAX_PATH);
		CString strExeDir = szExePath;
		int nPos = strExeDir.ReverseFind(_T('\\'));
		if (nPos >= 0)
			strExeDir = strExeDir.Left(nPos);
		
		static TCHAR BASED_CODE szFilter[] = _T("All Files(*.*)|*.*||");
		CFileDialog dlg(FALSE, _T("*.* | *"), _T("scanner_info.dat"), OFN_HIDEREADONLY, szFilter);
		dlg.m_ofn.lpstrInitialDir = strExeDir;
		if (IDOK == dlg.DoModal())
		{
			CString strPathName = dlg.GetPathName();

			FILE* fp = _tfopen((LPCTSTR)strPathName, _T("wb+"));
			if (fp)
			{
				// kiwa72(2023.01.21 18h) - New Calibration Data
				//fwrite(&GetPdxData()->current_setting, BLOCK_SIZE, 1, fp);
				fwrite(&GetPdxData()->current_setting, HARDWARE_SETTING_SIZE, 1, fp);
				fclose(fp);
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CAdminScannerInfo::OnBnClickedButtonLoadpc()
{
	// 실행 파일 경로 얻기
	TCHAR szExePath[MAX_PATH];
	GetModuleFileName(NULL, szExePath, MAX_PATH);
	CString strExeDir = szExePath;
	int nPos = strExeDir.ReverseFind(_T('\\'));
	if (nPos >= 0)
		strExeDir = strExeDir.Left(nPos);
	
	static TCHAR BASED_CODE szFilter[] = _T("All Files(*.*)|*.*||");
	CFileDialog dlg(TRUE, _T("*.*|*"), _T("scanner_info.dat"), OFN_HIDEREADONLY, szFilter);
	dlg.m_ofn.lpstrInitialDir = strExeDir;
	if (IDOK == dlg.DoModal())
	{
		CString strPathName = dlg.GetPathName();

		FILE* fp = _tfopen((LPCTSTR)strPathName, _T("rb"));
		if (fp)
		{
			// kiwa72(2023.01.21 18h) - New Calibration Data
			//fread(&GetPdxData()->current_setting, BLOCK_SIZE, 1, fp);
			fread(&GetPdxData()->current_setting, HARDWARE_SETTING_SIZE, 1, fp);
			fclose(fp);
			m_HARDWARE_SETTING = GetPdxData()->current_setting; // 2024-08-23. jg kim. 현재 setting update
			update_current_to_list();
			UpdateData(FALSE);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2026-02-27. Backup function - Creates "MAC_ADDR_Backup" folder and copies scanner_info.dat and predix.ini
void CAdminScannerInfo::OnBnClickedButtonBackup()
{
	if (update_list_to_current())
	{
		auto pGlobalData = GetPdxData();
		ASSERT(pGlobalData);

		// MAC 주소를 문자열로 변환
		CString strMacAddr;
		strMacAddr.Format(_T("%02X-%02X-%02X-%02X-%02X-%02X"),
			pGlobalData->current_setting.MAC_ADDR[0],
			pGlobalData->current_setting.MAC_ADDR[1],
			pGlobalData->current_setting.MAC_ADDR[2],
			pGlobalData->current_setting.MAC_ADDR[3],
			pGlobalData->current_setting.MAC_ADDR[4],
			pGlobalData->current_setting.MAC_ADDR[5]);

		// 백업 폴더 이름 생성
		CString strBackupFolder;
		strBackupFolder.Format(_T(".\\%s_Backup"), strMacAddr.GetString());

		// 백업 폴더 생성
		if (!CreateDirectory(strBackupFolder, NULL))
		{
			// 폴더가 이미 존재하는 경우는 무시
			DWORD dwError = GetLastError();
			if (dwError != ERROR_ALREADY_EXISTS)
			{
				MessageBox(_T("백업 폴더 생성에 실패했습니다."), _T("오류"), MB_OK | MB_ICONERROR);
				return;
			}
		}

		// scanner_info.dat 파일 저장
		CString strScannerInfoPath;
		strScannerInfoPath.Format(_T("%s\\scanner_info.dat"), strBackupFolder.GetString());

		FILE* fp = _tfopen((LPCTSTR)strScannerInfoPath, _T("wb+"));
		if (fp)
		{
			fwrite(&pGlobalData->current_setting, HARDWARE_SETTING_SIZE, 1, fp);
			fclose(fp);
		}
		else
		{
			MessageBox(_T("scanner_info.dat 파일 저장에 실패했습니다."), _T("오류"), MB_OK | MB_ICONERROR);
			return;
		}

		// predix.ini 파일 복사
		CString strPredixIniSrc = _T(".\\predix.ini");
		CString strPredixIniDst;
		strPredixIniDst.Format(_T("%s\\predix.ini"), strBackupFolder.GetString());

		if (!CopyFile(strPredixIniSrc, strPredixIniDst, FALSE))
		{
			DWORD dwError = GetLastError();
			if (dwError == ERROR_FILE_NOT_FOUND)
			{
				MessageBox(_T("predix.ini 파일을 찾을 수 없습니다."), _T("경고"), MB_OK | MB_ICONWARNING);
			}
			else
			{
				MessageBox(_T("predix.ini 파일 복사에 실패했습니다."), _T("오류"), MB_OK | MB_ICONERROR);
				return;
			}
		}

		CString strMsg;
		strMsg.Format(_T("백업이 완료되었습니다.\n\n폴더: %s\n\n파일:\n- scanner_info.dat\n- predix.ini"), strBackupFolder.GetString());
		MessageBox(strMsg, _T("백업 완료"), MB_OK | MB_ICONINFORMATION);
	}
}

void CAdminScannerInfo::OnBnClickedButtonDefault()// Step 0
{
	// 실행 파일 경로의 image*.log 파일 삭제
	TCHAR szExePath[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, szExePath, MAX_PATH);
	CString strExeDir = szExePath;
	int nPos = strExeDir.ReverseFind(_T('\\'));
	if (nPos > 0)
	{
		strExeDir = strExeDir.Left(nPos + 1);
		CString strSearchPattern = strExeDir + _T("image*.log");
		
		WIN32_FIND_DATA findFileData;
		HANDLE hFind = FindFirstFile(strSearchPattern, &findFileData);
		
		if (hFind != INVALID_HANDLE_VALUE)
		{
			do
			{
				if (!(findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					CString strFilePath = strExeDir + findFileData.cFileName;
					DeleteFile(strFilePath);
				}
			} while (FindNextFile(hFind, &findFileData) != 0);
			
			FindClose(hFind);
		}
	}

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);
	// 작업을 하다 초기화를 할 경우가 있어서 여기서 초기값을 넣도록 했음.
	// 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
	pGlobalData->g_nMeasuredScanSpeed = SCAN_SPEED_DEFAULT;
	double fBackwaredOffset = BACKWARD_OFFSET_DEFAULT;
	// 2025-06-18. jg kim. 초기값 항목 추가
	double fPositiningOffset = POSITIONING_OFFSET_DEFAULT;
	pGlobalData->current_setting.ip_eject_time = int(double(EJECT_TIME_DEFAULT)*1e8);
	int nEntryOffsetPixel2 = ENTRY_OFFSET_DEFAULT2; // 2024-10-23. jg kim. 최초 개발했을때와 차이가 커져 기본값 변경(330 -> 430).
	int nEntryOffsetPixel3 = ENTRY_OFFSET_DEFAULT3;
	pGlobalData->current_setting.BLDC_shift_index = BLDC_SHIFT_INDEX_DEFAULT;

	wchar_t c1_data[100] = { 0, };
	int count = 0;
	CString str;
	wsprintf(c1_data, L"%d", pGlobalData->g_nMeasuredScanSpeed);
	m_user_input.SetItemText(count++, 1, c1_data); // 0

	str.Format(L"%.1f", fBackwaredOffset); wcscpy_s(c1_data, str);
	m_user_input.SetItemText(count++, 1, c1_data); // 1

	str.Format(L"%.1f", fPositiningOffset); wcscpy_s(c1_data, str);
	m_user_input.SetItemText(count++, 1, c1_data); // 2

	wsprintf(c1_data, L"%d", nEntryOffsetPixel2);
	m_user_input.SetItemText(count++, 1, c1_data); // 3

	wsprintf(c1_data, L"%d", nEntryOffsetPixel3);
	m_user_input.SetItemText(count++, 1, c1_data); // 4

	double fBldcRPM = BLDC_RPM_DEFAULT; // 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
	count = 11;
	str.Format(L"%.3f", fBldcRPM); wcscpy_s(c1_data, str);
	m_user_input.SetItemText(count++, 1, c1_data);
	
	count = 34;
	unsigned short B1 = 0;
	unsigned short B2 = 0;
	unsigned short B3 = 0;
	unsigned short B4 = 0;
	unpackData(B1, B2, B3, B4, pGlobalData->current_setting.stcali.data[0]);
	B1=0;
	B2=0;
	pGlobalData->current_setting.stcali.data[0] = packData(B1, B2, B3, B4);
	unpackData(B1, B2, B3, B4, pGlobalData->current_setting.stcali.data[1]);
	B1=0;
	B2=0;
	pGlobalData->current_setting.stcali.data[1] = packData(B1, B2, B3, B4);
	for(int i=0; i<MAX_CALI_IMAGE_COUNT; i++)
		pGlobalData->g_NewCalibrationData.data1[i] = 0;
	for (int i = 0; i < (DEGREE_COUNT + 1)* MAX_CALI_IMAGE_COUNT; i++)
		pGlobalData->g_NewCalibrationData.data2[i] = 0;
	OnBnClickedButtonCalculate();
}

void CAdminScannerInfo::OnBnClickedButtonBldcScanSpeed()
{
	char buf[1024];
	sprintf(buf, "");
	writelog(buf, LOG_FILE_NAME);
	// 2024-11.12 jg kim. 기존에 여기서 실행하던 parameter extraction 함수를 thread에서 실행.
	
	OnBnClickedButtonCalculate();
	// 2025-06-18. jg kim. 영상에서 파라미터 추출하는 것을 하지 않도록 변경
	//int nValue = 0;		// event message ID
	//SetTimer(_TIMERID_PARAMETER_EXTRACTION_INFO, 500, NULL);// 2024-11.12 jg kim. 500ms마다 thread가 끝났는지 검사
	//g_bResultParameterExtraction = false; // 2024-11.15 jg kim. 초기화 추가
	//g_bDoneParameterExtraction = false;
	//AfxBeginThread(parameter_extraction_thread_function, &nValue, THREAD_PRIORITY_NORMAL, 0, 0);//myresult가 지역이면 thread동작중 	
}

void CAdminScannerInfo::OnBnClickedButtonBackwardOffset()
{
	// 2024-08-23. jg kim. 작업자 실수를 방지하기 위해 장비에 파라미터를 요청하고 타이머를 작동시킴
	send_protocol_to_device(nc_request_hardware_info, 0, 0);  // 2024-08-29. jg kim. 파라미터를 요청하는 코드가 빠져 추가
	m_bBackwardOffset = true;
	SetTimer(_TIMERID_HARDWARE_INFO, 500, NULL);
}

void CAdminScannerInfo::OnBnClickedButtonEntryOffset()
{
	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);
	char buf[1024];
	sprintf(buf, "");
	writelog(buf,LOG_FILE_NAME);

	//CString str;
	//int count = 0;
	//str = m_user_input.GetItemText(count++, 1);
	//str = m_user_input.GetItemText(count++, 1);
	//str = m_user_input.GetItemText(count++, 1);			// 2025-06-18. jg kim. Positioning offset 항목 추가
	//str = m_user_input.GetItemText(count++, 1);			int nEntryOffsetPixel2 = _tstoi(str); // Entry Offset Pixel 2
	//str = m_user_input.GetItemText(count++, 1);			int nEntryOffsetPixel3 = _tstoi(str); // Entry Offset Pixel 3
	OnBnClickedButtonCalculate();
	if (pGlobalData->map_origin.data() != null)
	{
		// 2025-06-18. jg kim. 영상에서 파라미터 추출하는 것을 하지 않도록 변경
		//float fAspectRatio = -1;
		//int nBldcShiftPixels = 65535;
		//int  width = geo_x;
		//int  height = geo_y;

		//int nScanMode = -1;
		//CHardwareInfo hi;
		//unsigned short nmean[10];
		//float coeff[50];
		//unsigned char mac[6];
		//// 2025-05-07. jg kim. 사용하지는 않지만, decodeGenorayTag 함수를 사용하기 위해 fdate, cur_info_index 설정
		//double fdate = 0;
		//unsigned short cur_info_index = 65535;
		//HI_decodeGenorayTag(pGlobalData->map_origin.data(), width, nmean, coeff, nScanMode, fdate, cur_info_index, mac);

		//int x;
		//int y;
		//int nBoundingWidth;
		//int nBoundingHeight;
		//int nKernelSize = 25;
		//unsigned short* source_img = new unsigned short[width * height];
		//memcpy(source_img, pGlobalData->map_origin.data(), sizeof(unsigned short)*width*height);
		//setLogFileNamePE(buf_filename);
		//getScannerTunningParameters(fAspectRatio, nBldcShiftPixels, x, y, nBoundingWidth, nBoundingHeight, source_img, width, height, nKernelSize, false);		

		//sprintf(buf, "Size%d, Boundin Rect = (x, y, width, height) : (%d, %d, %d, %d). Kernel size = %d\n",nScanMode, x,y, nBoundingWidth, nBoundingHeight, nKernelSize);
		//writelog(buf,LOG_FILE_NAME);

		//int nRefYEnd = 60 + 1640;
		//int nMeasuredYEnd = y + nBoundingHeight;
		//int nyOffset = nRefYEnd - nMeasuredYEnd;

		//sprintf(buf, "RefYend = %d, MeasuredYend = %d, offset = %d\n", nRefYEnd, nMeasuredYEnd, nyOffset);
		//writelog(buf,LOG_FILE_NAME);
		//SAFE_DELETE_ARRAY(source_img);

		//wchar_t c1_data[100] = { 0, };
		//count = 3;
		//if (nScanMode == 2)
		//{
		//	pGlobalData->g_nEntryOffsetPixel2 += nyOffset;
		//	wsprintf(c1_data, L"%d", pGlobalData->g_nEntryOffsetPixel2);
		//	m_user_input.SetItemText(count++, 1, c1_data);
		//	AfxMessageBox(L"Complete calculation of Entry time 2.");
		//}
		//else if (nScanMode == 3)
		//{
		//	count++;
		//	pGlobalData->g_nEntryOffsetPixel3 += nyOffset;
		//	wsprintf(c1_data, L"%d", pGlobalData->g_nEntryOffsetPixel3);
		//	m_user_input.SetItemText(count++, 1, c1_data);
		//	AfxMessageBox(L"Complete calculation of Entry time 3.");
		//}
	}
	else
	{
		AfxMessageBox(L"Please Scan image.");
	}
}

void CAdminScannerInfo::OnBnClickedButtonCalculate()
{
	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);
	CString str;
	int count = 0;
	
	str = m_user_input.GetItemText(count++, 1);			double fMeasuredScanSpeed = _tstof(str);
	str = m_user_input.GetItemText(count++, 1);//BackwaredOffset.
	double fBackwaredOffset = _tstof(str);
	uint16_t nEncodedBackwaredOffset = encode_bias_u16(fBackwaredOffset, 10);
	// 2025-06-18. jg kim. 영상에서 파라미터 추출하는 것을 하지 않도록 변경
	str = m_user_input.GetItemText(count++, 1);//PositioningOffset.
	double fPositioningOffset = _tstof(str);
	uint16_t nEncodedPositioningOffset = encode_bias_u16(fPositioningOffset, 10);// 소숫점 1자리까지
	str = m_user_input.GetItemText(count++, 1);
	double fEntryOffsetPixel2 = _tstof(str);
	uint16_t nEncodedEntryOffsetPixel2 = encode_bias_u16(fEntryOffsetPixel2, 1);//소숫점 사용 안함
	str = m_user_input.GetItemText(count++, 1);
	double fEntryOffsetPixel3 = _tstof(str);
	uint16_t nEncodedEntryOffsetPixel3 = encode_bias_u16(fEntryOffsetPixel3, 1);
	pGlobalData->current_setting.stcali.data[2] = packData(nEncodedBackwaredOffset, nEncodedPositioningOffset, nEncodedEntryOffsetPixel2, nEncodedEntryOffsetPixel3);

	char buf[1024];
	// 2024-06-26. jg kim. MPPC bias 전압 계산을 위해 추가.

	//CHardwareInfo hi;
	int MppcVoltageLow = MPPC_VOLTAGE_LOWER_BOUND; // 2024-10-25. MPPC 전압 조정범위 조정 (53~56V -> 50~59V)
 	int MppcVoltageHigh = MPPC_VOLTAGE_UPPER_BOUND;
	int MppcVoltages[MPPC_COUNT] = { 0, };
	wchar_t c1_data[100] = { 0, };
	count = 5;
	for (int i = 0; i < MPPC_COUNT; i++)
	{
		double voltage = _tstof(m_user_input.GetItemText(count + i * 2, 1));
		voltage += _tstof(m_user_input.GetItemText(count + i * 2 + 1, 1));
		double offset = 0;
		MppcVoltages[i] = int(voltage * 1000);
		MppcVoltages[i] = MppcVoltages[i] > MppcVoltageHigh ? MppcVoltageHigh : (MppcVoltages[i] < MppcVoltageLow ? MppcVoltageLow : MppcVoltages[i] );
		str.Format(L"%.2f", voltage); wcscpy_s(c1_data, str);
		m_user_input.SetItemText(count + i * 2, 1, c1_data);
		str.Format(L"%.2f", offset); wcscpy_s(c1_data, str); // mppc voltage에 offset을 더했으므로 offset은 무조건 0으로 함.
		m_user_input.SetItemText(count + i * 2 + 1, 1, c1_data);
	}
	count = 11;  // 2024-07-15. jg kim. BLDC RPM 예외처리.
	double fBldcRPM = _tstof(m_user_input.GetItemText(count, 1));
	if (fBldcRPM > BLDC_MAX_RPM)
	{
		fBldcRPM = BLDC_MAX_RPM; // spec상 최대 RPM
		wsprintf(c1_data, L"Exceed BLDC maximum RPM. It adjusted 2202 RPM.");
		MessageBox(c1_data);

		str.Format(L"%.3f", fBldcRPM); wcscpy_s(c1_data, str); // 2024-11-28. jg kim. BLDC RPM 표시 개선
		m_user_input.SetItemText(count, 1, c1_data);
	}
	pGlobalData->current_setting.BLDC_speed = HI_getBldcSpeed(fBldcRPM, MAIN_CLOCK_FREQUENCY);

	count = 0;
	pGlobalData->current_setting.MPPC1_bias = HI_Mppc_Convert_VoltageToDigitalValue(MppcVoltages[count++]);
	pGlobalData->current_setting.MPPC2_bias = HI_Mppc_Convert_VoltageToDigitalValue(MppcVoltages[count++]);
	pGlobalData->current_setting.MPPC3_bias = HI_Mppc_Convert_VoltageToDigitalValue(MppcVoltages[count++]);

	sprintf(buf, "fMeasuredScanSpeed : %f\n", fMeasuredScanSpeed);
	writelog(buf, LOG_FILE_NAME);
	sprintf(buf, "fEntryOffsetPixel (Size0~Size2) : %f\n", fEntryOffsetPixel2);
	writelog(buf, LOG_FILE_NAME);
	sprintf(buf, "fEntryOffsetPixel (Size3) : %f\n", fEntryOffsetPixel3);
	writelog(buf, LOG_FILE_NAME);
	sprintf(buf, "fBackwaredOffset : %f\n", fBackwaredOffset);
	writelog(buf, LOG_FILE_NAME);
	sprintf(buf, "fPositionOffset : %f\n", fPositioningOffset);
	writelog(buf, LOG_FILE_NAME);
	auto calcHandle = CreateParameterCalculator();
	double fDesignedScanSpeed = round(PC_Get_MotorPulsePeriod(calcHandle,PC_GetScanSpeed(calcHandle)));
	double scale = fDesignedScanSpeed / fMeasuredScanSpeed;
	PC_SetScale(calcHandle, scale);
	PC_SetBackwaredOffset(calcHandle, fBackwaredOffset);
	
	// 2025-07-03. jg kim. debug
	double startSpeed = PC_GetStartSpeed(calcHandle) * scale; // mm / sec
	double normalSpeed = PC_GetNormalSpeed(calcHandle) * scale; // mm / sec	
	double maxSpeed = PC_GetMaxSpeed(calcHandle) * scale; // mm / sec
	double ejectSpeed = double(EJECT_SPEED_DEFAULT) / scale; // 이건 모터 펄스 주기임. //2025-07-03. jg kim. Eject speed 계산 추가
	maxSpeed = maxSpeed > TRAY_MAX_SPEED ? TRAY_MAX_SPEED : maxSpeed; // Step 모터 펄스 주기를 2400(GenX-CR에서는 12.7831732323803 mm/sec ) 이하로 설정하면 탈조 위험이 있어 제한함.

	pGlobalData->current_setting.SM_speed = unsigned int(fMeasuredScanSpeed);
	pGlobalData->current_setting.start_speed = int(PC_Get_MotorPulsePeriod(calcHandle,startSpeed));
	pGlobalData->current_setting.normal_speed = int(PC_Get_MotorPulsePeriod(calcHandle, normalSpeed));
	pGlobalData->current_setting.max_speed = int(PC_Get_MotorPulsePeriod(calcHandle, maxSpeed)); //printf("Start speed = %.0f\nNormal Speed = %.0f\nMax Speed = %.0f\nScan Speed = %.0f\n", calc.get_MotorPulsePeriod(startSpeed), calc.get_MotorPulsePeriod(normalSpeed), calc.get_MotorPulsePeriod(maxSpeed), fMeasuredScanSpeed);
	pGlobalData->current_setting.eject_speed = int(ejectSpeed);

	double fBackwardDistance = PC_GetDistanceToBackward(calcHandle);
	double fTimeBackwardDistance = 0;

	PC_GetMovingTime2(calcHandle,INPUT OUTPUT &fTimeBackwardDistance, INPUT fBackwardDistance, INPUT startSpeed, INPUT normalSpeed, INPUT startSpeed, INPUT fBackwaredOffset); //printf("Backward distance =%.3f mm, Backward offset = %.3f mm. Total distance = %.3f mm.\nmoving time = %f s\t%ld\n", fBackwardDistance, fBackwaredOffset, fBackwardDistance - fBackwaredOffset, fTimeBackwardDistance, nTimeBackwardDistance);
	pGlobalData->current_setting.Backward_Offset_Length = unsigned int(PC_GetClocksFromSec(calcHandle,fTimeBackwardDistance));

	double constPixelSec = PC_GetScanSpeed(calcHandle) / 240; // measured scan speed는 결국 6 mm / sec를 구현하는 속도이기 때문.
													// 240 = 1800 (RPM) / 60 (sec) * 8 (폴리곤 미러 면 개수; 8면)

	double fDistancePosition = 0;
	double fDistanceEntry = 0;
	PC_GetDistanceToPositionEntry(calcHandle,OUTPUT &fDistancePosition, OUTPUT &fDistanceEntry, INPUT 3); // ip index 3
	double fTimePosition3 = 0;
	PC_GetMovingTime2(calcHandle,INPUT OUTPUT &fTimePosition3, INPUT fDistancePosition, startSpeed, normalSpeed, startSpeed, fBackwaredOffset); // positioning
	sprintf(buf, "fTimePosition3 : %f\n", fTimePosition3);
	writelog(buf, LOG_FILE_NAME);

	for (int nIndex = 0; nIndex < 4; nIndex++)
	{
		fDistancePosition = 0;
		fDistanceEntry = 0;
		double fTimePosition = 0;
		double fTimeEntry = 0;
		PC_SetBackwaredOffset(calcHandle,fBackwaredOffset);
		PC_GetDistanceToPositionEntry(calcHandle,OUTPUT &fDistancePosition, OUTPUT &fDistanceEntry, INPUT nIndex);
		if (nIndex < 3)
			fDistancePosition += fPositioningOffset;
		PC_GetMovingTime2(calcHandle,INPUT OUTPUT &fTimePosition, INPUT fDistancePosition, startSpeed, normalSpeed, startSpeed, fBackwaredOffset); // positioning
		fTimePosition -= fTimePosition3;
		/*
			Positioning::	startSpeed / normalSpeed / startSpeed	(Acceleration / Constant speed moving / Deceleration)
		*/
		
		switch (nIndex)
		{
		case 0:
		case 1:
		case 2:
		{
			fDistanceEntry -= fEntryOffsetPixel2 * constPixelSec;			
		}
		break;
		case 3:
		{
			fDistanceEntry -= fEntryOffsetPixel3 * constPixelSec;
		}
		break;
		}
		/*
			Entry::			startSpeed / maxSpeed / scanSpeed		(Acceleration / Constant speed moving / Deceleration)
		*/
		PC_GetMovingTime2(calcHandle,INPUT OUTPUT &fTimeEntry, INPUT fDistanceEntry, startSpeed, maxSpeed, fMeasuredScanSpeed, fBackwaredOffset); // entry.  //printf("Size%d :: Position = %d, Entry = %d\n", nIndex, long(fTimePosition*1e8), long(fTimeEntry*1e8));
		
		switch (nIndex)
		{
		case 0:
		{
			pGlobalData->current_setting.size0_positioning_time = int(PC_GetClocksFromSec(calcHandle,fTimePosition));
			pGlobalData->current_setting.scan_entry_time_0 = int(PC_GetClocksFromSec(calcHandle,fTimeEntry));
		}
		break;
		case 1:
		{
			pGlobalData->current_setting.size1_positioning_time = int(PC_GetClocksFromSec(calcHandle,fTimePosition));
			pGlobalData->current_setting.scan_entry_time_1 = int(PC_GetClocksFromSec(calcHandle,fTimeEntry));
		}
		break;
		case 2:
		{
			pGlobalData->current_setting.size2_positioning_time = int(PC_GetClocksFromSec(calcHandle,fTimePosition));
			pGlobalData->current_setting.scan_entry_time_2 = int(PC_GetClocksFromSec(calcHandle,fTimeEntry));
		}
		break;
		case 3:
		{
			pGlobalData->current_setting.size3_positioning_time = int(PC_GetClocksFromSec(calcHandle,fTimePosition));
			pGlobalData->current_setting.scan_entry_time_3 = int(PC_GetClocksFromSec(calcHandle,fTimeEntry));
		}
		break;
		}

	}
	// 2025-07-03. jg kim. Eject time을 수작업으로 맞추도록 방침이 변경되어 비활성화
	//double fEjectDistance = calc.getDistanceToEject(); // 거리
	//double fTimeEject = 0; // 시간
	//calc.getMovingTimeToEject(fTimeEject, fEjectDistance, normalSpeed, startSpeed);

	//pGlobalData->current_setting.ip_eject_time = int(calc.getClocksFromSec(fTimeEject));

	//sprintf(buf, "fEjectDistance : %f mm.\n", fEjectDistance);
	//writelog(buf,LOG_FILE_NAME);

	//sprintf(buf, "fTimeEject : %f s.\n", fTimeEject);
	//writelog(buf,LOG_FILE_NAME);

	//sprintf(buf, "ip_eject_time : %d\n", pGlobalData->current_setting.ip_eject_time);
	//writelog(buf,LOG_FILE_NAME);

	// 2025-05-07. jg kim. Eject time을 default값(2.5s)으로 변경하지 않도록 수정
	//fTimeEject = (float)TRAY_EJECT_TIME;
	//pGlobalData->current_setting.ip_eject_time = int(calc.getClocksFromSec(fTimeEject));

	//printf("EjectTime = %d\n", long(1.8*1e8));

	// 2024-07-04. jg kim. 장비 tunning에 사용하는 값 저장
	//>>장비 tunning을 하다 보면 이 앱을 종료, 재시작 하는 경우가 있는데, 그런 경우 아래 값들이 default 값으로 초기화 되어 수정한 값들이 유지되도록 ini에 값을 저장하도록 하였음
	// 2026-01-14. jg kim. 장비에 값을 직접 저장한는 것으로 변경.
	//wchar_t wstring[32];
	//wchar_t wvalue[32];
	//
	//wsprintf(wstring, L"backward_offset");
	//wsprintf(wvalue, TEXT("%d.%d"), (int)(GetPdxData()->g_fBackwaredOffset),(int)(GetPdxData()->g_fBackwaredOffset*10)%10); // 2024-10-17. jg kim. 디버깅
	//WritePrivateProfileString(L"Device_Process", wstring, wvalue, L".\\predix.ini");

	//wsprintf(wstring, L"EntryOffsetPixel_0-2");
	//wsprintf(wvalue, L"%d", GetPdxData()->g_nEntryOffsetPixel2);
	//WritePrivateProfileString(L"Device_Process", wstring, wvalue, L".\\predix.ini");

	//wsprintf(wstring, L"EntryOffsetPixel_3");
	//wsprintf(wvalue, L"%d", GetPdxData()->g_nEntryOffsetPixel3);
	//WritePrivateProfileString(L"Device_Process", wstring, wvalue, L".\\predix.ini");
	//>>Device tunning
	DestroyParameterCalculator(calcHandle);
	update_current_to_list(); // 오른쪽 list에 업데이트
}


void CAdminScannerInfo::OnLvnItemchangedListUserInput(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	UNREFERENCED_PARAMETER(pNMLV);
	*pResult = 0;
}

void CAdminScannerInfo::OnNMDblclkListUserInput(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	iSavedItemUserInput = pNMItemActivate->iItem;
	iSavedSubitemUserInput = pNMItemActivate->iSubItem;

	if (pNMItemActivate->iItem != -1)
	{
		if (pNMItemActivate->iSubItem == 1)
		{
			CRect rect;
			m_user_input.GetSubItemRect(pNMItemActivate->iItem, pNMItemActivate->iSubItem, LVIR_BOUNDS, rect);
			rect.left = rect.left + 5;
			rect.top = rect.top + 2;
			m_user_input.ClientToScreen(rect);
			this->ScreenToClient(rect);

			GetDlgItem(IDC_EDIT_USER_INPUT)->SetWindowText(m_user_input.GetItemText(pNMItemActivate->iItem, pNMItemActivate->iSubItem));
			GetDlgItem(IDC_EDIT_USER_INPUT)->SetWindowPos(NULL, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
			GetDlgItem(IDC_EDIT_USER_INPUT)->SetFocus();
		}
	}
	*pResult = 0;
}

void CAdminScannerInfo::OnNMClickListUserInput(NMHDR * pNMHDR, LRESULT * pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	iSavedItemUserInput = iSavedSubitemUserInput = -1;
	GetDlgItem(IDC_EDIT_USER_INPUT)->SetWindowPos(NULL, 0, 0, 0, 0, SWP_HIDEWINDOW);

	if (pNMItemActivate->iItem != -1)
	{
		iSavedItemUserInput = pNMItemActivate->iItem;
		iSavedSubitemUserInput = pNMItemActivate->iSubItem;
	}
	*pResult = 0;
}

void CAdminScannerInfo::OnBnClickedButtonMppc()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	OnBnClickedButtonCalculate();
}

void CAdminScannerInfo::OnBnClickedButtonMppcDefault()
{
	//auto pGlobalData = GetPdxData();
	//ASSERT(pGlobalData);
	// 작업을 하다 초기화를 할 경우가 있어서 여기서 초기값을 넣도록 했음.

	wchar_t c1_data[100] = { 0, };
	//CHardwareInfo hi;
	int count = 5;
	int decimal_place = 3;
	for (int i = 0; i < MPPC_COUNT; i++) // 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
	{
		int nvolt = HI_Mppc_Convert_DigitalValueToVoltage(MPPC_DIGITAL_VALUE_DEFAULT);
		int nOffset = 0;
		int integer_part = int(nvolt/(int)pow(10, decimal_place));
		int decimal_part = nvolt - integer_part * (int)pow(10, decimal_place);
		wsprintf(c1_data, L"%d.%03d", integer_part, decimal_part);
		m_user_input.SetItemText(count + i * 2, 1, c1_data);

		wsprintf(c1_data, L"%d.%03d", nOffset, nOffset);
		m_user_input.SetItemText(count + i * 2 + 1, 1, c1_data);
	}
	OnBnClickedButtonCalculate();
}

double CAdminScannerInfo::getBoundedValue(double value, double LowerBound, double UpperBound)
{
	return value > UpperBound ? UpperBound : (value < LowerBound ? LowerBound : value);
}
// 소숫점 표시를 개선하기 위함.
// 소숫점은 3자리까지 사용하고, 취급하는 숫자는 1000이 곱해져음.
unsigned short CAdminScannerInfo::getBoundedValue(unsigned short value, unsigned short LowerBound, unsigned short UpperBound)
{
	return value > UpperBound ? UpperBound : (value < LowerBound ? LowerBound : value);
}
// 2024-07-15. jg kim. BLDC RPM을 BLDC speed로 계산 하여 업데이트 하려는 목적.
void CAdminScannerInfo::OnBnClickedButtonBldc()
{
	OnBnClickedButtonCalculate();
}
// 2024-08-23. jg kim. 장비에서 수신한 parameter를 사용하기 위함.
void CAdminScannerInfo::OnTimer(UINT_PTR nIDEvent)
{
	SCANNER_STATE ss = ss_none;

	switch (nIDEvent)
	{
	case _TIMERID_HARDWARE_INFO:
	{
		ss = get_scanner_state_s();
		if (ss == ss_ready)
		{
			KillTimer(_TIMERID_HARDWARE_INFO);
			if (m_bBackwardOffset)
			{
				auto pGlobalData = GetPdxData();
				ASSERT(pGlobalData);
				if (pGlobalData->current_setting.selected_ip_size - 1000 == 3)  // 2024-08-13. jg kim. 작업자 실수 예외처리 추가
				{
					/*wchar_t wstring[32];
					wchar_t wvalue[32];*/
					OnBnClickedButtonCalculate();
				}
				else
				{  // 2024-08-13. jg kim. 경고
					char buf[1024];
					sprintf(buf, "\t\tWarn. The scan size is not 3. Please change it 3.\n");
					writelog(buf, LOG_FILE_NAME);
					AfxMessageBox(L"스캔 모드가 Size 3이 아닙니다.\n스캔 모드를 Size 3으로 변경하세요.");
					CAdminScannerInfo::EndDialog(1);
				}
				m_bBackwardOffset = false;
				send_protocol_to_device(nc_request_hardware_info, 0, 0);  // 2024-08-13. jg kim. 디버깅.
			}
			else if (m_bPositionOffset)  // 2025-06-18. jg kim. Positioning offset 추가.
			{
				auto pGlobalData = GetPdxData();
				ASSERT(pGlobalData);
				if (pGlobalData->current_setting.selected_ip_size - 1000 < 3)
				{
					OnBnClickedButtonCalculate();
				}
				else
				{
					char buf[1024];
					sprintf(buf, "\t\tWarn. The scan size is not 0-2. Please change it 0-2.\n");
					writelog(buf, LOG_FILE_NAME);
					AfxMessageBox(L"스캔 모드가 Size 0~2이 아닙니다.\n스캔 모드를 Size 0~2로 변경하세요.");
					CAdminScannerInfo::EndDialog(1);
				}
				m_bPositionOffset = false;
				send_protocol_to_device(nc_request_hardware_info, 0, 0);  // 2024-08-13. jg kim. 디버깅.
			}
			else
			{
				// kiwa72(2022.12.06 00h) - 장비 하드웨어 정보 로딩 후 화면 갱신
				update_current_to_list();
			}
			
		}
	}
	break;
	case _TIMERID_PARAMETER_EXTRACTION_INFO: // 2024-11.12 jg kim. parameter extraction thread가 종료되었는지 500ms 마다 검사
	{
		KillTimer(_TIMERID_PARAMETER_EXTRACTION_INFO);
		if (g_bDoneParameterExtraction) 
		{
			char buf[1024];
			auto pGlobalData = GetPdxData();
			ASSERT(pGlobalData);
			if (g_bResultParameterExtraction && g_nBldcShiftPixels != 65535)// -1, 65535는 각 파라미터의 유효하지 않은 값.
			{
				//float fRatio = float(g_nBoundingWidth) / float(1000);

				float sm = float(pGlobalData->current_setting.SM_speed) / g_fAspectRatio;// *fRatio;
				pGlobalData->g_nMeasuredScanSpeed = int(sm);
				pGlobalData->current_setting.SM_speed = int(sm);

				wchar_t c1_data[100] = { 0, };
				int count = 0;
				wsprintf(c1_data, L"%d", pGlobalData->g_nMeasuredScanSpeed);
				m_user_input.SetItemText(count++, 1, c1_data);

				sprintf(buf, "Aspect ratio : %.3f. Adjust Scan Speed  : %d.\n", g_fAspectRatio, pGlobalData->current_setting.SM_speed);
				writelog(buf, LOG_FILE_NAME);

				AfxMessageBox(L"Scan Speed를 업데이트 했습니다.");

				sprintf(buf, "BLDC Shift pixel: %d pixels. Bounding Rect(x,y,width,height) = (%d, %d, %d, %d)\n", g_nBldcShiftPixels, g_x, g_y, g_nBoundingWidth, g_nBoundingHeight);
				writelog(buf, LOG_FILE_NAME);

				unsigned int BLDC_shift_index = pGlobalData->current_setting.BLDC_shift_index + unsigned int(float(g_nBldcShiftPixels * 4)/* * fRatio*/);

				// 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
				if (BLDC_shift_index > BLDC_SHIFT_INDEX_DEFAULT - 1000 && BLDC_shift_index < BLDC_SHIFT_INDEX_DEFAULT + 1000) // BLDC shift index의 범위. 이 이외는 유효하지 않다고 본다.
					pGlobalData->current_setting.BLDC_shift_index = BLDC_shift_index;

				//count = 10;  // 2024-07-15. jg kim. BLDC RPM 예외처리.
				//CHardwareInfo hi;
				//float fBldcRPM = float(_tstof(m_user_input.GetItemText(count, 1))) * fRatio;
				//if (fBldcRPM > 2202)
				//{
				//	fBldcRPM = 2202; // spec상 최대 RPM
				//	wsprintf(c1_data, L"Exceed BLDC maximum RPM. It adjusted 2202 RPM.");
				//	MessageBox(c1_data);

				//	wsprintf(c1_data, L"%d", int(fBldcRPM));
				//	m_user_input.SetItemText(count, 1, c1_data);
				//}
				//pGlobalData->current_setting.BLDC_speed = HI_getBldcSpeed(fBldcRPM, 100);


				AfxMessageBox(L"BLDC Shift index를 업데이트 했습니다.");

			}
			else
			{
				if(g_bResultParameterExtraction == false)
					AfxMessageBox(L"Scan Speed가 정확하지 않아 현재 값을 유지합니다. 그리드 차트를 촬영했는지, IP나 장비 상태를 점검하세요.");
				if(g_nBldcShiftPixels == 65535)
					AfxMessageBox(L"BLDC Shift pixel가 정확하지 않아 현재 값을 유지합니다. IP나 장비 상태를 점검하세요.");
			}
				
			OnBnClickedButtonCalculate();
		}
		else
			SetTimer(_TIMERID_PARAMETER_EXTRACTION_INFO, 500, NULL);
	}
	break;
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CAdminScannerInfo::OnBnClickedButtonPositionOffset()
{
	// 2025-06-17. jg kim. positioning offset 구하는 코드 추가
	send_protocol_to_device(nc_request_hardware_info, 0, 0);
	m_bPositionOffset = true;
	SetTimer(_TIMERID_HARDWARE_INFO, 500, NULL);
}
	// 2026-01-07. jg kim. Wobble 보정 관련 Laser On/Off, 세로 줄 시작/끝 위치를 double 배열 하나(장비)에 넣기 위함. 
double CAdminScannerInfo::packData(unsigned short B1, unsigned short B2, unsigned short B3, unsigned short B4)
{
	uint64_t packedi = 
		uint64_t(uint64_t(B1) << 48) |
		uint64_t(uint64_t(B2) << 32) |
		uint64_t(uint64_t(B3) << 16) |
		uint64_t(B4);

	double packed;
	memcpy(&packed, &packedi, sizeof(packedi));
	return packed;
}

void CAdminScannerInfo::unpackData(unsigned short & B1, unsigned short & B2, unsigned short & B3, unsigned short & B4, double data)
{
	uint64_t packedi;
	memcpy(&packedi, &data, sizeof(data));

	uint64_t mask = 0xFFFFULL;
	B1 = unsigned short((packedi >> 48) & mask);
	B2 = unsigned short((packedi >> 32) & mask);
	B3 = unsigned short((packedi >> 16) & mask);
	B4 = unsigned short((packedi)		& mask);
}

/* value를 scale로 고정소수점화 후 bias 더해 uint16_t에 저장 */
 uint16_t CAdminScannerInfo::encode_bias_u16(double value, double scale)
{
	/* scale 유효성 (0 방지) */
	if (scale <= 0.0f) return (uint16_t)U16_BIAS;

	/* 표현 가능한 실수 범위 계산 */
	const double min_real = (-32768.0f) / scale;
	const double max_real = (32767.0f) / scale;

	/* saturation: 실수 입력을 표현 가능 범위로 clamp */
	double v = std::clamp(value, min_real, max_real);

	/* 고정소수점 정수화 (signed domain) */
	int32_t fixed = (std::int32_t)std::lround(v * scale); /* -32768..32767 보장 */

	/* bias 적용 후 raw 생성 */
	int32_t raw32 = fixed + U16_BIAS;        /* 0..65535 보장 */

	/* (이중 안전) raw 범위 saturation */
	if (raw32 < (int32_t)U16_RAW_MIN) raw32 = (int32_t)U16_RAW_MIN;
	if (raw32 > (int32_t)U16_RAW_MAX) raw32 = (int32_t)U16_RAW_MAX;

	return (uint16_t)raw32;
}

 double CAdminScannerInfo::decode_bias_u16(uint16_t raw, double scale)
{
	if (scale <= 0.0f) return 0.0f;

	/* unsigned -> signed domain 복원 */
	std::int32_t fixed = (std::int32_t)raw - U16_BIAS; /* -32768..32767 */

	return ((double)fixed) / scale;
}

uint32_t CAdminScannerInfo::encode_bias_u32(double value, double scale)
{
	/* 32비트 범위는 넓어서 double이 안전(곱셈 정밀도/범위) */
	if (scale <= 0.0) return (uint32_t)U32_BIAS;

	const double min_real = (-2147483648.0) / scale;
	const double max_real = (2147483647.0) / scale;

	/* saturation */
	double v = std::clamp(value, min_real, max_real);

	/* 고정소수점 정수화: int64로 받아 안전하게 */
	std::int64_t fixed = (std::int64_t)std::llround(v * scale); /* -2147483648..2147483647 보장 */

	/* bias 적용: 0..4294967295 */
	int64_t raw64 = fixed + (int64_t)U32_BIAS;

	/* raw 범위 saturation */
	if (raw64 < (std::int64_t)U32_RAW_MIN) raw64 = (std::int64_t)U32_RAW_MIN;
	if (raw64 > (std::int64_t)U32_RAW_MAX) raw64 = (std::int64_t)U32_RAW_MAX;

	return (uint32_t)raw64;
}

double CAdminScannerInfo::decode_bias_u32(uint32_t raw, double scale)
{
	if (scale <= 0.0) return 0.0;

	std::int64_t fixed = (std::int64_t)raw - (std::int64_t)U32_BIAS; /* -2147483648..2147483647 */

	return ((double)fixed) / scale;
}