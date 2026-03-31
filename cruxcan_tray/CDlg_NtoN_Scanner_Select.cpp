// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "Resource.h"
//#include "../predix_sdk/CTCP_Sock.h"
#include "../predix_sdk/tray_network.h"
#include "../predix_sdk/global_data.h"
#include "CDlg_NtoN_Scanner_Select.h"
#include "afxdialogex.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// macro constants 

#define TIMERID_SCANNER_CONNECT		4546
#if (_DEBUG_CONNECT_TIME_)
#define ELAPSE_SCANNER_CONNECT		(2999)			//! kiwa72(2023.04.23 11h) - NtoN 접속한 스캐너 소켓 리스트 컨트롤
#else
#define ELAPSE_SCANNER_CONNECT		(1000 / 10)		//! kiwa72(2024.01.22 12h) - 타이밍 수정 2000 -> 1000
#endif


// CDlg_NtoN_Scanner_Select 대화 상자

IMPLEMENT_DYNAMIC(CDlg_NtoN_Scanner_Select, CDialogEx)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CDlg_NtoN_Scanner_Select::CDlg_NtoN_Scanner_Select(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG_NtoNSCANNERSELECT, pParent)
	, m_apRadio_Scanner{ nullptr, }
	, m_nSelectScanner(-1)
	, m_nTimeCnt(0)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CDlg_NtoN_Scanner_Select::~CDlg_NtoN_Scanner_Select()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// data exchange
void CDlg_NtoN_Scanner_Select::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_SCANNER_1, m_Radio_Scanner_1);
	DDX_Control(pDX, IDC_RADIO_SCANNER_2, m_Radio_Scanner_2);
	DDX_Control(pDX, IDC_RADIO_SCANNER_3, m_Radio_Scanner_3);
	DDX_Control(pDX, IDC_RADIO_SCANNER_4, m_Radio_Scanner_4);
	DDX_Control(pDX, IDC_RADIO_SCANNER_5, m_Radio_Scanner_5);
	DDX_Control(pDX, IDC_STATIC_CONNECT_TIME, m_Static_ConnectTime);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// message map
BEGIN_MESSAGE_MAP(CDlg_NtoN_Scanner_Select, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_SCANNER_SELECT, &CDlg_NtoN_Scanner_Select::OnBnClickedButton_Scanner_Select)
	ON_BN_CLICKED(IDC_BUTTON_CANCEL, &CDlg_NtoN_Scanner_Select::OnBnClickedButton_Cancel)
	ON_WM_TIMER()
END_MESSAGE_MAP()


// CDlg_NtoN_Scanner_Select 메시지 처리기

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CDlg_NtoN_Scanner_Select::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_RETURN /*|| pMsg->wParam == VK_ESCAPE*/)
		{
			return TRUE;
		}
	}

	return CDialogEx::PreTranslateMessage(pMsg);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CDlg_NtoN_Scanner_Select::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	HICON hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON_TRAY_F)); //icon 변경
	this->SetIcon(hIcon, false); //icon 셋팅

	m_Radio_Scanner_1.SetWindowTextW(L"No Connect Sacnner");
	m_Radio_Scanner_2.SetWindowTextW(L"No Connect Sacnner");
	m_Radio_Scanner_3.SetWindowTextW(L"No Connect Sacnner");
	m_Radio_Scanner_4.SetWindowTextW(L"No Connect Sacnner");
	m_Radio_Scanner_5.SetWindowTextW(L"No Connect Sacnner");

	m_Radio_Scanner_1.EnableWindow(false);
	m_Radio_Scanner_2.EnableWindow(false);
	m_Radio_Scanner_3.EnableWindow(false);
	m_Radio_Scanner_4.EnableWindow(false);
	m_Radio_Scanner_5.EnableWindow(false);

	// kiwa72(2023.04.23 11h) - 접속 리스트 배열 저장
	m_apRadio_Scanner[0] = &m_Radio_Scanner_1;
	m_apRadio_Scanner[1] = &m_Radio_Scanner_2;
	m_apRadio_Scanner[2] = &m_Radio_Scanner_3;
	m_apRadio_Scanner[3] = &m_Radio_Scanner_4;
	m_apRadio_Scanner[4] = &m_Radio_Scanner_5;

	// kiwa72(2023.04.23 11h) - 접속 리스트 업데이트를 위한 타이머
	m_nTimeCnt = 0;	// 카임 카운트 초기화
	//SetTimer(TIMERID_SCANNER_CONNECT, 200/*1000*/, NULL);	//! kiwa72(2023.11.25 17h) - 타이머 1초에서 0.2초로 변경
	SetTimer(TIMERID_SCANNER_CONNECT, ELAPSE_SCANNER_CONNECT, NULL);

	return TRUE;  // return TRUE unless you set the focus to a control
				  // 예외: OCX 속성 페이지는 FALSE를 반환해야 합니다.
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief	Clicked Button Scanner Select
 * @details	접속 스캐너 리스트 중 선택
 * @param	none
 * @retval	none
 * @author	kiwa72
 * @date	kiwa72(2023.04.23 11h)
 * @version	0.0.1
 */
void CDlg_NtoN_Scanner_Select::OnBnClickedButton_Scanner_Select()
{
	if (m_Radio_Scanner_1.GetCheck() == 1)
	{
		m_nSelectScanner = 0;
	}
	else if (m_Radio_Scanner_2.GetCheck() == 1)
	{
		m_nSelectScanner = 1;
	}
	else if (m_Radio_Scanner_3.GetCheck() == 1)
	{
		m_nSelectScanner = 2;
	}
	else if (m_Radio_Scanner_4.GetCheck() == 1)
	{
		m_nSelectScanner = 3;
	}
	else if (m_Radio_Scanner_5.GetCheck() == 1)
	{
		m_nSelectScanner = 4;
	}
	else
	{
		MessageBox(L"Choose your scanner.", L"Scannner Select", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
		return;
	}

	//! kiwa72(2024.01.22 12h) - 이미 다른 PC에 접속해서 상태가 변했는 확인을 위한 지연 타이밍 적용
#if !(_DEBUG_CONNECT_TIME_)
	BeginWaitCursor();
	for (int i = 0; i < 5; i++)
	{
		if (!AfxGetApp()->PumpMessage())
		{
			;
		}
		Sleep(100);
	}
	EndWaitCursor();
#endif
	//...

	//! kiwa72(2023.11.25 17h) - 이미 다른 PC와 연결된 상태에서 상태 표시가 바뀌지 않은 경우 연결을 시도한 경우
	if (m_nSelectScanner >= 0)
	{
		CString str;
		m_apRadio_Scanner[m_nSelectScanner]->GetWindowTextW(str);

		if (str.Find(L"Already in use") >= 0)
		{
			MessageBox(L"Already in use.", L"Scannner Select", MB_OK | MB_TOPMOST | MB_ICONINFORMATION);
			return;
		}
	}
	//!...

	KillTimer(TIMERID_SCANNER_CONNECT);

	OnOK();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief	Clicked Button Cencel
 * @details	스캐너 선택 대화상자 닫기
 * @param	none
 * @retval	none
 * @author	kiwa72
 * @date	kiwa72(2023.04.23 11h)
 * @version	0.0.1
 */
void CDlg_NtoN_Scanner_Select::OnBnClickedButton_Cancel()
{
	KillTimer(TIMERID_SCANNER_CONNECT);

	OnCancel();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief	Timer
 * @details	타이머 이벤트
 * @param	none
 * @retval	none
 * @author	kiwa72
 * @date	kiwa72(2023.04.23 11h)
 * @version	0.0.1
 */
void CDlg_NtoN_Scanner_Select::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMERID_SCANNER_CONNECT)
	{
		CString sRadioInfo;
		int nRadio_idx = 0;

		auto pGlobalData = GetPdxData();
		ASSERT(pGlobalData);

		//! kiwa72(2023.05.01 11h) - UDP NtoN
		CRX_N2N_DEVICE* pNtoN_DeviceInfo = NULL;
		POSITION pos = pGlobalData->g_ptrClientSocketList.GetHeadPosition();
		while (pos != NULL)
		{
			pNtoN_DeviceInfo = (CRX_N2N_DEVICE*)pGlobalData->g_ptrClientSocketList.GetNext(pos);
			if (pNtoN_DeviceInfo != NULL)
			{
				wchar_t strUnicode[256] = { 0, };
				int nLen = MultiByteToWideChar(CP_ACP, 0, pNtoN_DeviceInfo->Name, strlen(pNtoN_DeviceInfo->Name), NULL, NULL);
				MultiByteToWideChar(CP_UTF8, 0, pNtoN_DeviceInfo->Name, strlen(pNtoN_DeviceInfo->Name), strUnicode, nLen);

				//! kiwa72(2023.08.30 11h) - 이미 다른 장비와 연결 상태 유/무, Sleep 모드 상태표시 추가
				sRadioInfo.Format(L"%s | %d.%d.%d.%d | %s(%d)",
					strUnicode,
					pNtoN_DeviceInfo->device_ipaddr[0],
					pNtoN_DeviceInfo->device_ipaddr[1],
					pNtoN_DeviceInfo->device_ipaddr[2],
					pNtoN_DeviceInfo->device_ipaddr[3],
					//! kiwa72(2023.08.30 11h) - Sleep 모드 상태표시 추가
					pNtoN_DeviceInfo->state == ss_disconnected ? L"Available" : pNtoN_DeviceInfo->state == ss_sleep ? L"Sleep Mode" : L"Already in use",
					pNtoN_DeviceInfo->state
				);

				//! kiwa72(2023.08.30 11h) - 이미 다른 장비와 연결 상태 유/무
				m_apRadio_Scanner[nRadio_idx]->EnableWindow(pNtoN_DeviceInfo->state == ss_disconnected ? true : false);

				//! kiwa72(2023.11.25 17h) - 연결 상태가 바뀐 스캐너 상태 표시
				if (pNtoN_DeviceInfo->state != ss_disconnected || pNtoN_DeviceInfo->state == ss_sleep)
				{
					if (m_apRadio_Scanner[nRadio_idx]->GetCheck() == 1)
					{
						m_apRadio_Scanner[nRadio_idx]->SetCheck(0);
						m_nSelectScanner = -1;
					}
				}
				//!...

				m_apRadio_Scanner[nRadio_idx]->SetWindowTextW(sRadioInfo);

				nRadio_idx++;
			}
		}

		//! jgkim(2026.01.28) - 접속 가능한 스캐너가 1대인 경우 자동 선택
		int nAvailableCount = 0;
		int nAvailableIndex = -1;
		for (int i = 0; i < nRadio_idx; i++)
		{
			if (m_apRadio_Scanner[i]->IsWindowEnabled())
			{
				nAvailableCount++;
				nAvailableIndex = i;
			}
		}

		if (nAvailableCount == 1 && nAvailableIndex >= 0)
		{
			// 접속 가능한 스캐너가 1대인 경우 자동 선택 후 다이얼로그 닫기
			if (m_apRadio_Scanner[nAvailableIndex]->GetCheck() != 1)
			{
				m_apRadio_Scanner[nAvailableIndex]->SetCheck(1);
				m_nSelectScanner = nAvailableIndex;
				
				// 자동으로 선택 버튼 이벤트 발생 및 다이얼로그 닫기
				OnBnClickedButton_Scanner_Select();
				return;
			}
		}
		//!...

		//! kiwa72(2023.11.25 17h) - 모든 스캐너가 연결할 수 없는 상태일 경우 [Scanner Connect] 버튼 비활성화
		int jCnt;
		for (jCnt = 0; jCnt < nRadio_idx; jCnt++)
		{
			if (m_apRadio_Scanner[jCnt]->GetCheck() == 1)
			{
				break;
			}
		}

		if (jCnt == nRadio_idx)
		{
			GetDlgItem(IDC_BUTTON_SCANNER_SELECT)->EnableWindow(false);
		}
		else
		{
			GetDlgItem(IDC_BUTTON_SCANNER_SELECT)->EnableWindow(true);
		}
		//!...

		m_nTimeCnt++;

		CString sInfo;
		sInfo.Format(L"%dcount, %dsec", nRadio_idx, m_nTimeCnt / 5);
		m_Static_ConnectTime.SetWindowTextW(sInfo);

		UpdateData(FALSE);
	}

	CDialogEx::OnTimer(nIDEvent);
}
