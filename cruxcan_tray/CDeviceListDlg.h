#pragma once


// CDeviceListDlg dialog
class CDeviceListDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDeviceListDlg)

public:
	// standard constructor
	CDeviceListDlg(CWnd* pParent = nullptr);

	// destructor
	virtual ~CDeviceListDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DEVICELIST_DLG };
#endif

public:

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void update_scanner_list(void);
	void init_scanner_list(void);
	long try_connect(int nItem);
	void show_result_of_try_connect(long value);

protected:
	CListCtrl m_wndListScanners;
	crxDevices* received_crxDevices;

	// event handlers
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButtonConnect();
	afx_msg void OnDblclkListScannerState(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonDisconnect();
	afx_msg void OnBnClickedButtonClose();
	LRESULT OnReciveDisconnectMessage(WPARAM wparam, LPARAM lparam);
};

extern CDeviceListDlg* device_dlg;
