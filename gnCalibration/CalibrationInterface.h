// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef DLL_DECLSPEC
#ifdef							DLL_EXPORT
#define							DLL_DECLSPEC					__declspec(dllexport)     
#else
#define							DLL_DECLSPEC					__declspec(dllimport)     
#endif
#endif


extern "C" DLL_DECLSPEC void __cdecl calculateCoefficients(float *coefficients, unsigned short *images, int width, int height, int L, int T, int W, int H, int count);

extern "C" DLL_DECLSPEC void __cdecl ParametersSetting(float *coefs, unsigned short* means, int nMeans);
extern "C" DLL_DECLSPEC void __cdecl SetMtfMode(bool bMTF);// 2026-01-12. jg kim. MTF 모드 설정 위해
extern "C" DLL_DECLSPEC int __cdecl GetCorrectWobbleResult();// 2026-01-12. jg kim. Wobble 보정 결과를 받기 위해
// 2026-01-13. jg kim. 장비에서 넘겨주는 반사판 영역의 Laser On/Off x 좌표 및, 세로 줄 범위(시작/끝 x 좌표)를 Wobble 보정에 활용하기 위함.
extern "C" DLL_DECLSPEC void __cdecl SetFwWobblePos(int nLaserOnPos, int nLaserOffPos, int nLineStartPos, int nLineEndPos);
// 2024-04-22. jg kim. 영상 보정결과 리턴값 추가
// 2024-12-09. jg kim. 공정툴용 영상처리와 통합
extern "C" DLL_DECLSPEC void __cdecl ProcessAPI(unsigned short **result_img, int &result_width, int &result_height, bool &bResult, unsigned short *ImgBin,
	unsigned short *source_img, int source_width, int source_height, int nIpIndex, bool bSaveCorrectionStep, int nImageProcessMode, bool bDoorOpen);
/* mode 
0: image correction (기존 ProcessAPI)
1: image correction_PE
2: image correction_PE, Extract IP
*/
// 2025-01-22. jg kim. 공정툴에서 BLDC shift index를 계산하던 알고리즘의 문제점을 확인하고,
// ImageProcessor 클래스의 methods를 이용하여 BLDC shift index를 계산하는 알고리즘을 개선하기 위해 구현
extern "C" DLL_DECLSPEC int __cdecl GetBldcShiftIndex(float **fpt_CornerInfo, int &nIpIndex, unsigned short *ImgBin, int width, int height);
	// 2024-03-28. jg kim. 장비에서 보내 준 IP index를 이용하기 위하여 함수의 인자 추가
extern "C" DLL_DECLSPEC void __cdecl getPspSize(int *width, int *height, int index);
// 2024-03-28. jg kim. Blanck scan을 한 경우 장비에서 보내 준 IP index를 이용하여 IP크기를 받아와 영상보정 결과영상으로 반환.
// 2024-04-22. jg kim. log 작성을 위한 함수 추가
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
//extern "C" DLL_DECLSPEC void __cdecl setLogFileName(char *name);
//extern "C" DLL_DECLSPEC void __cdecl writelogIC(char *filename, char *contents);
// 2024-06-25. jg kim. Cruxell 종속성 제거를 위해 구현
extern "C" DLL_DECLSPEC void __cdecl FindIPArea(unsigned short *input, int width, int height, 
	int &oWidth, int &oHeight, int &cx, int &cy, float &angle);
// 2024-07-08. jg kim. 영상 보정을 단계적으로 테스트하기 위해 외부 접근 함수 추가
// 공정툴 쪽으로만 유지
// 2024-10-31. jg kim. 공정툴에서도 영상처리를 하는 경우에는 RemoveWormPattern 함수를 사용하도록 옵션 추가
// 2024-12-09. jg kim. PreprocessForParameterExtraction, IP_Extract 제거