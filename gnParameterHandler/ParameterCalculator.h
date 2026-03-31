#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

#define OUTPUT
#define INPUT

#include "../predix_sdk/common_type.h"

#ifndef DLL_DECLSPEC
#ifdef							DLL_EXPORT
#define							DLL_DECLSPEC					__declspec(dllexport)     
#else
#define							DLL_DECLSPEC					__declspec(dllimport)     
#endif
#endif

class DLL_DECLSPEC CParameteCalculator
{
public:
	CParameteCalculator();
	~CParameteCalculator();
	
	// 각종 speed는 기본 값에 scale을 하도록 한다.
	// 수단은 마련해 놨지만 현재는 그냥 사용하자.
	// 추후 기구가 변경되거나 신규 장비를 만들때 사용계획.


	void setNormalSpeed(double value);
	// positioning time
	// backward offset length
	// parking length
	double getNormalSpeed();
	double getScanSpeed();
	double getEraseSpeed();
	double getStartSpeed();
	double getMaxSpeed();
	double getStep();
	double getDuration();
	double getScale();
	double getBackwaredOffset();
	void getDistanceToPositionEntry(double &position, double &entry, int nIpIndex);
	void getAccelerationDistanceTime(double &distance, double &time, double startSpeed, double targetSpeed, bool bAccleration = true);
	double getMovingDistance(double speed, double time);
	double getMovingTime(double speed, double distance);
	double get_MotorPulsePeriod(double mmPerSec);
	double getIP_Height(int nIndex);
	double get_mmPerSec(double motor_PulsePeriod);
	double getDistanceToBackward();
	double getDistanceToEject();
	double getDistanceToErase();
	// 등속 이동시 이동시간.
	void getMovingTime(OUTPUT double &fMovingTime, INPUT double fMovingDistance, double fStartSpeed, double fConstantSpeed, double fEndSpeed, double fOffset = 0);
	// 가속, 등속이동, 감속을 고려한 이동시간 계산.
	void getMovingTimeToEject(OUTPUT double &fMovingTime, INPUT double fMovingDistance, double fStartSpeed, double fEndSpeed, double fOffset = 0);
	// 2025-06-18. jg kim. IP Eject time 계산하는 함수.
	double getMainClockFrequency();
	double getMega();
	unsigned int getClocksFromSec(double fSec);


	void setScanSpeed(double value);	// scan
	void setEraseSpeed(double value);	// erase
	void setStartSpeed(double value);
	void setMaxSpeed(double value);
	void setStep(double value);
	void setDuration(double value);
	void setScale(double value);// 계산과 맞지 않을 경우 사용하기 위한 보정계수.
	void setBackwaredOffset(double value);
	/*
		fDistanceToScanLaser = 50.5; // mm. home_init sensor(photo sensor) to scan laser
		fDistanceToEraseLed = 12.15; // mm
		fDistanceToEject = 3.836; // mm
		fDistanceToBackward = 33; // mm
	*/

private:
	HardwareParameters m_param;
	double m_fBackwaredOffset;	
};

