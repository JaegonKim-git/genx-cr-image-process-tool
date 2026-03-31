// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "tray_network.h"
#include "scanner.h"
#include "common.h"
#include "common_type.h"
#include "stmyqueue.h"
#include "CUDPsc.h"
#include "CTCPsc.h"
#include "global_data.h"
#include "postprocess.h"
#include "../gnScanInformation/HardwareInfo.h"	// 2026-02-27. jg kim. decodeGenorayTag 사용을 위함
#include "../include/logger.h"

#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <ws2tcpip.h>
#include <afxmt.h>

#pragma comment(lib, "iphlpapi.lib")


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// macro constants


#define SIZE_OF_UDP_BUFFER		(UDP_BLOCK_SIZE * 21)


extern void ReceiveCommandFromSDK(unsigned char* data, DWORD size, void* Socket);
extern void ReceiveCommandFromDevice(unsigned char* data, DWORD size, void* Socket);
extern width crcCompute32(unsigned int* message, unsigned int nBytes);
extern void clear_raw();
extern void shift_raw(int size);
extern void write_calibrationdata_to_image(void);
extern void save_tiff_image(wchar_t* file_name, unsigned short int* data, int w, int h, int w2 = 0, float xresolution = 400.0f, float yresolution = 400.0f);
extern void send_tray_busy_to_device(int  is_busy);
extern int is_valid_stcalibration(stcalibration* stcali);


extern bool is_local_connected;
extern TCHAR working_path[1024];
//extern enum class PostProcessMode;
extern bool is_first_nc_hardware_info;
extern bool g_is_receiving_data;
extern SYSTEMTIME device_st;
extern long device_oldsec;
extern long device_cursec;
extern long device_differ;
extern long device_meansec;
extern long device_count;	//! 이름 수정 필요
extern int device_oldprogress;
extern int device_curprogress;
extern unsigned char* device_current;
extern stcalibration cur_calibrationdata;
extern stcalibration default_calibrationdata;
extern wchar_t loaded_image_name[MAX_PATH];
extern bool is_received_image;
extern BOOL is_tray_pp_ing;
extern CString pc_name;
CRX_N2N_PROTOCOL last_crx_n2n_protocol;
crxDevices last_received_crxdevices;


static CRX_N2N_PROTOCOL recv_crx_udp;
static unsigned char temp_buffer[512];


CString str_other_ip = _T("0.0.0.0");
bool need_yellow = false;
unsigned char c_udp_temp_buffer[SIZE_OF_UDP_BUFFER];	// 보통 MTU가 1500이니까  
int  c_udp_temp_buffer_data_count = 0;
CStringList LocalIPList;
bool g_is_sent_system_reset = false;
bool is_saved_chunk = false;
uint32_t sent_binary_checksum1 = 0;
uint32_t sent_binary_checksum2 = 0;
uint32_t receive_binary_checksum = 0;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


char isvalidUDPC(CRX_N2N_PROTOCOL* crxn2n)
{
	return (crxn2n->SignatureF[0] == 65533 &&
		crxn2n->SignatureF[1] == 1 &&
		crxn2n->SignatureF[2] == 65531 &&
		crxn2n->SignatureF[3] == 124 &&

		crxn2n->SignatureE[0] == 65532 &&
		crxn2n->SignatureE[1] == 1 &&
		crxn2n->SignatureE[2] == 65534 &&
		crxn2n->SignatureE[3] == 65535
		) ? 1 : 0;
}

char isValid_UDP_CF(unsigned char* bytes)
{
	unsigned short* dst = (unsigned short*)bytes;

	return (dst[0] == 65533 &&
		dst[1] == 1 &&
		dst[2] == 65531 &&
		dst[3] == 124
		) ? 1 : 0;
}

char isValid_Signature(unsigned char* data)
{
	return (data[0] == 0xFD &&	// 65533
		data[1] == 0xFF &&	// 65535
		data[2] == 0x01 &&
		data[3] == 0x00 &&
		data[4] == 0xFB &&	// 65531
		data[5] == 0xFF		// 65535
		) ? 1 : 0;
}

int Find_SOF(unsigned char* data, int size)
{
	// find normal udp

	if (size < 8)
	{
		return -1;
	}

	int pos = -1;
	int j = size - 8 + 1;

	for (int i = 0; (pos == -1) && (i < j); i++)
	{
		if (isValid_Signature(&data[i]) || isValid_UDP_CF(&data[i]))
		{
			pos = i;
		}
	}

	return pos;
}

void num2arr(const unsigned long num, unsigned char* arr)
{
	arr[0] = (num >> 0) & 0x000000FF;
	arr[1] = (num >> 8) & 0x000000FF;
	arr[2] = (num >> 16) & 0x000000FF;
	arr[3] = (num >> 24) & 0x000000FF;
}

void arr2num(const unsigned char* arr, unsigned long& num)
{
	num = ((arr[0] << 0) & 0x000000FF)
		| ((arr[1] << 8) & 0x0000FF00)
		| ((arr[2] << 16) & 0x00FF0000)
		| ((arr[3] << 24) & 0xFF000000);
}

void set_cruxcan_head_tray(NETWORK_COMMAND enc, unsigned char* bytes)
{
	bytes[0] = 0xFD;	// 65533
	bytes[1] = 0xFF;	// 65535
	bytes[2] = 1;		// 1
	bytes[3] = 0;		// 0
	bytes[4] = 0xFB;	// 65531
	bytes[5] = 0xFF;	// 65535
	bytes[6] = (unsigned char)enc;
	bytes[7] = 0;
};

void set_cruxcan_head(unsigned char* data)
{
	set_cruxcan_head_tray(nc_none, data);
};

bool is_cruxcan_head(unsigned char* data)
{
	return (data[0] == 0xFD &&	// 65533
		data[1] == 0xFF &&	// 65535
		data[3] == 0x00 &&
		data[4] == 0xFB &&	// 65531
		data[5] == 0xFF		// 65535
		);
}

void CBAPI OnReceiveFromTray(char ipaddr[], TCP_TYPE type, long size, unsigned char* data)
{
	UNREFERENCED_PARAMETER(ipaddr);
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(data);
}

void CBAPI OnReceiveFromSDK(char ipaddr[], TCP_TYPE type, long size, unsigned char* data)
{
	UNREFERENCED_PARAMETER(ipaddr);
	UNREFERENCED_PARAMETER(type);
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(data);
}


//-------------------------------------------------------------------

int Do_ReceiveDataRawUDPFromCenter(DWORD size, unsigned char* data, void* Socket);

int c_pop_udp_buffer(unsigned char* cur, DWORD received_data_size, void* Socket)
{
	KLOG3A("received_data_size = %d", received_data_size);

	while (received_data_size > 7)
	{
		// 수신된 UDP 패킷의 SOF 확인
		int pos = Find_SOF(cur, received_data_size);
		//KLOG3A("pos = %d", pos);
		//KLOG3A("received_data_size = %d", received_data_size);
		//KLOG3A("received_data_size - pos = %d", (received_data_size - pos));

		if (pos != -1)
		{
			if ((received_data_size - pos) < UDP_BLOCK_SIZE)
			{
				memmove(c_udp_temp_buffer, &cur[pos], received_data_size - pos);
				c_udp_temp_buffer_data_count = received_data_size - pos;
				received_data_size = 0;
			}
			else
			{
				memmove(&temp_buffer, &cur[pos], sizeof(stUDP));

				//-------------------------------------------------------------
				//! kiwa72(2023.05.01 11h) - UDP 패킷 분석 및 장비 리스트에 등록. 같은 장비는 재등록 안함.
				int nRes = Do_ReceiveDataRawUDPFromCenter(UDP_BLOCK_SIZE, temp_buffer, Socket);
				KLOG3A(" ... nRes = %d", nRes);
				if (nRes == 0)
				{
					//CRX_N2N_DEVICE* pNtoN_DeviceInfo = &((CRX_N2N_PROTOCOL*)cur)->devices[0];
					CRX_N2N_DEVICE* pNtoN_DeviceInfo = new CRX_N2N_DEVICE;
					if (pNtoN_DeviceInfo == NULL)
					{
						return (-1);
					}

					memcpy(pNtoN_DeviceInfo, &((CRX_N2N_PROTOCOL*)cur)->devices[0], sizeof(CRX_N2N_DEVICE));

					KLOG3A("pNtoN_DeviceInfo->Name = %s",
						pNtoN_DeviceInfo->Name);
					KLOG3A("pNtoN_DeviceInfo->device_ipaddr = %d.%d.%d.%d",
						pNtoN_DeviceInfo->device_ipaddr[0],
						pNtoN_DeviceInfo->device_ipaddr[1],
						pNtoN_DeviceInfo->device_ipaddr[2],
						pNtoN_DeviceInfo->device_ipaddr[3]);
					KLOG3A("pNtoN_DeviceInfo->host_ipaddr = %d.%d.%d.%d",
						pNtoN_DeviceInfo->host_ipaddr[0],
						pNtoN_DeviceInfo->host_ipaddr[1],
						pNtoN_DeviceInfo->host_ipaddr[2],
						pNtoN_DeviceInfo->host_ipaddr[3]);

					//! kiwa72(2023.05.01 11h) - 접속한 장비(클라이언트) 저장
					GetPdxData()->g_ptrClientSocketList.AddTail(pNtoN_DeviceInfo);
				}
				//! kiwa72(2023.08.30 11h) - 동일한 장비에 대한 상태 정보 갱신을 위해 추가
				else if (nRes == 1)
				{
					CRX_N2N_DEVICE NtoN_DeviceInfo;
					memcpy(&NtoN_DeviceInfo, &((CRX_N2N_PROTOCOL*)cur)->devices[0], sizeof(CRX_N2N_DEVICE));

					POSITION pos2 = GetPdxData()->g_ptrClientSocketList.GetHeadPosition();
					while (pos2 != NULL)
					{
						CRX_N2N_DEVICE* pNtoN_DeviceInfo2 = (CRX_N2N_DEVICE*)GetPdxData()->g_ptrClientSocketList.GetNext(pos2);
						if (pNtoN_DeviceInfo2 != NULL)
						{
							//! - IP주소가 동일하면 정보(이름, 상태)를 갱신한다.
							if (memcmp(pNtoN_DeviceInfo2->device_ipaddr, &NtoN_DeviceInfo.device_ipaddr, sizeof(NtoN_DeviceInfo.device_ipaddr)) == 0)
							{
								//KLOG3A("pNtoN_DeviceInfo2: %s | %u.%u.%u.%u | %d",
								//	pNtoN_DeviceInfo2->Name,
								//	pNtoN_DeviceInfo2->device_ipaddr[0], pNtoN_DeviceInfo2->device_ipaddr[1], pNtoN_DeviceInfo2->device_ipaddr[2], pNtoN_DeviceInfo2->device_ipaddr[3],
								//	pNtoN_DeviceInfo2->state);
								//KLOG3A("NtoN_DeviceInfo: %s | %u.%u.%u.%u | %d",
								//	NtoN_DeviceInfo.Name,
								//	NtoN_DeviceInfo.device_ipaddr[0], NtoN_DeviceInfo.device_ipaddr[1], NtoN_DeviceInfo.device_ipaddr[2], NtoN_DeviceInfo.device_ipaddr[3],
								//	NtoN_DeviceInfo.state);

								memcpy(pNtoN_DeviceInfo2, &NtoN_DeviceInfo, sizeof(CRX_N2N_DEVICE));

								KLOG3A("Change pNtoN_DeviceInfo2: %s | %u.%u.%u.%u | %d",
									pNtoN_DeviceInfo2->Name,
									pNtoN_DeviceInfo2->device_ipaddr[0], pNtoN_DeviceInfo2->device_ipaddr[1], pNtoN_DeviceInfo2->device_ipaddr[2], pNtoN_DeviceInfo2->device_ipaddr[3],
									pNtoN_DeviceInfo2->state);
								break;
							}
						}
					}
				}
				//!...
				//-------------------------------------------------------------

				cur += (UDP_BLOCK_SIZE + pos);
				received_data_size -= (UDP_BLOCK_SIZE + pos);	// 혹시 뒤에 또 있을수도 있으니 다시 while문 으로 
				c_udp_temp_buffer_data_count = 0;	// 일단 조립한건 무조건 다 썼으니 비워도 됨
			}
		}
		else
		{
			received_data_size = 7;		// while 탈출
			cur += (received_data_size - 7);
			c_udp_temp_buffer_data_count = 0;	// 찌꺼기 버림 

			KLOG3A("received_data_size = 7;");
		}
	}

	if (received_data_size > 0)
	{
		KLOG3A("received_data_size = %d", received_data_size);

		memmove(&c_udp_temp_buffer[c_udp_temp_buffer_data_count], cur, received_data_size);
		c_udp_temp_buffer_data_count += received_data_size;	// temp_buffer_count 증가
	}

	//KLOG3A("... c_udp_temp_buffer_data_count = %d", c_udp_temp_buffer_data_count);

	return c_udp_temp_buffer_data_count;
}

int Do_ReceiveDataRawUDPFromCenter(DWORD size, unsigned char* data, void* pSocket)
{
	UNREFERENCED_PARAMETER(size);
	UNREFERENCED_PARAMETER(data);
	UNREFERENCED_PARAMETER(pSocket);

	KLOG3W();

	CRX_N2N_PROTOCOL* received = (CRX_N2N_PROTOCOL*)data;

	//KLOG3A("received SOF = [%04X][%04X][%04X]", received->SignatureF[0], received->SignatureF[1], received->SignatureF[2]);
	//KLOG3A("received SOE = [%04X][%04X][%04X][%04X]", received->SignatureE[0], received->SignatureE[1], received->SignatureE[2], received->SignatureE[3]);

	//! kiwa72(2023.05.01 11h) - 장비 이름 출력 테스트
	CRX_N2N_DEVICE* pNtoN_DeviceInfo = &received->devices[0];
	//KLOG3A("pNtoN_DeviceInfo->Name = %s", pNtoN_DeviceInfo->Name);

	if (is_valid_crx_n2n_protocol(received) < 0)
	{
		//KLOG3(_T("it is not crx_n2n_protocol from Device [%s]"), (LPCTSTR)((CUDPReceiveSocket *)Socket)->strSendersIp);
		return (-1);	// 오류(무의미 패킷)
	}

	//---------------------------------------------------------------
	//! kiwa72(2023.05.01 11h) - 동일 장비 접속 유/무 확인
	POSITION pos = GetPdxData()->g_ptrClientSocketList.GetHeadPosition();
	CRX_N2N_DEVICE* pNtoN_DeviceInfo2 = NULL;
	int nSelect_Cnt = 0;
	while (pos != NULL)
	{
		pNtoN_DeviceInfo2 = (CRX_N2N_DEVICE*)GetPdxData()->g_ptrClientSocketList.GetNext(pos);
		if (pNtoN_DeviceInfo2 != NULL)
		{
			//! kiwa72(2023.08.30 11h) - 동일 장치를 IP 어드레스로 확인
			if (memcmp(pNtoN_DeviceInfo2->device_ipaddr, pNtoN_DeviceInfo->device_ipaddr, sizeof(pNtoN_DeviceInfo->device_ipaddr)) == 0)
			{
				//KLOG3A("Same Name = %s, %s", pNtoN_DeviceInfo2->Name, pNtoN_DeviceInfo->Name);
				return (1);	// 같은 장비가 접속
			}

			nSelect_Cnt++;
		}
	}

	KLOG3A("Other Name = %s, %s", pNtoN_DeviceInfo2->Name, pNtoN_DeviceInfo->Name);

	return (0);	// 새로운 장비가 접속
}

//! 장비로부터 UDP 데이터(장비이름, IP주소) 수신
void CBAPI OnReceiveDataRawUDPFromCenter(DWORD size, unsigned char* data, void* Socket)
{
	KLOG3A("size = %d", size);
	unsigned char* cur = NULL;

	if (c_udp_temp_buffer_data_count == 0)
	{
		if (size < 8)
		{
			memcpy(c_udp_temp_buffer, data, size);
			c_udp_temp_buffer_data_count += size;
			KLOG3A("size is to small than UDP_PACKET_SIZE[%03d]", size);
			return;
		}

		static DWORD oldtick = GetTickCount();
		static DWORD curtick = 0;
		curtick = GetTickCount();
		oldtick = curtick;
		c_pop_udp_buffer(data, size, Socket);
	}
	else
	{
		if ((c_udp_temp_buffer_data_count + size) < UDP_BLOCK_SIZE)
		{
			KLOG3A("[UDP]minimum udp3!!!!");
			memmove(&c_udp_temp_buffer[c_udp_temp_buffer_data_count], recv, size);
			c_udp_temp_buffer_data_count += size;
			return;
		}
		else
		{
			int remained_size = size + c_udp_temp_buffer_data_count;
			KLOG3A("remained_size = %d", remained_size);

			if (remained_size > SIZE_OF_UDP_BUFFER)
			{
				KLOG3A("[UDP]temp buffer while.... !!!");

				int next_pos = SIZE_OF_UDP_BUFFER - c_udp_temp_buffer_data_count;
				memmove(&c_udp_temp_buffer[c_udp_temp_buffer_data_count], recv, next_pos);
				cur = c_udp_temp_buffer;
				c_pop_udp_buffer(cur, SIZE_OF_UDP_BUFFER, Socket); // SIZE_OF_UDP_BUFFER안의 데이터를 판단하고 텅 비움 마지막은 

				int temp_buffer_free_size = 0;
				unsigned char* src = data;

				while (remained_size > 0)
				{
					temp_buffer_free_size = SIZE_OF_UDP_BUFFER - c_udp_temp_buffer_data_count;
					if (temp_buffer_free_size < remained_size)
					{
						memmove(&c_udp_temp_buffer[c_udp_temp_buffer_data_count], src, temp_buffer_free_size);
						KLOG3A("[UDP]temp buffer pop_udp_buffer 1!!!!");
						c_pop_udp_buffer(c_udp_temp_buffer, SIZE_OF_UDP_BUFFER, Socket); // SIZE_OF_UDP_BUFFER안의 데이터를 판단하고 텅 비움 마지막은 
						remained_size -= temp_buffer_free_size;
						src += temp_buffer_free_size;
					}
					else
					{
						KLOG3A("[UDP]temp buffer pop_udp_buffer 2!!!!");

						memmove(&c_udp_temp_buffer[c_udp_temp_buffer_data_count], src, remained_size);
						c_pop_udp_buffer(src, remained_size, Socket);
						remained_size = 0;
					}
				}
			}
			else
			{
				memmove(&c_udp_temp_buffer[c_udp_temp_buffer_data_count], recv, size);
				cur = c_udp_temp_buffer;

				KLOG3A("[UDP] temp buffer pop_udp_buffer 0!!!!");

				c_pop_udp_buffer(cur, remained_size, Socket);
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


int get_image_size(IP_SIZE ip, IMAGE_RESOLUTION res)
{
	int size = 0;
	switch (ip)
	{
	case IP_SIZE::eis_IP0:	size = size0_w * size0_h * sizeof(unsigned short);	break;
	case IP_SIZE::eis_IP1:	size = size1_w * size1_h * sizeof(unsigned short);	break;
	case IP_SIZE::eis_IP2:	size = size2_w * size2_h * sizeof(unsigned short);	break;
	default:
	case IP_SIZE::eis_IP3:	size = size3_w * size3_h * sizeof(unsigned short);	break;
	}

	size /= (res == IMAGE_RESOLUTION::eir_StRes) ? 4 : 1;
	return size;
}

char* get_f_open_errorstr(int index)
{
	const int nResItems = 20;
	static char* fresultstr[nResItems] =
	{
		"FR_OK",					/* (0) Succeeded */
		"FR_DISK_ERR",				/* (1) A hard error occurred in the low level disk I/O layer */
		"FR_INT_ERR",				/* (2) Assertion failed */
		"FR_NOT_READY",				/* (3) The physical drive cannot work */
		"FR_NO_FILE",				/* (4) Could not find the file */
		"FR_NO_PATH",				/* (5) Could not find the path */
		"FR_INVALID_NAME",			/* (6) The path name format is invalid */
		"FR_DENIED",				/* (7) Access denied due to prohibited access or directory full */
		"FR_EXIST",					/* (8) Access denied due to prohibited access */
		"FR_INVALID_OBJECT",		/* (9) The file/directory object is invalid */
		"FR_WRITE_PROTECTED",		/* (10) The physical drive is write protected */
		"FR_INVALID_DRIVE",			/* (11) The logical drive number is invalid */
		"FR_NOT_ENABLED",			/* (12) The volume has no work area */
		"FR_NO_FILESYSTEM",			/* (13) There is no valid FAT volume */
		"FR_MKFS_ABORTED",			/* (14) The f_mkfs() aborted due to any parameter error */
		"FR_TIMEOUT",				/* (15) Could not get a grant to access the volume within defined period */
		"FR_LOCKED",				/* (16) The operation is rejected according to the file sharing policy */
		"FR_NOT_ENOUGH_CORE",		/* (17) LFN working buffer could not be allocated */
		"FR_TOO_MANY_OPEN_FILES",	/* (18) Number of open files > _FS_SHARE */
		"FR_INVALID_PARAMETER"		/* (19) Given parameter is invalid */
	};

	if (index < 0 || index >= nResItems)
	{
		ASSERT(0);
		return "unknown";
	}
	else
	{
		return fresultstr[index];
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


BOOL InitTCPsc()
{
	qinit(&tcpqueuesdk, tcp_q_data_sdk, MY_QUEUE_SIZE + 1);	// use_tcp_queue
	qinit(&tcpqueuedevice, tcp_q_data_device, MY_QUEUE_SIZE + 1);

	// use server socket
	if (GetPdxData()->g_TCP_Server->Ready_to_TCPSockets() == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

BOOL InitUDPsc(void)
{
	KLOG3A("initUDPsc Port = %d", PREDIX_UDP_PORT);

	// tray는 udp 수신할 일 없음 현재 그래서 더미 포트로 함 
	if (GetPdxData()->g_pUDPsc->Ready_to_UDPSockets(OnReceiveDataRawUDPFromCenter) == FALSE)
	{
		return FALSE;
	}

	return TRUE;
}

// UDP 브로드캐스팅 시작
void startLeader(void)
{
	GetPdxData()->g_pUDPsc->Start_Tray_Leader();
}

// UDP 브로드캐스팅 종료
void endLeader(void)
{
	auto pGlobalData = GetPdxData();
	pGlobalData->g_pUDPsc->End_Tray_Leader();
}

void Disconnect_by_sleep(void)
{
	KLOG3A("call g_TCP_Server.disconnect_by_sleep");
	GetPdxData()->g_TCP_Server->disconnect_by_sleep();
}

void Resume_from_sleep(void)
{
	KLOG3A("call g_TCP_Server.Resume_from_sleep");
	GetPdxData()->g_TCP_Server->resume_from_sleep();
}

bool reset_scanner(void)
{
	if (get_scanner_state_s() != ss_ready)
	{
		return false;
	}

	unsigned long a = 1;
	unsigned char temp[4];
	num2arr(a, temp);
	return send_protocol_to_device(nc_system_reset, temp, 4);
}

bool delete_all_image(void)
{
	if (get_scanner_state_s() != ss_ready)
	{
		return false;
	}

	unsigned long a = 1;
	unsigned char temp[4];
	num2arr(a, temp);
	return send_protocol_to_device(nc_delete_all_image, temp, 4);
}

bool request_image_tray(int image_num)
{
	if (get_scanner_state_s() != ss_ready)
	{
		return false;
	}

	KLOG3A("image_num = %d", image_num);

	unsigned char temp[4];
	num2arr(image_num, temp);
	return send_protocol_to_device(nc_request_image, temp, 4);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void CBAPI ReceiveDataFromLocalSDK(DWORD size, unsigned char* data, void* Socket)
{
	KLOG3A();

	CTCPClientSock* pThis = (CTCPClientSock*)Socket;
	if (pThis->nReserved == 0)
	{//예약크기없음
		int len = size;
		unsigned char* cur = data;
		for (int i = 0; i < (len / TCP_COMMAND_SIZE); i++)
		{
			Push_TCP_Queue_SDK(&tcpqueuesdk, cur, TCP_COMMAND_SIZE, Socket);
			len -= TCP_COMMAND_SIZE;
			cur += TCP_COMMAND_SIZE;
		}
		if (len > 0)
		{
			Push_TCP_Queue_SDK(&tcpqueuesdk, cur, len, Socket);
		}
	}
	else
	{
		KLOG3A("sdk doesn't send data except command");
	}
}

void CBAPI OnCloseFromLocalSDK(int nErrorCode)
{
	is_local_connected = false;
	KLOG3A("nErrorCode = %d", nErrorCode);

	send_protocol_to_device(nc_sdk_disconnected, null, 0);

	pc_name = L"";
	//g_pLocalSocket = NULL;
	KLOG3A("sdk is disconnected -> call self close");

	//self_close();
	if (GetPdxData()->self_close)
	{
		GetPdxData()->self_close();
	}
}

void CBAPI ReceiveDataFromRemoteSDK(DWORD size, unsigned char* data, void* Socket)
{
	ReceiveDataFromLocalSDK(size, data, Socket);
}

void CBAPI OnConnectFromRemoteSDK(int nErrorCode)
{
	UNREFERENCED_PARAMETER(nErrorCode);
	KLOG3A("connected from remote sdk");
}

void CBAPI OnCloseFromRemoteSDK(int nErrorCode)
{
	KLOG3A("nErrorCode = %d", nErrorCode);
}

//! kiwa72(2023.03.30 01h) - 장비로 부터 데이터 수신 콜백 함수
void CBAPI ReceiveDataFromDevice(DWORD size, unsigned char* data, void* Socket)
{
	//KLOG3W();

	CTCPClientSock* pThis = (CTCPClientSock*)Socket;

	static TCHAR recvtemp[256];
	static BOOL need_progress_updating = false;

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	if (pThis->nReserved == 0)
	{
		// 예약크기없음
		//

		int len = size;
		unsigned char* cur = data;
		for (int i = 0; i < (len / TCP_COMMAND_SIZE); i++)
		{
			Push_TCP_Queue_Device(&tcpqueuedevice, cur, TCP_COMMAND_SIZE, Socket, pThis->nReserved);

			len -= TCP_COMMAND_SIZE;
			cur += TCP_COMMAND_SIZE;
		}

		if (len > 0)
		{
			Push_TCP_Queue_Device(&tcpqueuedevice, cur, len, Socket, pThis->nReserved);
		}
	}
	else
	{
		// 예약크기 있음
		//

		memcpy(device_current, data, size);

		device_current += size;
		pThis->nReceived += size;

		device_curprogress = (pThis->nReceived * 100) / pThis->nReserved;

		if (!is_local_connected)
		{
			pGlobalData->set_progressbar_position(device_curprogress);
		}

		if (device_curprogress != device_oldprogress && (device_curprogress % 5 == 0))
		{
			device_count++;

			GetLocalTime(&device_st);
			device_cursec = device_st.wSecond;
			device_differ = device_cursec < device_oldsec ? device_cursec + 60 - device_oldsec : device_cursec - device_oldsec;
			device_meansec += device_differ;
			_stprintf_s(recvtemp, _T("recv[%03d][%02d]sec_mean[%02d]sec\n"), device_curprogress, device_differ, device_meansec / device_count);
			device_oldsec = device_cursec;

			if (device_curprogress - device_oldprogress > 10 || device_curprogress == 100)
			{
				device_oldprogress = device_curprogress;
				need_progress_updating = true;
			}
			else
			{
				need_progress_updating = false;
			}

			switch (device_curprogress)
			{
			case 5:
			case 50:
			case 100:
				break;
			default:
				break;
			}

			unsigned char temp[4];
			num2arr(device_curprogress, temp);
			if (is_local_connected && (pThis->nReserved > (1024 * 1024)) && need_progress_updating)
			{
				send_protocol_to_local(nc_tray_progress, temp, 4);
			}
		}

		if (pThis->nReceived >= pThis->nReserved)
		{
			g_is_receiving_data = false;
			device_oldprogress = 0;//clear 

			KLOG3A("received blob data nReserved[%d] received[%d]\n", pThis->nReserved, pThis->nReceived);

			int remained_size = pThis->nReceived - pThis->nReserved;
			unsigned char* remained_data = device_current - remained_size;

			set_scanner_state_s(ss_ready);

			static SYSTEMTIME st;
			GetLocalTime(&st);
			wsprintfW(loaded_image_name, L"%04d%02d%02d_%02d%02d%02d recv.raw",	// to use name of received image 
				st.wYear,
				st.wMonth,
				st.wDay,
				st.wHour,
				st.wMinute,
				st.wSecond
			);

			is_received_image = true;

			if (is_local_connected)
			{
				unsigned char temp4[4];
				num2arr(100, temp4);
				send_protocol_to_local(nc_tray_progress, temp4, 4);
				Sleep(50);

				NETWORK_COMMAND received_nc = pThis->nc;
				KLOG3A(">>>received all<<<[%03d]sec will forward to local[%03d]", device_meansec, received_nc);

				if (pThis->nc == nc_scan_image_size)
				{
					pGlobalData->end_tick = GetTickCount();

					KLOG3A("---------------------------------");
					KLOG3A(" predix transfer time %03d msec  ", (pGlobalData->end_tick - pGlobalData->start_tick) / 1000);
					KLOG3A("---------------------------------");

					unsigned long size2 = pThis->nReserved;
					pThis->nReserved = 0;//초기화
					pThis->nc = NETWORK_COMMAND::nc_none;
					pThis->nReceived = 0;//received all
					pThis->nChecked = 0;

					is_tray_pp_ing = true;
					preprocess_image_and_save(size2, data, Socket);//before forward to sdk
					pThis->nReserved = 0;//초기화
					pThis->nc = NETWORK_COMMAND::nc_none;
					pThis->nReceived = 0;//received all

					//int to_send = size2_w * size2_h * sizeof(unsigned short int);
					int to_send = pGlobalData->last_image_width * pGlobalData->last_image_height * sizeof(unsigned short int);
					pThis->nChecked = 0;

					KLOG3A("w:%d, h:%d | byte:%d [%s]", pGlobalData->last_image_width, pGlobalData->last_image_height, to_send, "R");
					forward_big_data_to_local((unsigned char*)pGlobalData->last_image_data, to_send, received_nc);	// image or imagelist

					is_tray_pp_ing = false;
				}
				else
				{
					KLOG3A("send big data except image[%d]/[%d][%d]", pThis->nReceived, pThis->nReserved, received_nc);

					pThis->nReserved = 0;	// 초기화
					pThis->nc = NETWORK_COMMAND::nc_none;

					int recv = pThis->nReceived;
					pThis->nReceived = 0;//received all
					forward_big_data_to_local(pThis->buffer_l, recv, received_nc);	// image or imagelist
					pThis->nChecked = 0;
				}

				if (remained_size > 0)
				{
					int l = remained_size;
					unsigned char* cur = remained_data;
					for (int i = 0; i < l / TCP_COMMAND_SIZE; i++)
					{
						KLOG3A("Push_TCP_Queue_Device  512 byte after receiving data");
						Push_TCP_Queue_Device(&tcpqueuedevice, cur, TCP_COMMAND_SIZE, Socket, pThis->nReserved);
						l -= TCP_COMMAND_SIZE;
						cur += TCP_COMMAND_SIZE;
					}

					if (l > 0)
					{
						Push_TCP_Queue_Device(&tcpqueuedevice, cur, l, Socket, pThis->nReserved);
					}
				}
			}
			else
			{
				// stand alone
				//
				pGlobalData->set_progressbar_position(100);

				KLOG3A("received all[%03d]sec will do something", device_meansec);

				if (pThis->nc == nc_scan_image_size)
				{
					pGlobalData->end_tick = GetTickCount();
					KLOG3A("---------------------------------");
					KLOG3A(" predix transfer time2 %03d msec ", (pGlobalData->end_tick - pGlobalData->start_tick));
					KLOG3A("---------------------------------");
				}

				unsigned long size2 = pThis->nReserved;
				NETWORK_COMMAND nc = pThis->nc;
				pThis->nReserved = 0;//초기화
				pThis->nc = NETWORK_COMMAND::nc_none;
				pThis->nReceived = 0;//received all
				pThis->nChecked = 0;

				pGlobalData->do_something(pThis->buffer_l, size2, nc, Socket);

				if (remained_size > 0)
				{
					int l = remained_size;
					unsigned char* cur = remained_data;

					for (int i = 0; i < l / TCP_COMMAND_SIZE; i++)
					{
						KLOG3A("Push_TCP_Queue_Device 512 byte after receiving data");

						Push_TCP_Queue_Device(&tcpqueuedevice, cur, TCP_COMMAND_SIZE, Socket, pThis->nReserved);
						l -= TCP_COMMAND_SIZE;
						cur += TCP_COMMAND_SIZE;
					}

					if (l > 0)
					{
						KLOG3A("Push_TCP_Queue_Device something smaller than 512 [%d]", l);
						Push_TCP_Queue_Device(&tcpqueuedevice, cur, l, Socket, pThis->nReserved);
					}
				}
			}
		}
	}
	//wait.Restore();
}

void CBAPI OnConnectFromDevice(int nErrorCode)
{
	UNREFERENCED_PARAMETER(nErrorCode);

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	pGlobalData->current_setting = none_setting;
	KLOG3A("connected from device\n");
	pGlobalData->g_plogFile->SetValue(_T("current"), _T("step"), _T("[2/3] connected  and waiting  first data"));

	pGlobalData->notify_status_changed(1);
}

void CBAPI OnCloseFromDevice(int nErrorCode)
{
	UNREFERENCED_PARAMETER(nErrorCode);

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	if ( pGlobalData->g_pDeviceSocket != nullptr )
	{
		KLOG3W(_T("[NETWORK EVENT] disconnected from/to device [%s] [NETWORK EVENT]"), (LPCTSTR)pGlobalData->g_pDeviceSocket->PeerIP);

		// eiohlei
		//if ((pGlobalData->g_pDeviceSocket->nReserved - pGlobalData->g_pDeviceSocket->nReceived) > 0)
		//{
		//	pGlobalData->g_pTtray_Dlg->PostMessage(WM_FROM_TCP, 3, device_curprogress);
		//}
		//else
		//{
		//	pGlobalData->g_pTtray_Dlg->PostMessage(WM_FROM_TCP, 1, device_curprogress);
		//}
		// 원본 코드의 Ccruxcan_trayDlg::OnFromTCP 를 참고해서 필요한 동작을 취하도록 변경.
		// TODO: Makeit - 혹시나.. thread 작업중일 경우에는 작업을 취소하고 progress dialog를 닫도록 해야 함.
		{
			set_scanner_state_s(ss_disconnected);
			pGlobalData->notify_status_changed(0);
		}

		device_curprogress = 0;

		unsigned char temp[4];
		num2arr(device_curprogress, temp);

		if (is_local_connected)
		{
			send_protocol_to_local(nc_tray_progress, temp, 4);
		}

		// Memo: Makeit - CRUXELL측에서 SDK 형태로 사용할 것을 고려하지 않고 개발한 관계로...
		// 비정상 종료되는 상황이 자주 발생하고 있다. 그래서 이렇게 검사하지 않으면 전체 리팩토링을 해야 하는 관계로
		// 일단 이렇게 틀어 막아둔다.
		if ( pGlobalData->g_pDeviceSocket )
		{
			pGlobalData->g_pDeviceSocket->nReserved = 0;
			pGlobalData->g_pDeviceSocket->nReceived = 0;
			pGlobalData->g_pDeviceSocket->nChecked = 0;
			pGlobalData->g_pDeviceSocket->nc = NETWORK_COMMAND::nc_none;
			pGlobalData->g_pDeviceSocket->m_isClientSocketEnd = true;

			Sleep(500);		// to close thread function

			SAFE_DELETE(pGlobalData->g_pDeviceSocket);

			// 정상적인 상황이라면 발생해서는 안되는 경우이지만... 어쩔 수 없음. 일단 안죽도록 막자.
			if ( pGlobalData->g_pDeviceSocket )
				set_scanner_state_s(ss_warmup);	// ss_disconnected -> ss_warmup으로 변경
			else
				// 연결이 해제된 상황인데 waiting으로 계속 기다리면 안된다. (warmup으로 지정하면 waiting상태가 됨)
				set_scanner_state_s(ss_disconnected);
		}
		else
		{
			Sleep(500);		// to close thread function

			set_scanner_state_s(ss_disconnected);
		}
	}
	else
	{
		set_scanner_state_s(ss_disconnected);
	}
	pGlobalData->notify_status_changed(0);

	pGlobalData->current_setting = default_setting;	// clear

	is_first_nc_hardware_info = false;	// clear
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


void CBAPI OnConnectFromLocalSDK(int nErrorCode)
{
	UNREFERENCED_PARAMETER(nErrorCode);

	KLOG3A("connected from localHost");
	is_local_connected = true;
}

/**
 * @brief	Send FreeData To Device
 * @details	임의의 대용량 데이터 전송 | SW ->> FW
 * @param	NETWORK_COMMAND nc	[IN]	NETWORK_COMMAND
 * @param	unsigned char* data	[IN]	전송 데이터 버퍼
 * @param	int size			[IN]	전송 크기
 * @retval	성공(=0), 실패(<0)
 * @author	kiwa72
 * @date	kiwa72(2023.04.21 09h)
 * @version	0.0.1
 */
int Send_FreeData_To_Device(NETWORK_COMMAND nc, unsigned char* data, int size)
{
	if (GetPdxData()->g_pDeviceSocket == NULL)
	{
		KLOG3A("Device Socket is Null! (-1)");
		return (-1);
	}

	//! 512 Byte 단위로 크기 변경
	const UINT size512 = ((size + (TCP_PACKET_MIN_SIZE - 1)) / TCP_PACKET_MIN_SIZE) * TCP_PACKET_MIN_SIZE;

	unsigned char temp[128] = { 0, };

	//! Header - 데이터 크기 저장
	num2arr((unsigned long)size512, temp);

	unsigned int nCRC32 = 0;

	unsigned char* pBuf_Data = new unsigned char[size512];
	if (pBuf_Data != NULL)
	{
		::memset((unsigned char*)pBuf_Data, 0x00, size512);
		::memcpy((unsigned char*)pBuf_Data, (unsigned char*)data, size);

		//! CRC32 구하기
		nCRC32 = crcCompute32((unsigned int*)pBuf_Data, size512 / sizeof(unsigned int));
	}
	else
	{
		KLOG3A("Heap memory creation failed! (-2)");
		return (-2);
	}

	//! Header - CRC32 저장
	::memcpy(&temp[4], (unsigned char*)&nCRC32, sizeof(nCRC32));

	//---------------------------------------------------------------------
	// 헤더 데이터 전송
	bool bResult = send_protocol_to_device(nc, temp, sizeof(temp));
	//---------------------------------------------------------------------

	if (bResult == FALSE)
	{
		KLOG3A("failed to send nc first! (-3)");
		return (-3);
	}

	unsigned char* pCur = pBuf_Data;
	unsigned int sent_total = 0;
	unsigned int sent_size = 0;

	while (sent_total < size512)
	{
		//---------------------------------------------------------------------
		// 바디 데이터 전송
		sent_size = GetPdxData()->g_pDeviceSocket->Send(pCur + sent_total, size512 - sent_total);
		//---------------------------------------------------------------------

		if (sent_size == SOCKET_ERROR)
		{
			DWORD dwerror = GetLastError();
			if (dwerror == WSAEWOULDBLOCK)
			{
				MYLOGOLDW(L"oops WSAEWOULDBLOCK is occured! (-4)\n");
				Sleep(300);
				//전송할 버퍼가 없다!!! 쉬엄쉬엄 보내라
				continue;
			}
		}

		sent_total += sent_size;
		Sleep(10);
	}
	SAFE_DELETE_ARRAY(pBuf_Data);

	return (0);
}

bool send_binary_to_device(NETWORK_COMMAND nc, unsigned char* data, int size, char* filename)
{
	if (GetPdxData()->g_pDeviceSocket == NULL)
	{
		KLOG3A("devicesocket is null");
		return false;
	}

	int size512 = ((size + (TCP_PACKET_MIN_SIZE - 1)) / TCP_PACKET_MIN_SIZE) * TCP_PACKET_MIN_SIZE;
	int sent_total_size = 0, sent_size = 0;

	unsigned char temp[128] = { 0, };
	num2arr((unsigned long)size512, temp);

	MYLOGOLDA(filename);

	// kiwa72(2022.11.25 03h) - cheksum -> chksum1 수정
	unsigned int chksum1 = 0;

	// kiwa72(2022.11.25 03h) - CRC-32bit/MPEG-2 적용
	unsigned int Word512Cnt = (size / TCP_PACKET_MIN_SIZE) + (size % TCP_PACKET_MIN_SIZE != 0 ? 1 : 0);
	unsigned char* pWordBuf512 = new unsigned char[Word512Cnt * TCP_PACKET_MIN_SIZE];
	if (pWordBuf512 != NULL)
	{
		memset((unsigned char*)pWordBuf512, 0x00, Word512Cnt * TCP_PACKET_MIN_SIZE);
		memcpy((unsigned char*)pWordBuf512, (unsigned char*)data, size);

		// iwa72(2023.04.06 09h) - CRC-32bit/MPEG-2 검사
		chksum1 = crcCompute32((unsigned int*)pWordBuf512, (Word512Cnt * TCP_PACKET_MIN_SIZE) / sizeof(unsigned int));

		SAFE_DELETE_ARRAY(pWordBuf512);
	}
	else
	{
		KLOG3A("Heap memory creation failed! (1)");
		return false;
	}

	KLOG3A("CRC-32bit/MPEG-2 ------------> send checksum1[%08X]", chksum1);

	sent_binary_checksum1 = chksum1;
	sent_binary_checksum2 = 0;

	if (nc == nc_binary_update)
	{
		strcpy((char*)&temp[4], filename);
	}
	else
	{
		memcpy(&temp[4], (unsigned char*)&sent_binary_checksum1, sizeof(sent_binary_checksum1));
	}

	//------------------------------------------------------------------
	// kiwa72(2022.11.26 23h) - 헤더 데이터 전송(128Byte)
	bool bResult = send_protocol_to_device(nc, temp, sizeof(temp));
	//------------------------------------------------------------------

	if (bResult == true)
	{
		KLOG3A("send nc first");
		Sleep(300);

		bResult = true;

		KLOG3A("send 512 multiplied size data[%d]", size512);

		unsigned char* data2 = new unsigned char[size512];
		if (data2 == NULL)
		{
			KLOG3A("Heap memory creation failed! (2)");
			return false;
		}
		memset(data2, 0x00, size512);
		memmove(data2, data, size);

		sent_total_size = 0;

		int count = size512 / TCP_PACKET_MIN_SIZE;
		//const int  need_chunk_size = (8 * 1024 * 1024) + TCP_PACKET_MIN_SIZE;//predix cr FIFO buffer size is 8MB

		const int  need_chunk_size = (8 * 1024 * 1024);
		//const int  need_chunk_size = (1 * 1024);

		int pause_size = SDCARD_TRANSFER_SIZE * 100;

		if (size512 < pause_size)
		{
			pause_size = size512 / 2;
		}

		unsigned char* pCur = data2;
		unsigned int next_size = 0;
		unsigned int sent_chunk_size = 0;

		is_saved_chunk = false;
		//uint32_t sent_checksum = 0;

		// kiwa72(2022.11.26 05h) - 변경: Sleep(1000) -> Sleep(100)
		Sleep(100);

		// kiwa72(2022.11.26 05h) - 한 번에 전송할 패킷 크기 16,384byte
		const int sent_data_size = (512 * 2 * 4 * 4);

		// kiwa72(2022.11.26 05h) - 펌웨어 바이너리 데이터 전송
		while ((sent_total_size < size512) && (count > 0))
		{
			if ((size512 - sent_total_size) < sent_data_size)
			{
				next_size = size512 - sent_total_size;
			}
			else
			{
				next_size = sent_data_size;
			}

			KLOG3A("size512 = %d, sent_total_size = %d, sent_size = %d", size512, sent_total_size, sent_size);

			//------------------------------------------------------------------
			// kiwa72(2022.11.26 05h) - 데이터 전송 API
			sent_size = GetPdxData()->g_pDeviceSocket->Send(pCur, next_size);
			//------------------------------------------------------------------

			for (int ss = 0; ss < sent_size; ss++)
			{
				//sent_checksum += pCur[ss];
				sent_binary_checksum2 += pCur[ss];
			}

			if (sent_size == SOCKET_ERROR)
			{
				DWORD dwerror = GetLastError();
				if (dwerror == WSAEWOULDBLOCK)
				{
					KLOG3A("oops WSAEWOULDBLOCK is occured");
					Sleep(300);
					//전송할 버퍼가 없다!!! 쉬엄쉬엄 보내라
				}
			}
			else
			{
				if ((UINT)sent_size != next_size)
				{
					KLOG3A("sleep once more %d", 0);
					Sleep(100);
				}

				sent_total_size += sent_size;
				sent_chunk_size += sent_size;
				pCur += sent_size;

				Sleep(10);

				count--;

#if (1)
				// kiwa72(2022.11.26 23h) - 장비와 네트워크 동기화를 위해 필요한 코드
				if ((need_chunk_size <= size512) && sent_chunk_size >= (UINT)pause_size)
				{
					// 1.5MByte 마다 대기 
					// 

					KLOG3A("sent_chunk_size : %d, sent_binary_checksum2[%08X]", sent_chunk_size, sent_binary_checksum2);

					is_saved_chunk = false;

					int time_out = 300;	// 약 300 == 15sec

					while ((is_saved_chunk == false) && (--time_out > 0))
					{
						AfxGetApp()->PumpMessage();	// 쓰레드 처리가 안되어 있어서? 멈춘거임 아놔
						Sleep(10);
					}

					if (time_out < 1)
					{
						KLOG3A("timeout saved_chunk packet");
					}
					else
					{
						KLOG3A("received saved_chunk packet");
					}

					is_saved_chunk = false;
					sent_chunk_size = 0;	// clear
				}
#endif
			}

		}// end of while ((sent_total_size < size512) && (count > 0))

		SAFE_DELETE_ARRAY(data2);

		bResult = (sent_total_size >= size512);
		if (bResult)
		{
			switch (nc)
			{
			case nc_firmware_update:
				KLOG3A("succeeded to send fw update data");
				break;
			case nc_gui_update:
				KLOG3A("succeeded to send gui update data");
				break;
			case nc_bootloader_update:	// kiwa72(2023.03.28 00h) - 추가
				KLOG3A("succeeded to send bootloader update data");
				break;

			case nc_text_update:
				KLOG3A("succeeded to send text update data");
				break;
			case nc_fpga_update:
				KLOG3A("succeeded to send fpga update data");
				break;
			case nc_binary_update:
				KLOG3A("succeeded to send binary data");
				break;
			}
		}
		else
		{
			KLOG3A("failed to send update binary");
		}

		return bResult;
	}
	else
	{
		KLOG3A("failed to send nc first");
		return false;
	}
}

//! kiwa72(2023.01.01 00h) - 장비에서 전달되는 명령 수신
void ReceiveCommandFromDevice(unsigned char* data, DWORD size, void* Socket)
{
	static TCHAR recvtemp[256];

	CTCPClientSock* pThis = (CTCPClientSock*)Socket;
	ASSERT(pThis);

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	NETWORK_COMMAND nc = (NETWORK_COMMAND)data[6];
	pThis->packet_number++;

	// 네트워크 로그
	char logMsg[256];
	sprintf_s(logMsg, sizeof(logMsg), "TCP ReceiveCommandFromDevice: Command=%d, Size=%lu, Packet#=%d\n", nc, size, pThis->packet_number);
	writelog(logMsg, "predix_network.log");

	switch (nc)
	{

	case nc_HR_SR_mode_state:
	{
		if (data[8])
		{
			// 2024-03-28. jg kim. 장비에서 보내주는 해상도 정보 활요하기 위함
			pGlobalData->current_setting.image_resolution = eir_StRes;
			KLOG3A("nc_HR_SR_mode_state :: SR !!!!!");
		}
		else
		{
			// 2024-03-28. jg kim. 장비에서 보내주는 해상도 정보 활요하기 위함
			pGlobalData->current_setting.image_resolution = eir_HiRes;
			KLOG3A("nc_HR_SR_mode_state :: HR !!!!!");
		}
		pGlobalData->do_something(data, size, nc, Socket);
	}
	break;
	case nc_X_ray_Low_Pwer_state:
		if (data[8])
		{
			pGlobalData->do_something(data, size, nc, Socket);
			KLOG3A("nc_X_ray_Low_Pwer_state :: low power !!!!!");
		}
		else
		{
			pGlobalData->do_something(data, size, nc, Socket);
			KLOG3A("nc_X_ray_Low_Pwer_state :: normal condition !!!!!");
		}
		break;
	case nc_systemError: // 2024-06-13. jg kim. 시스템 에러 검출 및 전달
	{
		pGlobalData->g_enSystemError = Error_FLAG_Resetted; // 2024-06-14. jg kim. 변수 초기화 추가
		KLOG3A("System Error %d\n", data[8]);
		pGlobalData->g_enSystemError = en_Error_System(data[8]);
		pGlobalData->do_something(data, size, nc, Socket);
	}
	break;
	case nc_Notification: // 2024-06-13. jg kim. Notification 검출 및 전달
	{
		//pGlobalData->g_nNotification = Notification_Resetted; // 2024-06-14. jg kim. 변수 초기화 추가
		pGlobalData->g_nNotification = (int)data[8]; // 2024-06-14. jg kim. 변수 초기화 추가
		if (data[8] & 0x01)
		{
			pGlobalData->g_nNotification |= No_Image;			KLOG3A("No_Image\n");
			// Memo: Makeit - 에러는 이벤트 수신에서 확인하지 않고 영상처리 결과에서 확인하기로 함.
			// 이벤트 수신에서 확인하면 이상한 타이밍에 에러가 뜨거나 두번 뜰 가능성이 있어서 한곳에서 한번만 하기로 함.
			// CCaptureXrayGenXCR2::CheckLastReceivedImageStatus() 확인할것.
			//pGlobalData->do_something(data, size, nc, Socket);
			OutputDebugString(_T("[nc_Notification] No_Image event received from GenX-CR.\n"));
		}
		if (data[8] & 0x02)
		{
			pGlobalData->g_nNotification |= Low_XRAY;			KLOG3A("Low_XRAY\n");
			// Memo: Makeit - 에러는 이벤트 수신에서 확인하지 않고 영상처리 결과에서 확인하기로 함.
			// 이벤트 수신에서 확인하면 이상한 타이밍에 에러가 뜨거나 두번 뜰 가능성이 있어서 한곳에서 한번만 하기로 함.
			// CCaptureXrayGenXCR2::CheckLastReceivedImageStatus() 확인할것.
			//pGlobalData->do_something(data, size, nc, Socket);
			OutputDebugString(_T("[nc_Notification] Low_XRAY event received from GenX-CR.\n"));
		}
		if (data[8] & 0x04)
		{
			pGlobalData->g_nNotification |= DOOR_OPEN;			KLOG3A("DOOR_OPEN\n");
			OutputDebugString(_T("[nc_Notification] DOOR_OPEN event received from GenX-CR.\n"));
		}
		pGlobalData->do_something(data, size, nc, Socket);
	}
	break;
	case nc_chunk_saved:
		is_saved_chunk = true;
		OutputDebugString(_T("!!!chunk saved\n"));
		break;

	case nc_binary_saved:
		receive_binary_checksum = *(uint32_t*)&data[9];	// CRC32 값 저장

		switch (data[8])
		{
		case 99:// 테스트용. 임의의 바이너리를 전송했을 경우
			OutputDebugString(_T("!!! get nc_binary_saved but no checksum\n"));
			pGlobalData->is_binary_saved = 99;
			// kiwa72(2023.04.19 13h)
			::MessageBox(NULL, _T("Binary Update Success !."), _T("Update"), MB_TOPMOST | MB_OK | MB_ICONINFORMATION);
			pGlobalData->g_pUpdateDialog->EndDialog(IDOK);
			break;

		case 1:// ok
			OutputDebugString(_T("!!! get nc_binary_saved and ok \n"));
			pGlobalData->is_binary_saved = 1;
			// kiwa72(2023.04.19 13h)
			::MessageBox(NULL, _T("Firmware Update completed."), _T("Update"), MB_TOPMOST | MB_OK | MB_ICONINFORMATION);
			pGlobalData->g_pUpdateDialog->EndDialog(IDOK);
			break;

		case 3://failed
			OutputDebugString(_T("!!! get nc_binary_saved and error\n"));
			pGlobalData->is_binary_saved = 3;
			// kiwa72(2023.04.19 13h)
			::MessageBox(NULL, _T("CRC error occurred. Try again.\nReboot the device and try again."), _T("Update fail"), MB_TOPMOST | MB_OK | MB_ICONINFORMATION);
			pGlobalData->g_pUpdateDialog->EndDialog(IDOK);
			//
			// kiwa72(2023.04.06 09h) - CRC32 불일치 에러 ->> 재전송이 필요함. void CUpdateDialog::OnBnClickedUpdateButton()
			//
			break;

		case 0:
			OutputDebugString(_T("!!! get nc_binary_saved old ver\n"));
			pGlobalData->is_binary_saved = 2;
			break;

		default:
			OutputDebugString(_T("!!! get nc_binary_saved unknown\n"));
			pGlobalData->is_binary_saved = 98;
			break;
		}

		//pGlobalData->is_binary_saved = true;	// kiwa72(2023.03.28 00h) - 이게 왜 여기서 ㅡ.ㅡ
		break;

	case nc_hardware_info:
		// kiwa72(2023.01.21 18h) - New Calibration Data
		//memcpy((void*)&current_setting, data, sizeof(HARDWARE_SETTING));
		memcpy((void*)&pGlobalData->current_setting, data, HARDWARE_SETTING_SIZE);

		KLOG3A("receive nc_hardware_info [%d]", pGlobalData->current_setting.state);
		KLOG3A(">> kiwa72: current_setting.Scan_StartY   = %d", pGlobalData->current_setting.Scan_StartY);
#if (0)
		KLOG3A(">> kiwa72: current_setting.tsCal_Saved   = %d", pGlobalData->current_setting.tsCal_Saved);
		KLOG3A(">> kiwa72: current_setting.tsCal_X_org   = %d", pGlobalData->current_setting.tsCal_X_org);
		KLOG3A(">> kiwa72: current_setting.tsCal_X_limit = %d", pGlobalData->current_setting.tsCal_X_limit);
		KLOG3A(">> kiwa72: current_setting.tsCal_Y_org   = %d", pGlobalData->current_setting.tsCal_Y_org);
		KLOG3A(">> kiwa72: current_setting.tsCal_Y_limit = %d", pGlobalData->current_setting.tsCal_Y_limit);
#else
		//! kiwa72(2023.04.26 14h) - 터치 보정 구조체 적용
		//KLOG3A(">> kiwa72: current_setting.sttouch.calibrated_checksum = %d", current_setting.sttouch.calibrated_checksum);
		//KLOG3A(">> kiwa72: current_setting.sttouch.touchleft    = %d", current_setting.sttouch.touchleft);
		//KLOG3A(">> kiwa72: current_setting.sttouch.touchright   = %d", current_setting.sttouch.touchright);
		//KLOG3A(">> kiwa72: current_setting.sttouch.touchtop     = %d", current_setting.sttouch.touchtop);
		//KLOG3A(">> kiwa72: current_setting.sttouch.touchbottom  = %d", current_setting.sttouch.touchbottom);
#endif

		if (!is_first_nc_hardware_info)
		{
			is_first_nc_hardware_info = true;
			pGlobalData->g_plogFile->SetValue(_T("current"), _T("step"), _T("[3/3] received first data"));
			KLOG3A("recevied first current setting");
		}

		//if (get_scanner_state_s() < ss_ready)
		//{
		//	KLOG3A(">>>>>>>>>>>> ss_ready");
		//	set_scanner_state_s_only(ss_ready);
		//}

		KLOG3A("recevied current setting - scanner_state = %d \n", get_scanner_state_s());

		if (is_local_connected)
		{
			forward_data_to_local(data, size);	// current setting
			if (get_scanner_state_s() < ss_ready)
			{
				KLOG3A("first ss_ready");
				set_scanner_state_s_only(ss_ready);
			}
		}
		else
		{
			pGlobalData->do_something(data, size, nc, Socket);
		}
		break;

	case nc_image_list:
		//pThis->nReserved =  arr2num()
		arr2num(&data[8], pThis->nReserved);
		g_is_receiving_data = true;
		pThis->nReceived = 0;//nc_image_list
		pThis->nc = nc;
		device_current = pThis->buffer_l;

		KLOG3A("image list size[%d]\n", pThis->nReserved);

		GetLocalTime(&device_st);
		device_oldsec = device_st.wSecond;
		device_cursec = 0;
		device_differ = 0;
		device_meansec = 0;
		device_count = 0;
		device_oldprogress = 0;
		device_curprogress = 0;
		forward_data_to_local(data, size);//image list header
		pGlobalData->set_progressbar_position(0);
		break;

	case nc_image_list_with_name:
		//pThis->nReserved =  arr2num()
		arr2num(&data[8], pThis->nReserved);
		g_is_receiving_data = true;
		pThis->nReceived = 0;//nc_image_list_with_name
		pThis->nc = nc;
		device_current = pThis->buffer_l;

		KLOG3A("image list_with name size[%d]\n", pThis->nReserved);

		GetLocalTime(&device_st);
		device_oldsec = device_st.wSecond;
		device_cursec = 0;
		device_differ = 0;
		device_meansec = 0;
		device_count = 0;
		device_oldprogress = 0;
		device_curprogress = 0;
		if (is_local_connected)
		{
			forward_data_to_local(data, size);//image list header
		}
		pGlobalData->set_progressbar_position(0);
		break;

	case nc_request_hardware_info2:
		arr2num(&data[8], pThis->nReserved);
		g_is_receiving_data = true;
		pThis->nReceived = 0;//nc_image_list_with_name
		pThis->nc = nc;
		device_current = pThis->buffer_l;

		KLOG3A("nc_request_hardware_info2 size[%d]\n", pThis->nReserved);

		GetLocalTime(&device_st);
		device_oldsec = device_st.wSecond;
		device_cursec = 0;
		device_differ = 0;
		device_meansec = 0;
		device_count = 0;
		device_oldprogress = 0;
		device_curprogress = 0;
		if (is_local_connected)
		{
			forward_data_to_local(data, size);//image list header
		}
		pGlobalData->set_progressbar_position(0);
		break;

	case nc_scan_image_size://image header
		pGlobalData->start_tick = GetTickCount();
		arr2num(&data[8], pThis->nReserved);
		g_is_receiving_data = true;
		pThis->nReceived = 0;//nc_scan_image_size
		pThis->nc = nc;
		device_current = pThis->buffer_l;

		KLOG3A("image size[%d]", pThis->nReserved);
		if (is_valid_stcalibration((stcalibration*)&data[14]))
		{
			memcpy(&cur_calibrationdata, (stcalibration*)&data[14], sizeof(stcalibration));
			KLOG3A(">>> received calibration data  %e/%e/%e/%e/%e", cur_calibrationdata.data[0], cur_calibrationdata.data[1], cur_calibrationdata.data[2], cur_calibrationdata.data[3], cur_calibrationdata.data[4]);
			pGlobalData->command_clear_stage();
		}
		else
		{
			KLOG3A(">>> no calibration data  will use default value");
			memcpy(&cur_calibrationdata, &default_calibrationdata, sizeof(stcalibration));
		}

		set_scanner_state_s(ss_transfer);

		GetLocalTime(&device_st);
		device_oldsec = device_st.wSecond;
		device_cursec = 0;
		device_differ = 0;
		device_meansec = 0;
		device_count = 0;
		device_oldprogress = 0;
		device_curprogress = 0;
		pGlobalData->scanning_progress_tray = 0.f;
		// TODO: Makeit - 영상의 해상도가 아닌 현재 장비 해상도를 넘겨주는 것이 아닌가 의심됨. 제대로 들어오지 않음. 확인중.
		pGlobalData->nLastScannedRes = (IMAGE_RESOLUTION)data[12];//image resolution
		pThis->resScanImg = (IMAGE_RESOLUTION)data[12];//image resolution
		set_image_speed_mode(pThis->resScanImg);
		set_ip_size((IP_SIZE)data[13]);// will do something;

		if (is_local_connected)
		{
			long newsize = get_image_size(get_ip_size(), pThis->resScanImg);
			KLOG3A("set_cruxcan_head_tray: %d ip[%d],res[%d]", newsize, (int)get_ip_size(), (int)pThis->resScanImg);
			num2arr(newsize, &data[8]);
			set_cruxcan_head_tray(nc_scan_image_size_prep, data);//change network_command
			forward_data_to_local(data, size);//image header
		}
		else
		{
			pGlobalData->notify_progress_changed(0);
			pGlobalData->scanning_progress_tray = 0.f;
		}
		break;

	case nc_scan_image_send_fail:
		if (is_local_connected)
		{
			forward_data_to_local(data, size);//image fail
		}
		else
		{
			unsigned long error_code = 0;//1~ 19
			unsigned long error_index = 0;
			arr2num(&data[8], error_code);
			arr2num(&data[11], error_index);

			KLOG3A("f_open error[%d][%s]", (int)error_index, get_f_open_errorstr(error_code));

			pGlobalData->do_something(data, size, nc, Socket);
		}
		break;

	case nc_image_done:
		KLOG3A("received scanned event");
		if (is_local_connected)
		{
			forward_data_to_local(data, size);//image send
		}
		else
		{
			pGlobalData->do_something(data, size, nc, Socket);
		}
		break;

	default:
		KLOG3A(">>>receive from device unknown nc : %d / %d", nc, search_header_in_data(data, size, &crxtags[0], sizeof(crxtags)));

		if (is_local_connected)
		{
			forward_data_to_local(data, size);//unknown
		}
		else
		{
			pGlobalData->do_something(data, size, nc, Socket);
		}
		break;
	}
}

int send_protocol_to_device(unsigned char* data, int size)
{
	KLOG3A();

	if (GetPdxData()->g_pDeviceSocket == NULL)
	{
		KLOG3A("devicesocket is null");
		return false;
	}

	return GetPdxData()->g_pDeviceSocket->Send(data, size);
}

bool send_protocol_to_device(NETWORK_COMMAND nc, unsigned char* data, int size)
{
	//KLOG3A();

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	// 네트워크 로그
	
	// deviceSocekt
	if (pGlobalData->g_pDeviceSocket == NULL)
	{
		KLOG3A("devicesocket is null");
		writelog("TCP send_protocol_to_device: ERROR - Device socket is NULL\n", "predix_network.log");
		return false;
	}

	//! Header - 헤더 선언
	stProtocol protocol;

	//! Header - Signature 셋팅
	set_cruxcan_head_tray(nc, protocol.bytes);

	//! Header - 추가 데이터 셋팅
	if (size > 0)
	{
		memcpy(protocol.headers, data, size);
	}

	//! Header - 전송
	int send_size = pGlobalData->g_pDeviceSocket->Send(protocol.bytes, sizeof(stProtocol));
	if (send_size == SOCKET_ERROR)
	{
		KLOG3A("[$$$$$$]send data is errored socket_error [%d]", GetLastError());
		return false;
	}
	else if (send_size < sizeof(stProtocol))
	{
		int remained = sizeof(stProtocol) - send_size;

		unsigned char* cur = protocol.bytes;
		cur += send_size;

		while (remained && send_size != SOCKET_ERROR)
		{
			send_size = pGlobalData->g_pDeviceSocket->Send(cur, remained);
			if (send_size == SOCKET_ERROR)
			{
				break;
			}
			else
			{
				remained -= send_size;
			}
		}

		if (send_size == SOCKET_ERROR)
		{
			KLOG3A("[$$$$$$]send data is errored socket_error2 [%d]", GetLastError());
		}
		else
		{
			KLOG3A("[$$$$$$]send  sliced data  ok anyway...");
			if (nc == nc_system_reset)
			{
				g_is_sent_system_reset = true;
			}

			return true;
		}
	}
	else
	{
		if (nc == nc_system_reset)
		{
			g_is_sent_system_reset = true;
		}

		return true;
	}

	return false;
}

bool send_protocol_to_local(NETWORK_COMMAND nc, unsigned char* data, int size)
{
	KLOG3A();

	Sleep(30);

	static char* state_strs[] =
	{
		"disconnected",
		"waiting"
		"ready",
		"scan",
		"transfer",
		"busy",
		"system",
		"sleep",
		" ",
	};

	stProtocol protocol;
	unsigned long prog;
	unsigned long state = 0;

	set_cruxcan_head_tray(nc, protocol.bytes);
	if (size > 0)
	{
		memcpy(protocol.headers, data, size);
	}

	switch (nc)
	{
	case nc_scanner_state:
		arr2num(data, state);
		if (state == ss_sleep)
		{
			KLOG3A("state %s", state_strs[ss_system + 1]);
		}
		else if (state < 7 && state > -1)
		{
			KLOG3A("state %s", state_strs[nc]);
		}
		break;
	case nc_tray_progress:
		arr2num(&data[0], prog);
		switch (prog)
		{
		case 0:
			KLOG3A("progress 0%%");
			break;
		case 50:
			KLOG3A("progress 50%%");
			break;
		case 100:
			KLOG3A("progress 100%%");
			break;
		default:
			KLOG3A("progress ...");
			break;
		}
		break;
	default:
		break;
	}

	bool breturn = GetPdxData()->crx_pipe.push_write_pipe(protocol.bytes, sizeof(stProtocol)) > 0 ? true : false;

	Sleep(50);//
	return breturn;
}

bool forward_data_to_local(unsigned char* data, int size)
{
	if (size > 512)
	{
		KLOG3A("[pipe]pipe doesnot send over 512 byte data [%d]", size);
		return false;
	}

	KLOG3A("size = %d bytes[%u]", size, data[6]);
	return GetPdxData()->crx_pipe.push_write_pipe(data, size);
}

bool forward_big_data_to_local(unsigned char* data, int size, NETWORK_COMMAND nc)
{
	KLOG3A();

	if (size < MIN_TCP_SOCKET_BUFFER)
	{
		unsigned char temp[4];
		num2arr((unsigned long)size, temp);
		send_protocol_to_local(nc_tray_data, temp, 4);	// nc_tray_data head

		return forward_data_to_local(data, size);
	}
	else
	{
		TCHAR temp[200] = { 0, };	//because protocoal struct size is 512  path must less than 200

		CString filename = size > (350 * 1024) ? _T("lastimg.crx") : _T("lastdata.lst");
		_stprintf_s(temp, _T("%s%s"), working_path, (LPTSTR)(LPCTSTR)filename);

		FILE* fp = _wfopen((CT2W)temp, L"wb");
		if (!fp)
		{
			KLOG3A("[CRXE0301]failed to save lastdata.dmp");

			unsigned char temp2[4];
			num2arr((unsigned long)size, temp2);
			send_protocol_to_local(nc_tray_data, temp2, 4);	// nc_tray_data head

			KLOG3A("save failed -> %d bytes", size);

			return forward_data_to_local(data, size);
		}

		KLOG3A("save to file and send path to local");

		temp[199] = (unsigned short)nc;
		fwrite(data, size, 1, fp);
		fclose(fp);

		return send_protocol_to_local(nc_tray_file_path, (unsigned char*)&temp[0], sizeof(TCHAR) * 201);
	}
}

int command_start_scan(void)
{
	if (get_scanner_state_s() != ss_ready)
	{
		return 0;
	}

	return send_protocol_to_device(nc_request_image, NULL, 0);
}

void Push_TCP_Queue_SDK(stmyqueue* q, unsigned char* data, int len, void* Socket)
{
	// 512이하로 넘어옴. sdk -> tray 경우 받을 데이터가 따로 있으면 걍 저쪽으로 넘김

	enqueues(q, data, len);//Push_TCP_Queue_SDK

	if (q->qdatasize < sizeof(crxtags))
	{
		return;//헤더보다 작으니 판단필요없음
	}

	if (q->qdatasize >= TCP_COMMAND_SIZE)
	{
		int count = q->qdatasize / TCP_COMMAND_SIZE;
		for (int i = 0; i < count; i++)
		{
			dequeues(q, &validtcppacketsdk[0], TCP_COMMAND_SIZE);
			ReceiveCommandFromSDK(validtcppacketsdk, TCP_COMMAND_SIZE, Socket);
		}
	}
}

void Push_TCP_Queue_Device(stmyqueue* q, unsigned char* data, int len, void* Socket, bool is_reserved_exist)
{
	UNREFERENCED_PARAMETER(is_reserved_exist);

	// 512Byte 이하로 넘어옴

	enqueues(q, data, len);

	if (q->qdatasize < sizeof(crxtags))
	{
		return;
	}

	if (q->qdatasize >= TCP_COMMAND_SIZE)
	{
		int count = q->qdatasize / TCP_COMMAND_SIZE;

		for (int i = 0; i < count; i++)
		{
			dequeues(q, &validtcppacketdevice[0], TCP_COMMAND_SIZE);
			ReceiveCommandFromDevice(validtcppacketdevice, TCP_COMMAND_SIZE, Socket);
		}
	}
}

BOOL Receive_PipeDataFromSDK(unsigned char* data, DWORD size)
{
	if (!is_local_connected)
	{
		OnConnectFromLocalSDK(0);
	}

	ReceiveCommandFromSDK(data, size, NULL);

	return TRUE;
}

int is_scanner_used(char* tgtip)
{
	if (last_received_crxdevices.datas.size() < 1)
	{
		return en2n_poor_network;
	}

	char temp[32] = { "", };
	std::vector<CRX_N2N_DEVICE>::iterator iter;

	for (iter = last_received_crxdevices.datas.begin(); iter != last_received_crxdevices.datas.end(); iter++)
	{
		sprintf(temp, "%u.%u.%u.%u", iter->device_ipaddr[0], iter->device_ipaddr[1], iter->device_ipaddr[2], iter->device_ipaddr[3]);

		if (strcmp(tgtip, temp) == 0)
		{
			if (*(int*)&iter->host_ipaddr == 0)
			{
				return en2n_already_requested;
			}

			return en2n_used_other;
		}
	}

	return en2n_poor_network;
}

void preprocess_image_and_save(DWORD size, unsigned char* data, void* Socket)
{
	KLOG3A();

	UNREFERENCED_PARAMETER(data);

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	CTCPClientSock* pThis = (CTCPClientSock*)Socket;
	clear_raw();
	pGlobalData->clear_imagectrl();
	memcpy(pGlobalData->map_raw.data(), pThis->buffer_l, size);

	shift_raw(size);
	//write_calibrationdata_to_image();
	// 2024-02-24. jg kim. Cruxell calibration data를 image에 기록하는 함수이기 때문에 비활성화
	pGlobalData->map_origin = pGlobalData->map_raw;	// backup origianl raw // xldosxl

	for (auto iterf = pGlobalData->map_fin.begin(); iterf != pGlobalData->map_fin.end(); iterf++)
	{
		*iterf = 0;//clear first
	}

	// 2026-02-27. jg kim. ImageList에서 요청한 영상은 raw만 저장하고 바로 return
	if (pGlobalData->bImageListMode)
	{
		// raw image에서 meta 정보 (scan date time, image list index) 추출
		CHardwareInfo hwi;
		stDecodedHardwareParam hwParam;
		bool bDecodedMeta = hwi.decodeGenorayTag(pGlobalData->map_raw.data(), geo_x, hwParam);
		
		CString filename;
		if (bDecodedMeta && hwParam.fdate > 0 && hwParam.cur_info_index > 0)
		{
			// fdate를 날짜/시간 문자열로 변환 (Excel 날짜 형식: 1900년 1월 1일부터의 일수)
			time_t scan_time = (time_t)(hwParam.fdate * 86400. - 2209161600.);
			struct tm* t = gmtime(&scan_time);
			if (t != nullptr)
			{
				filename.Format(_T("idx_%03d_%04d-%02d-%02d_%02d-%02d-%02d_raw.tif"), 
					hwParam.cur_info_index, 
					t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
					t->tm_hour, t->tm_min, t->tm_sec);
				KLOG3A("ImageListMode: Decoded meta info - Index: %d, Date: %04d-%02d-%02d %02d:%02d:%02d", 
					hwParam.cur_info_index,
					t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
					t->tm_hour, t->tm_min, t->tm_sec);
			}
			else
			{
				filename.Format(_T("idx_%03d_unknown_date_raw.tif"), hwParam.cur_info_index);
				KLOG3A("ImageListMode: Failed to convert scan time, using index only: %d", hwParam.cur_info_index);
			}
		}
		else
		{
			filename = _T("image_list_raw.tif");
			KLOG3A("ImageListMode: Failed to decode meta info (fdate: %f, index: %d)", hwParam.fdate, hwParam.cur_info_index);
		}
		
		// raw image 저장
		save_tiff_image((CT2W)filename, pGlobalData->map_raw.data(), geo_x, geo_y, geo_x, 400.0f, 400.0f);
		KLOG3A("ImageListMode: Raw image saved as %S", (LPCWSTR)filename);
		
		// 플래그 해제 및 리턴
		pGlobalData->bImageListMode = FALSE;
		
		// 이미지를 화면에 표시
		if (!is_local_connected)
		{
			pGlobalData->command_LoadTiffToStage((CT2W)filename);
		}
		
		return;
	}
	
	// 2026-02-27. jg kim. 일반 모드일 경우 기존 방식대로 처리
	CString filenameBase = pGlobalData->GetSaveFilenameFromUI();

	CString filename = _T("recv.tif");
	// 2024-08-26. jg kim. cal image 획득 모드 추가
	if (pGlobalData->bAcquireCalImage && pGlobalData->g_iCalImgCount < 5)
	{
		CString str;
		str.Format(L"_%d_raw.tif", pGlobalData->g_iCalImgCount); // 2024-10-28. jg kim. debug
		filename = pGlobalData->GetSaveFilenameFromUI() + str;
	}
	else
	{
		// 2026-02-27. jg kim. image list에서 요청한 영상은 decoded meta 정보 사용
		filename = filenameBase + _T("_raw.tif");
		pGlobalData->bAcquireCalImage = FALSE;
	}
	
	save_tiff_image((CT2W)filename, pGlobalData->map_raw.data(), geo_x, geo_y, geo_x, 400.0f, 400.0f);

	if (!pGlobalData->bAuto_pp)
	{
		for (auto iterf = pGlobalData->map_fin.begin(), iterr = pGlobalData->map_raw.begin(); iterf != pGlobalData->map_fin.end(); iterf++, iterr++)
		{
			*iterf = *iterr;
		}
	}
	else
	{
		KLOG3A("will to postprocessing.................");
		send_tray_busy_to_device(1);

		// 2024-12-09. jg kim. 공정툴용 영상처리와 통합
		// 2026-03-04. jg kim. 영상 파일 이름 설정
		g_strCurrentImageFileName = filenameBase;
		pre_postprocess_by_dll();	//new_process	kiwa72(2023.01.18 19h) - 이미지 프로세싱 테스트 진행 중
		char buf[1024];
		sprintf_s(buf, "\n");
		writelog(buf, "ImageProcessing.log");
		if (GetPdxData()->nCorrectWobbleResult == 0)
			AfxMessageBox(L"Wobble 보정에 실패했습니다.");
		if (GetPdxData()->nCorrectWobbleResult == -1)
			AfxMessageBox(L"Wobble 보정을 위한 적절한 반사판이 없습니다.");
		// 2026-02-27. jg kim. image list에서 요청한 영상은 decoded meta 정보 사용
		filename = filenameBase + _T("_pped.tif");
		save_tiff_image((CT2W)filename, pGlobalData->last_image_data, pGlobalData->last_image_width, pGlobalData->last_image_height, pGlobalData->last_image_width, 400.0f, 400.0f);

		last_image_to_fin_by_dll();
		send_tray_busy_to_device(0);
		for (auto iter = pGlobalData->map_fin.begin(); iter != pGlobalData->map_fin.end(); iter++)
		{
			*iter = 0;
		}
	}

	if (!is_local_connected)
	{
		pGlobalData->map_geo = pGlobalData->map_fin;
		pGlobalData->img_tex.size = pGlobalData->map_fin.d();
		pGlobalData->img_tex.channels = 1;
		if (!pGlobalData->bAuto_pp)
		{
			 // 2024-10-28. jg kim. debug
			if (pGlobalData->bAcquireCalImage && pGlobalData->g_iCalImgCount < 5)
			{
				CString str;
				str.Format(L"_%d_raw.tif", pGlobalData->g_iCalImgCount++);
				filename = pGlobalData->GetSaveFilenameFromUI() + str;
			}
			else
			{// 2026-02-27. jg kim. image list에서 요청한 영상은 decoded meta 정보 사용
				filename = filenameBase + _T("_raw.tif");
			}
		}
		else
		{
			// 2026-02-27. jg kim. image list에서 요청한 영상은 decoded meta 정보 사용
			filename = filenameBase + _T("_pped.tif");
		}

		pGlobalData->command_LoadTiffToStage((CT2W)filename);
	}
}

bool send_patient_name_by_tray(const char* name)
{
	KLOG3W();

	// utf8 or ascii text
	//CStringA utf8_text(name_data);
	unsigned char data[MAX_NAME_LENGTH + 8];//헤더 넣을 공간 
	memset(data, 0, MAX_NAME_LENGTH);
	memcpy(&data[8], name, MAX_NAME_LENGTH);

	if (data[MAX_NAME_LENGTH + 7] != '\0')
	{
		data[MAX_NAME_LENGTH + 7] = '\0';
	}

	set_cruxcan_head_tray(nc_send_patient_name, data);
	ReceiveCommandFromSDK(data, MAX_NAME_LENGTH + 8, NULL);

	return true;
}

void force_upver(void)
{
	if (get_scanner_state_s() != ss_ready)
	{
		return;
	}

	unsigned long a = 1;
	unsigned char temp[4];
	num2arr(a, temp);
	send_protocol_to_device(nc_change_2_upver, temp, 4);
}

void retore_ftver(void)
{
	if (get_scanner_state_s() != ss_ready)
	{
		return;
	}

	unsigned long a = 1;
	unsigned char temp[4];
	num2arr(a, temp);
	send_protocol_to_device(nc_change_2_ftver, temp, 4);
}
