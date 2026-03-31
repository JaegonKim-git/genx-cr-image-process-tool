// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CDlg_NtoN_Scanner_Select 대화 상자
class CDlg_NtoN_Scanner_Select : public CDialogEx
{
	DECLARE_DYNAMIC(CDlg_NtoN_Scanner_Select)

public:
	CDlg_NtoN_Scanner_Select(CWnd* pParent = nullptr);	// 표준 생성자입니다.
	virtual ~CDlg_NtoN_Scanner_Select();

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG_NtoNSCANNERSELECT };
#endif


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public methods
public:
	int GetSelectedScanner(void) const { return m_nSelectScanner; };


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// protected methods
protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// protected members
protected:
	CButton m_Radio_Scanner_1;
	CButton m_Radio_Scanner_2;
	CButton m_Radio_Scanner_3;
	CButton m_Radio_Scanner_4;
	CButton m_Radio_Scanner_5;
	CStatic m_Static_ConnectTime;
	CButton* m_apRadio_Scanner[5];	//! 접속 스캐너 리스트

	int m_nSelectScanner;			//! 접속한 스캐너 중 선택한 스캐너 인덱스 저장
	int m_nTimeCnt;					//! 타임 카운트


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// event handlers
	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedButton_Scanner_Select();
	afx_msg void OnBnClickedButton_Cancel();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
