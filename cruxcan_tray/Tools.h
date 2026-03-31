#pragma once


#include "../predix_sdk/global_data.h"
#include "../cruxcan_tray/CAdapterInfoDlg.h"
#include "cruxcan_trayDlg.h"


#define MALLOC(x)				HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x)					HeapFree(GetProcessHeap(), 0, (x))
#define WORKING_BUFFER_SIZE		15000
#define MAX_TRIES				3


extern int Get_Ethernet_Info(std::vector<ST_AdapterInfo>& vAdapterInfoss);
extern void Get_CurrentPC_IP_Address(void);
//extern std::string get_ip_from_interface(char* tgt_mac);
//extern void check_usb_ethernet_adapter(void);
extern void show_ethernet_setting(CWnd* parent);
//extern void save_ethernet_setting(CString mac);

extern void g_callback_scroll(int c, int w);


extern Ccruxcan_trayDlg* g_pTtray_Dlg;
