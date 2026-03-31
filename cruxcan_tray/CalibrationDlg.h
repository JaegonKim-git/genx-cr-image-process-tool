// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CCalibrationDlg 대화 상자
class CCalibrationDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CCalibrationDlg)

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public constructor and destructor
public:
	// 표준 생성자입니다.
	CCalibrationDlg(CWnd* pParent = nullptr);

	// destructor
	virtual ~CCalibrationDlg();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_CALIBRATION };
#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// protected methods
protected:
	long check_image(CString filename);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// protected members
protected:
	CString m_strMemo;
	CString m_strCoefs;
	CStringList m_ImageList;
	int m_nImage_count;
	int m_iValidWidth;
	int m_iValidHeight;
	BOOL m_bUpdate_ValidArea_to_Image;
	BOOL m_bDefinedArea;

	CButton m_btnSaveParam;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// event handlers
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL OnInitDialog();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedButtonAddRaw();
	afx_msg void OnBnClickedButtonCalc();
	afx_msg void OnBnClickedButtonSaveFile();// 2023-11-22. jg kim. 버튼 이름 변경
	afx_msg void OnBnClickedButtonSaveScanner();
	afx_msg void OnBnClickedButtonResetCalibration();
};
