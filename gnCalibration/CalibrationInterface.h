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

