#include "stdafx.h"
#include "pch.h"
#include "ParameterCalculator.h"


CParameteCalculator::CParameteCalculator()
	:m_fBackwaredOffset(0)
{
}

CParameteCalculator::~CParameteCalculator()
{
}

void CParameteCalculator::setNormalSpeed(double value)
{
	m_param.fNormalSpeed = value;
}

double CParameteCalculator::getNormalSpeed()
{
	return m_param.fNormalSpeed;
}

void CParameteCalculator::setScanSpeed(double value)
{
	m_param.fScanSpeed = value;
}

double CParameteCalculator::getScanSpeed()
{
	return m_param.fScanSpeed;
}

void CParameteCalculator::setEraseSpeed(double value)
{
	m_param.fEraseSpeed = value;
}

double CParameteCalculator::getEraseSpeed()
{
	return m_param.fEraseSpeed;
}

void CParameteCalculator::setStartSpeed(double value)
{
	m_param.fStartSpeed = value;
}

double CParameteCalculator::getStartSpeed()
{
	return m_param.fStartSpeed;
}

void CParameteCalculator::setMaxSpeed(double value)
{
	m_param.fMaxSpeed = value;
}

double CParameteCalculator::getMaxSpeed()
{
	return m_param.fMaxSpeed;
}

void CParameteCalculator::setStep(double value)
{
	m_param.fStep = value;
}

double CParameteCalculator::getStep()
{
	return m_param.fStep;
}

void CParameteCalculator::setDuration(double value)
{
	m_param.fDuration = value;
}

double CParameteCalculator::getDuration()
{
	return m_param.fDuration;
}

void CParameteCalculator::setScale(double value)
{
	m_param.fScale = value;
}

double CParameteCalculator::getScale()
{
	return m_param.fScale;
}

void CParameteCalculator::setBackwaredOffset(double value)
{
	m_fBackwaredOffset = value;
}

double CParameteCalculator::getBackwaredOffset()
{
	return m_fBackwaredOffset;
}

void CParameteCalculator::getDistanceToPositionEntry(double &position, double &entry, int nIpIndex)
{
	double shade = m_param.fWidthShade; // 21
	double DistanceToBackward = m_param.fDistanceToBackward /*- m_fBackwaredOffset*/; // 33
	double height = getIP_Height(nIpIndex) - shade;
	double distToLaser = m_param.fDistanceToScanLaser; // 50.5
	
	position = DistanceToBackward - height;
	entry = distToLaser - height + m_fBackwaredOffset;
}

double CParameteCalculator::getIP_Height(int nIndex)
{
	return m_param.fHeightIP[nIndex];
}

void CParameteCalculator::getAccelerationDistanceTime
(double & distance/* mm */, double & time/* seconds */, 
	double startSpeed/* mm per seconds */, double targetSpeed/* mm per seconds */, bool bAccleration)
{	
	double dur = m_param.fDuration / (m_param.fMainClockFrequency * m_param.fMega); // sec
	double StartPulse = get_MotorPulsePeriod(startSpeed);
	double TargetPulse = get_MotorPulsePeriod(targetSpeed);
	double pulseDifference = abs(TargetPulse - StartPulse);
	double fSteps = pulseDifference / m_param.fStep;	
	int nSteps = int(round(fSteps))+1;
	double mm_sec = startSpeed;
	double pulse = StartPulse;

	while(nSteps >0)
	{
		double sec = dur * pulse;
		distance += mm_sec * sec;
		time += sec;

		if (bAccleration)
		{
			pulse -= m_param.fStep;
			pulse = pulse < TargetPulse ? TargetPulse : pulse;
		}
		else
		{
			pulse += m_param.fStep;
			pulse = pulse > TargetPulse ? TargetPulse : pulse;
		}
		mm_sec = get_mmPerSec(pulse);
		nSteps -= 1;
	}
}

double CParameteCalculator::getMovingDistance(double speed, double time)
{
	return speed * time;
}

double CParameteCalculator::getMovingTime(double speed, double distance)
{
	if (speed == 0)
		return -1;
	else
		return distance / speed;
}

double CParameteCalculator::get_mmPerSec(double motor_PulsePeriod)
{
	if (motor_PulsePeriod > 0)
		return m_param.fConstant / motor_PulsePeriod;
	else
		return -100; // pulse period가 0 보다 작거나 같을 경우
}

double CParameteCalculator::getDistanceToBackward()
{
	return m_param.fDistanceToBackward;
}

double CParameteCalculator::getDistanceToEject()
{
	return m_param.fDistanceToEject;
}

double CParameteCalculator::getDistanceToErase()
{
	return m_param.fDistanceToEraseLed;
}

void CParameteCalculator::getMovingTime(OUTPUT double &fMovingTime,
	INPUT double fMovingDistance, double fStartSpeed, double fConstantSpeed, double fEndSpeed, double fOffset)
{
	double distAccel = 0;
	double timeAccel = 0;
	double distDecel = 0;
	double timeDecel = 0;
	double distConstSpeed = 0;
	double timeConstSpeed = 0;



	bool bAcceleration = true;
	getAccelerationDistanceTime(distAccel, timeAccel, fStartSpeed, fConstantSpeed, bAcceleration);
	bAcceleration = false; // Deceleration
	getAccelerationDistanceTime(distDecel, timeDecel, fConstantSpeed, fEndSpeed, bAcceleration);

	//if (fMovingDistance > (distAccel + distDecel))
	{
		distConstSpeed = fMovingDistance - distAccel - distDecel - fOffset;
		timeConstSpeed = getMovingTime(fConstantSpeed, distConstSpeed);
		fMovingTime = timeAccel + timeConstSpeed + timeDecel;
	}
	/*else
		fMovingTime = 0;*/
	
}

void CParameteCalculator::getMovingTimeToEject(OUTPUT double & fMovingTime, INPUT double fMovingDistance, double fStartSpeed, double fEndSpeed, double fOffset)
{
	double distDecel = 0;
	double timeDecel = 0;
	double distConstSpeed = 0;
	double timeConstSpeed = 0;

	bool bAcceleration = false; // Deceleration
	getAccelerationDistanceTime(distDecel, timeDecel, fStartSpeed, fEndSpeed, bAcceleration);

	//if (fMovingDistance > (distAccel + distDecel))
	{
		distConstSpeed = fMovingDistance -  distDecel - fOffset;
		timeConstSpeed = getMovingTime(fEndSpeed, distConstSpeed);
		fMovingTime = timeConstSpeed + timeDecel;
	}
}

double CParameteCalculator::getMainClockFrequency()
{
	return m_param.fMainClockFrequency;
}

double CParameteCalculator::getMega()
{
	return m_param.fMega;
}

unsigned int CParameteCalculator::getClocksFromSec(double fSec)
{
	return unsigned int(fSec * getMainClockFrequency() * getMega() );
}

double CParameteCalculator::get_MotorPulsePeriod(double mmPerSec)
{
	if (mmPerSec > 0)
		return m_param.fConstant / mmPerSec;
	else
		return -100; // mmPerSec가 0 보다 작거나 같을 경우
}
