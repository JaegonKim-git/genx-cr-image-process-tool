#pragma once
#include <vector>


#ifdef DLL_EXPORT
#define DLL_DECLSPEC					__declspec(dllexport)
#else
#define DLL_DECLSPEC					__declspec(dllimport)
#endif


enum ERROR_CODE : int
{
	SUCCESS = 1,
	ER_DLL_FAIL_FIND_IP_START_END = -101,	// check_image_ip_start_end 비정상 이미지로 보임
	ER_DLL_FAIL_FIND_COUNTOUR = -102,		// countour 찾기 실패
	ER_DLL_FAIL_MAKSKING = -103,			// 마스킹과정에서 발견된 에러. 대부분 밝기가 너무낮은 이미지
	ER_KALEIDO_API_vaildArea = -104,		// 칼레이도 vaildArea에러
	ER_KALEIDO_API_Enhance6 = -105,			// 칼레이도 Enhance6에러

	ER_SW_NONE = -1000,
	ER_SW_DIALOG_CANCLE = 1001,
	ER_SW_OPEN_WRONG_RAW_SIZE = 1002,		// 인식할수없는 RAW Size 파일을 열었음
	ER_SW_OPEN_WRONG_IMAGE = 1003,			// CRUXCAN 테스트용 이미지로 적합하지 않은포맷
	ER_SW_NO_PP = 1004,						// 의도적으로 프로세싱을 진행하지 않는 옵션 선택상태
};

// 2023.02.13 post-processing mode 추가
enum class PostProcessMode
{
	None,
	Enhanced,
	Natural
};

struct crxSIZE
{
	int width;
	int height;
};

struct crxPOINT
{
	int x, y;
};

struct crxPOINTF
{
	float x, y;
};

struct crxPOINT3F
{
	float x, y, z;
};

enum class RESOLUTION : unsigned char
{
	High = 0,
	Low = 1,
};

struct InfoProcessing
{
	// read form ini
	//int							size_tgt_belt;

	RESOLUTION					resolution;
	crxSIZE						size_raw;
	crxSIZE						size_pp;
	int							image_end;
	int							filter; // 0 : error / 4 : 4T / 6 : 6T
	int							bri_avr_bg;
	int							pos_ip_start;
	int							pos_ip_end;
	int							size_ip; // 0, 1, 2, 3
	int							lcd_emulation; // 0 : fail / 1 : success

	int							result;
//	unsigned short* result_img;
//	cv::Mat result_img;
	std::vector<unsigned short> result_img;
};

struct							InfoIP
{
	int							width;
	int							height;
	float						rad;
};

struct CRX_RECT2
{
	struct
	{
		int left;
		int right;
		int top;
		int bottom;
	} box;
	struct
	{
		float anlge;
		int cx;
		int cy;
		int width;
		int height;
	} rot;
};

/*
extern "C"
{
	DLL_DECLSPEC CRX_RECT2 __cdecl pp_find_ip(int width, int height, unsigned short* cs_img);

	DLL_DECLSPEC void __cdecl pp_PostSetting(Option_PostProcessing option);
	DLL_DECLSPEC [[nodiscard]] InfoProcessing* __cdecl pp_Processing_crossroads(int width, int height, unsigned short* cs_img);
	DLL_DECLSPEC [[nodiscard]] InfoProcessing* __cdecl pp_GetEmptySize2(void);
}
*/