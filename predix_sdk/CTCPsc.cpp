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

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <icmpapi.h>


#pragma comment(lib, "ws2_32.lib")


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// macro constants

extern void scan_data_init();	// scanner.cpp
extern void scan_data_destroy();	//! 사용 유/무 확인
CString pc_name = L"";
bool g_is_receiving_data = false;
SYSTEMTIME device_st;

long device_oldsec = 0;
long device_cursec = 0;
long device_differ = 0;
long device_meansec = 0;
long device_count = 0;	//! 이름 수정 필요
int device_oldprogress = 0;
int device_curprogress = 0;
unsigned char* device_current = NULL;

unsigned char validtcppacketsdk[TCP_COMMAND_SIZE];
unsigned char validtcppacketdevice[TCP_COMMAND_SIZE];

BOOL is_tray_pp_ing = false;
bool is_local_connected = false;
bool is_first_nc_hardware_info = false;


stcalibration default_calibrationdata =
{
	{65533, 1, 65531, 65534},
	{0.0, 0.0, 0.0, 0.0, 0.0},
	{65533, 1, 65531, 65533}
};

stcalibration  cur_calibrationdata =
{
	{65533, 1, 65531, 65534},
	{0.0, 0.0, 0.0, 0.0, 0.0},
	{65533, 1, 65531, 65533}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CTCPsc::CTCPsc()
	: m_pServerSock(nullptr)
	, m_pClientSock(nullptr)
{
	m_pServerSock = new CTCPServerSock();
	m_pClientSock = new CTCPClientSock();

	scan_data_init();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CTCPsc::~CTCPsc()
{
	scan_data_destroy();

	try
	{
		Server_Close();
	}
	catch (...)
	{
		MYPLOGDG(L"error in ~CTPCsc\n");
	}

	if (m_pServerSock != NULL)
	{
		delete m_pServerSock;
		m_pServerSock = NULL;
	}

	if (m_pClientSock != NULL)
	{
		delete m_pClientSock;
		m_pClientSock = NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CTCPsc::Ready_to_TCPSockets()
{
	KLOG3A("port : %d", PREDIX_TCP_PORT);

	AfxSocketInit(NULL);

	//! 서버 소켓 생성
	if (m_pServerSock->Create(PREDIX_TCP_PORT, SOCK_STREAM, GetPdxData()->g_strmyIPs[GetPdxData()->g_selected_adapter_index]) == FALSE)
	{
		UINT uErr = GetLastError();
		KLOG3A("[%u port] Send Socket Create() failed: %d", PREDIX_TCP_PORT, uErr);

		CString msg;
		DWORD le = GetLastError();
		msg.Format(L"Creating TCP-socket failed with error %lu", le);
		AfxMessageBox(msg, MB_OK | MB_ICONSTOP);

		return FALSE;
	}

	if (GetPdxData()->g_TCP_Server->TCPListen() == FALSE)
	{
		return FALSE;
	}

	//----------------------------------------------------------------------------

	//! 장비에 서버 소켓이 있고 클라이언트로 접속할 소켓 생성
	if (m_pClientSock->Create() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL CTCPsc::TCPListen(void)
{
	KLOG3A();

	if (m_pServerSock->Listen() == FALSE)
	{
		KLOG3A("failed to start listenning tray tcp server [CTCPsc::TCPListen]");
		return FALSE;
	}

	KLOG3A("start listenning tray tcp server");

	return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void CTCPsc::SetOnConnect(void* OnConnect)
{
	m_pClientSock->fnOnConnect = (ON_CONNECT_SOCKET)OnConnect;
}

void CTCPsc::SetOnClose(void* Onclose)
{
	m_pClientSock->fnOnClose = (ON_CLOSE_SOCKET)Onclose;
}

void CTCPsc::SetOnReceive(void* OnReceive)
{
	m_pClientSock->fnOnReceivedDataRaw_tcp = (ON_RECEIVE_DATA_RAW)OnReceive;
}

BOOL CTCPsc::Connect(CString addr, int port)
{
	MYPLOGDG("try to connect to tray\n");
	return m_pClientSock->Connect(addr, port);
}

void CTCPsc::Server_Close()
{
	KLOG3A("before Close 8");

	m_pClientSock->ShutDown(CSocket::both);
	m_pClientSock->Close();	// check1

	//! 장비 연결 상태 해제 - 장비에 상태 전송 ->> 브로드캐스팅 시작
	//set_scanner_state_s(ss_disconnected);
	set_scanner_state_s_only(ss_disconnected);

	// Memo: Makeit - global data로 옮겼기 때문에 소멸자에서 GetPdxData()를 호출하면 여러가지 문제가 발생할 수 있다.
	// global_data의 소멸자에서 0으로 설정하도록 위치 변경.
	////! 접속 장비 개수 초기화
	//GetPdxData()->g_nConnected_ScannerCnt = 0;

	//!
	//GetPdxData()->g_pTicker->endTicker();

	KLOG3A("before Close 9");

	//.................................................

	m_pServerSock->ShutDown(CSocket::both);
	m_pServerSock->Close();
}

bool CTCPsc::SendData(char* data, int size)
{
	int sent = m_pClientSock->Send(data, size);
	return (sent == size);
}

void CTCPsc::resume_from_sleep(void)
{
	if (m_pClientSock)
		m_pClientSock->resume_from_sleep();
}

void CTCPsc::disconnect_by_sleep(void)
{
	if (m_pClientSock)
		m_pClientSock->disconnect_by_sleep();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void send_pcname_to_device(bool isconneceted)
{
	NETWORK_COMMAND nc;
	if (pc_name == L"" || !isconneceted)
	{
		KLOG3A("sdk is not running -> nc_request_hardware_info");
		nc = nc_request_hardware_info;
		send_protocol_to_device(nc, NULL, 0);
	}
	else
	{
		nc = nc_pcname;
		KLOG3A("sdk is running -> pc_name");
		CStringA  p = CT2A((LPTSTR)(LPCTSTR)pc_name);
		send_protocol_to_device(nc, (unsigned char*)(LPCSTR)p, p.GetLength());
	}
}

void send_tray_busy_to_device(int  is_busy)
{
	unsigned char temp[4];
	num2arr(is_busy, temp);
	send_protocol_to_device(nc_tray_busy, temp, 4);
}

int is_valid_stcalibration(stcalibration* stcali)
{
	return (
		(stcali->sigs[0] == 65533 && stcali->sigs[2] == 65531 && stcali->sigs[3] == 65534) &
		(stcali->sige[0] == 65533 && stcali->sige[2] == 65531 && stcali->sige[3] == 65533)) > 0 ? 1 : 0;
}

void write_calibrationdata_to_image(void)
{
	KLOG3A();

	//
	// write 호출시점에 이미 cur_calibration 갱신되어 있으니 신경쓸 필요 없음 
	//

	const int calibration_pos_x = 5;

	if (is_valid_stcalibration(&cur_calibrationdata) < 1)
	{
		memcpy(&cur_calibrationdata, &default_calibrationdata, sizeof(stcalibration));
		KLOG3A("using default");
	}
	else
	{
		KLOG3A("using current");
	}

	KLOG3A("will write %e %e %e %e %e", cur_calibrationdata.data[0], cur_calibrationdata.data[1], cur_calibrationdata.data[2], cur_calibrationdata.data[3], cur_calibrationdata.data[4]);

	raw_t* tagdata = GetPdxData()->map_raw.data();
	raw_t* calidata = (raw_t*)&cur_calibrationdata;

	for (int i = 0; i < (sizeof(stcalibration) / sizeof(raw_t)); i++)
	{
		tagdata[i * geo_x + calibration_pos_x] = calidata[i];
	}
}

void do_nc_connect_scanner(int value)
{
	KLOG3A("value = %d", value);

	if (value > 0)
	{
		GetPdxData()->g_pUDPsc->Start_Tray_Leader();
	}
	else
	{
		GetPdxData()->g_pUDPsc->End_Tray_Leader();
		Sleep(100);
	}
}

int get_local_ip_end()
{
	DWORD dwRet;
	PIP_ADAPTER_ADDRESSES pAdpAddrs;
	PIP_ADAPTER_ADDRESSES pThis;
	PIP_ADAPTER_UNICAST_ADDRESS pThisAddrs;
	unsigned long ulBufLen = sizeof(IP_ADAPTER_ADDRESSES);

	pAdpAddrs = (PIP_ADAPTER_ADDRESSES)malloc(ulBufLen);
	if (!pAdpAddrs)
	{
		ASSERT(0);
		return 0;
	}

	int nFamily = AF_INET;

	dwRet = GetAdaptersAddresses(nFamily, 0, NULL, pAdpAddrs, &ulBufLen);
	if (dwRet == ERROR_BUFFER_OVERFLOW)
	{
		free(pAdpAddrs);
		pAdpAddrs = NULL;
	}
	ASSERT(ulBufLen > 0);

	pAdpAddrs = (PIP_ADAPTER_ADDRESSES)malloc(ulBufLen);
	if (!pAdpAddrs)
	{
		ASSERT(0);
		return 0;
	}

	dwRet = GetAdaptersAddresses(nFamily, 0, NULL, pAdpAddrs, &ulBufLen);
	if (dwRet != NO_ERROR)
	{
		free(pAdpAddrs);
		return 0;
	}

	for (pThis = pAdpAddrs; NULL != pThis; pThis = pThis->Next)
	{
		for (pThisAddrs = pThis->FirstUnicastAddress; NULL != pThisAddrs; pThisAddrs = pThisAddrs->Next)
		{
			if (nFamily == pThisAddrs->Address.lpSockaddr->sa_family)
			{
				struct sockaddr_in* pAddr = (struct sockaddr_in*)pThisAddrs->Address.lpSockaddr;
				MYLOGOLDA(inet_ntoa(pAddr->sin_addr));

				auto nReturn = pAddr->sin_addr.S_un.S_un_b.s_b4;
				free(pAdpAddrs);
				return nReturn;
			}
		}
	}

	free(pAdpAddrs);

	return 0;
}

void check_pcname()
{
	char temp[128];
	int ip = get_local_ip_end();

	sprintf(temp, "chair%03d", ip);
	MYLOGOLDA(temp);

	pc_name = CA2W(temp);
	GetPdxData()->g_piniFile->SetValue(_T("predix"), _T("pcname"), pc_name);
}

void send_greeting_to_host(void* Socket)
{
	KLOG3A();

	UNREFERENCED_PARAMETER(Socket);

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	stDevice device;

	set_cruxcan_head_tray(nc_tray, device.bytes);
	strcpy((char*)device.devicename, (char*)pGlobalData->current_setting.Name);
	*(long*)&device.ip_addr = *(long*)&pGlobalData->current_setting.ipaddr;
	*(long*)&device.version = 0;// *(long*)&current_setting.HardWare_version;
	device.state = pGlobalData->current_setting.state;

	memmove(&device.mac_addr, &pGlobalData->current_setting.MAC_ADDR, 6);
	memmove(&device.udi, &pGlobalData->current_setting.UDI, 51);

	pGlobalData->crx_pipe.push_write_pipe(device.bytes, sizeof(stDevice));

	MYPLOGDN(L"send greeting(first hw info) to host\n");
}

void ReceiveCommandFromSDK(unsigned char* data, DWORD size, void* Socket)
{
	KLOG3W();

	// 2018.07.23 현재 host는 요청만 있다.

	NETWORK_COMMAND nc = (NETWORK_COMMAND)data[6];
	if (is_tray_pp_ing && nc == nc_request_image)
	{
		KLOG3A("[CRXE0105] cancel  request image  because image processing is not finished");
		return;
	}

	if (nc > nc_threshold)
	{
		// host <-> tray 간 명령

		unsigned long num = 0;

		switch (nc)
		{
		case nc_pcname:
			//if (pThis->stType == enSocketType::enLocal)
		{
			check_pcname();
			unsigned long value = 0;
			arr2num(&data[8], value);
			if (value != 0)
			{
				KLOG3A("[tray->sdk]send greeting packet to sdk");
				send_greeting_to_host(Socket);
			}
			else
			{
				KLOG3A("updated pcname from sdk");
			}

			send_pcname_to_device(true);	// sdk's first order
		}
		break;

		case nc_connect_scanner:
			arr2num(&data[8], num);
			do_nc_connect_scanner(num);
			break;

		default:
			KLOG3A("unknown cmd from SDK [%d][%d]", nc, search_header_in_data(data, size, &crxtags[0], sizeof(crxtags)));
			break;
		}
	}
	else
	{
		if (nc == nc_send_patient_name)
		{
			char name_data[MAX_NAME_LENGTH];
			memcpy(name_data, &data[8], MAX_NAME_LENGTH);

			NETWORK_COMMAND nc2 = nc_send_patient_name;
			send_protocol_to_device(nc2, (unsigned char*)name_data, MAX_NAME_LENGTH);
		}

		switch (nc)
		{
		case nc_sdk_disconnected:
			KLOG3A("receive nc_sdk_disconnected from pipe maybe");
			OnCloseFromLocalSDK(0);
			break;
		case nc_request_image_list_with_name:// 20191003 연훈 - 이미지에 다국어 환자이름 이미지까지 사용하기위해
		case nc_request_hardware_info:
		case nc_request_image_list:
		case nc_request_image:
		case nc_delete_all_image:
			if (GetPdxData()->g_pDeviceSocket != NULL)
			{
				GetPdxData()->g_pDeviceSocket->Send(data, size);
				KLOG3A("sent request of host  to device[%u]", (unsigned char)nc);
			}
			else
			{
				KLOG3A("recevied request packet from host but devicesocket is null");
				KLOG3A("need to develop failed return protocol");
			}
			break;

		default:
			KLOG3A("to do except request image,list,hw info1 or legacy");
			break;
		}
	}

	KLOG3A(" done Receive Command From SDK");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




