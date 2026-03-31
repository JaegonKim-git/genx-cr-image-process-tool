#pragma once


enum enSocketType
{
	enNone		= 0,
	enDevice	= 1,
	enHost		= 2,
	enLocal		= 3,
};


class CTCPClientSock;


//! TCP 서버 소켓
//class AFX_EXT_CLASS CTCPServerSock : public CAsyncSocket
class AFX_EXT_CLASS CTCPServerSock : public CSocket
{
public:
	CTCPServerSock();
	virtual ~CTCPServerSock();

public:
	virtual void OnAccept(int nErrorCode);

	//! iwa72(2023.04.23 11h) - NtoN 최종 선택된 스캐너의 초기화 셋팅
	void Init_Scanner_Connect(CTCPClientSock* pSock);

	CString GetMacAddress(CString strIP);

	int CTCPServerSock::checkMacaddress(CString mac);

private:
	UINT Thread_ProcessClient(LPVOID Arg);
};
