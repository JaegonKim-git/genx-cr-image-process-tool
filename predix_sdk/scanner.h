// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "cruxcan_sdk.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

extern int scan_width;
extern int scan_height;

void scan_data_init();
void scan_data_destroy();

void set_new_scanner(int num);
int get_new_scanner();

void set_scan_running(int num);
int get_scan_running();

void set_scanner_connected(int num);
int get_scanner_connected();

void set_image_speed_mode(IMAGE_RESOLUTION ir);
IMAGE_RESOLUTION get_image_speed_mode();

void set_image_ip_size(IP_SIZE ip);
IP_SIZE get_image_ip_size();

void set_ip_size(IP_SIZE is);
IP_SIZE get_ip_size();

void set_scanner_progress_s(int pct);
int get_scanner_progress_s();

AFX_EXT_API void set_scanner_state_s(SCANNER_STATE ss);
AFX_EXT_API void set_scanner_state_s_only(SCANNER_STATE ss);
AFX_EXT_API SCANNER_STATE get_scanner_state_s();

AFX_EXT_API int is_scanner_busy();
