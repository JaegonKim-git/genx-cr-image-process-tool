// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once


//! TCP 클라이언트 소켓
//class CTCPClientSock : public CAsyncSocket
class CTCPClientSock : public CSocket
{
public:
	// constructor
	CTCPClientSock();

	// destructor
	virtual ~CTCPClientSock();

public:
	bool block_sendData(char* data, int len);

	virtual void OnReceive(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);
	void clear();

	void start_ping();
	void end_ping();

	void disconnect_by_sleep();
	void resume_from_sleep();


public:
	HANDLE m_hThisHandle;		// just for checking this instance.
	CWinThread* m_pThread;

	unsigned long nReserved;
	unsigned long nReceived;
	unsigned long nChecked;

	NETWORK_COMMAND nc;

	unsigned char* buffer_l;	// for full size
	unsigned char* buffer_s;	// for OnReceive 

	CString	PeerIP;
	CString PeerMac;
	UINT PeerPort;

	enSocketType stType;

	ON_RECEIVE_DATA_RAW fnOnReceivedDataRaw_tcp;
	ON_CONNECT_SOCKET fnOnConnect;
	ON_CLOSE_SOCKET fnOnClose;

	IMAGE_RESOLUTION resScanImg;
	IP_SIZE ipsize;

	bool m_isClientConnected;
	volatile bool m_isClientSocketEnd;

	int isSleep;
	int isBusy;

	unsigned long packet_number;


private:
	static UINT TCP_Ping_ThreadFunction(LPVOID pVoid);
};
