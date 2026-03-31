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
#include "CTCPClientSock.h"

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <icmpapi.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// macro constants

extern bool g_is_sent_system_reset;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CTCPClientSock::CTCPClientSock()
	: m_hThisHandle(nullptr)
	, nReserved(0)
	, nReceived(0)
	, nChecked(0)
	, buffer_l(nullptr)
	, buffer_s(nullptr)
	, PeerPort(0)
	, stType(enNone)
	, resScanImg(eir_HiRes)
	, m_isClientConnected(false)
	, m_isClientSocketEnd(false)
	, isSleep(0)
	, isBusy(0)
	, packet_number(0)
	, m_pThread(nullptr)
{
	buffer_l = new unsigned char[MAX_TCP_SOCKET_BUFFER];
	buffer_s = new unsigned char[MIN_TCP_SOCKET_BUFFER];

	fnOnReceivedDataRaw_tcp = NULL;
	fnOnConnect = nullptr;
	fnOnClose = nullptr;
	m_isClientSocketEnd = false;

	m_hThisHandle = this;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CTCPClientSock::~CTCPClientSock()
{
	SAFE_DELETE_ARRAY(buffer_l);
	SAFE_DELETE_ARRAY(buffer_s);

	m_isClientSocketEnd = true;
	fnOnReceivedDataRaw_tcp = NULL;

	ShutDown(CSocket::both);

	if (m_hSocket != INVALID_SOCKET)
	{
		ShutDown(CSocket::both);
		Close();
	}

	// done.
	m_hThisHandle = nullptr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UINT CTCPClientSock::TCP_Ping_ThreadFunction(LPVOID pVoid)
{
	CTCPClientSock* pThis = (CTCPClientSock*)pVoid;
	ASSERT(pThis);

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	pThis->isSleep = 0;

	AfxSocketInit(NULL);	// thread단위 호출로 안다

	struct hostent* phe = gethostbyname(CT2A(pThis->PeerIP));

	HINSTANCE hIcmp = LoadLibrary(_T("ICMP.DLL"));

	if (!hIcmp)
	{
		KLOG3W(L"failed to load ICMP.dll");
		return (UINT)-1;
	}

	//typedef struct
	//{
	//	DWORD Address;					// Replying address
	//	unsigned long  Status;			// Reply status
	//	unsigned long  RoundTripTime;	// RTT in milliseconds
	//	unsigned short DataSize;		// Echo data size
	//	unsigned short Reserved;		// Reserved for system use
	//	void* Data;						// Pointer to the echo data
	//	IP_OPTION_INFORMATION Options;	// Reply options
	//} IP_ECHO_REPLY, * PIP_ECHO_REPLY;

	typedef HANDLE(WINAPI* pfnHV)(VOID);
	typedef BOOL(WINAPI* pfnBH)(HANDLE);
	typedef DWORD(WINAPI* pfnDHDPWPipPDD)(HANDLE, DWORD, LPVOID, WORD, PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD); // evil, no?

	pfnHV pIcmpCreateFile = (pfnHV)GetProcAddress(hIcmp, "IcmpCreateFile");
	pfnBH pIcmpCloseHandle = (pfnBH)GetProcAddress(hIcmp, "IcmpCloseHandle");
	pfnDHDPWPipPDD pIcmpSendEcho = (pfnDHDPWPipPDD)GetProcAddress(hIcmp, "IcmpSendEcho");
	UNREFERENCED_PARAMETER(pIcmpCloseHandle);

	HANDLE hIP = pIcmpCreateFile();
	char acPingBuffer[64];

	memset(acPingBuffer, '\xAA', sizeof(acPingBuffer));

	PIP_ECHO_REPLY pIpe = (PIP_ECHO_REPLY)GlobalAlloc(GMEM_FIXED | GMEM_ZEROINIT, sizeof(IP_ECHO_REPLY) + sizeof(acPingBuffer));

	pIpe->Data = acPingBuffer;
	pIpe->DataSize = sizeof(acPingBuffer);

	pGlobalData->check_iniFile();

	int ping_limit = DEFAULT_PING_LIMIT_COUNT;

	while (!pThis->m_isClientSocketEnd)
	{
		if ( pThis->m_hThisHandle == nullptr )
		{
			ASSERT(0);
			return 0;
		}

		if (pThis->isSleep == 1)
		{
			pThis->m_isClientSocketEnd = true;

			KLOG3A("do disconnect by sleep/resume mode 1 ");

			pThis->ShutDown(CSocket::both);
			pThis->Close();

			//! 접속 장비 개수 초기화
			pGlobalData->g_nConnected_ScannerCnt = 0;

			//! 장비 연결 상태 해제 - 장비에 상태 전송 ->> 브로드캐스팅 시작
			//set_scanner_state_s(ss_disconnected);
			set_scanner_state_s_only(ss_disconnected);

			//!
			pGlobalData->g_pTicker->endTicker();

			//KLOG3A("eiohlei skip OnClose(-1) 1 ");
			//pThis->OnClose(-1);
			pThis->isSleep = 0;
			continue;
		}

		for (int i = 0; i < 20 && !pThis->m_isClientSocketEnd; i++)
		{
			if (pThis->isSleep)
			{
				break;;
			}
			Sleep(50);
		}

		if (pThis->isSleep == 1)
		{
			continue;
		}

		if (pThis->isBusy || (pThis->nReserved && pThis->nReceived && pThis->nReceived != pThis->nChecked))
		{
			pThis->nChecked = pThis->nReceived;
			ping_limit = DEFAULT_PING_LIMIT_COUNT;
			continue;
		}

		if (pThis->isSleep == 1)
		{
			continue;
		}

		DWORD dwStatus = pIcmpSendEcho(
			hIP,
			*((DWORD*)phe->h_addr_list[0]),
			acPingBuffer,
			sizeof(acPingBuffer),
			NULL,
			pIpe,
			sizeof(IP_ECHO_REPLY) + sizeof(acPingBuffer),
			500);

		if (dwStatus != 0)
		{
			ping_limit = DEFAULT_PING_LIMIT_COUNT;
		}
		else
		{
			KLOG3A("~~~~ping error");

			if (g_is_sent_system_reset == true)
			{
				g_is_sent_system_reset = false;

				// 시스템 리셋 전송시 공유기 단에서는 연결끊김 인식까지 시간이 오래 걸리는것 보완
				ping_limit -= 10;
			}

			ping_limit--;
		}

		if (pThis->isSleep == 1)
		{
			continue;
		}

		if (ping_limit < 1 && pThis->isSleep != 1)
		{
			pThis->m_isClientSocketEnd = true;

			KLOG3A("~~~~~~ limit out in ping");

			// 프로그램 x같이 짜놨네.. ㅡ.,ㅡ^
			//if (pThis != NULL)
			if ( pThis && pThis->m_hThisHandle != nullptr && pThis->m_hThisHandle != (HANDLE)0xdddddddd )
			{
				//! 접속 장비 개수 초기화
				pGlobalData->g_nConnected_ScannerCnt = 0;

				//! 장비 연결 상태 해제 - 장비에 상태 전송 ->> 브로드캐스팅 시작
				//set_scanner_state_s(ss_disconnected);
				set_scanner_state_s_only(ss_disconnected);

				if ( pThis->m_pThread->m_hThread != nullptr )
				{
					//pThis->AsyncSelect(FD_CLOSE/*0*/);
					pThis->ShutDown(CSocket::both);
					pThis->Close();
				}

				//! 장비 연결 인덱스 초기화
				pGlobalData->g_pUDPsc->m_NtoN_SelectDevice = -1;

				//!
				pGlobalData->g_pTicker->endTicker();
			}
		}
	}

	GlobalFree(pIpe);
	FreeLibrary(hIcmp);

	return 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CTCPClientSock::resume_from_sleep(void)
{
}

void CTCPClientSock::disconnect_by_sleep(void)
{
	KLOG3W();

	isSleep = 1;
}

//! kiwa72(2023.04.23 11h) - Ping 사용을 못하게 한다. 필요가 없다.
void CTCPClientSock::start_ping(void)
{
	KLOG3A("~~~~~~~~CTCPClientSock::start_ping\n");
	ASSERT(m_pThread == nullptr);

	m_pThread = AfxBeginThread(CTCPClientSock::TCP_Ping_ThreadFunction, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (m_pThread)
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
}

void CTCPClientSock::end_ping(void)
{
	m_isClientSocketEnd = true;
	if (m_pThread)
	{
		ASSERT((UINT_PTR)m_pThread != 0xDDDDDDDD);
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
		delete m_pThread;
		m_pThread = NULL;
	}
}

void CTCPClientSock::clear(void)
{
	memset(buffer_l, 0, MAX_TCP_SOCKET_BUFFER);
	memset(buffer_s, 0, MIN_TCP_SOCKET_BUFFER);

	nReserved = 0;
	nReceived = 0;
	nChecked = 0;
}

bool CTCPClientSock::block_sendData(char* data, int len)
{
	static bool busy = false;

	while (busy)
		Sleep(0);

	busy = true;

	int nRecv = Send(data, len);

	busy = false;

	return nRecv == len;
}

void CTCPClientSock::OnReceive(int nErrorCode)
{
	DWORD dwReadLen;

	IOCtl(FIONREAD, &dwReadLen);	// 패킷 길이를 먼저 파악하고

	if (dwReadLen > MIN_TCP_SOCKET_BUFFER)
	{
#if (0)	// org
		unsigned char* largebuf = new unsigned char[dwReadLen];
		int nRead = Receive(largebuf, dwReadLen);
#endif


#if (1)	// update
		unsigned char* largebuf = new unsigned char[dwReadLen];
		int nRead = Receive(largebuf, dwReadLen);
		if (nRead == SOCKET_ERROR)
		{
			LPVOID lpMsgBuf;
			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER |
				FORMAT_MESSAGE_FROM_SYSTEM |
				FORMAT_MESSAGE_IGNORE_INSERTS,
				NULL,
				nErrorCode,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),	// Default language
				(LPTSTR)&lpMsgBuf,
				0,
				NULL);

			KLOG3A("%d : %s", nErrorCode, (char*)(CT2A)(LPCTSTR)lpMsgBuf);
			LocalFree(lpMsgBuf);
		}
#endif

		if (fnOnReceivedDataRaw_tcp)
		{
			unsigned char* cur = largebuf;
			int sent = 0;
			int total = (dwReadLen / TCP_PACKET_MIN_SIZE) * TCP_PACKET_MIN_SIZE;

			for (sent = 0; sent < total && m_isClientSocketEnd != true; sent += TCP_PACKET_MIN_SIZE, cur += TCP_PACKET_MIN_SIZE)
			{
				Sleep(0);

				if ( fnOnReceivedDataRaw_tcp && m_isClientSocketEnd != true )
					fnOnReceivedDataRaw_tcp(TCP_PACKET_MIN_SIZE, cur, this);
				else
					break;
			}

			if ( dwReadLen - total > 0 && fnOnReceivedDataRaw_tcp )
			{
				fnOnReceivedDataRaw_tcp(dwReadLen - total, cur, this);
			}
		}

		SAFE_DELETE_ARRAY(largebuf);
	}
	else
	{
#if (1)	// org
		Receive(this->buffer_s, dwReadLen);
#endif

		if (fnOnReceivedDataRaw_tcp)
		{
			unsigned char* cur = this->buffer_s;
			int sent = 0;
			int total = (dwReadLen / TCP_PACKET_MIN_SIZE) * TCP_PACKET_MIN_SIZE;

			for (sent = 0; sent < total; sent += TCP_PACKET_MIN_SIZE, cur += TCP_PACKET_MIN_SIZE)
			{
				fnOnReceivedDataRaw_tcp(TCP_PACKET_MIN_SIZE, cur, this);
			}

			if (dwReadLen - total > 0)
			{
				fnOnReceivedDataRaw_tcp(dwReadLen - total, cur, this);
			}

			//fnOnReceivedDataRaw(dwReadLen, this->buffer_s, this);
		}
	}

	CSocket::OnReceive(nErrorCode);
}

void CTCPClientSock::OnConnect(int nErrorCode)
{
	KLOG3W();

	if (fnOnConnect)
	{
		fnOnConnect(nErrorCode);
	}

	CSocket::OnConnect(nErrorCode);
	nReceived = nReserved = nChecked = 0;
};

void CTCPClientSock::OnClose(int nErrorCode)
{
	KLOG3W();

	if (fnOnClose)
	{
		fnOnClose(nErrorCode);
	}

	//end_ping();

	nReceived = 0;
	nReserved = 0;
	nChecked = 0;

	CSocket::OnClose(nErrorCode);
};
