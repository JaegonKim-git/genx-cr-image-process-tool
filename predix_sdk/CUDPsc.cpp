// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "common_type.h"
#include "scanner.h"
#include "cruxcan_defines.h"
#include "tray_network.h"
#include "CUDPsc.h"
#include "CTCPsc.h"
#include "global_data.h"
#include "../include/logger.h"

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
using namespace Concurrency;


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■
// UDP 클라이언트
//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■


//! kiwa72(2023.08.30 11h) - 현재 접속중인 장비 정보
//CRX_N2N_DEVICE g_CurrentConnectionDevice;
//!...


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CUDPsc::CUDPsc()
	: m_pUDPs(nullptr)
	, m_NtoN_SelectDevice(-1)
{
	m_pUDPs = new CUDPReceiveSocket();
	ASSERT(m_pUDPs);

	//! kiwa72(2023.05.01 11h) - 브로드캐스팅 패킷 초기화
	m_Broadcast_Packet.Reset(nc_tray);

	//! kiwa72(2023.08.30 11h) - 브로드캐스팅 관련 초기화
	m_Broadcasting_State = 255;
	m_Direct_Broadcasting_Count = 0;
	//!...
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CUDPsc::~CUDPsc()
{
	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	if (pGlobalData->g_pTicker != NULL)
	{
		pGlobalData->g_pTicker->endTicker();
	}

	if (m_pUDPs != NULL)
	{
		m_pUDPs->fnOnReceivedDataRaw_Udp = NULL;
	}

	if (m_pUDPs != NULL)
	{
		if (m_pUDPs->m_hSocket != INVALID_SOCKET)
		{
			m_pUDPs->ShutDown(CSocket::both);
			m_pUDPs->Close();
		}

		delete m_pUDPs;
		m_pUDPs = NULL;
	}

	//! kiwa72(2023.05.01 11h) - UDP NtoN 접속 장비 정보 제거
	POSITION pos = pGlobalData->g_ptrClientSocketList.GetHeadPosition();
	while (pos != NULL)
	{
		CRX_N2N_DEVICE* p = (CRX_N2N_DEVICE*)pGlobalData->g_ptrClientSocketList.GetNext(pos);
		if (p != NULL)
		{
			delete p;
			p = NULL;
		}
	}

	pGlobalData->g_ptrClientSocketList.RemoveAll();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CUDPsc::Ready_to_UDPSockets(ON_RECEIVE_DATA_RAW OnReceiveData)
{
	KLOG3A("port : %d", PREDIX_UDP_PORT);

	BOOL bRet = m_pUDPs->Create(PREDIX_UDP_PORT, SOCK_DGRAM, GetPdxData()->g_strmyIPs[GetPdxData()->g_selected_adapter_index]);
	if (bRet != TRUE)
	{
		UINT uErr = GetLastError();
		KLOG3A("[%u port] Send Socket Create() failed: %d", PREDIX_UDP_PORT, uErr);

		CString msg;
		DWORD le = GetLastError();
		msg.Format(L"Creating UDP-socket failed with error %lu", le);
		AfxMessageBox(msg, MB_OK | MB_ICONSTOP);

		return FALSE;
	}

	m_pUDPs->fnOnReceivedDataRaw_Udp = OnReceiveData;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CUDPsc::BroadcastData(unsigned char* data, unsigned long size, LPCTSTR sendip, int port)
{
	if (m_pUDPs == NULL)
	{
		//KLOG3A("m_pUDPs is NULL / sendip = %s", sendip);
		writelog("UDP BroadcastData: m_pUDPs is NULL\n", "predix_network.log");
		return;
	}

	// 네트워크 로그
	char logMsg[512];
	sprintf_s(logMsg, sizeof(logMsg), "UDP BroadcastData: IP=%S, Port=%d, Size=%lu bytes\n", sendip, port, size);
	writelog(logMsg, "predix_network.log");

	// TEST: Makeit - stack 이 계속 망가져서 KLOG3W 매크로를 함수로 변경. 테스트 필요.
	KLOG3W(_T("m_pUDPs is not NULL / sendip = %s , port = %d, size = %d"), sendip, port, size);
	//KLOG3W(_T("m_pUDPs is not NULL / sendip = %s , port = %d, size = %d"), _T("192.168.33.255"), port, size);

	SOCKADDR addr;
	memset((char*)&addr, 0, sizeof(addr));

	int flag = 1;
	m_pUDPs->SetSockOpt(SO_BROADCAST, &flag, sizeof(flag));
	m_pUDPs->SendTo(data, size, port, (LPCTSTR)sendip);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CUDPsc::SendData(unsigned char* data, unsigned long size, CString ip, unsigned long port)
{
	KLOG3W();

	//int flag = 0;

	if (m_pUDPs == NULL)
	{
		MYPLOGMA("m_udps is null\n");
		return;
	}

	m_pUDPs->SendTo(data, size, port, (LPCTSTR)ip);
}

//! kiwa72(2024.08.17 15h) - 브로드케스팅 IP 셋팅
CString Get_Broadcasting_IP()
{
	int idx = GetPdxData()->g_selected_adapter_index;
	CString sIPaddr = GetPdxData()->g_strudps[idx];
	CString sIPmask = GetPdxData()->g_strMask[idx];

	//.........................................................

	int nIP[5] = { 0, };
	CString Seperator = _T(".");
	int Position = 0, i = 0;
	CString Token;
	Token = sIPaddr.Tokenize(Seperator, Position);
	nIP[i++] = _ttoi(Token);
	while (!Token.IsEmpty())
	{
		// Get next token.
		Token = sIPaddr.Tokenize(Seperator, Position);
		nIP[i++] = _ttoi(Token);
	}


	//.........................................................

	int nMask[5] = { 0, };
	Position = 0, i = 0;
	Token = sIPmask.Tokenize(Seperator, Position);
	nMask[i++] = _ttoi(Token);
	while (!Token.IsEmpty())
	{
		// Get next token.
		Token = sIPmask.Tokenize(Seperator, Position);
		nMask[i++] = _ttoi(Token);
	}


	//.........................................................


	CString sBroadcastIP;
	sBroadcastIP.Format(_T("%d.%d.%d.%d"), 
		(unsigned char) (nIP[0] | ~nMask[0]), (unsigned char)(nIP[1] | ~nMask[1]), (unsigned char)(nIP[2] | ~nMask[2]), (unsigned char)(nIP[3] | ~nMask[3]));
	KLOG3W(L"-->>> sBroadcast IP = %s", sBroadcastIP.GetBuffer());
	sBroadcastIP.ReleaseBuffer();

	//.........................................................

	return sBroadcastIP;
}


void save_log_of_udp(int mode)
{
	wchar_t temp[MAX_PATH];
	CString selected = GetPdxData()->g_piniFile->GetValueH(_T("network"), _T("selected"));

	if (selected == L"NONE" || selected.IsEmpty() )
	{
		GetPdxData()->g_plogFile->SetValue(_T("current"), _T("used ethernet adapter"), _T("default"));
	}
	else
	{
		GetPdxData()->g_plogFile->SetValue(_T("current"), _T("used ethernet adapter"), selected);
	}

	if (mode == 1)
	{
		wsprintfW(temp, L"[1/3]single mode : %s", (LPTSTR)(LPCTSTR)GetPdxData()->single_target);
	}
	else
	{
		wsprintfW(temp, L"[1/3]normal mode : %s", LPCTSTR(Get_Broadcasting_IP()));
	}

	GetPdxData()->g_plogFile->SetValue((CString)_T("current"), (CString)_T("step"), (CString)(CW2T)temp);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief	Broadcast Tray UDP
 * @details
 * @param	bool isFirst		[IN]	최초 실행 유/무
 * @param	NETWORK_COMMAND enc	[IN]	네트워크 명령
 * @param	int nSelDevice		[IN]	연결 장비 선택 유(-1)/무(0)
 * @retval	none
 * @author	kiwa72
 * @date	2023.03.22 22h
 * @version	0.0.1
 */

 //! kiwa72(2023.08.30 11h) - 추가
#include <algorithm>
using namespace std;
//!...

void CUDPsc::BroadcastTrayUDP(bool isFirst, NETWORK_COMMAND enc)
{
	KLOG3W();

	// 네트워크 로그
	char logMsg[512];
	sprintf_s(logMsg, sizeof(logMsg), "UDP BroadcastTrayUDP: Command=%d, Device=%s, IP=%d.%d.%d.%d, isFirst=%d\n",
		enc,
		GetPdxData()->g_CurrentConnectionDevice.Name,
		GetPdxData()->g_CurrentConnectionDevice.device_ipaddr[0],
		GetPdxData()->g_CurrentConnectionDevice.device_ipaddr[1],
		GetPdxData()->g_CurrentConnectionDevice.device_ipaddr[2],
		GetPdxData()->g_CurrentConnectionDevice.device_ipaddr[3],
		isFirst);
	writelog(logMsg, "predix_network.log");

	//! kiwa72(2023.05.01 11h) - 브로드캐스팅 패킷 초기화
	m_Broadcast_Packet.Reset(enc);

	//! kiwa72(2023.08.30 11h) - 접속할 장비 정보
	strcpy(m_Broadcast_Packet.name, GetPdxData()->g_CurrentConnectionDevice.Name);
	memcpy(&m_Broadcast_Packet.ipaddr, GetPdxData()->g_CurrentConnectionDevice.device_ipaddr, sizeof(GetPdxData()->g_CurrentConnectionDevice.device_ipaddr));
	//!...

	// 네트워크 명령 저장
	m_Broadcast_Packet.bytes[6] = (UCHAR)enc; 
	// 시간, CRC 저장
	set_time2udp(m_Broadcast_Packet, isFirst);
	set_checksum_udp(m_Broadcast_Packet);

	if (isFirst == true)
	{
		save_log_of_udp(GetPdxData()->g_inetwork_mode);
	}

	// 네트워크 연결 취소 시 처리
	if (enc == nc_tray_cancel && m_NtoN_SelectDevice == -1)
	{
		m_NtoN_SelectDevice = -1;

		for (int j = 0; j < 2; j++)
		{
			for (int i = 0; i < GetPdxData()->g_adaptercount; i++)
			{
#if (0)
				BroadcastData(m_Broadcast_Packet.bytes, sizeof(stUDP), LPCTSTR(GetPdxData()->g_strudps[i] + _T(".255")), PREDIX_UDP_PORT);
#else
				//! kiwa72(2024.08.17 15h) - 브로드케스팅 IP 셋팅
				BroadcastData(m_Broadcast_Packet.bytes, sizeof(stUDP), LPCTSTR(Get_Broadcasting_IP()), PREDIX_UDP_PORT);
#endif
			}

			Sleep(50);
		}

		return;
	}

	KLOG3A("g_inetwork_mode = %d", GetPdxData()->g_inetwork_mode);
	KLOG3A("-->> m_Broadcast_Packet.name = %s", m_Broadcast_Packet.name);
	KLOG3A("-->> m_Broadcast_Packet.ipaddr = %d.%d.%d.%d", m_Broadcast_Packet.ipaddr[0], m_Broadcast_Packet.ipaddr[1], m_Broadcast_Packet.ipaddr[2], m_Broadcast_Packet.ipaddr[3]);

	//! kiwa72(2023.08.30 11h) - 장비 지정(이름, IP주소) 브로드캐스팅 시 카운트
	if (strlen(m_Broadcast_Packet.name) > 0 && strcmp(m_Broadcast_Packet.name, "none") != 0)
	{
		//!
		//! kiwa72(2024.01.28 15h) - 장치 지정 브로드캐스팅 카운트가 3회 이상이면 연결 실패로 'Ccruxcan_trayDlg::OnTimer'에서 처리
		//! 
		OutputDebugString(_T("trying to Set_Broadcasting_State(1)\n"));
		m_Direct_Broadcasting_Count++;
		Set_Broadcasting_State(1);
	}
	else
	{
		OutputDebugString(_T("Reset_Direct_Broadcasting_Count()\n"));
		Reset_Direct_Broadcasting_Count();
		OutputDebugString(_T("trying to Set_Broadcasting_State(255)\n"));
		Set_Broadcasting_State(255);
	}
	KLOG3A("m_Direct_Broadcasting_Count = %d", m_Direct_Broadcasting_Count);
	//!...

	switch (GetPdxData()->g_inetwork_mode)
	{
	case 0:
		//! kiwa72(2023.08.30 11h) - 전체 장비 조회와 기존 장치와의 연결을 개선
		if (strlen(m_Broadcast_Packet.name) == 0 || strcmp(m_Broadcast_Packet.name, "none") == 0)
		{
			//! kiwa72(2024.08.17 15h) - 브로드케스팅 IP 셋팅
			CString broadcastIP = Get_Broadcasting_IP();
			KLOG3W(L"--> Broadcasting_IP = %s", broadcastIP.GetBuffer());
			broadcastIP.ReleaseBuffer();

			BroadcastData(m_Broadcast_Packet.bytes, sizeof(stUDP), (LPCTSTR)broadcastIP, PREDIX_UDP_PORT);
			//!...
		}
		else
		{
			GetPdxData()->g_strudps[GetPdxData()->g_selected_adapter_index].Format(L"%u.%u.%u.%u",
				m_Broadcast_Packet.ipaddr[0],
				m_Broadcast_Packet.ipaddr[1],
				m_Broadcast_Packet.ipaddr[2],
				m_Broadcast_Packet.ipaddr[3]);
			KLOG3A("--> g_strudps[g_selected_adapter_index]");
			BroadcastData(m_Broadcast_Packet.bytes, sizeof(stUDP), LPCTSTR(GetPdxData()->g_strudps[GetPdxData()->g_selected_adapter_index]), PREDIX_UDP_PORT);
		}
		//!...

		if (isFirst)
		{
			KLOG3(_T("broadcast tgt UDP : %s%s"), LPCTSTR(GetPdxData()->g_strudps[GetPdxData()->g_selected_adapter_index]), _T(".255"));
		}
		break;

	case 1:
		SendData(m_Broadcast_Packet.bytes, sizeof(stUDP), (LPTSTR)(LPCTSTR)GetPdxData()->single_target, PREDIX_UDP_PORT);
		if (isFirst)
		{
			KLOG3(_T("send m_Broadcast_Packet tgt UDP : %s"), (LPCTSTR)GetPdxData()->single_target);
		}
		break;

	default:
		for (int i = 0; i < GetPdxData()->g_adaptercount; i++)
		{
			BroadcastData(m_Broadcast_Packet.bytes, sizeof(stUDP), LPCTSTR(GetPdxData()->g_strudps[i] + _T(".255")), PREDIX_UDP_PORT);
			Sleep(70);
			if (isFirst)
			{
				KLOG3(_T(" multi tgt UDP : %s%s"), (LPCTSTR)(GetPdxData()->g_strudps[i]), _T(".255"));
			}
		}
		break;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * @brief	Start Tray Leader
 * @details	트레이 리더(대화상자) 시작
 * @param	none
 * @retval	none
 * @author	kiwa72
 * @date	kiwa72(2023.04.23 11h)
 * @version	0.0.1
 */
void CUDPsc::Start_Tray_Leader(void)
{
	KLOG3W();

	// Ticker 생성
	if (GetPdxData()->g_pTicker == NULL)
	{
		KLOG3A("Create g_pTicker : g_pTicker = new CTicker();");
		GetPdxData()->g_pTicker = new CTicker();
	}
}

/**
 * @brief	End Tray Leader
 * @details	트레이 리더(대화상자) 종료
 * @param	none
 * @retval	none
 * @author	kiwa72
 * @date	kiwa72(2023.04.23 11h)
 * @version	0.0.1
 */
void CUDPsc::End_Tray_Leader()
{
	KLOG3W();

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);
	
	if (!pGlobalData->hCloseMutex)
	{
		DWORD dwErrorCode = 0;
		Sleep(0);

		pGlobalData->hCloseMutex = CreateMutex(NULL, TRUE, _T("CRUXCAN_CRX-1000_TRAY_END_MUTEX"));
		dwErrorCode = GetLastError();
		if (dwErrorCode == 0x0005 || dwErrorCode == 0x00B7)
		{
			KLOG3A("already CRUXCAN_CRX-1000_TRAY_END_MUTEX");
		}
		else
		{
			KLOG3A("create CRUXCAN_CRX-1000_TRAY_END_MUTEX");
		}
	}
	else
	{
		KLOG3A("hCloseMutex is not null");
	}

	//! kiwa72(2023.10.01 06h) - 제거
	if (pGlobalData->g_pTicker != NULL)
	{
		pGlobalData->g_pTicker->endTicker();
		delete pGlobalData->g_pTicker;
	}
	pGlobalData->g_pTicker = NULL;

	Sleep(0);
	//!...

	Force_Disconnect_DeviceSocket();
	KLOG3A("done Force_Disconnect_DeviceSocket");
}

void CUDPsc::set_time2udp(stUDP& udp, bool isfirst)
{
	time_t g_time = time(NULL);
	time_t l_time;
	struct tm* local_time = gmtime(&g_time);

	l_time = mktime(local_time);

	if (isfirst)
	{
		udp.starttime = (double)(g_time + (g_time - l_time) + 2209161600.) / 86400.;	// tray간 순서 정하기 용

		KLOG3A("!!!!!!!!!send double value :: %lf", udp.starttime);
	}

	udp.currenttime = (double)(g_time + (g_time - l_time) + 2209161600.) / 86400.;
}

#ifdef USE_UDP_CRC_CHECK
#include "stm32_crc.h"
#endif

void CUDPsc::set_checksum_udp(stUDP& udp)
{
#ifdef USE_UDP_CRC_CHECK
#if (DEF_CRC32_MPEG2)
	// kiwa72(2022.11.24 18h) - CRC-32bit/MPEG-2 적용
	unsigned long  checksum5 = crcCompute32((unsigned int*)&udp.params[3], (sizeof(stUDP) - (sizeof(uint32_t) * 3)) / sizeof(uint32_t));
#else
	// kiwa72(2022.11.24 18h) - CRC-8bit
	unsigned long  checksum5 = crcCompute((unsigned char*)&udp.params[3], sizeof(stUDP) - sizeof(uint32_t) * 3);
#endif

	KLOG3A("checksum5 = %08X", checksum5);
	num2arr(checksum5, (unsigned char*)&udp.checksum);
#else
	unsigned long checksum = udp.params[3];
	for (int i = 4; i < UDP_PACKET_SIZE / sizeof(long); i++)
	{
		checksum += udp.params[i];
	}
	num2arr(checksum, (unsigned char*)&udp.checksum);
#endif
}

void CUDPsc::Force_Disconnect_DeviceSocket(void)
{
	KLOG3W();

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	if (pGlobalData->g_pDeviceSocket != NULL)
	{
		KLOG3A("step 1");

		pGlobalData->g_pDeviceSocket->m_isClientSocketEnd = true;
		pGlobalData->g_pDeviceSocket->ShutDown(CSocket::both);
		pGlobalData->g_nConnected_ScannerCnt = 0;

		delete pGlobalData->g_pDeviceSocket;
		pGlobalData->g_pDeviceSocket = NULL;

		Sleep(500);
	}
	else
	{
		KLOG3A("there is no g_pDeviceSocket");
	}
}
