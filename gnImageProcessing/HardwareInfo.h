// Added by JG Kim, on 2024/03/28.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#pragma warning(disable:4305)
#include "../predix_sdk/constants.h" // 2025-02-11. jg kim. 이동된 macro들을 사용하기 위함

#ifndef DLL_DECLSPEC
#ifdef							DLL_EXPORT
#define							DLL_DECLSPEC					__declspec(dllexport)     
#else
#define							DLL_DECLSPEC					__declspec(dllimport)     
#endif
#endif


typedef struct stDecodedHardwareParam
{
	unsigned short nMeans[MAX_CALI_IMAGE_COUNT]; // 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
	float fCoefficients[MAX_CALI_IMAGE_COUNT * (DEGREE_COUNT + 1)];
	int nIpIdx;
	double fdate;
	unsigned short cur_info_index;
	unsigned char nMac[6];
	bool bValidMacAddress = false;
	unsigned char nFwVer[5];
	bool bValidFwVer = false;
	unsigned short wobbleRelates[8];
	bool bValidWobble = false;
	/*
	2026-01-13. jg kim.  장비에서 넘겨주는 반사판 영역의 Laser On/Off x 좌표 및, 세로 줄 범위(시작/끝 x 좌표) decode 추가
	x = 6, y = 0~7
	y-pos	range		Default		Description
	0		0~200		30			Left  Laser On Position (pixel unit)
	1		0~200		80			Left  Laser Off Position (pixel unit)
	2		1400~1530	1440		Right Laser On Position (pixel unit)
	3		1400~1530	1490		Right Laser Off Position (pixel unit)
	4		0~200		40			Left  Line Start Position (pixel unit)
	5		0~200		50			Left  Line End Position (pixel unit)
	6		1400~1530	1450		Right  Line Start Position (pixel unit)
	7		1400~1530	1560		Right  Line Start Position (pixel unit)
	*/

	int Bs[3];
	// B0: 1 : "Normal X-ray", 0 : "Low X-ray Power",-1 : "Invalid Xray tag"
	// B1: 1 : "Door Closed",  0 : "Door Open",		-1 : "Invalid Door tag",
	// B2: 1 : "HR",			  0 : "SR",				-1 : "Invalid Resolution tag"
	bool bValidTags = false;
} HardwareParam;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CHardwareInfo class that provides GenX-CR device information.
class DLL_DECLSPEC CHardwareInfo
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public constructors and destructor
public:
	// constructor
	CHardwareInfo();

	// constructor
	CHardwareInfo(unsigned short Flags, unsigned short FlagsWritten);

	// destructor
	~CHardwareInfo();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public methods
public:
	// bit0 Low x-ray power. (1: Normal, 0: Low X-ray power)
	int getLowXrayStatus(void);

	// bit1 Door open (1: Door clsed, 0: Door open)
	int getDoorStatus(void);

	// bit2 Resolution (1: HR, 0: SR)
	// 2024-05-02. jg kim. cruxcan_sdk.h에 정의된 IMAGE_RESOLUTION과 순서가 반대임.
	// 여기서 수성하면 코드 수정할 게 많아 변경하지 않음.
	int getResolution(void);

	void setFlags(unsigned short Flags, unsigned short FlagsWritten);
	unsigned short getBitMask(int nBit);

	// 장비에서 보내오는 영상이 blank scan(Low X-ray Power)한 영상인지 판별하기 위함. fw 코드 활용함
	bool checkLowXrayPower(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);

	// 영상보정을 하기 전 calibration coefficients의 유효성을 검사하고 유요하지 않을 경우 기본 값을 주기 위해 추가
	// 2024-08-08. jg kim. calibration coefficients의 갯수를 지정하도록 함.
	// 2024-08-18. jg kim. default calibration coefficients를 5개로 사용하니 장비와 맞지 않는 경우가 있어 10개로 복원
	void getDefaultCalibrationCoefficients(unsigned short* pMeans, float* pCoefficients, int nmean = 10);

	// 장비에서 보내주는 scan mode 정보 활용
	// 2024-06-25. jg kim. Mac address decode 추가
	// 2025-02-13. jg kim. scan 일시, current index 추가
	// 2026-01-13. jg kim. 장비에서 넘겨주는 반사판 영역의 Laser On/Off x 좌표 및, 세로 줄 범위(시작/끝 x 좌표)를 Wobble 좌표 decode 추가
	bool decodeGenorayTag(unsigned short* pRawImage, int nWidth, unsigned short* pMeans, float* pCoefficients, int& nIpIdx, 
		double &fdate, unsigned short &cur_info_index, 
		unsigned char *pMac,
		unsigned short &nLeftLaserOnPos, unsigned short &nLeftLaserOffPos,
		unsigned short &nRightLaserOnPos, unsigned short &nRightLaserOffPos,
		unsigned short &nLeftLineStartPos, unsigned short &nLeftLineEndPos,
		unsigned short &nRightLineStartPos, unsigned short &nRightLineEndPos);

		bool decodeGenorayTag(unsigned short* pRawImage, int nWidth, stDecodedHardwareParam& param);
	// PSP 투입구가 열려 있을 경우 ambient light data를 억제
	void SuppressAmbientLight(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);

	bool checkLowResolution(unsigned short* img, int width, int height);

	// FW에서 기록한 태그를 decoding. HardwareInfo class를 활용.
	bool decodeTag(unsigned short* img, int& b0, int& b1, int& b2);

	bool _CheckValidIpIndex(int& nIpIdx);

	// Calibration coefficient의 유효성 검사
	bool _CheckValidCoefficients(unsigned short* pMeans, float* pCoefficients);

	// PSP 투입구가 열려 있는지 검사
	bool _CheckDoorClosed(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);

	// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
	//void writelog(char* filename, char* contents);

	unsigned short* WriteTag(unsigned short* source_img, int width, int height);

	// 2024-06-26. jg kim. MPPC bias 전압 및 digital value 변환 코드 추가
	// 공정툴 쪽으로만 유지
	// 2025-03-27. jg kim. MPPC 값이 변경되는 것을 수정하기 위해 형 변경
	int Mppc_Convert_VoltageToDigitalValue(int Voltage);
	int Mppc_Convert_DigitalValueToVoltage(int nDigitalValue);
	// 2024-07-15. jg kim. BLDC speed, BLDC RPM 계산 기능 추가
	// 2025-03-27. jg kim. BLDC RPM 계산의 정확도를 높이기 위해 형 변경
	unsigned int getBldcSpeed(double fRPM, unsigned int nClock/*MHz unit*/);
	double getBldcRPM(unsigned int BldcSpeed, unsigned int nClock/*MHz unit*/);
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private methods
private:
	// bit3~bit15 :: reserved
	int getBitStatus(int nBit)
	{
		if (nBit < 3)
		{
			unsigned short hBit = (getBitMask(nBit) & m_nFlagsWritten) >> nBit;// 해당 bit가 firmware에서 기록되었는지 검사
			unsigned short lBit = (getBitMask(nBit) & m_nFlags) >> nBit;	// 해당 bit의 상태를 리턴
			return hBit ? lBit + 2 : 0;	// 0: invalid, 1: 0 상태, 2: 1 상태
		}

		// 2024-04-26. jg kim. 현재는 3가지 상태만 정의했기 때문에 리턴 값을 제한함.
		return -1;
	}


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private members
private:
	unsigned short m_nFlags;
	unsigned short m_nFlagsWritten;
};
