// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "tray_network.h"
#include "CTCPServerSock.h"
#include "CTCPClientSock.h"


class CTCPsc
{
public:
	CTCPsc();
	~CTCPsc();

public:
	BOOL Ready_to_TCPSockets();
	BOOL TCPListen(void);

	void SetOnConnect(void* onconnect);
	void SetOnClose(void* onclose);
	void SetOnReceive(void* onreceive);
	bool SendData(char* data, int size);
	BOOL Connect(CString addr, int port);
	void resume_from_sleep(void);
	void disconnect_by_sleep(void);

	void Server_Close(void);

public:
	CTCPServerSock* m_pServerSock;
private:
	CTCPClientSock* m_pClientSock;
};


