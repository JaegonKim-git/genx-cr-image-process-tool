// Added by JG Kim, on 2026/03/11.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "HardwareInfoInterface.h"
#include "HardwareInfo.h"

// MPPC 관련 함수들
int __cdecl HI_Mppc_Convert_VoltageToDigitalValue(int Voltage)
{
	CHardwareInfo hi;
	return hi.Mppc_Convert_VoltageToDigitalValue(Voltage);
}

int __cdecl HI_Mppc_Convert_DigitalValueToVoltage(int nDigitalValue)
{
	CHardwareInfo hi;
	return hi.Mppc_Convert_DigitalValueToVoltage(nDigitalValue);
}

// BLDC 관련 함수들
unsigned int __cdecl HI_getBldcSpeed(double fRPM, unsigned int nClock)
{
	CHardwareInfo hi;
	return hi.getBldcSpeed(fRPM, nClock);
}

double __cdecl HI_getBldcRPM(unsigned int BldcSpeed, unsigned int nClock)
{
	CHardwareInfo hi;
	return hi.getBldcRPM(BldcSpeed, nClock);
}

// 유효성 검사 함수들
bool __cdecl HI_CheckValidCoefficients(unsigned short* pMeans, float* pCoefficients)
{
	if (pMeans == nullptr || pCoefficients == nullptr)
		return false;

	CHardwareInfo hi;
	return hi._CheckValidCoefficients(pMeans, pCoefficients);
}

bool __cdecl HI_CheckDoorClosed(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes)
{
	if (pRawImage == nullptr)
		return false;

	CHardwareInfo hi;
	return hi._CheckDoorClosed(pRawImage, nWidth, nHeight, nLowRes);
}

bool __cdecl HI_CheckValidIpIndex(int* pnIpIdx)
{
	if (pnIpIdx == nullptr)
		return false;

	CHardwareInfo hi;
	return hi._CheckValidIpIndex(*pnIpIdx);
}

// 하드웨어 정보 디코딩 함수들
bool __cdecl HI_decodeGenorayTag(
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
	unsigned short* pnRightLineEndPos)
{
	if (pRawImage == nullptr || pMeans == nullptr || pCoefficients == nullptr || 
		pnIpIdx == nullptr || pfdate == nullptr || pcur_info_index == nullptr)
		return false;

	CHardwareInfo hi;
	return hi.decodeGenorayTag(
		pRawImage, nWidth, pMeans, pCoefficients, *pnIpIdx,
		*pfdate, *pcur_info_index, pMac,
		*pnLeftLaserOnPos, *pnLeftLaserOffPos,
		*pnRightLaserOnPos, *pnRightLaserOffPos,
		*pnLeftLineStartPos, *pnLeftLineEndPos,
		*pnRightLineStartPos, *pnRightLineEndPos);
}

bool __cdecl HI_decodeGenorayTag_Simple(
	unsigned short* pRawImage,
	int nWidth,
	stDecodedHardwareParam* pParam)
{
	if (pRawImage == nullptr || pParam == nullptr)
		return false;

	CHardwareInfo hi;
	return hi.decodeGenorayTag(pRawImage, nWidth, *pParam);
}

bool __cdecl HI_decodeTag(
	unsigned short* pRawImage,
	int* pb0,
	int* pb1,
	int* pb2)
{
	if (pRawImage == nullptr || pb0 == nullptr || pb1 == nullptr || pb2 == nullptr)
		return false;

	CHardwareInfo hi;
	return hi.decodeTag(pRawImage, *pb0, *pb1, *pb2);
}

// 이미지 처리 함수들
bool __cdecl HI_checkLowResolution(unsigned short* img, int width, int height)
{
	if (img == nullptr)
		return false;

	CHardwareInfo hi;
	return hi.checkLowResolution(img, width, height);
}

bool __cdecl HI_checkLowXrayPower(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes)
{
	if (pRawImage == nullptr)
		return false;

	CHardwareInfo hi;
	return hi.checkLowXrayPower(pRawImage, nWidth, nHeight, nLowRes);
}

void __cdecl HI_SuppressAmbientLight(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes)
{
	if (pRawImage == nullptr)
		return;

	CHardwareInfo hi;
	hi.SuppressAmbientLight(pRawImage, nWidth, nHeight, nLowRes);
}

unsigned short* __cdecl HI_WriteTag(unsigned short* source_img, int width, int height)
{
	if (source_img == nullptr)
		return nullptr;

	CHardwareInfo hi;
	return hi.WriteTag(source_img, width, height);
}

// Calibration 관련 함수들
void __cdecl HI_getDefaultCalibrationCoefficients(
	unsigned short* pMeans,
	float* pCoefficients,
	int nmean)
{
	if (pMeans == nullptr || pCoefficients == nullptr)
		return;

	CHardwareInfo hi;
	hi.getDefaultCalibrationCoefficients(pMeans, pCoefficients, nmean);
}
