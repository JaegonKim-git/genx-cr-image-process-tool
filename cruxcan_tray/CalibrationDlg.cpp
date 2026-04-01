// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <algorithm> // 2023-11-22. jg kim.
#include "../gnImageProcessing/ImageProcessingInterface.h" // 2023-11-22. jg kim.  Genoray calibration dll 사용을 위한 header
#include "../predix_sdk/common.h"
#include "../predix_sdk/scanner.h"
#include "../predix_sdk/tray_network.h"
#include "../predix_sdk/tiff_image.h"
#include "../predix_sdk/global_data.h"
#include "../predix_sdk/constants.h" // 2025-02-11. jg kim. macro들을 constants.h로 이동
#include "afxdialogex.h"
//#include "crx_processing.hpp"
#include "../ExtLibs/predix_preprocessing/crx_processing.hpp"
#include "cruxcan_tray.h"
#include "cruxcan_trayDlg.h"
#include "CalibrationDlg.h"
#include "../gnImageProcessing/HardwareInfo.h" // 2024-08-26. jg kim. calibration data의 유효성을 검사하기 위해 추가

using namespace crux;

struct CRX_RECT
{
	int left;
	int right;
	int top;
	int bottom;
}; // 2024-06-13. jg kim. 코드 정리를 위해 calibration.h에서 가져옴


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

raw_t* tgt_images = NULL;
int  buffer_index = 0;
CRX_RECT  image_rects[MAX_CALI_IMAGE_COUNT];

// CCalibrationDlg 대화 상자

IMPLEMENT_DYNAMIC(CCalibrationDlg, CDialogEx)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CCalibrationDlg::CCalibrationDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_CALIBRATION, pParent)
	, m_strMemo(_T(""))
	, m_iValidWidth(DEFAULT_VALID_WIDTH)
	, m_iValidHeight(DEFAULT_VALID_HEIGHT)
	, m_bUpdate_ValidArea_to_Image(FALSE)
	, m_strCoefs(_T(""))
	, m_bDefinedArea(FALSE)
	, m_nImage_count(0)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CCalibrationDlg::~CCalibrationDlg()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// data exchange
void CCalibrationDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_strMemo);
	//DDX_Text(pDX, IDC_EDIT_VALID_WIDTH, m_iValidWidth);
	//DDX_Text(pDX, IDC_EDIT_VALID_HEIGHT, m_iValidHeight);
	//DDX_Check(pDX, IDC_CHECK_VALID_AREA, m_bUpdate_ValidArea_to_Image);
	//DDX_Check(pDX, IDC_CHECK_USE_DEFINED_AREA, m_bDefinedArea);
	//DDX_Text(pDX, IDC_EDIT_COEFS, m_strCoefs);
	DDX_Control(pDX, IDC_BUTTON_SAVE_FILE, m_btnSaveParam);// 2023-11-22. jg kim. 
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// message map
BEGIN_MESSAGE_MAP(CCalibrationDlg, CDialogEx)
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BUTTON_ADD_RAW, &CCalibrationDlg::OnBnClickedButtonAddRaw)
	ON_BN_CLICKED(IDC_BUTTON_CALC, &CCalibrationDlg::OnBnClickedButtonCalc)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_FILE, &CCalibrationDlg::OnBnClickedButtonSaveFile)// 2023-11-22. jg kim. 
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BUTTON_SAVE_SCANNER, &CCalibrationDlg::OnBnClickedButtonSaveScanner)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BUTTON_RESET_CALIBRATION, &CCalibrationDlg::OnBnClickedButtonResetCalibration)
END_MESSAGE_MAP()


// CCalibrationDlg 메시지 처리기

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dialog initializing
BOOL CCalibrationDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	DragAcceptFiles(true);

	if (tgt_images)
	{
		ASSERT(0);
		delete[] tgt_images;
	}

	tgt_images = new raw_t[geo_x * geo_y * (MAX_CALI_IMAGE_COUNT + 1)];// 마지막위치는 임시버퍼로 사용가능
	ASSERT(tgt_images);

	buffer_index = geo_x * geo_y * MAX_CALI_IMAGE_COUNT;
	m_nImage_count = 0;
	memset(&image_rects, 0, sizeof(image_rects));

	if (GetPdxData()->g_piniFile != NULL)
	{
		CString strUseFileParam = GetPdxData()->g_piniFile->GetValueL(L"calibration", L"use_ini");
		if (strUseFileParam == L"none" || strUseFileParam == L"0")
		{
			m_btnSaveParam.EnableWindow(true);
			// 2023-11-22. jg kim. 기존에는 ini 파일에 use_ini = 1 일 경우에만 저장 버튼이 활성화 되었지만
			// 무조건 저장하는 것으로 변경
		}

		UpdateData(FALSE);
	}

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CCalibrationDlg::OnClose()
{
	CDialogEx::OnClose();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CCalibrationDlg::OnDropFiles(HDROP hDropInfo)
{
	UpdateData(TRUE);
	int ucount = DragQueryFile(hDropInfo, 0xffffffff, nullptr, 0);
	if (ucount == CALI_IMAGE_COUNT) // 2024-08-26. jg kim. 작성자 실수를 방지하기 위해 추가
									// 2024-02-11. jg kim. 의미를 분명하게 하기 위해 매크로로 변경
	{
		// 2026-03-05. jg kim. 파일들을 먼저 수집하고 오름차순으로 정렬
		std::vector<CString> fileList;

#ifdef _UNICODE
		LPWSTR buffer;
#else
		char* buffer;
#endif
		buffer = nullptr;

		// 파일 경로 수집
		for (int i = 0; i < ucount; i++)
		{
			int nLength = DragQueryFile(hDropInfo, i, nullptr, 0);
#ifdef _UNICODE
			buffer = (LPWSTR)new BYTE[2 * (nLength + 1)]; // 여기서 Uncode는 2 Byte이므로 2배로 해줘야 한다.
#else
			buffer = (char*)new char[nLength + 1];
#endif
			DragQueryFile(hDropInfo, i, buffer, nLength + 1);
			WSTRING file = buffer;
			if (is_raw_file(file) || is_tiff_file(file))
			{
				fileList.push_back(CString(buffer));
			}
			SAFE_DELETE_ARRAY(buffer);
		}

		// 파일명 오름차순 정렬
		std::sort(fileList.begin(), fileList.end());

		// 정렬된 파일들을 순서대로 처리
		for (const auto& filePath : fileList)
		{
			if (check_image(filePath) < -1)
			{
				MessageBox(L"등록이미지는 10장을 초과할 수 없습니다.", L"이미지 갯수 확인", MB_TOPMOST);
				break;
			}
		}
	}
	else
	{
		MessageBox(L"영상은 5장을 입력하십시오.", L"영상 갯수 확인", MB_TOPMOST);
	}

	CDialogEx::OnDropFiles(hDropInfo);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CCalibrationDlg::OnBnClickedButtonAddRaw()
{
	UpdateData(TRUE);

	TCHAR szFilter[] = _T("predix image (*.tif;*.tiff)|*.tif;*.tiff||");
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, szFilter, this);

	if (IDOK == dlg.DoModal())
	{
		WSTRING file = dlg.GetPathName();
		if (is_tiff_file(file))
		{
			check_image(dlg.GetPathName());
		}
		else
		{
			MessageBox(L"tiff 파일을 선택하십시오.", L"파일 형식 확인", MB_TOPMOST);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CCalibrationDlg::OnBnClickedButtonCalc()
{
	// 계산 이미지 계산
	UpdateData(TRUE);
	if (m_nImage_count < 1)
	{
		MessageBox(L"등록된 이미지가 없습니다.", L"작업확인", MB_ICONWARNING | MB_TOPMOST);
		return;
	}


    // 2023-11-22. jg kim.
	int L = 0;
	int T = 0;
	int W = 2147483647;
	int H = 2147483647;
	for (int c = 0; c < m_nImage_count; c++) {
		CRX_RECT rt = image_rects[c];
		if (rt.left > L)				L = rt.left;
		if (rt.top > T)					T = rt.top;
		if (rt.right - rt.left + 1 < W)	W = rt.right - rt.left + 1;
		if (rt.bottom - rt.top + 1 < H)	H = rt.bottom - rt.top + 1;
	}
	memset(GetPdxData()->g_NewCalibrationData.data1, 0, sizeof(unsigned short)*MAX_CALI_IMAGE_COUNT);
	memset(GetPdxData()->g_NewCalibrationData.data2, 0, sizeof(float)*MAX_CALI_IMAGE_COUNT * (DEGREE_COUNT + 1));
	int N = MAX_CALI_IMAGE_COUNT + 1;
	float* coefficients = new float[(DEGREE_COUNT + 2) * N];
	memset(coefficients, 0, sizeof(float) * (DEGREE_COUNT + 2) * N);
	calculateCoefficients(coefficients, tgt_images, geo_x, geo_y, L, T, W, H, m_nImage_count);
	// calibration data를 sorting 을 위해 임시로 data 복사
	NewCalibrationData nc;
	int cnt = 1;
	for (int n = 0; n < m_nImage_count; n++)
		nc.data1[n] = (unsigned short)coefficients[cnt++];
	cnt += (DEGREE_COUNT + 1);
	for (int n = 0; n < m_nImage_count*(DEGREE_COUNT + 1); n++)
		nc.data2[n]= coefficients[cnt++]; // float
	delete[] coefficients;

	// calibration data를 sorting 하는 부분
	std::vector<unsigned short> vtMean;
	for (int i = 0; i < m_nImage_count; i++)
		vtMean.push_back(nc.data1[i]);
	std::sort(vtMean.begin(), vtMean.end());
	for (int i = 0; i < m_nImage_count; i++)
	{
		int idx = 0;
		while (idx < m_nImage_count)
		{
			if (vtMean.at(i) == nc.data1[idx])
			{
				GetPdxData()->g_NewCalibrationData.data1[i] = nc.data1[idx];
				int ofsSrc = idx * (DEGREE_COUNT + 1);
				int ofsDst = i * (DEGREE_COUNT + 1);
				memcpy(GetPdxData()->g_NewCalibrationData.data2+ofsDst, nc.data2+ofsSrc, sizeof(float)*(DEGREE_COUNT + 1));
				break;
			}
			idx++;
		}
	}
	// 2024-08-26. jg kim. calibration data의 유효성을 검사하기 위해 추가
	CHardwareInfo hwi;
	if (hwi._CheckValidCoefficients(GetPdxData()->g_NewCalibrationData.data1, GetPdxData()->g_NewCalibrationData.data2))
		MessageBox(L"계산이 완료되었습니다. Calibration data는 유효합니다.");
	else
		MessageBox(L"계산된 calibration data가 유효하지 않습니다. 영상을 점검십시오.");
	UpdateData(FALSE);
}


raw_t get_avg_of_image(raw_t* image, CRX_RECT2 rect)
{
	double sum = 0;
	int count = 0;

	for (int y = rect.box.top; y <= rect.box.bottom; y++)
	{
		for (int x = rect.box.left; x <= rect.box.right; x++)
		{
			sum += image[(y * geo_x) + x];
			count++;
		}
	}

	int result = (int)(sum / count);
	return (raw_t)(result > 65535 ? 65535 : result < 1 ? 0 : result);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
long CCalibrationDlg::check_image(CString filename)
{
	if (m_nImage_count >= MAX_CALI_IMAGE_COUNT)
	{
		return -99;
	}

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	load_tiff(pGlobalData->img_tex, (LPCTSTR)filename);
	memcpy(&tgt_images[buffer_index], pGlobalData->map_raw.data(), sizeof(raw_t) * geo_x * geo_y);//임시 버퍼에 담기
	int width = geo_x, height = geo_y;
	CRX_RECT2 rt;
	FindIPArea(&tgt_images[buffer_index], width, height,rt.rot.width, rt.rot.height, rt.rot.cx, rt.rot.cy, rt.rot.anlge);

	if (rt.rot.width > rt.rot.height)
	{
		int temp = rt.rot.width;
		rt.rot.width = rt.rot.height;
		rt.rot.height = temp;
	}

	int angle_offset = 0;

	if (rt.rot.anlge > 5. || rt.rot.anlge < -5.)
	{//5도이상 기울어져있다면 5도마다 5%정도씩 감소시키자 안전하게
		angle_offset = abs((int)(rt.rot.anlge + 0.5));
	}

	if (m_iValidWidth < 100 && m_iValidWidth > 0)
	{
		rt.rot.width = (rt.rot.width * (m_iValidWidth - angle_offset)) / 100;
	}

	if (m_iValidHeight < 100 && m_iValidHeight > 0)
	{
		rt.rot.height = (rt.rot.height * (m_iValidHeight - angle_offset)) / 100;
	}

	//rt.box.left = rt.box.left  * ivali
	rt.box.left = rt.rot.cx - (rt.rot.width) / 2;
	rt.box.right = rt.rot.cx + (rt.rot.width) / 2;
	rt.box.top = rt.rot.cy - (rt.rot.height) / 2;
	rt.box.bottom = rt.rot.cy + (rt.rot.height) / 2;

	int avg = get_avg_of_image(&tgt_images[buffer_index], rt);

	wchar_t msgbuf[128];
	WSTRING wpath = (LPCTSTR)filename;
	WSTRING wfile = extract_filename(wpath);
	wsprintf(msgbuf, L"%s의 가운데 평균밝기는  %05d 입니다.\n추가하겠습니까?", wfile.c_str(), avg);  // 2024-08-26. jg kim. 불필요한 문구 제거

	if (m_bUpdate_ValidArea_to_Image)
	{
		raw_t* cur = pGlobalData->map_fin.data();
		for (int y = rt.box.top; y < rt.box.bottom; y++)
		{
			for (int x = rt.box.left; x < rt.box.right; x++)
				cur[(y * geo_x) + x] = 65535;
		}
	}

	if (MessageBox(msgbuf, L"이미지 확인", MB_TOPMOST | MB_ICONINFORMATION | MB_YESNO) == IDYES)
	{
		wchar_t winfo[32];

		wsprintf(winfo, L" avg %04d \r\n", avg);
		m_strMemo.Append(extract_filename(wfile).c_str());
		m_strMemo.Append(winfo);
		memmove(&tgt_images[m_nImage_count * (geo_x * geo_y)], &tgt_images[buffer_index], sizeof(raw_t) * (geo_x * geo_y));
		image_rects[m_nImage_count] = { rt.box.left, rt.box.right, rt.box.top,rt.box.bottom };
		m_nImage_count++;
		m_ImageList.AddTail(wfile.c_str());
		UpdateData(FALSE);
		return 1;
	}

	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CCalibrationDlg::OnBnClickedButtonSaveFile() // 2023-11-22. jg kim. 
{
	// 파일에 저장
	// 2026-01-12. jg kim. 메시지 수정 
	if ((MessageBox(L"Calibration data를 predix.ini에 저장하시겠습니까?", L"", MB_TOPMOST | MB_YESNO) == IDYES))
	{
		// 2024-08-26. jg kim. calibration data의 유효성을 검사하기 위해 추가
		CHardwareInfo hwi;
		if (hwi._CheckValidCoefficients(GetPdxData()->g_NewCalibrationData.data1, GetPdxData()->g_NewCalibrationData.data2))
		{
			wchar_t wstring[32];
			wchar_t wvalue[32];
			for (int i = 0; i < MAX_CALI_IMAGE_COUNT; i++)
			{
				swprintf(wstring, L"mean%02d", i);
				swprintf(wvalue, L"%d", GetPdxData()->g_NewCalibrationData.data1[i]);
				WritePrivateProfileString(L"calibration", wstring, wvalue, L".\\predix.ini");
			}
			for (int i = 0; i < MAX_CALI_IMAGE_COUNT * (DEGREE_COUNT + 1); i++)
			{
				swprintf(wstring, L"coef%02d", i);
				swprintf(wvalue, L"%e", GetPdxData()->g_NewCalibrationData.data2[i]);
				WritePrivateProfileString(L"calibration", wstring, wvalue, L".\\predix.ini");
			}
		}
		else
			MessageBox(L"Calibration data가 유효하지 않습니다. predix.ini파일에 data를 저장하지 않습니다.");
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CCalibrationDlg::OnBnClickedButtonSaveScanner()
{
	if (get_scanner_state_s() != ss_ready)
	{
		if (MessageBox(L"스캐너가 연결되어있지 않거나 스캐너가 ready상태가 아닙니다.\n지금연결 하겠습니까? ", L"스캐너 상태 확인", MB_TOPMOST | MB_ICONERROR | MB_YESNO) != IDYES)
		{
			return;
		}
		else
		{
			startLeader();
			MessageBox(L"스캐너 연결 시도중입니다.  잠시후  다시 시도해 주세요", L"연결후 재작업 필요", MB_TOPMOST | MB_ICONINFORMATION);
		}

		return;
	}
	// 2024-08-26. jg kim. calibration data의 유효성을 검사하기 위해 추가
	CHardwareInfo hwi; 
	if (hwi._CheckValidCoefficients(GetPdxData()->g_NewCalibrationData.data1, GetPdxData()->g_NewCalibrationData.data2))
	{
		// 2023-11-22. jg kim. NewCalibrationData를 장비로 전송하도록 수정
		unsigned char* pSendData = NULL;


		pSendData = new unsigned char[sizeof(HARDWARE_SETTING) + sizeof(NewCalibrationData)];
		memset(pSendData, 0x0, sizeof(HARDWARE_SETTING) + sizeof(NewCalibrationData));

		int offset = 0;
		memcpy(pSendData + offset, &GetPdxData()->current_setting, sizeof(HARDWARE_SETTING));
		offset += sizeof(HARDWARE_SETTING);
		memcpy(pSendData + offset, &GetPdxData()->g_NewCalibrationData, sizeof(NewCalibrationData));
		BOOL send_ret = Send_FreeData_To_Device(nc_new_calib_hardware_info, pSendData, sizeof(HARDWARE_SETTING) + sizeof(NewCalibrationData));
		Sleep(500);
		delete[] pSendData;
		pSendData = NULL;
		if (send_ret == 0)
		{
			MessageBox(L"전송 성공 Success.");	// kiwa72(2023.01.21 18h) - 뭘 보고 Success 냐?
		}
		else
		{
			MessageBox(L"전송 실패. Failed.");
		}
	}
	else
		MessageBox(L"Calibration data가 유효하지 않습니다. 장비에 data를 전송하지 않습니다.");
}


void clear_calibrationdata_of_image(void)
{
	const int calibration_pos_x = 5;
	//stcalibration temp_calibrationdata;
	raw_t* rawdata = GetPdxData()->map_raw.data();
	//raw_t* calidata = (raw_t*)&temp_calibrationdata;

	for (int i = 0; i < sizeof(stcalibration) / sizeof(raw_t); i++)
	{
		rawdata[i * geo_x + calibration_pos_x] = 0;//싹 다 0으로 채워서 날려버리기
	}
}


void calc_calibration_image(void)
{
	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	int width = geo_x, height = geo_y;
	CRX_RECT2 rt; 
	FindIPArea(pGlobalData->map_raw.data(), width, height, rt.rot.width, rt.rot.height, rt.rot.cx, rt.rot.cy, rt.rot.anlge);

	if (rt.rot.width > rt.rot.height)
	{//
		int temp = rt.rot.width;
		rt.rot.width = rt.rot.height;
		rt.rot.height = temp;
	}

	int angle_offset = 0;

	if (rt.rot.anlge > 5. || rt.rot.anlge < -5.)
	{//5도이상 기울어져있다면 5도마다 5%정도씩 감소시키자 안전하게
		angle_offset = abs((int)(rt.rot.anlge + 0.5));
	}

	rt.rot.width = (rt.rot.width * (DEFAULT_VALID_WIDTH - angle_offset)) / 100;
	rt.rot.height = (rt.rot.height * (DEFAULT_VALID_HEIGHT - angle_offset)) / 100;

	rt.box.left = rt.rot.cx - (rt.rot.width) / 2;
	rt.box.right = rt.rot.cx + (rt.rot.width) / 2;
	rt.box.top = rt.rot.cy - (rt.rot.height) / 2;
	rt.box.bottom = rt.rot.cy + (rt.rot.height) / 2;
	int avg = get_avg_of_image(pGlobalData->map_raw.data(), rt);

	CString filename;
	filename.Format(L"avg %04d  /center[%04d,%04d]w[%04d]h[%04d]angle[%.2f]", avg, rt.rot.cx, rt.rot.cy, rt.rot.width, rt.rot.height, 0.f);
	if (avg < 1000)
	{
		MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, L"영상의 평균밝기가 너무 어둡거나 잘못된 영상입니다.", (LPTSTR)(LPCTSTR)filename, MB_TOPMOST | MB_ICONERROR);
		return;
	}

	if (rt.rot.width < ((1080 * DEFAULT_VALID_WIDTH) * 9 / 1000) ||
		rt.rot.height < ((2160 * DEFAULT_VALID_HEIGHT) * 9 / 1000))
	{
	// 2026-02-05. jg kim 메시지 수정
		MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, L"영상이 size2가 아니거나 해상도가 HR이 아닙니다.", (LPTSTR)(LPCTSTR)filename, MB_TOPMOST | MB_ICONERROR);
		return;
	}

	filename.Format(L"size2_%04d_%s.tif", avg, (rt.rot.cx > (1536 / 2)) ? L"R" : L"L");
	clear_calibrationdata_of_image();//기존 데이터 지우기 안 헷갈리게
	// 2026-02-05. jg kim. tiff 파일에 장비에 맞는 해상도 설정
	save_tiff_image((LPTSTR)(LPCTSTR)filename, pGlobalData->map_raw.data(), geo_x, geo_y, geo_x, 400.0f, 400.0f);

	CString strmsg;
	strmsg.Format(L"size3 중심평균밝기 %d  %s 영상이 잘 저장되었습다", avg, (rt.rot.cx > (1536 / 2)) ? L"오른쪽" : L"왼쪽");
	MessageBox(AfxGetApp()->GetMainWnd()->m_hWnd, (LPCTSTR)strmsg, (LPCTSTR)filename, MB_TOPMOST | MB_ICONINFORMATION);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// WM_DESTROY event handler
void CCalibrationDlg::OnDestroy()
{
	if (tgt_images)
	{
		delete[]tgt_images;
	}

	tgt_images = NULL;
	CDialogEx::OnDestroy();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CCalibrationDlg::OnBnClickedButtonResetCalibration()
{
	if (MessageBox(L"읽은 영상들과 계산치를 버리겠습니까?", L"초기화 확인", MB_TOPMOST | MB_ICONWARNING | MB_YESNO) != IDYES)
	{
		return;
	}

	if (!tgt_images)
	{
		tgt_images = new raw_t[geo_x * geo_y * (MAX_CALI_IMAGE_COUNT + 1)];// 마지막위치는 임시버퍼로 사용가능
	}

	memset(tgt_images, 0, geo_x * geo_y * (MAX_CALI_IMAGE_COUNT + 1) * sizeof(raw_t));
	m_ImageList.RemoveAll();

	m_nImage_count = 0;
	m_strMemo = L"";
	m_strCoefs = L"";
	// 2023-11-22. jg kim. NewCalibrationData로 수정
	memset(GetPdxData()->g_NewCalibrationData.data2, 0, sizeof(float)*MAX_CALI_IMAGE_COUNT*(DEGREE_COUNT+1));
	memset(GetPdxData()->g_NewCalibrationData.data1, 0, sizeof(unsigned short)*MAX_CALI_IMAGE_COUNT);

	UpdateData(FALSE);
}
