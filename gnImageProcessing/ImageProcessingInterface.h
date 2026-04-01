// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#ifdef							GNIMAGEPROCESSING_EXPORTS
#define							GNIMAGEPROCESSING_API			__declspec(dllexport)
#else
#define							GNIMAGEPROCESSING_API			__declspec(dllimport)
#endif

//#include <winsock.h>
#include <stdio.h>
#include "inline_imgProc.h"
#include "HardwareInfo.h"

#ifdef __cplusplus
extern "C" {
#endif

 GNIMAGEPROCESSING_API void __cdecl PostProcessingParametersSetting(IMG_PROCESS_PARAM params);
// 2024-04-22. jg kim. 영상처리리턴값 추가
 GNIMAGEPROCESSING_API void __cdecl PostProcess(unsigned short* result_img, int &result_width, int &result_height, bool &bResult,
	unsigned short* source_img, int source_width, int source_height, bool bSaveImageProcessingStep);
// 2024-04-26. jg kim. 로그파일 작성을 위함
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
// GNIMAGEPROCESSING_API void __cdecl setLogFileNameIP(char *name);
// GNIMAGEPROCESSING_API void __cdecl writelogIP(char *filename, char *contents);
// Line correction function
 GNIMAGEPROCESSING_API void __cdecl ApplyLinecorrect(float* Output_img, float* Input_img, int nWidth, int nHeight, int nPreTerm, int nPostTerm, bool bVertical);

 // MPPC 관련 함수들
GNIMAGEPROCESSING_API int __cdecl HI_Mppc_Convert_VoltageToDigitalValue(int Voltage);
GNIMAGEPROCESSING_API int __cdecl HI_Mppc_Convert_DigitalValueToVoltage(int nDigitalValue);

// BLDC 관련 함수들
GNIMAGEPROCESSING_API unsigned int __cdecl HI_getBldcSpeed(double fRPM, unsigned int nClock);
GNIMAGEPROCESSING_API double __cdecl HI_getBldcRPM(unsigned int BldcSpeed, unsigned int nClock);

// 유효성 검사 함수들
GNIMAGEPROCESSING_API bool __cdecl HI_CheckValidCoefficients(unsigned short* pMeans, float* pCoefficients);
GNIMAGEPROCESSING_API bool __cdecl HI_CheckDoorClosed(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);
GNIMAGEPROCESSING_API bool __cdecl HI_CheckValidIpIndex(int* pnIpIdx);

// 하드웨어 정보 디코딩 함수들
GNIMAGEPROCESSING_API bool __cdecl HI_decodeGenorayTag(
	unsigned short* pRawImage, 
	int nWidth, 
	unsigned short* pMeans, 
	float* pCoefficients, 
	int* pnIpIdx,
	double* pfdate, 
	unsigned short* pcur_info_index, 
	unsigned char* pMac,
	unsigned short* pnLeftLaserOnPos, 
	unsigned short* pnLeftLaserOffPos,
	unsigned short* pnRightLaserOnPos, 
	unsigned short* pnRightLaserOffPos,
	unsigned short* pnLeftLineStartPos, 
	unsigned short* pnLeftLineEndPos,
	unsigned short* pnRightLineStartPos, 
	unsigned short* pnRightLineEndPos);

GNIMAGEPROCESSING_API bool __cdecl HI_decodeGenorayTag_Simple(
	unsigned short* pRawImage, 
	int nWidth, 
	stDecodedHardwareParam* pParam);

GNIMAGEPROCESSING_API bool __cdecl HI_decodeTag(
	unsigned short* pRawImage, 
	int* pb0, 
	int* pb1, 
	int* pb2);

// 이미지 처리 함수들
GNIMAGEPROCESSING_API bool __cdecl HI_checkLowResolution(unsigned short* img, int width, int height);
GNIMAGEPROCESSING_API bool __cdecl HI_checkLowXrayPower(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);
GNIMAGEPROCESSING_API void __cdecl HI_SuppressAmbientLight(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);
GNIMAGEPROCESSING_API unsigned short* __cdecl HI_WriteTag(unsigned short* source_img, int width, int height);

// Calibration 관련 함수들
GNIMAGEPROCESSING_API void __cdecl HI_getDefaultCalibrationCoefficients(
	unsigned short* pMeans, 
	float* pCoefficients, 
	int nmean);

GNIMAGEPROCESSING_API void __cdecl calculateCoefficients(float *coefficients, unsigned short *images, int width, int height, int L, int T, int W, int H, int count);

GNIMAGEPROCESSING_API void __cdecl ParametersSetting(float *coefs, unsigned short* means, int nMeans);
GNIMAGEPROCESSING_API int __cdecl GetCorrectWobbleResult();// 2026-01-12. jg kim. Wobble 보정 결과를 받기 위해
// 2026-01-13. jg kim. 장비에서 넘겨주는 반사판 영역의 Laser On/Off x 좌표 및, 세로 줄 범위(시작/끝 x 좌표)를 Wobble 보정에 활용하기 위함.
GNIMAGEPROCESSING_API void __cdecl SetFwWobblePos(int nLaserOnPos, int nLaserOffPos, int nLineStartPos, int nLineEndPos);
// 2024-04-22. jg kim. 영상 보정결과 리턴값 추가
// 2024-12-09. jg kim. 공정툴용 영상처리와 통합
GNIMAGEPROCESSING_API void __cdecl ProcessAPI(unsigned short **result_img, int &result_width, int &result_height, bool &bResult, unsigned short *ImgBin,
	unsigned short *source_img, int source_width, int source_height, int nIpIndex, bool bSaveCorrectionStep, int nImageProcessMode, bool bDoorOpen);
/* mode 
0: image correction (기존 ProcessAPI)
1: image correction_PE
2: image correction_PE, Extract IP
*/
// 2025-01-22. jg kim. 공정툴에서 BLDC shift index를 계산하던 알고리즘의 문제점을 확인하고,
// ImageProcessor 클래스의 methods를 이용하여 BLDC shift index를 계산하는 알고리즘을 개선하기 위해 구현
GNIMAGEPROCESSING_API int __cdecl GetBldcShiftIndex(float **fpt_CornerInfo, int &nIpIndex, unsigned short *ImgBin, int width, int height);
	// 2024-03-28. jg kim. 장비에서 보내 준 IP index를 이용하기 위하여 함수의 인자 추가
GNIMAGEPROCESSING_API void __cdecl getPspSize(int *width, int *height, int index);
// 2024-03-28. jg kim. Blanck scan을 한 경우 장비에서 보내 준 IP index를 이용하여 IP크기를 받아와 영상보정 결과영상으로 반환.
// 2024-04-22. jg kim. log 작성을 위한 함수 추가
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
//GNIMAGEPROCESSING_API void __cdecl setLogFileName(char *name);
//GNIMAGEPROCESSING_API void __cdecl writelogIC(char *filename, char *contents);
// 2024-06-25. jg kim. Cruxell 종속성 제거를 위해 구현
GNIMAGEPROCESSING_API void __cdecl FindIPArea(unsigned short *input, int width, int height, 
	int &oWidth, int &oHeight, int &cx, int &cy, float &angle);
// 2024-07-08. jg kim. 영상 보정을 단계적으로 테스트하기 위해 외부 접근 함수 추가
// 공정툴 쪽으로만 유지
// 2024-10-31. jg kim. 공정툴에서도 영상처리를 하는 경우에는 RemoveWormPattern 함수를 사용하도록 옵션 추가
// 2024-12-09. jg kim. PreprocessForParameterExtraction, IP_Extract 제거

#ifdef __cplusplus
}
#endif