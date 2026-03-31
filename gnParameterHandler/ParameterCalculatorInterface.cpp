// Added by JG Kim
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "pch.h"
#include "ParameterCalculatorInterface.h"
#include "ParameterCalculator.h"

// 생성 및 소멸
ParameterCalculatorHandle __cdecl CreateParameterCalculator(void)
{
	CParameteCalculator* pCalc = new CParameteCalculator();
	return static_cast<ParameterCalculatorHandle>(pCalc);
}

void __cdecl DestroyParameterCalculator(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		delete pCalc;
	}
}

// Setter 함수들
void __cdecl PC_SetNormalSpeed(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setNormalSpeed(value);
	}
}

void __cdecl PC_SetScanSpeed(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setScanSpeed(value);
	}
}

void __cdecl PC_SetEraseSpeed(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setEraseSpeed(value);
	}
}

void __cdecl PC_SetStartSpeed(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setStartSpeed(value);
	}
}

void __cdecl PC_SetMaxSpeed(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setMaxSpeed(value);
	}
}

void __cdecl PC_SetStep(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setStep(value);
	}
}

void __cdecl PC_SetDuration(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setDuration(value);
	}
}

void __cdecl PC_SetScale(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setScale(value);
	}
}

void __cdecl PC_SetBackwaredOffset(ParameterCalculatorHandle handle, double value)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->setBackwaredOffset(value);
	}
}

// Getter 함수들
double __cdecl PC_GetNormalSpeed(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getNormalSpeed();
	}
	return 0.0;
}

double __cdecl PC_GetScanSpeed(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getScanSpeed();
	}
	return 0.0;
}

double __cdecl PC_GetEraseSpeed(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getEraseSpeed();
	}
	return 0.0;
}

double __cdecl PC_GetStartSpeed(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getStartSpeed();
	}
	return 0.0;
}

double __cdecl PC_GetMaxSpeed(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getMaxSpeed();
	}
	return 0.0;
}

double __cdecl PC_GetStep(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getStep();
	}
	return 0.0;
}

double __cdecl PC_GetDuration(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getDuration();
	}
	return 0.0;
}

double __cdecl PC_GetScale(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getScale();
	}
	return 0.0;
}

double __cdecl PC_GetBackwaredOffset(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getBackwaredOffset();
	}
	return 0.0;
}

double __cdecl PC_GetDistanceToBackward(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getDistanceToBackward();
	}
	return 0.0;
}

double __cdecl PC_GetDistanceToEject(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getDistanceToEject();
	}
	return 0.0;
}

double __cdecl PC_GetDistanceToErase(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getDistanceToErase();
	}
	return 0.0;
}

double __cdecl PC_GetMainClockFrequency(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getMainClockFrequency();
	}
	return 0.0;
}

double __cdecl PC_GetMega(ParameterCalculatorHandle handle)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getMega();
	}
	return 0.0;
}

// 계산 함수들
void __cdecl PC_GetDistanceToPositionEntry(ParameterCalculatorHandle handle, double *position, double *entry, int nIpIndex)
{
	if (handle != nullptr && position != nullptr && entry != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->getDistanceToPositionEntry(*position, *entry, nIpIndex);
	}
}

void __cdecl PC_GetAccelerationDistanceTime(ParameterCalculatorHandle handle, double *distance, double *time, double startSpeed, double targetSpeed, bool bAccleration)
{
	if (handle != nullptr && distance != nullptr && time != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->getAccelerationDistanceTime(*distance, *time, startSpeed, targetSpeed, bAccleration);
	}
}

double __cdecl PC_GetMovingDistance(ParameterCalculatorHandle handle, double speed, double time)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getMovingDistance(speed, time);
	}
	return 0.0;
}

double __cdecl PC_GetMovingTime(ParameterCalculatorHandle handle, double speed, double distance)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getMovingTime(speed, distance);
	}
	return 0.0;
}

double __cdecl PC_Get_MotorPulsePeriod(ParameterCalculatorHandle handle, double mmPerSec)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->get_MotorPulsePeriod(mmPerSec);
	}
	return 0.0;
}

double __cdecl PC_GetIP_Height(ParameterCalculatorHandle handle, int nIndex)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getIP_Height(nIndex);
	}
	return 0.0;
}

double __cdecl PC_Get_mmPerSec(ParameterCalculatorHandle handle, double motor_PulsePeriod)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->get_mmPerSec(motor_PulsePeriod);
	}
	return 0.0;
}

void __cdecl PC_GetMovingTime2(ParameterCalculatorHandle handle, double *fMovingTime, double fMovingDistance, double fStartSpeed, double fConstantSpeed, double fEndSpeed, double fOffset)
{
	if (handle != nullptr && fMovingTime != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->getMovingTime(*fMovingTime, fMovingDistance, fStartSpeed, fConstantSpeed, fEndSpeed, fOffset);
	}
}

void __cdecl PC_GetMovingTimeToEject(ParameterCalculatorHandle handle, double *fMovingTime, double fMovingDistance, double fStartSpeed, double fEndSpeed, double fOffset)
{
	if (handle != nullptr && fMovingTime != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		pCalc->getMovingTimeToEject(*fMovingTime, fMovingDistance, fStartSpeed, fEndSpeed, fOffset);
	}
}

unsigned int __cdecl PC_GetClocksFromSec(ParameterCalculatorHandle handle, double fSec)
{
	if (handle != nullptr)
	{
		CParameteCalculator* pCalc = static_cast<CParameteCalculator*>(handle);
		return pCalc->getClocksFromSec(fSec);
	}
	return 0;
}
