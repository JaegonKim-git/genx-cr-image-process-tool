// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "cruxcan_defines.h"
#include "common_type.h"
#include <afxsock.h> // fpr asyncsock.h
#include <iphlpapi.h>
#include <icmpapi.h>


#define WM_FROM_TCP		(WM_USER + 337)


enum enN2NRESULT
{
	en2n_no_center = -98,
	en2n_poor_network = -10,	// Use
	en2n_user_canceled = -5,
	en2n_time_out = -4,
	en2n_used_other = -3,	// Use
	en2n_request_failed = -2,
	en2n_invalid_param = -1,	// Use

	en2n_already_requested = 0,	// Use
	en2n_request_ok = 1,
	en2n_connected_already = 2,	// Use
};


enum TCP_TYPE : long
{
	tt_none = 0,
	tt_image,
	tt_imglist,
	tt_devices,
	tt_hosts,
	tt_fw,
	tt_fpga,
	tt_gui,
	tt_setting
};


AFX_EXT_API void num2arr(const unsigned long num, unsigned char *arr);
AFX_EXT_API void arr2num(const unsigned char * arr, unsigned long &num);


void set_cruxcan_head_tray(NETWORK_COMMAND enc, unsigned char * bytes);
void set_cruxcan_head(unsigned char * data);
bool is_cruxcan_head(unsigned char * data);


#ifndef _IP_OPTION_INFORMATION_
#define _IP_OPTION_INFORMATION_
typedef struct
{
	DWORD Address;					// Replying address
	unsigned long  Status;			// Reply status
	unsigned long  RoundTripTime;	// RTT in milliseconds
	unsigned short DataSize;		// Echo data size
	unsigned short Reserved;		// Reserved for system use
	void *Data;						// Pointer to the echo data
	IP_OPTION_INFORMATION Options;	// Reply options
} IP_ECHO_REPLY, * PIP_ECHO_REPLY;


typedef HANDLE(WINAPI* pfnHV)(VOID);
typedef BOOL(WINAPI* pfnBH)(HANDLE);
typedef DWORD(WINAPI* pfnDHDPWPipPDD)(HANDLE, DWORD, LPVOID, WORD, PIP_OPTION_INFORMATION, LPVOID, DWORD, DWORD); // evil, no?
#endif


 //------------------------------------------------------------------------
 // for UDP
 //------------------------------------------------------------------------

typedef void(CBAPI* ON_RECEIVE_DATA_RAW)(DWORD size, unsigned char* data, void* Socket);
typedef void(CBAPI* ON_CONNECT_SOCKET)(int nErrorCode);
typedef void(CBAPI* ON_CLOSE_SOCKET)(int nErrorCode);


//------------------------------------------------------------------------
// for tcp
//------------------------------------------------------------------------

typedef union __stProtocol__
{
	struct
	{
		union
		{
			unsigned short  Signature[4];

			struct
			{
				unsigned char sig1;
				unsigned char sig2;
				unsigned char ver;
				unsigned char sig3;
				unsigned char sig4;
				unsigned char sig5;
				unsigned char nc;
				unsigned char sig6;
			};
		};

		unsigned char headers[TCP_PACKET_MIN_SIZE - (sizeof(unsigned short) * 4)];
	};

	int params[TCP_PACKET_MIN_SIZE / sizeof(int)];

	unsigned char bytes[TCP_PACKET_MIN_SIZE];

	__stProtocol__()
	{
		memset(bytes, 0, TCP_PACKET_MIN_SIZE);
	}

} stProtocol;


typedef union __stDevice__
{
	struct
	{
		unsigned short	Signature[4];
		unsigned char	version[4];
		unsigned char	ip_addr[4];		// ipaddr에서 변경
		unsigned char	devicename[20];
		//unsigned char	hostname[20];
		SCANNER_STATE	state;
		//20181115 추가
		unsigned char	fw_ver[4];		// firmware version 1
		unsigned char	fpga_ver[4];	// firmware version 2
		unsigned char	mac_addr[6];	// mac address
		unsigned char	reserved1[2];	// for 32bit memory addressing
		char			udi[51];		// udi 50bytes + null(1byte)
		char			reserved2[1];	// for 32bit memory addressing
	};

	int params[TCP_PACKET_MIN_SIZE/sizeof(int)];

	unsigned char bytes[TCP_PACKET_MIN_SIZE];

	__stDevice__()
	{
		set_cruxcan_head_tray(nc_tray,(unsigned char*)Signature);
	}

	__stDevice__(NETWORK_COMMAND enc)
	{
		memset(bytes, 0, TCP_PACKET_MIN_SIZE );
		set_cruxcan_head_tray(enc, (unsigned char*)Signature);
	}

} stDevice;


//------------------------------------------------------------------------
// for UDP
//------------------------------------------------------------------------

typedef union __stUDP__
{
public:
	struct
	{
		// scanner와 주고받는 데이터는 TCHAR 쓰지말자

		unsigned short	Signature[4];	// 8 byte
		unsigned long	checksum;		// 4 byte, UDP는 CRC 체크 없으므로 
		unsigned char	version[4];		// 4 byte
		unsigned char	ipaddr[4];		// 4 byte
		double			starttime;		// 8 byte
		double			currenttime;	// 8 byte
		unsigned long	port;			// 4 byte
		char			name[20];		// 20 byte, Tot 44 byte
		// Total : 56 byte
	};

	unsigned int params[UDP_PACKET_SIZE / sizeof(int)];	// 512 byte
	unsigned char bytes[UDP_PACKET_SIZE];	// 512 byte

	void Reset(NETWORK_COMMAND enc)
	{
		memset((void*)this, 0, sizeof(__stUDP__));
		set_cruxcan_head_tray(enc, (unsigned char*)Signature);
	}

	__stUDP__(NETWORK_COMMAND enc)
	{
		memset(bytes, 0, UDP_PACKET_SIZE);
		set_cruxcan_head_tray(enc, (unsigned char*)Signature);
	}

	__stUDP__()
	{
		Reset(nc_tray);
	}

} stUDP;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef void (CBAPI* ON_RECEIVE_DATA)(char ipaddr[], TCP_TYPE type, long size, unsigned char* data);
void CBAPI OnReceiveFromTray(char ipaddr[], TCP_TYPE type, long size, unsigned char* data);
void CBAPI OnReceiveFromSDK(char ipaddr[], TCP_TYPE type, long size, unsigned char* data);
void CBAPI OnReceiveDataRawUDPFromCenter(DWORD size, unsigned char* data, void* Socket);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


AFX_EXT_API BOOL InitTCPsc();
AFX_EXT_API BOOL InitUDPsc();

AFX_EXT_API void startLeader();
AFX_EXT_API void endLeader();

AFX_EXT_API void Disconnect_by_sleep();
AFX_EXT_API void Resume_from_sleep();

AFX_EXT_API bool reset_scanner();
AFX_EXT_API bool delete_all_image();
AFX_EXT_API bool request_image_tray(int image_num);

void CBAPI ReceiveDataFromLocalSDK(DWORD size, unsigned char *data, void* Socket);
void CBAPI OnCloseFromLocalSDK(int nErrorCode);
void CBAPI ReceiveDataFromRemoteSDK(DWORD size, unsigned char* data, void* Socket);
void CBAPI OnConnectFromRemoteSDK(int nErrorCode);
void CBAPI OnCloseFromRemoteSDK(int nErrorCode);
void CBAPI ReceiveDataFromDevice(DWORD size, unsigned char* data, void* Socket);
void CBAPI OnConnectFromDevice(int nErrorCode);
void CBAPI OnCloseFromDevice(int nErrorCode);

AFX_EXT_API void CBAPI OnConnectFromLocalSDK(int nErrorCode);

AFX_EXT_API int Send_FreeData_To_Device(NETWORK_COMMAND nc, unsigned char* data, int size);
AFX_EXT_API bool send_binary_to_device(NETWORK_COMMAND nc, unsigned char* data, int size, char * filename);

AFX_EXT_API int send_protocol_to_device(unsigned char* data, int size);	// legacy
AFX_EXT_API bool send_protocol_to_device(NETWORK_COMMAND nc, unsigned char* data, int size);

AFX_EXT_API bool send_protocol_to_local(NETWORK_COMMAND nc, unsigned char* data, int size);
AFX_EXT_API bool forward_data_to_local( unsigned char* data, int size);
AFX_EXT_API bool forward_big_data_to_local(unsigned char* data, int size, NETWORK_COMMAND nc);

AFX_EXT_API int command_start_scan();

AFX_EXT_API void Push_TCP_Queue_SDK(stmyqueue *  q, unsigned char * data, int len, void * Socket);
AFX_EXT_API void Push_TCP_Queue_Device(stmyqueue *  q, unsigned char * data, int len, void * Socket, bool is_reserved_exist);
AFX_EXT_API BOOL Receive_PipeDataFromSDK(unsigned char* data, DWORD size);

AFX_EXT_API int is_scanner_used(char * tgtip);

AFX_EXT_API void preprocess_image_and_save(DWORD size, unsigned char* data, void* Socket);
AFX_EXT_API bool send_patient_name_by_tray(const char* name);
AFX_EXT_API void force_upver(void);
AFX_EXT_API void retore_ftver(void);
