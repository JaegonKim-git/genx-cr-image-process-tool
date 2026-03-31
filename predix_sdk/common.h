#pragma once
//==============================================================================================
//  common.h                    cruxcan controller common abstractions [**...]
//  CRUXELL.RND.software		yh kim, jin lee
//  2017.11.06					2018.03.12
//------------------------------+---------------------------------------------------------------
#include "cruxcan_defines.h"
#include <algorithm>
#include <array>
#include <bitset>
#include <iomanip>
#include <vector>
#include <list>
#include <string>
#include <type_traits>
#include <sstream>
#include <fstream>
#include <cctype>
#include <atomic>
#include <mutex>
#include <map>
#include <cassert>
#include <cmath>
#include <thread>
#include <chrono>
#include <ctime>
#include <functional>
#include <random>
#include <complex>
#include <ppl.h>
#include "base.h"
#include "point.h"
#include "strings.h"
#include "rect.h"
#include "util.h"
#include "common_type.h"


#define	SAVE_STEP_GEO_IMAGE				0
#define	REMOVE_STRIPE_OLD1				1
#define REMOVE_STRIPE_KALEDIO1			2
#ifdef FACTORY_MODE
	#define EMPTY_IMAGE_DEFAULT	0
#else
//20200416 »çÀå´Ô Áö½Ã·Î  ¸ù¶¥ ´Ù ²û
	#define EMPTY_IMAGE_DEFAULT	0
#endif


extern double g_dbandFractionRatio;
extern int g_ifilterThickness;
#define DBANDFRACTIONRATIO_DEFAULT		128.
#define IFILTERTHICKNESS_DEFAULT		16
#define TEST_BHLEE_ONLY				0


#ifndef P_N_W
#define P_N_W	400
#define P_N_H	60
#endif


#pragma  warning(disable:4996)

using namespace crux;

using raw_t						= u16;
using geo_t						= f32;
using fin_t						= u16;

//------------------------------+---------------------------------------------------------------

#define cm
#define mm
#define um

#define Gry
#define ADU

//------------------------------+---------------------------------------------------------------

cex i32 support_resolutions[]
{
    25 um,
    50 um,
};

cex i32 facets					= 8;
cex i32 um_scale				= 1000 um / support_resolutions[ 0 ];
cex i32 overrate				= 1;
cex i32 geo_x					= 1536;
cex i32 geo_y = 2360;		// 2019.03.06

cex i32 raw_x					= geo_x;
cex i32 raw_y					= geo_y;

cex i32 size0_w = 880;
cex i32 size0_h = 1240;
cex i32 size1_w = 960;
cex i32 size1_h = 1600;

cex i32 size2_w = 1240;
cex i32 size2_h = 1640;
cex i32 size3_w = 1080;
cex i32 size3_h = 2160;

//------------------------------+---------------------------------------------------------------

struct							TEX
{
	P<void>						id;
	XY<i32>						size;
	int							channels;
};

//------------------------------+---------------------------------------------------------------

AFX_EXT_API bool load_tiff(TEX& t, co wchar_t* str);
AFX_EXT_API bool get_image_of_tiff(co wchar_t* wstr, IN OUT int &w, IN OUT int &h);

extern bool g_bemptyimage;

//==============================================================================================

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(a)	{ if (a) { delete [] a; a = NULL; } }
#endif
#ifndef SAFE_DELETE
#define SAFE_DELETE(a)			{ if (a) { delete a; a = NULL; } }
#endif

#ifndef WM_REFRESH_LIST
#define WM_REFRESH_LIST	(WM_USER + 1013)
#endif

//==============================================================================================

typedef unsigned long	width;
