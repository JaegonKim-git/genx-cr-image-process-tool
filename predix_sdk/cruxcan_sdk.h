#pragma once


// Image Resolution ( 0 : Super Resolution, 1 : High Resolution )
/* millimeters */
#define SR_PIXEL_REAL_SIZE   (0.025)
#define HR_PIXEL_REAL_SIZE   (0.050)


typedef enum IMAGE_RESOLUTION : unsigned char
{
#if 0
	//20220525
	//eir_SR = 0,
	//eir_HR = 1,
#else
	eir_HiRes = 0,
	eir_StRes = 1,
#endif
} IMAGE_RESOLUTION;

// IP Size ( 0, 1, 2, 3 )
typedef enum IP_SIZE : long
{
	eis_None = -1,
	eis_IP0 = 0,
	eis_IP1 = 1,
	eis_IP2 = 2,
	eis_IP3 = 3,
	eis_IPEND = 0x7FFFFFFF	/*컴파일러 관계없이 4바이트로 강제하기 위해 추가됨*/
} IP_SIZE;

// 16bit Gray Image
typedef struct CRUXCAN_IMAGE
{
	unsigned short		offset;
	IP_SIZE				ip;
	IMAGE_RESOLUTION	image_resolution;
	int					width;
	int					height;
	unsigned short*		image_data;
} CRUXCAN_IMAGE;

// Scanner Status
typedef enum _SCANNER_STATE_
{
	ss_disconnected = 0,
	ss_warmup,		// changed from ss_warmup
	ss_ready,
	ss_scan,

	//! kiwa72(2023.06.24 11h) - 신규 추가 (엘 발생 - 위치 변경 필요)
	ss_erasing,		// 스캔 후 ip 삭제 중
	ss_preview = 5,	// 스캔 후 바로보기 화면 상태
	//...

	ss_transfer,
	ss_busy,
	ss_system = 8,
	ss_occupied,

	ss_sleep = 0x7FFFFFFE,
	ss_none = 0x7FFFFFFF
} SCANNER_STATE;

// Image List
#define MAX_IMAGE_COUNT 500
#define MAX_NAME_LENGTH ( 128 -  ( 8 + 1 + 4+ 4))  /*20211028  메모리 넣고 빼기 좋게 구조체를 128바이트로 만듬  */

typedef struct IMAGE_LIST
{
	unsigned int		cur_image_num;							// lastest image index
	double				image_date[MAX_IMAGE_COUNT];			// image date
	IMAGE_RESOLUTION	image_mode[MAX_IMAGE_COUNT];			// image resolution
	char				name[MAX_IMAGE_COUNT][MAX_NAME_LENGTH];	// patient name
	unsigned int		name_index[MAX_IMAGE_COUNT];			// patient id
	IP_SIZE				ip_size[MAX_IMAGE_COUNT];				// IP size
} IMAGE_LIST;

#pragma pack(push, 1)
typedef struct stpvIMAGE_INFO
{
	double				image_date;			// 8
	unsigned int		name_index;			// 4
	IP_SIZE				ip_size;			// 4
	IMAGE_RESOLUTION	image_resolution;	// 1
	char				name[MAX_NAME_LENGTH];	//
} stpvIMAGE_INFO;

typedef struct stpvIMAGE_INFO_LIST
{
	unsigned int	cur_image_num;
	unsigned int	dummy[128 - 1/*cur_image_num*/];
	stpvIMAGE_INFO	imageinfos[MAX_IMAGE_COUNT];
}stpvIMAGE_INFO_LIST;
#pragma pack(pop)

#ifndef __CRX_N2N_DEFINES__
#define __CRX_N2N_DEFINES__
struct CRX_N2N_DEVICE
{
	char			Name[12];
	unsigned char	device_ipaddr[4];
	unsigned char	host_ipaddr[4];
	SCANNER_STATE	state;	// 4byte

	int same(CRX_N2N_DEVICE &b)
	{
		return (*(long *)device_ipaddr == *(long *)b.device_ipaddr);
	}

	int equal(CRX_N2N_DEVICE &b)
	{
		return same(b) && (*(long *)host_ipaddr == *(long *)b.host_ipaddr) && (state == b.state) && (strcmp(Name, b.Name) == 0);
	}

	int operator == (CRX_N2N_DEVICE &b)
	{
		return equal(b);
	}

	CRX_N2N_DEVICE& operator = (const CRX_N2N_DEVICE &r)
	{
		memcpy(this, &r, sizeof(CRX_N2N_DEVICE));
		return *this;
	}
}; // 20200625
#endif // __CRX_N2N_DEFINES__


#include <vector>

struct crxDevices
{
	std::vector<CRX_N2N_DEVICE> datas;

	CRX_N2N_DEVICE* Data()
	{
		return datas.data();
	}

	CRX_N2N_DEVICE operator[](int n)
	{
		return datas[n];
	}

	CRX_N2N_DEVICE at(int n)
	{
		return datas[n];
	}
}; // to wrapping devices; // 20200625


#define CBAPI	cdecl

typedef void	(CBAPI* ON_SCANNER_STATE_EVENT)			(SCANNER_STATE scanner_status);
typedef void	(CBAPI* ON_IMAGE_RCV_DONE_EVENT)		(CRUXCAN_IMAGE *cruxcan_image);
typedef void	(CBAPI* ON_IMAGE_LIST_RCV_DONE_EVENT)	(IMAGE_LIST *image_list);

typedef void	(CBAPI* ON_IMAGE_LIST_WITH_NAME_RCV_DONE_EVENT)		(IMAGE_LIST *image_list);

typedef void	(CBAPI* ON_IMAGE_PROGRESS_EVENT)		(int pct);
//>>
typedef void	(CBAPI* ON_TWAIN_SCAN_EVENT)			(char* file_name);
typedef void	(CBAPI* ON_TWAIN_CANCEL_EVENT)			();


// TODO: Makeit - 다른 SDK(구버전 FireCR SDK)와의 충돌 때문에 일단 이름을 변경함. 이름을 변경하는 것을 협의해야 할듯.

typedef	struct CALLBACK_SET // for legercy (ex. pointnix)
{
	ON_SCANNER_STATE_EVENT			scanner_state_event;
	ON_IMAGE_RCV_DONE_EVENT			image_rcv_done_event;
	ON_IMAGE_LIST_RCV_DONE_EVENT	image_list_rcv_done_event;
	ON_IMAGE_PROGRESS_EVENT			image_progress_event;
	//>>
	ON_TWAIN_SCAN_EVENT				twain_scan_event;
	ON_TWAIN_CANCEL_EVENT			twain_cancel_event;
} TCallBackSet_;

typedef	struct CALLBACK_SET2
{
	ON_SCANNER_STATE_EVENT			scanner_state_event;
	ON_IMAGE_RCV_DONE_EVENT			image_rcv_done_event;
	ON_IMAGE_LIST_RCV_DONE_EVENT	image_list_rcv_done_event;
	ON_IMAGE_LIST_WITH_NAME_RCV_DONE_EVENT	image_list_with_name_rcv_done_event;
	ON_IMAGE_PROGRESS_EVENT			image_progress_event;
	//>>    
	ON_TWAIN_SCAN_EVENT				twain_scan_event;
	ON_TWAIN_CANCEL_EVENT			twain_cancel_event;

} TCallBackSet2;

typedef	struct CALLBACK_SET3
{
	ON_SCANNER_STATE_EVENT			scanner_state_event;
	ON_IMAGE_RCV_DONE_EVENT			image_rcv_done_event;
	ON_IMAGE_LIST_RCV_DONE_EVENT	image_list_rcv_done_event;
	ON_IMAGE_LIST_WITH_NAME_RCV_DONE_EVENT	image_list_with_name_rcv_done_event;
	ON_IMAGE_PROGRESS_EVENT			image_progress_event;
	//>>    
	ON_TWAIN_SCAN_EVENT				twain_scan_event;
	ON_TWAIN_CANCEL_EVENT			twain_cancel_event;

} TCallBackSet3;	//20200625

typedef union SCANNER_INFORMATION
{
	// ver 1.0.0.0
	struct
	{
		unsigned char	struct_ver[4];	// version of struct
		char			name[20];		// reserved
		unsigned char	fw_ver[4];		// firmware version 1
		unsigned char	fpga_ver[4];	// firmware version 2
		unsigned char   ip_addr[4];		// ip address
		unsigned char   mac_addr[6];	// mac address
		unsigned char   reserved1[2];	// for 32bit memory addressing
		char			udi[51];		// udi 50bytes + null(1byte)
		char			reserved2[1];	// for 32bit memory addressing
		SCANNER_STATE	state;			// scanner status
	};
	unsigned char		bytes[512];
}SCANNER_INFORMATION;

#ifdef __cplusplus
extern "C"
{
#endif
	__declspec(dllexport) BOOL	CBAPI	open_scanner_sdk(HWND hwnd, CALLBACK_SET* cb_set, wchar_t* dir_path);
	__declspec(dllexport) BOOL	CBAPI	open_scanner_sdk2(HWND hwnd, CALLBACK_SET2* cb_set, wchar_t* dir_path);
	__declspec(dllexport) BOOL	CBAPI	open_scanner_sdk3(HWND hwnd, CALLBACK_SET3* cb_set3, wchar_t* dir_path);//20200625
	__declspec(dllexport) void	CBAPI	close_scanner_sdk();

	__declspec(dllexport) BOOL	CBAPI	request_image_list();
	__declspec(dllexport) BOOL	CBAPI	request_image_list_with_name();
	__declspec(dllexport) BOOL	CBAPI	request_image(int index);

	__declspec(dllexport) int		CBAPI	get_scan_progress();
	__declspec(dllexport) SCANNER_STATE	CBAPI	get_scanner_state();
	__declspec(dllexport) IMAGE_RESOLUTION	CBAPI	get_resolution();

	__declspec(dllexport) BOOL	CBAPI	show_option_dlg(int show);
	__declspec(dllexport) BOOL	CBAPI	show_image_list_dlg(int show);
	__declspec(dllexport) BOOL	CBAPI	show_twain_main_dlg(int show);
	__declspec(dllexport) BOOL	CBAPI	set_acquire_after_scan(BOOL push_image);

	__declspec(dllexport) BOOL	CBAPI	set_patient_name(char* patient_name);

	__declspec(dllexport) BOOL	CBAPI	get_scanner_information(SCANNER_INFORMATION* scanner_information);
	__declspec(dllexport) BOOL	CBAPI	delete_all_image_of_scanner();
	__declspec(dllexport) long	CBAPI	connect_scanner_n2n(char * tgtip);//20200625	20200707 BOOL -> long
#ifdef __cplusplus
}
#endif

//________________________________________________________________________________________________________________________________
// for explicit link

typedef	BOOL(CBAPI* OPEN_SCANNER_SDK)(HWND hwnd, CALLBACK_SET* cb_set, wchar_t* dir_path);
typedef	BOOL(CBAPI* OPEN_SCANNER_SDK2)(HWND hwnd, CALLBACK_SET2* cb_set, wchar_t* dir_path);
typedef	BOOL(CBAPI* OPEN_SCANNER_SDK3)(HWND hwnd, CALLBACK_SET3* cb_set3, wchar_t* dir_path); // 20200625
typedef	void (CBAPI* CLOSE_SCANNER_SDK)(void);
typedef BOOL(CBAPI* REQUEST_IMAGE_LIST)(void);
typedef BOOL(CBAPI* REQUEST_IMAGE_LIST_WITH_NAME)(void);
typedef BOOL(CBAPI* REQUEST_IMAGE)(int index);
typedef int (CBAPI* GET_SCAN_PROGRESS)(void);
typedef SCANNER_STATE(CBAPI* GET_SCANNER_STATE)(void);
typedef IMAGE_RESOLUTION(CBAPI* GET_RESOLUTION)(void);
typedef BOOL(CBAPI* SHOW_OPTION_DLG)(int show);
typedef BOOL(CBAPI* SHOW_IMAGE_LIST_DLG)(int show);
typedef BOOL(CBAPI* SHOW_TWAIN_MAIN_DLG)(int show);
typedef BOOL(CBAPI* SET_ACQUIRE_AFTER_SCAN)(BOOL push_image);
typedef BOOL(CBAPI* SET_PATIENT_NAME)(char* patient_name);
typedef BOOL(CBAPI* GET_SCANNER_INFORMATION)(SCANNER_INFORMATION* scanner_information);
typedef BOOL(CBAPI* DELETE_ALL_IMAGE_OF_SCANNER)(void);
typedef long(CBAPI* CONNECT_SCANNER_N2N)(char* scanner_ip);	// 20200707 BOOL -> long
typedef long(CBAPI* DISCONNECT_SCANNER_N2N)();	// 20200707 BOOL -> long
typedef long(CBAPI* UPDATE_SCANNER_NAME)(char*);	// max 11 byte 	20200707 BOOL -> long
typedef crxDevices (CBAPI* GET_SCANER_LISTVECTOR)(void);	// 20200625 
typedef CRX_N2N_DEVICE* (CBAPI*	GET_SCANER_LIST_DATA)(void);	// 20200625 
typedef int (CBAPI*	GET_SCANER_LIST_COUNT)(void);	// 20200625 
