// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "../cruxcan_tray/resource.h"		// main symbols
#include "../predix_sdk/scanner.h"
#include "../predix_sdk/common.h"
#include "../predix_sdk/common_type.h"
#include "../predix_sdk/tray_network.h"
#include "CDeviceListDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// macro constants

#define WM_DISCONNECT_SOCKET_IN_MAIN_THREAD	(WM_USER + 3134)


CString get_scanner_state_string(SCANNER_STATE state)
{
	CString temp;

	switch (state)
	{
	case ss_disconnected:	return _T("ss_disconnected");
	case ss_warmup:			return _T("ss_warmup");
	case ss_ready:			return _T("ss_ready");
	case ss_scan:			return _T("ss_scan");
	case ss_transfer:		return _T("ss_transfer");
	case ss_busy:			return _T("ss_busy");
	case ss_system:			return _T("ss_system");
	case ss_sleep:			return _T("ss_sleep");
	//case ss_connected:	return _T("ss_connected");
	case ss_none:			return _T("ss_none");
	default:
		temp.Format(_T("unknown :%d"), (long)state);
		return temp;
	}
}

CDeviceListDlg* device_dlg = NULL;// will create in g_pTtray_Dlg

// CDeviceListDlg dialog

IMPLEMENT_DYNAMIC(CDeviceListDlg, CDialogEx)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CDeviceListDlg::CDeviceListDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DEVICELIST_DLG, pParent)
	, received_crxDevices(nullptr)
{
	received_crxDevices = new crxDevices;
	ASSERT(received_crxDevices);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CDeviceListDlg::~CDeviceListDlg()
{
	SAFE_DELETE(received_crxDevices);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// data exchange
void CDeviceListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_SCANNER_STATE, m_wndListScanners);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
LRESULT CDeviceListDlg::OnReciveDisconnectMessage(WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(lparam);

	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// message map
BEGIN_MESSAGE_MAP(CDeviceListDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_CONNECT, &CDeviceListDlg::OnBnClickedButtonConnect)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_SCANNER_STATE, &CDeviceListDlg::OnDblclkListScannerState)
	ON_BN_CLICKED(IDC_BUTTON_DISCONNECT, &CDeviceListDlg::OnBnClickedButtonDisconnect)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CDeviceListDlg::OnBnClickedButtonClose)
	ON_MESSAGE(WM_DISCONNECT_SOCKET_IN_MAIN_THREAD, &CDeviceListDlg::OnReciveDisconnectMessage)
END_MESSAGE_MAP()


// CDeviceListDlg message handlers

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CDeviceListDlg::OnBnClickedButtonClose()
{
	OnCancel();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CDeviceListDlg::update_scanner_list(void)
{
	m_wndListScanners.DeleteAllItems();

	int index = 0;
	wchar_t wstr_ipaddr[32];
	for (auto iter = received_crxDevices->datas.begin(); iter != received_crxDevices->datas.end(); iter++, index++)
	{
		wsprintf(wstr_ipaddr, L"%u.%u.%u.%u", iter->device_ipaddr[0], iter->device_ipaddr[1], iter->device_ipaddr[2], iter->device_ipaddr[3]);
		m_wndListScanners.InsertItem(index, wstr_ipaddr);
		m_wndListScanners.SetItemText(index, 1, CA2T(iter->Name));
		m_wndListScanners.SetItemText(index, 2, get_scanner_state_string(iter->state));
		wsprintf(wstr_ipaddr, L"%u.%u.%u.%u", iter->host_ipaddr[0], iter->host_ipaddr[1], iter->host_ipaddr[2], iter->host_ipaddr[3]);
		m_wndListScanners.SetItemText(index, 3, wstr_ipaddr);
	}
	m_wndListScanners.Invalidate();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CDeviceListDlg::init_scanner_list(void)
{
	received_crxDevices->datas.clear();//clear vector
	CRect rt;
	m_wndListScanners.DeleteAllItems();
	m_wndListScanners.GetWindowRect(&rt);
	m_wndListScanners.InsertColumn(0, _T("ip addr"), LVCFMT_CENTER, rt.Width() / 4 - 1 + 10);
	m_wndListScanners.InsertColumn(1, _T("name"), LVCFMT_CENTER, rt.Width() / 4 - 1 - 10);
	m_wndListScanners.InsertColumn(2, _T("state"), LVCFMT_CENTER, rt.Width() / 4 - 1 - 10);
	m_wndListScanners.InsertColumn(3, _T("host"), LVCFMT_CENTER, rt.Width() / 4 - 1 + 10);
	m_wndListScanners.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	UpdateData(FALSE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dialog initializing
BOOL CDeviceListDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	init_scanner_list();

	return TRUE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CDeviceListDlg::show_result_of_try_connect(long value)
{
	switch (value)
	{
	case -99:	// not selected;
		AfxMessageBox(_T("Scanner is not selected"));
		break;
	case -98:	// sdk dll does not support connect function
		AfxMessageBox(_T("SDK DLL does not provide connect functions"));
		break;
	case 1:
		AfxMessageBox(_T("Succeeded to request\nwait until result"));
		break;
	case 2:
		AfxMessageBox(_T("Already connected"));
		break;
	case -1:
		AfxMessageBox(_T("Target IP address is invalid"));
		break;
	case -2:
		AfxMessageBox(_T("Failed to request by network"));
		break;
	case -3:
		AfxMessageBox(_T("Other software is using scanner now"));
		break;
	case -4:
		AfxMessageBox(_T("Failed to request by parameter file"));
		break;
	case 0:
		AfxMessageBox(_T("Failed to request by parameter file"));
		break;
	default:
		MYPLOGMA(" show_result_of_try_connect unknown result [%d]\n", value);
		AfxMessageBox(_T("unknown error"));
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CDeviceListDlg::OnDblclkListScannerState(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	show_result_of_try_connect(try_connect(pNMItemActivate->iItem));

	*pResult = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
long CDeviceListDlg::try_connect(int nItem)
{
	if (nItem == -1)
	{
		return -99;
	}

	if (get_scanner_state_s() > ss_warmup)
	{
		return en2n_connected_already;	// =2, 이미 연결됨
	}

	auto connect_scanner_n2n = [](const char* tgtip)
	{
		char stgtip[128] = { 0, };
		::strcpy(stgtip, tgtip);
		MYPLOGMA("connect_scanner_n2n::%s\n", tgtip);

		if (strcmp(stgtip, "0.0.0.0") == 0)
		{
			return (long)en2n_invalid_param;
		}

		const int is_used = is_scanner_used(stgtip);	// 0, -3 , -10 네트워크가 나뻐서 들어온 데이터 없음 
		if (is_used != 0)
		{
			if (is_used == -10)
			{
				MYPLOGDG("NIN_POOR_NETWORK but ignore now...");
			}
			else
			{
				return (long)is_used;	// -3 N2N_USED_BY_OTHER
			}
		}

		return 0l;	//result;
	};

	CString host_ip = m_wndListScanners.GetItemText(nItem, 3);
	CString device_ip = m_wndListScanners.GetItemText(nItem, 0);

	auto ip_addr = CT2A((LPCTSTR)device_ip);

	return connect_scanner_n2n((LPSTR)(LPCSTR)ip_addr);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CDeviceListDlg::OnBnClickedButtonConnect()
{
	POSITION pos = m_wndListScanners.GetFirstSelectedItemPosition();
	const int nItem = m_wndListScanners.GetNextSelectedItem(pos);

	show_result_of_try_connect(try_connect(nItem));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CDeviceListDlg::OnBnClickedButtonDisconnect()
{
	m_wndListScanners.EnableWindow(FALSE);

	UpdateData(FALSE);

	PostMessage(WM_DISCONNECT_SOCKET_IN_MAIN_THREAD, 1, 0);
	m_wndListScanners.Invalidate();
}
