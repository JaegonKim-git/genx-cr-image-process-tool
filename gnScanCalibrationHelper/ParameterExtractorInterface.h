// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#ifdef							DLL_EXPORT
#define							DLL_DECLSPEC					__declspec(dllexport)     
#else
#define							DLL_DECLSPEC					__declspec(dllimport)     
#endif


#define OUTPUT
#define INPUT
// 2024-11-12. jg kim. 실행 결과를 받아오도록 수정
extern "C" DLL_DECLSPEC bool __cdecl getScannerTunningParameters(float &fAspectRatio, int &nBldcShiftPixels, int &x, int &y, int &nBoundingWidth, int &nBoundingHeight, unsigned short *imgRaw, int nImgWidth, int nImgHeight, int nKernelSize, bool bUseCorrelation = true);
extern "C" DLL_DECLSPEC void __cdecl setLogFileNamePE(char *name);
extern "C" DLL_DECLSPEC void __cdecl writelogPE(char *filename, char *contents);