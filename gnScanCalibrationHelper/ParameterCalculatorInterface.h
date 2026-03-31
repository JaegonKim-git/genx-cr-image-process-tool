// Added by JG Kim
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#ifndef DLL_DECLSPEC
#ifdef							DLL_EXPORT
#define							DLL_DECLSPEC					__declspec(dllexport)     
#else
#define							DLL_DECLSPEC					__declspec(dllimport)     
#endif
#endif

#define OUTPUT
#define INPUT

// CParameteCalculator를 위한 C 인터페이스
// C++ 클래스를 직접 export하는 대신 C 인터페이스를 통해 DLL 경계를 넘습니다.

#ifdef __cplusplus
extern "C" {
#endif

// Calculator 핸들 타입 (불투명 포인터)
typedef void* ParameterCalculatorHandle;

// 생성 및 소멸
DLL_DECLSPEC ParameterCalculatorHandle __cdecl CreateParameterCalculator(void);
DLL_DECLSPEC void __cdecl DestroyParameterCalculator(ParameterCalculatorHandle handle);

// Setter 함수들
DLL_DECLSPEC void __cdecl PC_SetNormalSpeed(ParameterCalculatorHandle handle, double value);
DLL_DECLSPEC void __cdecl PC_SetScanSpeed(ParameterCalculatorHandle handle, double value);
DLL_DECLSPEC void __cdecl PC_SetEraseSpeed(ParameterCalculatorHandle handle, double value);
DLL_DECLSPEC void __cdecl PC_SetStartSpeed(ParameterCalculatorHandle handle, double value);
DLL_DECLSPEC void __cdecl PC_SetMaxSpeed(ParameterCalculatorHandle handle, double value);
DLL_DECLSPEC void __cdecl PC_SetStep(ParameterCalculatorHandle handle, double value);
DLL_DECLSPEC void __cdecl PC_SetDuration(ParameterCalculatorHandle handle, double value);
DLL_DECLSPEC void __cdecl PC_SetScale(ParameterCalculatorHandle handle, double value);
DLL_DECLSPEC void __cdecl PC_SetBackwaredOffset(ParameterCalculatorHandle handle, double value);

// Getter 함수들
DLL_DECLSPEC double __cdecl PC_GetNormalSpeed(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetScanSpeed(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetEraseSpeed(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetStartSpeed(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetMaxSpeed(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetStep(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetDuration(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetScale(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetBackwaredOffset(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetDistanceToBackward(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetDistanceToEject(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetDistanceToErase(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetMainClockFrequency(ParameterCalculatorHandle handle);
DLL_DECLSPEC double __cdecl PC_GetMega(ParameterCalculatorHandle handle);

// 계산 함수들
DLL_DECLSPEC void __cdecl PC_GetDistanceToPositionEntry(ParameterCalculatorHandle handle, OUTPUT double *position, OUTPUT double *entry, INPUT int nIpIndex);
DLL_DECLSPEC void __cdecl PC_GetAccelerationDistanceTime(ParameterCalculatorHandle handle, OUTPUT double *distance, OUTPUT double *time, INPUT double startSpeed, INPUT double targetSpeed, INPUT bool bAccleration);
DLL_DECLSPEC double __cdecl PC_GetMovingDistance(ParameterCalculatorHandle handle, double speed, double time);
DLL_DECLSPEC double __cdecl PC_GetMovingTime(ParameterCalculatorHandle handle, double speed, double distance);
DLL_DECLSPEC double __cdecl PC_Get_MotorPulsePeriod(ParameterCalculatorHandle handle, double mmPerSec);
DLL_DECLSPEC double __cdecl PC_GetIP_Height(ParameterCalculatorHandle handle, int nIndex);
DLL_DECLSPEC double __cdecl PC_Get_mmPerSec(ParameterCalculatorHandle handle, double motor_PulsePeriod);
DLL_DECLSPEC void __cdecl PC_GetMovingTime2(ParameterCalculatorHandle handle, OUTPUT double *fMovingTime,
	INPUT double fMovingDistance, INPUT double fStartSpeed, INPUT double fConstantSpeed, INPUT double fEndSpeed, INPUT double fOffset);
DLL_DECLSPEC void __cdecl PC_GetMovingTimeToEject(ParameterCalculatorHandle handle, OUTPUT double *fMovingTime, INPUT double fMovingDistance, INPUT double fStartSpeed, INPUT double fEndSpeed, INPUT double fOffset);
DLL_DECLSPEC unsigned int __cdecl PC_GetClocksFromSec(ParameterCalculatorHandle handle, double fSec);

#ifdef __cplusplus
}
#endif
