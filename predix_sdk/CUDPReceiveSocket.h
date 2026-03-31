#pragma once


//class AFX_EXT_CLASS CUDPReceiveSocket : public CAsyncSocket
class AFX_EXT_CLASS CUDPReceiveSocket : public CSocket
{
public:
	CUDPReceiveSocket();
	virtual ~CUDPReceiveSocket();

public:
	CString strRecieverIp;
	CString strSendersIp;
	UINT uSendersPort;

	ON_RECEIVE_DATA_RAW fnOnReceivedDataRaw_Udp;

public:
	void printSocketErrorMessage(int nErrorCode);

private:
	void OnReceive(int nErrorCode);
};
