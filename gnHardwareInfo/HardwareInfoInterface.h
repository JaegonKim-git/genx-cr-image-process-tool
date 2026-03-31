// Added by JG Kim, on 2026/03/11.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef DLL_DECLSPEC
#ifdef							DLL_EXPORT
#define							DLL_DECLSPEC					__declspec(dllexport)     
#else
#define							DLL_DECLSPEC					__declspec(dllimport)     
#endif
#endif

#include "HardwareInfo.h"

#define OUTPUT
#define INPUT

// CHardwareInfo를 위한 C 인터페이스
// C++ 클래스를 직접 export하는 대신 C 인터페이스를 통해 DLL 경계를 넘습니다.

#ifdef __cplusplus
extern "C" {
#endif

// MPPC 관련 함수들
DLL_DECLSPEC int __cdecl HI_Mppc_Convert_VoltageToDigitalValue(int Voltage);
DLL_DECLSPEC int __cdecl HI_Mppc_Convert_DigitalValueToVoltage(int nDigitalValue);

// BLDC 관련 함수들
DLL_DECLSPEC unsigned int __cdecl HI_getBldcSpeed(double fRPM, unsigned int nClock);
DLL_DECLSPEC double __cdecl HI_getBldcRPM(unsigned int BldcSpeed, unsigned int nClock);

// 유효성 검사 함수들
DLL_DECLSPEC bool __cdecl HI_CheckValidCoefficients(unsigned short* pMeans, float* pCoefficients);
DLL_DECLSPEC bool __cdecl HI_CheckDoorClosed(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);
DLL_DECLSPEC bool __cdecl HI_CheckValidIpIndex(int* pnIpIdx);

// 하드웨어 정보 디코딩 함수들
DLL_DECLSPEC bool __cdecl HI_decodeGenorayTag(
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

DLL_DECLSPEC bool __cdecl HI_decodeGenorayTag_Simple(
	unsigned short* pRawImage, 
	int nWidth, 
	stDecodedHardwareParam* pParam);

DLL_DECLSPEC bool __cdecl HI_decodeTag(
	unsigned short* pRawImage, 
	int* pb0, 
	int* pb1, 
	int* pb2);

// 이미지 처리 함수들
DLL_DECLSPEC bool __cdecl HI_checkLowResolution(unsigned short* img, int width, int height);
DLL_DECLSPEC bool __cdecl HI_checkLowXrayPower(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);
DLL_DECLSPEC void __cdecl HI_SuppressAmbientLight(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);
DLL_DECLSPEC unsigned short* __cdecl HI_WriteTag(unsigned short* source_img, int width, int height);

// Calibration 관련 함수들
DLL_DECLSPEC void __cdecl HI_getDefaultCalibrationCoefficients(
	unsigned short* pMeans, 
	float* pCoefficients, 
	int nmean);

#ifdef __cplusplus
}
#endif
