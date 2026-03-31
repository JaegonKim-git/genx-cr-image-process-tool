// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "cruxcan_defines.h"
#include "stm32_crc.h"
#include "postprocess.h"
#include "common_type.h"
#include "common.h"
#include "scanner.h"
#include "tiff_image.h"
#include "tiff/tiffio.h"
#include "stmyqueue.h"
#include "tray_network.h"
#include "CUDPsc.h"
#include "CTCPsc.h"
#include "global_data.h"
#include "CTCPServerSock.h"

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <icmpapi.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern bool is_local_connected;
extern CString pc_name;

extern void check_pcname();
extern void send_pcname_to_device(bool isconneceted);
extern int checkMacaddress(CString mac);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CTCPServerSock::CTCPServerSock()
{
	//GetPdxData()->g_ptrClientSocketList.RemoveAll();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CTCPServerSock::~CTCPServerSock()
{
	ShutDown(CSocket::both);
	Close();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//! kiwa72(2023.05.01 11h) - 장비 TCP 연결
void CTCPServerSock::OnAccept(int nErrorCode)
{
	KLOG3A();

	CTCPClientSock* pClient = new CTCPClientSock;
	ASSERT(pClient);

	//int flag = 1;		/* 일정 시간(통상 2시간)동안 해당 소켓을 통해 어떤 자료도 송수신되지 않을 때, 커널에서 상대방의 상태를 확인하는 패킷을 전송 */
	//if (pClient->SetSockOpt(SO_KEEPALIVE, &flag, sizeof(flag)) == false)
	//{
	//	AfxMessageBox(L"- ERROR: Configuring SetSockOpt SO_KEEPALIVE on a new client failed. !!!");
	//	return;
	//}

	//struct timeval tv_timeo = { 3, 0 };	/* 3.0 second - 수신 timeout 설정*/
	//if (pClient->SetSockOpt(SO_RCVTIMEO, &tv_timeo, sizeof(tv_timeo)) == false)
	//{
	//	AfxMessageBox(L"- ERROR: Configuring SetSockOpt SO_RCVTIMEO on a new client failed. !!!");
	//	return;
	//}

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	//! kiwa72(2023.05.01 11h) - 선택한 장비 TCP 접속 허용
	if (Accept(*pClient) == FALSE)
	{
		delete pClient;
		pClient = NULL;
		AfxMessageBox(L"- ERROR: Failed to accept new client !!!");

		//! kiwa72(2023.12.23 18h) - 추가 코드
		CSocket::OnAccept(nErrorCode);
		return;
	}

	// 연결괴 클라이언트의 IP주소와 포트 번호를 구한다.
	pClient->GetPeerName(pClient->PeerIP, pClient->PeerPort);
	// Mac 정보 구한다.
	pClient->PeerMac = GetMacAddress(pClient->PeerIP);

	// kiwa72(2023.04.23 11h) - 접속 완료 상태 플래그. 최종 선택 후 true 설정
	pClient->m_isClientConnected = false;

	// 접속한 장비 개수
	pGlobalData->g_nConnected_ScannerCnt++;

	// kiwa72(2023.04.23 11h) - 접속 클라이언트 정보(IP, Mac)
	KLOG3W(_T("============================================================"));
	KLOG3W(_T(">>> Accept(*pClient);"));
	KLOG3W(_T("------------------------------------------------------------"));
	KLOG3W(_T(">>> g_nConnected_ScannerCnt = %d"), pGlobalData->g_nConnected_ScannerCnt);
	KLOG3W(_T(">>> PeerIP = %s"), (LPCTSTR)pClient->PeerIP);
	KLOG3W(_T(">>> PeerMac = %s"), (LPCTSTR)pClient->PeerMac);
	KLOG3W(_T("============================================================"));

	//---------------------------------------------------------------
	//! kiwa72(2023.05.01 11h) - 선택한 스캐너 사용환경 셋팅
	pGlobalData->g_TCP_Server->m_pServerSock->Init_Scanner_Connect(pClient);

	pGlobalData->g_pUDPsc->m_NtoN_SelectDevice = -1;
	//---------------------------------------------------------------

	CSocket::OnAccept(nErrorCode);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CTCPServerSock::checkMacaddress(CString mac)
{
	if (mac == L"00:00:00:00:00:00")
	{
		return (int)enLocal;
	}

	return (int)enDevice;
}

UINT CTCPServerSock::Thread_ProcessClient(LPVOID Arg)
{
	CTCPClientSock* pClient_Sock = (CTCPClientSock*)Arg;

	Sleep(1 * 1000);

	// close socket()
	pClient_Sock->ShutDown(CSocket::both);
	pClient_Sock->Close();

	//!
	GetPdxData()->g_pTicker->endTicker();

	return (0);
}

/**
 * @brief	Init Scanner
 * @details	NtoN 최종 선택된 스캐너의 초기화 셋팅
 * @param	[IN]
 * @param	[OUT]
 * @retval
 * @author	kiwa72
 * @date	kiwa72(2023.04.23 11h)
 * @version	0.0.1
 */
void CTCPServerSock::Init_Scanner_Connect(CTCPClientSock* pSock)
{
	KLOG3A(">>>");

	pSock->m_isClientConnected = true;
	pSock->m_isClientSocketEnd = false;

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	switch (checkMacaddress(pSock->PeerMac))
	{
	case enDevice:
		qinit(&tcpqueuedevice, tcp_q_data_device, MY_QUEUE_SIZE + 1);	// use_tcp_queue

		KLOG3A(">> qinit for device");

		pSock->stType = enDevice;
		pSock->fnOnReceivedDataRaw_tcp = ReceiveDataFromDevice;	// kiwa72(2023.01.01 00h) - 장비로 부터 데이터 수신 콜백함수 등록
		pSock->fnOnConnect = OnConnectFromDevice;			// kiwa72(2023.01.01 00h) - 장비 접속 콜백함수 등록
		pSock->fnOnClose = OnCloseFromDevice;				// kiwa72(2023.01.01 00h) - 장비 연결 끊기 콜백함수 등록

		KLOG3(_T("connected  crx-1000 and send pcname[%s]"), (LPCTSTR)pSock->PeerIP);

		pGlobalData->notify_status_changed(true);	// 이벤트 받고나서 이벤트 함수 연결이므로

		KLOG3(_T(">>> Setting: g_pDeviceSocket = pSock;"));
		pGlobalData->g_pDeviceSocket = pSock;	// set g_pDeviceSocket

		// start check ping;
		pGlobalData->g_pDeviceSocket->start_ping();

		Sleep(1000);

		if (is_local_connected)
		{
			KLOG3A("send resume packet1");
			NETWORK_COMMAND nc = (FALSE ? nc_sleep_scanner : nc_resume_scanner);
			send_protocol_to_device(nc, NULL, 0);

		}

		if (pc_name == L"" || pc_name.MakeLower() == "none")
		{
			KLOG3A("pc_name is null");
			check_pcname();
		}

		// greeting with device
		send_pcname_to_device(true);
		break;
	}
}

CString CTCPServerSock::GetMacAddress(CString strIP)
{
	if (strIP.MakeUpper() == _T("127.0.0.1") || strIP.MakeUpper() == _T("LOCALHOST"))
	{
		KLOG3A("localhost\n");
		return _T("00:00:00:00:00:00");
	}

	IPAddr  ipAddr;
	ULONG   pulMac[2];
	ULONG   ulLen;

	ipAddr = inet_addr((LPSTR)(LPCSTR)CT2A(strIP));
	memset(pulMac, 0xff, sizeof(pulMac));

	ulLen = 6;
	HRESULT hr = SendARP(ipAddr, 0, pulMac, &ulLen);
	if (hr != NO_ERROR)
	{
		KLOG3A("failed to sendARP\n");
		return _T("00:00:00:00:00:00");
	}

	size_t i, j;
	char* szMac = new char[ulLen * 3];
	PBYTE pbHexMac = (PBYTE)pulMac;

	for (i = 0, j = 0; i < ulLen - 1; ++i)
	{
		j += sprintf(szMac + j, "%02X:", pbHexMac[i]);
	}

	sprintf(szMac + j, "%02X", pbHexMac[i]);
	CString strMac = CA2T(szMac);
	delete[] szMac;

	return strMac;
}
