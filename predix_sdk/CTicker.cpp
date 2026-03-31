// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <stdlib.h>
#include <stdio.h>
#include "common_type.h"
#include "scanner.h"
#include "CUDPsc.h"
#include "global_data.h"
#include "CTicker.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


/**
 * @brief	UDP Thread Function main
 * @details	1초 마다 브로드캐스팅
 * @param	[IN]	LPVOID pVoid	CTicker 객체 포인터
 * @retval	none
 * @author	kiwa72
 * @date	2023.05.01 11h
 * @version	0.0.1
 */
UINT CTicker::UDP_ThreadFunction_main(LPVOID pVoid)
{
	KLOG3A("Start UDP_ThreadFunction_main Tick");

	CTicker* pThis = (CTicker*)pVoid;

	while (pThis->m_bEnd == false)
	{
		if (get_scanner_state_s() < ss_ready)
		{
			// 스캐너 연결상태 확인
			KLOG3A("get_scanner_state_s() = %d", get_scanner_state_s());

			//! 브로드캐스팅 <<----------------^^
			GetPdxData()->g_pUDPsc->BroadcastTrayUDP(false);
		}

		//! kiwa72(2023.08.30 11h) - 장비 이름이 변경되어 기존 연결 정보로 연결이 안되는 경우 즉시 확인 대응
		if (GetPdxData()->g_bSel_Btn_Connect == TRUE
			&& GetPdxData()->g_bSel_Btn_ResetConnect == FALSE
			&& get_scanner_state_s() < ss_ready
		)
		{
			//! - 결과 확인: =1 시간 초과, =2 장비 이름이 다름
			int bResult = 0;
			int cnt = 0;
			POSITION pos = GetPdxData()->g_ptrClientSocketList.GetHeadPosition();
			while (pos != NULL)
			{
				CRX_N2N_DEVICE* pNtoN_DeviceInfo = (CRX_N2N_DEVICE*)GetPdxData()->g_ptrClientSocketList.GetNext(pos);
				if (pNtoN_DeviceInfo != NULL)
				{
					KLOG3A("pNtoN_DeviceInfo->Name = %s", pNtoN_DeviceInfo->Name);
					KLOG3A("pNtoN_DeviceInfo->device_ipaddr = %u.%u.%u.%u", pNtoN_DeviceInfo->device_ipaddr[0], pNtoN_DeviceInfo->device_ipaddr[1], pNtoN_DeviceInfo->device_ipaddr[2], pNtoN_DeviceInfo->device_ipaddr[3]);

					KLOG3A("g_pUDPsc->m_Broadcast_Packet.name = %s", GetPdxData()->g_pUDPsc->m_Broadcast_Packet.name);
					KLOG3A("g_pUDPsc->m_Broadcast_Packet.ipaddr = %u.%u.%u.%u", GetPdxData()->g_pUDPsc->m_Broadcast_Packet.ipaddr[0], GetPdxData()->g_pUDPsc->m_Broadcast_Packet.ipaddr[1], GetPdxData()->g_pUDPsc->m_Broadcast_Packet.ipaddr[2], GetPdxData()->g_pUDPsc->m_Broadcast_Packet.ipaddr[3]);

					KLOG3A("cnt = %d", cnt);
					KLOG3A("cnt = %d", cnt);

					cnt++;

					if (strcmp(_strupr(pNtoN_DeviceInfo->Name), _strupr(GetPdxData()->g_pUDPsc->m_Broadcast_Packet.name)) != 0)
					{
						bResult = 2;
					}

					if (cnt == 1)
					{
						break;
					}
				}
			}

			//! - 메세지 출력: 장치에 연결할 수 없습니다. 장비를 다시 찾아야 합니다.
			if (bResult > 0)
			{
				AfxMessageBox(L"Unable to connect to device.\nYou'll have to find your equipment again.");

				pThis->endTicker();
			}
		}
		//!...

		// 1초 마다 브로드캐스팅 하도록 타이밍
		if (pThis->m_bEnd == false)
		{
#if (_DEBUG_CONNECT_TIME_)
			Sleep(1000 * 2);
#else
			Sleep(1000 / 2);	//! kiwa72(2024.01.22 12h) - 타이밍 수정 2000 -> 500
#endif
		}
	}

	KLOG3A("end UDP_ThreadFunction_main tick function");

	return 0;
}

//■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■

CTicker::CTicker()
{
	KLOG3W();

	m_pThread_main = NULL;
	m_bEnd = false;
	m_bRunning = false;

	startTicker();
}

CTicker::~CTicker()
{
	endTicker();
}

void CTicker::startTicker(void)
{
	KLOG3W();

	if (m_pThread_main != NULL)
	{
		ASSERT((UINT_PTR)m_pThread_main != 0xDDDDDDDD);
		KLOG3A("Thread is exist");
		return;
	}

	m_bEnd = false;
	m_bRunning = false;

	m_pThread_main = AfxBeginThread(UDP_ThreadFunction_main, this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

	if (m_pThread_main != NULL)
	{
		m_pThread_main->m_bAutoDelete = FALSE;
		//m_pThread_main->m_bAutoDelete = TRUE;

		resumeTicker(true);
	}
}

void CTicker::endTicker(void)
{
	m_bEnd = true;

	pauseTicker();

	if (m_pThread_main != NULL)
	{
		ASSERT((UINT_PTR)m_pThread_main != 0xDDDDDDDD);
		//WaitForSingleObject(m_pThread_main->m_hThread, INFINITE);

		delete m_pThread_main;
		m_pThread_main = NULL;
	}
}

void CTicker::pauseTicker(void)
{
	if (m_bRunning == false)
	{
		return;
	}

	m_pThread_main->SuspendThread();
	m_bRunning = false;
}

void CTicker::resumeTicker(bool is_first)
{
	UNREFERENCED_PARAMETER(is_first);

	if (m_bRunning == true)
	{
		return;
	}

	ASSERT((UINT_PTR)m_pThread_main != 0xDDDDDDDD);
	m_pThread_main->ResumeThread();
	m_bRunning = true;
}
