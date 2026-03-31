// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "common_type.h"
#include "scanner.h"
#include "cruxcan_defines.h"
#include "tray_network.h"
#include "CUDPsc.h"
#include "global_data.h"
#include "CUDPReceiveSocket.h"

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// UDP 패킷 수신
//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CUDPReceiveSocket::CUDPReceiveSocket()
	: uSendersPort(0)
	, fnOnReceivedDataRaw_Udp(nullptr)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CUDPReceiveSocket::~CUDPReceiveSocket()
{
}

void CUDPReceiveSocket::OnReceive(int nErrorCode)
{
	KLOG3W();

	static int count = 0;

	count++;

	unsigned char buff[4096];
	int nRead;
	int error;

	// Could use Receive here if you don't need the senders address & port
	nRead = ReceiveFrom(buff, 4096, strSendersIp, uSendersPort);
	KLOG3A("nRead = %d", nRead);

	switch (nRead)
	{
	case 0:		// Connection was closed.
		ShutDown(CSocket::both);
		Close();
		break;

	case SOCKET_ERROR:
		error = GetLastError();
		if (error != WSAEWOULDBLOCK)
		{
			MYPLOGDG("[CRXE0606][%u]udp socket error occured", uSendersPort);
			printSocketErrorMessage(error);

			ShutDown(CSocket::both);
			Close();
		}
		break;

	default:	// Normal case: Receive() returned the # of bytes received.
		if (fnOnReceivedDataRaw_Udp)
		{
			fnOnReceivedDataRaw_Udp(nRead, buff, this);
		}
		break;
	}

	CSocket::OnReceive(nErrorCode);
}

void CUDPReceiveSocket::printSocketErrorMessage(int nErrorCode)
{
	switch (nErrorCode)
	{
	case WSANOTINITIALISED:	// 성공적으로 AfxSocketInit 이 API를 사용 하기 전에 발생 합니다.
		KLOG3A("socekt error case 1 WSANOTINITIALISED");
		break;
	case WSAENETDOWN:		// Windows 소켓 구현이 감지 네트워크 하위 시스템 실패 합니다.
		KLOG3A("socekt error case 2 WSAENETDOWN");
		break;
	case WSAENOTCONN:		// 소켓이 연결 되어 있지 않습니다.
		KLOG3A("socekt error case 3 WSAENOTCONN");
		break;
	case WSAEINPROGRESS:	// Windows 소켓 차단 작업이 진행 중입니다.
		KLOG3A("socekt error case 4 WSAEINPROGRESS");
		break;
	case WSAENOTSOCK:		// 설명자가 소켓이 아닙니다.
		KLOG3A("socekt error case 5 WSAENOTSOCK");
		break;
	case WSAEOPNOTSUPP:		// MSG_OOB 지정 된 소켓 형식의 아닙니다 SOCK_STREAM.
		KLOG3A("socekt error case 6 WSAEOPNOTSUPP");
		break;
	case WSAESHUTDOWN:		// 아래로 소켓 이미 종료 된 호출할 수 없는 수신 후 소켓 ShutDown 호출 된	nHow 0 또는 2로 설정 합니다.
		KLOG3A("socekt error case 7 WSAESHUTDOWN");
		break;
	case WSAEWOULDBLOCK:	// 소켓 표시 되어 비블로킹으로 및 수신 수 작업을 차단 합니다.
		KLOG3A("socekt error case 8 WSAEWOULDBLOCK");
		break;
	case WSAEMSGSIZE:		// 데이터 그램이 너무 커서 지정한 버퍼로 하므로 잘렸습니다.
		KLOG3A("socekt error case 9 WSAEMSGSIZE");
		break;
	case WSAEINVAL:			// 소켓에는 바인딩되지 않은 바인딩할.
		KLOG3A("socekt error case 10 WSAEINVAL");
		break;
	case WSAECONNABORTED:	// 가상 회로 시간 초과 나 기타 오류로 인해 중단 되었습니다.
		KLOG3A("socekt error case 11 WSAECONNABORTED");
		break;
	case WSAECONNRESET:		// 가상 회로 원격 나란히 원래 대로 설정 했습니다.
		KLOG3A("socekt error case 12 WSAECONNRESET");
		break;
	case 0:
		KLOG3A("no error in socket 0");
		break;
	default:
		KLOG3A("socekt error case  unknown[%d]", nErrorCode);
		break;
	}
}
