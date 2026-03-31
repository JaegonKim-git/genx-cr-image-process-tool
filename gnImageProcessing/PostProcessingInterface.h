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



extern "C" GNIMAGEPROCESSING_API void __cdecl PostProcessingParametersSetting(IMG_PROCESS_PARAM params);
// 2024-04-22. jg kim. 영상처리리턴값 추가
extern "C" GNIMAGEPROCESSING_API void __cdecl PostProcess(unsigned short* result_img, int &result_width, int &result_height, bool &bResult,
	unsigned short* source_img, int source_width, int source_height, bool bSaveImageProcessingStep);
// 2024-04-26. jg kim. 로그파일 작성을 위함
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
//extern "C" GNIMAGEPROCESSING_API void __cdecl setLogFileNameIP(char *name);
//extern "C" GNIMAGEPROCESSING_API void __cdecl writelogIP(char *filename, char *contents);
// Line correction function
extern "C" GNIMAGEPROCESSING_API void __cdecl ApplyLinecorrect(float* Output_img, float* Input_img, int nWidth, int nHeight, int nPreTerm, int nPostTerm, bool bVertical);