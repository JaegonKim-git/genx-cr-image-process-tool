// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "../predix_sdk/common_type.h"
#include "../predix_sdk/global_data.h"
#include "../predix_sdk/tray_network.h"
#include "../cruxcan_sdk/resource.h"
#include "afxdialogex.h"
#include "ImageListDlg.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



//#define DISPLAY_IP_SIZE	


HWND imagelist_wnd = 0;


IMPLEMENT_DYNAMIC(CImageListDlg, CDialogEx)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CImageListDlg::CImageListDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_IMAGE_LIST_DIALOG, pParent)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CImageListDlg::~CImageListDlg()
{
	imagelist_wnd = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CImageListDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IMAGE_LIST, m_image_list);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// message map
BEGIN_MESSAGE_MAP(CImageListDlg, CDialogEx)
	ON_NOTIFY(NM_DBLCLK, IDC_IMAGE_LIST, &CImageListDlg::OnNMDblclkImageList)
	ON_BN_CLICKED(IDC_BUTTON_CLOSE, &CImageListDlg::OnBnClickedButtonClose)
	ON_BN_CLICKED(IDC_BUTTON_REFRESH, &CImageListDlg::OnBnClickedButtonRefresh)
	ON_BN_CLICKED(IDC_BUTTON_ACQUIRE, &CImageListDlg::OnBnClickedButtonAcquire)
	ON_WM_CLOSE()
END_MESSAGE_MAP()


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CImageListDlg::update_local_language(void)
{
	if (GetPdxData()->g_obl_code == eoc_genoray)	// 일본어
	{
		SetWindowTextW(L"画像リスト");
		GetDlgItem(IDC_BUTTON_ACQUIRE)->SetWindowTextW(L"取得");
		GetDlgItem(IDC_BUTTON_REFRESH)->SetWindowTextW(L"更新");
		GetDlgItem(IDC_BUTTON_CLOSE)->SetWindowTextW(L"閉じる");
		UpdateData(FALSE);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CImageListDlg::update_current_to_list_image(void)
{
	struct tm* t = nullptr;

	MYPLOGMA(">>>>>size  check [%d][%d]\n", sizeof(stpvIMAGE_INFO), sizeof(stpvIMAGE_INFO_LIST));

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	for (int i = 0; i < MAX_IMAGE_COUNT; i++)
	{
		//int index = current_image_list.cur_image_num - i - 1;
		int index = pGlobalData->current_pvIMAGE_INFO_list.cur_image_num - i - 1;		
		if (index < 0)
		{
			index += MAX_IMAGE_COUNT;
		}

		wchar_t data_t[100];
		_stprintf(data_t, L"%3d", index + 1);
		m_image_list.SetItemText(i, 0, data_t);

		time_t image_time;
		//image_time = (time_t)(current_image_list.image_date[index] * DOUBLE_TIME_CONSTANT1 - DOUBLE_TIME_CONSTANT2);
		image_time = (time_t)(pGlobalData->current_pvIMAGE_INFO_list.imageinfos[index].image_date * 86400. - 2209161600.);

		if (image_time > 0)
		{
			t = gmtime(&image_time);
			_stprintf(data_t, L"%04d.%02d.%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);
			m_image_list.SetItemText(i, 1, data_t);

			_stprintf(data_t, L"%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
			m_image_list.SetItemText(i, 2, data_t);
		}
		else
		{
			_stprintf(data_t, L"%04d.%02d.%02d", 0, 0, 0);
			m_image_list.SetItemText(i, 1, data_t);

			_stprintf(data_t, L"%02d:%02d:%02d", 0, 0, 0);
			m_image_list.SetItemText(i, 2, data_t);
		}

		// kiwa72(2022.11.23 00h) - HR, SR 표기 문제 수정
#if (0)
		//if (current_image_list.image_mode[index])
		if (pGlobalData->current_image_info_list.imageinfos[index].image_resolution != 0)
			_stprintf(data_t, L"HR");
		else
			_stprintf(data_t, L"SR");
#else
		if (pGlobalData->current_pvIMAGE_INFO_list.imageinfos[index].image_resolution == eir_HiRes)
			_stprintf(data_t, L"HR");
		else
			_stprintf(data_t, L"SR");
#endif
		m_image_list.SetItemText(i, 3, data_t);

		//MultiByteToWideChar(CP_ACP, 0, current_image_list.name[index], strlen(current_image_list.name[index]) + 1, data_t, 100);
		MultiByteToWideChar(CP_ACP, 0, pGlobalData->current_pvIMAGE_INFO_list.imageinfos[index].name,
			strlen(pGlobalData->current_pvIMAGE_INFO_list.imageinfos[index].name) + 1, data_t, 100);
		m_image_list.SetItemText(i, 4, data_t);
	}

	UpdateData(FALSE);
	MYPLOGMA("updated done\n");
}


// CImageListDlg 메시지 처리기입니다.

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// dialog initializing
BOOL CImageListDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

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


	imagelist_wnd = this->m_hWnd;
	{
		m_image_list.InsertColumn(0, _T("Index"), LVCFMT_LEFT, 50);
		m_image_list.InsertColumn(1, _T("Date"), LVCFMT_LEFT, 75 + 20);
		m_image_list.InsertColumn(2, _T("Time"), LVCFMT_LEFT, 75);
		m_image_list.InsertColumn(3, _T("Resolution"), LVCFMT_LEFT, 70);
		m_image_list.InsertColumn(4, _T("Name"), LVCFMT_LEFT, 70 + 50 + 45);
	}

	m_image_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	for (int i = 0; i < MAX_IMAGE_COUNT; i++)
	{
		wchar_t data_t[10];
		_stprintf(data_t, L"%d", i + 1);
		m_image_list.InsertItem(i, data_t);  //current_image_list.image_date[i]);
	}

	update_current_to_list_image();
	update_local_language();

	return TRUE;
}


UINT messagebox_thread_function(LPVOID pVoid)
{
	//Sleep(50);
	ASSERT(pVoid);

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	const int index = *(int*)pVoid;

	switch (index)
	{
	case 1534:
		if (pGlobalData->g_obl_code == eoc_genoray)	// 일본어
		{
			::MessageBox(imagelist_wnd, L"装置が他の作業中であったり、まだ接続されていないため映像リクエストがキャンセルされました。", L"リクエストがキャンセルされました", MB_TOPMOST | MB_ICONWARNING);
		}
		else
		{
			::MessageBox(imagelist_wnd, L"not connected or scanner is not ready", L"failed", MB_TOPMOST | MB_ICONWARNING);
		}
		break;

	case 1535:
		if (pGlobalData->g_obl_code == eoc_genoray)	// 일본어
		{
			::MessageBox(imagelist_wnd, L"要請した映像の番号が正常ではありません。", L"リクエストがキャンセルされました", MB_TOPMOST | MB_ICONWARNING);
		}
		else
		{
			::MessageBox(imagelist_wnd, L"invaild image index", L"invalid index", MB_TOPMOST | MB_ICONWARNING);
		}
		break;
	}

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CImageListDlg::OnNMDblclkImageList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	if (pNMItemActivate->iItem != -1)
	{
		CString str = m_image_list.GetItemText(pNMItemActivate->iItem, 0);
		const int value = _tstoi(str);
		SetWindowText(str);
		UpdateData(FALSE);

		if (value >= 1 && value <= MAX_IMAGE_COUNT)
		{
#if (0)
			if (request_image_tray(value))
			{
				GetPdxData()->command_clear_stage();
			}
			else
			{
				int nValue = 1534;		// event message ID
				AfxBeginThread(messagebox_thread_function, &nValue, THREAD_PRIORITY_NORMAL, 0, 0);//myresult가 지역이면 thread동작중 
			}
#else
			//! kiwa72(2023.10.01 06h) - 이미지 리스트에서 슬립모드 시 이미지 받을 수 있도록 처리
			// 2026-02-27. jg kim. ImageList에서 요청한 영상은 영상처리 없이 raw만 저장
			GetPdxData()->bImageListMode = TRUE;
			KLOG3A("Image List > request_image_tray %d (ImageListMode enabled)", value);
			unsigned char temp[4] = { 0, };
			num2arr(value, temp);
			send_protocol_to_device(nc_request_image, temp, 4);
			//!...
#endif
		}
		else
		{
			int nValue = 1535;		// event message ID
			AfxBeginThread(messagebox_thread_function, &nValue, THREAD_PRIORITY_NORMAL, 0, 0);//myresult가 지역이면 thread동작중 
		}
	}

	*pResult = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CImageListDlg::OnBnClickedButtonClose()
{
	this->ShowWindow(SW_HIDE);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CImageListDlg::OnBnClickedButtonRefresh()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CImageListDlg::OnBnClickedButtonAcquire()
{
	POSITION pos = m_image_list.GetFirstSelectedItemPosition();
	int nItem = m_image_list.GetNextSelectedItem(pos);

	if (nItem != -1)
	{
		CString str = m_image_list.GetItemText(nItem, 0);
		const int value = _tstoi(str);
		SetWindowText(str);
		UpdateData(FALSE);

		if (value >= 1 && value <= MAX_IMAGE_COUNT)
		{
#if (0)
			if (request_image_tray(value))
			{
				//no action
				GetPdxData()->command_clear_stage();
			}
			else
			{
				int nValue = 1534;
				AfxBeginThread(messagebox_thread_function, &nValue, THREAD_PRIORITY_NORMAL, 0, 0);//myresult가 지역이면 thread동작중 
			}
#else
			//! kiwa72(2023.10.06 19h) - 이미지 리스트에서 슬립모드 시 [Acquire] 버튼 선택하면 이미지 받을 수 있도록 처리
			// 2026-02-27. jg kim. ImageList에서 요청한 영상은 영상처리 없이 raw만 저장
			GetPdxData()->bImageListMode = TRUE;
			KLOG3A("Image List > request_image_tray %d (ImageListMode enabled)", value);
			unsigned char temp[4] = { 0, };
			num2arr(value, temp);
			send_protocol_to_device(nc_request_image, temp, 4);
			//!...
#endif
		}
		else
		{
			int nValue = 1535;
			AfxBeginThread(messagebox_thread_function, &nValue, THREAD_PRIORITY_NORMAL, 0, 0);//myresult가 지역이면 thread동작중 

		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CImageListDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default

	CDialogEx::OnClose();
}
