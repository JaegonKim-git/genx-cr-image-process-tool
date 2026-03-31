// Added by JG Kim, on 2024/03/28.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "HardwareInfo.h"
#include "../include/logger.h" // 2026-02-02. jg kim. logger 사용하도록 수정

const char* LOG_FILE_NAME = "HardwareInfo.log";
#ifdef _MFC_VER
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CHardwareInfo::CHardwareInfo()
	: m_nFlags(0)
	, m_nFlagsWritten(0)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CHardwareInfo::CHardwareInfo(unsigned short Flags, unsigned short FlagsWritten)
	: m_nFlags(0)
	, m_nFlagsWritten(0)
{
	setFlags(Flags, FlagsWritten);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CHardwareInfo::~CHardwareInfo()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CHardwareInfo::setFlags(unsigned short Flags, unsigned short FlagsWritten)
{
	m_nFlags = Flags;
	m_nFlagsWritten = FlagsWritten;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bit0 Low x-ray power. (1: Normal, 0: Low X-ray power)
int CHardwareInfo::getLowXrayStatus()
{
	// bit0
	return getBitStatus(0);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bit1 Door open (1: Door clsed, 0: Door open)
int CHardwareInfo::getDoorStatus()
{
	// bit1
	return getBitStatus(1);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//bit2 Resolution (1: HR, 0: SR)
int CHardwareInfo::getResolution()
{
	// bit2
	// 2024-05-02. jg kim. cruxcan_sdk.h에 정의된 IMAGE_RESOLUTION과 순서가 반대임.
	// 여기서 수성하면 코드 수정할 게 많아 변경하지 않음.
	return getBitStatus(2);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// private
//bit3~bit15 :: reserved
unsigned short CHardwareInfo::getBitMask(int nBit)
{
	nBit = nBit < 0 ? 0 : (nBit > 15 ? 15 : nBit);
	return unsigned short(1) << nBit;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2024-03-28. jg kim. 장비에서 보내오는 영상이 blank scan(Low X-ray Power)한 영상인지 판별하기 위함. fw 코드 활용함
bool CHardwareInfo::checkLowXrayPower(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes)
{
	bool bLowXrayPower = false;

	int startY = 65;
	int downLevel = 8;
	bool bXray_Low_Power = bLowXrayPower;

	// FW code를 그대로 사용하기 위해 변수명 맵핑
	unsigned short *input = pRawImage;
	int i_width = nWidth;
	int i_height = nHeight;
	int shrinked_width = i_width / downLevel;
	int shrinked_height = i_height / downLevel;
	unsigned char* shrink = new unsigned char[shrinked_width*shrinked_height];
	int lowRes = nLowRes;
	// check low resolution mode
	int downLevelY = lowRes == 1 ? downLevel / 2 : downLevel;
	int down2 = downLevel * downLevelY;
	// 의미가 명확하도록 변수명 변경
	// extreme_min, extreme_max => background_min, background_max
	int min, max, background_min, background_max;
	// check door open
	bool bDoor_open = !_CheckDoorClosed(input, i_width, i_height, lowRes);
	UNREFERENCED_PARAMETER(bDoor_open); // FW  코드 수정을 최소화 하기 위해
	// shrink and 1st low power check
	// 2023.02.22 low power check
	{
		// 의미를 명확하게 하기 위해 변수 추가
		int margin = 40; // start width or height
		int LaserStart = 150;
		int LaserOnWidth = 1280;
		int LaserEnd = LaserStart + LaserOnWidth;
		min = max = input[margin * i_width + LaserStart];
		int y2 = lowRes == 1 ? i_height / 2 - margin : i_height - margin;
		for (int y = margin; y < y2; y++)
		{
			for (int x = LaserStart; x < LaserEnd; x++)
			{
				int v = input[y * i_width + x];
				if (min > v)	min = v;
				if (max < v)	max = v;
			}
		}

		background_min = background_max = min;
		for (int y = 1; y <= startY / 2 || y <= 1; y++) // 상단부의 40x40 영역
		{
			for (int x = LaserEnd; x < LaserEnd + margin; x++)
			{
				int v = input[y * i_width + x];
				if (background_min > v)	background_min = v;
				if (background_max < v)	background_max = v;
			}
		}
		for (int y = y2 - 1; y > y2 - margin; y--) // 하단부의 40x40 영역
		{
			for (int x = LaserEnd; x < LaserEnd + margin; x++)
			{
				int v = input[y * i_width + x];
				if (background_min > v)	background_min = v;
				if (background_max < v)	background_max = v;
			}
		}
		background_max += 20;
		if (background_max >= min * 2)
			background_max = min * 2;
#ifdef USE_PRINTF
		printf("background %d, %d\n", background_min, background_max);
#endif
		// 20231214 : low power
		if (max <= background_max)
		// Laser On 구간에서 구한 max값이 background에서 구한 max 값보다 작으면 x-ray가 조사되지 않았다고 판단
		{
			bXray_Low_Power = true;
			//return -107;
		}
		if (max < 1000)							// 240429 Added.
			max += 1000;
	}

	// shrink image, remove tag and 2nd low power check
	{
		int histogram[256] = { 0 };
		// shrink image
		for (int y = 0; y < shrinked_height; y++)
		{
			for (int x = 0; x < shrinked_width; x++)
			{
				int mean = 0;
				// mean :: 8x8 or 8x4 영역의 평균
				for (int ty = 0; ty < downLevelY; ty++) // downLeveY :: 8(HR mode) or 4 (SR mode)
				{
					for (int tx = 0; tx < downLevel; tx++) // downLevel :: 8
					{
						int v = input[(y * downLevelY + ty) * i_width + (x * downLevel + tx)];
						// 20231123 : background_max를 min으로 사용
						v = (v - background_max) * 255 / (max - background_max);
						// background_max ~ (max - background_max) 밝기를 0~255로 조정
						if (v < 0) v = 0;
						else if (v > 255)	v = 255;
						mean += v;
						histogram[v]++;
					}
				}
				mean /= down2; // down2 :: 8x8 or 8x4 영역의 픽셀 개수
// 코드 정리 후 불필요해서 비활성화 시킴
//#if 1
//			sum = (sum - extreme_max) * 255 / (max - extreme_max);
//			if (sum < 0)	sum = 0;
//			else if (sum > 255)	sum = 255;
//#else
//			sum = (sum - min) * 255 / (max - min);
//			if (sum < 0)	sum = 0;
//			else if (sum > 255)	sum = 255;
//#endif
				shrink[y * shrinked_width + x] = unsigned char(mean);
			}
		}

		// remove tag
		{
			int tagWidth = 2;//downLevel을 고려하여 tag가 있는 영역을 계산. shrinked image에서는 2 pixel이면 충분
			for (int y = 0; y < shrinked_height; y++)
			{
				int v = shrink[y * shrinked_width + tagWidth];
				for (int x = 0; x < tagWidth; x++)
				{// remove left side guide
					shrink[y * shrinked_width + x] = 0;
				}

				v = shrink[(y + 1) * shrinked_width - tagWidth];
				for (int x = shrinked_width - tagWidth; x < shrinked_width; x++)
				{// remove right side tag
					shrink[y * shrinked_width + x] = unsigned char(v);
				}
			}
		}

		/*if (bDebug)
		{
			printf("max %d, min %d\n", max, min);
			char name[256];
			sprintf_s(name, 256, "shrink_%dx%d.raw", shrinked_width, shrinked_height);
			SaveToFile(name, shrink, shrinked_width * shrinked_height);
		}*/

		// 20231123 : low power 기준을 background 영역의 비율로 함
		int backgroundPixelRatio = histogram[0] * 100 / (i_width * i_height);

		// 이하 코드 리팩토링
		if (lowRes)
			backgroundPixelRatio *= 2;

		// 2nd low power check
		if (backgroundPixelRatio >= 90)	// 배경이 90% 이상
		{
			bXray_Low_Power = true;
			//return -107;
		}
	}
	SAFE_DELETE_ARRAY(shrink);
	bLowXrayPower = bXray_Low_Power;
	return bLowXrayPower;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2024-04-22. jg kim. 영상보정을 하기 전 calibration coefficients의 유효성을 검사하고 유요하지 않을 경우 기본 값을 주기 위해 추가
// 2024-08-08. jg kim. calibration data 갯수를 입력 받도록 변경
void CHardwareInfo::getDefaultCalibrationCoefficients(unsigned short* pMeans, float* pCoefficients, int nmean)
{
	int ncoeffs = 50;
	unsigned short means[10] = { 2579,4264,5243,8180,9973,13465,16205,19548,23438,27131 }; // 2024-07-08. jg kim. 디버깅
	float coeff[50] = { 9.582349e+02,-1.398507e+00,1.792794e-02,-2.120235e-05,6.650398e-09,
						1.398702e+03,-1.152341e+00,2.685818e-02,-3.205693e-05,9.830561e-09,
						1.479021e+03,1.467330e-01,2.945505e-02,-3.622564e-05,1.114694e-08,
						2.453717e+03,3.482937e-02,4.768012e-02,-6.002840e-05,1.907367e-08,
						2.791963e+03,3.849012e+00,4.518967e-02,-5.905443e-05,1.847158e-08,
						5.802547e+03,-5.258582e+00,8.151331e-02,-9.750012e-05,3.050402e-08,
						6.585694e+03,2.826815e-01,8.055300e-02,-1.028910e-04,3.334747e-08,
						8.833425e+03,-9.814845e-01,9.275055e-02,-1.152703e-04,3.624044e-08,
						1.201769e+04,-5.169065e+00,1.135846e-01,-1.382684e-04,4.354217e-08,
						1.356153e+04,4.905722e+00,9.749184e-02,-1.265209e-04,4.004591e-08 };
	memcpy(pMeans, means, sizeof(unsigned short) * nmean);
	memcpy(pCoefficients, coeff, sizeof(float) * ncoeffs);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2024-03-28. jg kim. 장비에서 보내주는 scan mode 정보 활용
bool CHardwareInfo::decodeGenorayTag(unsigned short * pRawImage, int nWidth, unsigned short * pMeans, float * pCoefficients, int & nIpIdx, 
	double & fdate, unsigned short & cur_info_index, unsigned char * pMac, 
	unsigned short &nLeftLaserOnPos, unsigned short &nLeftLaserOffPos,
	unsigned short &nRightLaserOnPos, unsigned short &nRightLaserOffPos,
	unsigned short &nLeftLineStartPos, unsigned short &nLeftLineEndPos,
	unsigned short &nRightLineStartPos, unsigned short &nRightLineEndPos)
{
	cur_info_index = pRawImage[RAW_WIDTH]; // Imagelist 상의 current index 정보. (마지막 촬영된 영상의 index임)
	unsigned short bufDate[4] = { 0, };
	for (int i = 0; i < 4; i++)
		bufDate[i] = pRawImage[RAW_WIDTH + 4 - i]; // scan 일시 정보. double 형식으로 8 byte를 사용하여 기록함.

	memcpy(&fdate, bufDate, sizeof(double));
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
#define WOBBLE_TAG_WIDTH 6
	nLeftLaserOnPos		= pRawImage[RAW_WIDTH * 0 + WOBBLE_TAG_WIDTH];
	nLeftLaserOffPos	= pRawImage[RAW_WIDTH * 1 + WOBBLE_TAG_WIDTH];
	nRightLaserOnPos	= pRawImage[RAW_WIDTH * 2 + WOBBLE_TAG_WIDTH];
	nRightLaserOffPos	= pRawImage[RAW_WIDTH * 3 + WOBBLE_TAG_WIDTH];
	nLeftLineStartPos	= pRawImage[RAW_WIDTH * 4 + WOBBLE_TAG_WIDTH];
	nLeftLineEndPos		= pRawImage[RAW_WIDTH * 5 + WOBBLE_TAG_WIDTH];
	nRightLineStartPos	= pRawImage[RAW_WIDTH * 6 + WOBBLE_TAG_WIDTH];
	nRightLineEndPos	= pRawImage[RAW_WIDTH * 7 + WOBBLE_TAG_WIDTH];

	const int nTagPos = 5;
	unsigned short buf[256] = { 0, };
	for (int n = 0; n < 256; n++)
		buf[n] = pRawImage[n * nWidth + nTagPos];
	int nsig = 4;
	// 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
	int nmean = MAX_CALI_IMAGE_COUNT;
	int ncoeffs = MAX_CALI_DATA_COUNT;
	int nmac = 6;
	int nScale = sizeof(float) / sizeof(unsigned short);
	unsigned short sigsTemplate[4] = { 65535,32766,32766,65535 };
	unsigned short sigeTemplate[4] = { 65535,	 1,	   1,65535 };
	int ofs = 0;
	int ofsSige = nsig + nmean + ncoeffs * nScale + nmac + 1;
	int valid = 1;
	for (int n = 0; n < nsig; n++) // sigs, sige가 유효한지 검사
	{
		if (sigsTemplate[n] == buf[n + ofs])
			valid *= 1;
		if (sigeTemplate[n] == buf[n + ofsSige])
			valid *= 1;
		else
			valid *= 0;
	}

	if (valid == 1)
	{
		ofs += nsig; // +4 => 4
		for (int i = 0; i < nmean; i++) // data1 : mean values
			pMeans[i] = buf[i + ofs];

		ofs += nmean; // +10 => 14
		float* data = new float[ncoeffs];
		memcpy(data, buf + ofs, sizeof(float) * ncoeffs);
		for (int i = 0; i < ncoeffs; i++) // data2 : coefficients
			pCoefficients[i] = data[i];

		// 2024-06-25. jg kim. Mac address decode 추가
		if (pMac != nullptr)
		{
			for (int i = 0; i < nmac; i++)
				pMac[i] = unsigned char(buf[ofsSige - 1 - nmac + i]);
		}

		nIpIdx = buf[ofsSige - 1]; // sige 바로 전 pixel에 scan mode를 기록함.
		SAFE_DELETE_ARRAY(data);

		return true;
	}

	return false;
}

bool CHardwareInfo::decodeGenorayTag(unsigned short *pRawImage, int nWidth, stDecodedHardwareParam &param)
{
	param.cur_info_index = pRawImage[RAW_WIDTH];
	unsigned short bufDate[4] = { 0, };
	for (int i = 0; i < 4; i++)
		bufDate[i] = pRawImage[RAW_WIDTH + 4 - i];

	memcpy(&param.fdate, bufDate, sizeof(double));
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
#define WOBBLE_TAG_WIDTH 6
	for (int n = 0; n < 8; n++)
		param.wobbleRelates[n] = pRawImage[RAW_WIDTH * n + WOBBLE_TAG_WIDTH];
	if (param.wobbleRelates[2]>=1400 && param.wobbleRelates[3]<=1530 && param.wobbleRelates[6]>=1400 && param.wobbleRelates[7]<=1530)
		param.bValidWobble = true;
	else
	{
		param.wobbleRelates[2] = 1400;
		param.wobbleRelates[3] = 1530;
		param.wobbleRelates[6] = 1400;
		param.wobbleRelates[7] = 1530;
	}
	for (int n = 0; n < 5; n++)
	{
		param.bValidFwVer = pRawImage[RAW_WIDTH * 2 + n]<=255? true : false;
		param.nFwVer[n] = unsigned char(pRawImage[RAW_WIDTH * 2 + n]);
	}
	const int nTagPos = 5;
	unsigned short buf[256] = { 0, };
	for (int n = 0; n < 256; n++)
		buf[n] = pRawImage[n * nWidth + nTagPos];
	int nsig = 4;
	// 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
	int nmean = MAX_CALI_IMAGE_COUNT;
	int ncoeffs = MAX_CALI_DATA_COUNT;
	int nmac = 6;
	int nScale = sizeof(float) / sizeof(unsigned short);
	unsigned short sigsTemplate[4] = { 65535,32766,32766,65535 };
	unsigned short sigeTemplate[4] = { 65535,	 1,	   1,65535 };
	int ofs = 0;
	int ofsSige = nsig + nmean + ncoeffs * nScale + nmac + 1;
	int valid = 1;
	for (int n = 0; n < nsig; n++) // sigs, sige가 유효한지 검사
	{
		if (sigsTemplate[n] == buf[n + ofs])
			valid *= 1;
		if (sigeTemplate[n] == buf[n + ofsSige])
			valid *= 1;
		else
			valid *= 0;
	}

	decodeTag(pRawImage,param.Bs[0], param.Bs[1], param.Bs[2]);
	if (valid == 1)
	{
		ofs += nsig; // +4 => 4
		for (int i = 0; i < nmean; i++) // data1 : mean values
			param.nMeans[i] = buf[i + ofs];

		ofs += nmean; // +10 => 14
		float* data = new float[ncoeffs];
		memcpy(data, buf + ofs, sizeof(float) * ncoeffs);
		for (int i = 0; i < ncoeffs; i++) // data2 : coefficients
			param.fCoefficients[i] = data[i];

		// 2024-06-25. jg kim. Mac address decode 추가
		for (int i = 0; i < nmac; i++)
			param.nMac[i] = unsigned char(buf[ofsSige - 1 - nmac + i]);

		if((param.nMac[0] == 0x24 && param.nMac[1] == 0xC9 && param.nMac[2] == 0xDE) || // Genoray official MAC address for GenX-CR
 			(param.nMac[0] == 0x00 && param.nMac[1] == 0x08 && param.nMac[2] == 0xDC)) // Default MAC address when not set
			param.bValidMacAddress = true;
		else
			param.bValidMacAddress = false;

		param.nIpIdx = buf[ofsSige - 1]; // sige 바로 전 pixel에 scan mode를 기록함.

		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2024-04-22. jg kim. PSP 투입구가 열려 있을 경우 ambient light data를 억제
void CHardwareInfo::SuppressAmbientLight(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes)
{
	int sy = 0;
	int ey = 0;
	if (nLowRes == 1) // low resolution 처리 
	{
		ey = nHeight / 2;
	}
	else
	{
		ey = nHeight;
	}
	// 2024-06-06. jg kim. 하드코딩 했던 값들을 변수화 함. 
	// 레드마인 18519 이슈에 대응하기 위하여 skip width, measure width 값 변경
	int nMeasureWidth = 90; // pixel
	int nMeasureHeight = 90;// pixel
	int nSkipWidth = 10; 	// pixel
	int localMax = 0;
	for (int y = nSkipWidth; y < nSkipWidth + nMeasureHeight; y++) // raw image의 좌측 상단에서 local max를 구하고
	{
		for (int x = nSkipWidth; x < nSkipWidth + nMeasureWidth; x++)
		{
			if (pRawImage[y * nWidth + x] > localMax)
				localMax = pRawImage[y * nWidth + x];
		}
	}

	for (int y = sy; y < ey; y++)
	{
		int max = 0;
		for (int x = nSkipWidth; x < nSkipWidth + nMeasureWidth; x++) // 각 라인의 좌측에서 측정
		{
			if (pRawImage[y * nWidth + x] > max)
				max = pRawImage[y * nWidth + x];
		}
		// for (int x = nWidth - nSkipWidth - nMeasureWidth; x < nWidth - nSkipWidth; x++) // 각 라인의 우측. wobble 보정을 위한 부분이므로 제외
		// {
		// 	if (pRawImage[y * nWidth + x] > max)
		// 		max = pRawImage[y * nWidth + x];
		// }
		// 각 라인의 좌, 우측의 일정 영역에서 max 값을 구한다.
		for (int x = 0; x < nWidth; x++)
		{
			if (pRawImage[y * nWidth + x] <= unsigned short(float(max)*1.05f)) // 각 line에서 max * 1.05f보다 작은 값은 local max 값으로 변경한다.
				pRawImage[y * nWidth + x] = unsigned short(localMax);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
bool CHardwareInfo::checkLowResolution(unsigned short* img, int width, int height)
{
	return img[(height / 2 + 20) * width + width - 10] == 0? true : false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
bool CHardwareInfo::decodeTag(unsigned short* pRawImage, int& b0, int& b1, int& b2)
{
	// pRawImage[0] : sigs
	// pRawImage[3] : sige
	if (pRawImage[0] == 60000 && pRawImage[3] == 60001)
	{
		unsigned short Flags = pRawImage[1];
		unsigned short FlagsWritten = pRawImage[2];
		setFlags(Flags, FlagsWritten);
		// 0~3
		// 해당 flag를 firmware에서 썼을 때 1보다 큰 값을 가짐 (2 or 3)
		b0 = getLowXrayStatus();
		b1 = getDoorStatus();
		b2 = getResolution();

		b0 = b0 > 1 ? b0 - 2 : -1; // 1 : "Normal X-ray", 0 : "Low X-ray Power",-1 : "Invalid Xray tag"
		b1 = b1 > 1 ? b1 - 2 : -1; // 1 : "Door Closed",  0 : "Door Open",		-1 : "Invalid Door tag",
		b2 = b2 > 1 ? b2 - 2 : -1; // 1 : "HR",			  0 : "SR",				-1 : "Invalid Resolution tag"
		return true;
	}
	else
	{
		return false; // invalid tag
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2024-04-22. jg kim. 장비에서 보내오는 스캔모드 정보의 유효성을 검사
bool CHardwareInfo::_CheckValidIpIndex(int& nIpIdx)
{
	if (nIpIdx < 0 || nIpIdx > 3)
	{
		nIpIdx = 2; // 유효하지 않은 경우에 기본 값
		return false; // Invalid
	}
	else
		return true; // Valid
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2024-04-22. jg kim. Calibration coefficient의 유효성 검사
bool CHardwareInfo::_CheckValidCoefficients(unsigned short* pMeans, float* pCoefficients)
{
	int validCoefficient = 1;
	int nCount = 0;
	while (pMeans[nCount]>0 && nCount < MAX_CALI_IMAGE_COUNT) // 2024-07-08. jg kim. calibration data의 개수를 변경 몇개의 data를 사용하는지 체크
									// 2025-02-11. jg kim. 오타 수정
		nCount++;

	if (nCount < CALI_IMAGE_COUNT)  // 2024-07-19. jg kim. 유효한 숫자(5개) calibration data가 없을 경우
									// 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
		validCoefficient *= 0;
	else
	{
		// 2024-08-08. jg kim. calibration data가 유효한지 검사하는 기준 변경.
		// 2024-08-16. jg kim. calibration data가 유효한지 검사하는 기준 변경.
		for (int i = 0; i < nCount-1; i++)
		{
			if (float(pMeans[i + 1]) <  float(pMeans[i])*1.07f) 
				// 조사시간이 증가할 수록 평균값이 최소 10% 이상 증가함. 이러한 패텀을 보이지 않을 경우 유효하지 않는 것으로 판단함.
				// 판단 기준으로는 여유를 더우 7% 증가율로 함.
				validCoefficient *= 0;
				// 2025-09-12. jg kim. x-ray 캘리브레이션 계수의 유효성 테스트가 의미가 없어 테스트를 하지 않도록 수정.
			//int ofs = i * (DEGREE_COUNT + 1);
			//validCoefficient *= ((pCoefficients[ofs] > 0 ) * (pCoefficients[ofs + 2] > 0) * (pCoefficients[ofs + 3] < 0)* (pCoefficients[ofs + 4] > 0)); 
			// 4차 fitting 계수의 0차, 2차, 4차 계수는 양수, 3차 계수는 음수의 패턴을 보임. 이러한 패턴을 보이지 않는 계수는 유효하지 않는 것으로 판단함.
		}		
	}

	if (!validCoefficient)
	{
		// 2024-08-16. jg kim. 검사한 calibration data는 유효하지 않기 때문에 메모리를 초기화 하고 default 값을 받아오도록 수정
		// 2025-02-11. jg kim. 오타 수정
		memset(pMeans, 0, sizeof(unsigned short) * MAX_CALI_IMAGE_COUNT);
		memset(pCoefficients, 0, sizeof(float) * MAX_CALI_IMAGE_COUNT * (DEGREE_COUNT + 1));
		CHardwareInfo::getDefaultCalibrationCoefficients(pMeans, pCoefficients); // 유효하지 않을 경우 기본값 받아옴
		return false; //Invalid
	}
	else
	{
		return true; // Valid
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 2024-04-22. jg kim. PSP 투입구가 열려 있는지 검사
bool CHardwareInfo::_CheckDoorClosed(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes)
{
	int counts = 0;
	int sy = 0;
	int ey = 0;
	if (nLowRes == 1) // low resolution 처리 
		ey = nHeight / 2;
	else
		ey = nHeight;

	// 2024-06-06. jg kim. 하드코딩 했던 값들을 변수화 함. 
	// 레드마인 18519 이슈에 대응하기 위하여 skip width, measure width 값 변경
	int nMeasureWidth = 90; // pixel
	int nMeasureHeight = 90;// pixel
	int nSkipWidth = 10; 	// pixel
	int max = 0;
	for (int y = nSkipWidth; y < nSkipWidth + nMeasureHeight; y++)
	{
		for (int x = nSkipWidth; x < nSkipWidth + nMeasureWidth; x++)
		{
			if (pRawImage[y * nWidth + x] > max)
				max = pRawImage[y * nWidth + x];
		}/*for (int x = nWidth - nSkipWidth - nMeasureWidth; x < nWidth - nSkipWidth; x++)
		{
			if (pRawImage[y * nWidth + x] > max)
				max = pRawImage[y * nWidth + x];
		}*/// 2025-10-23. jg kim. Wobble 보정을 위해 추가한 반사판으로 인해 PSP 투입구가 열린 것으로 판단하게 되어 우측을 체크하는 코드 비활성화
	}

	for (int y = sy; y < ey; y++)
	{
		for (int x = nSkipWidth; x < nSkipWidth + nMeasureWidth; x++)
		{
			if (pRawImage[y * nWidth + x] > unsigned short((float)max * 1.05f))
				counts++;
		}/*for (int x = nWidth - nSkipWidth - nMeasureWidth; x < nWidth - nSkipWidth; x++)
		{
			if (pRawImage[y * nWidth + x] > unsigned short((float)max * 1.05f))
				counts++;
		}*/// 2025-10-23. jg kim. Wobble 보정을 위해 추가한 반사판으로 인해 PSP 투입구가 열린 것으로 판단하게 되어 우측을 체크하는 코드 비활성화
	}

	if (counts > 10)
	{
		return false;
	}
	else
		return true;
}


// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
//void CHardwareInfo::writelog(char* filename, char* contents)
//{
//	FILE* fh;
//	if (fopen_s(&fh, filename, "at") == 0)
//		fprintf(fh, contents);
//	fclose(fh);
//}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
unsigned short* CHardwareInfo::WriteTag(unsigned short* source_img, int width, int height)
{
	bool bResol = checkLowResolution(source_img, width, height); // bit2
	int nLowRes = bResol;
	bool bDoor = _CheckDoorClosed(source_img, width, height, nLowRes);
	bool bLowXrayPower = checkLowXrayPower(source_img, width, height, nLowRes);

	unsigned short b0 = bLowXrayPower ? 0 : getBitMask(0);			// 0: low x-ray power, 1: normal condition
	unsigned short b1 = unsigned short(bDoor) ? getBitMask(1) : 0;	// 0: door open, 1: door closed
	unsigned short b2 = bResol ? 0 : getBitMask(2);					// 0: SR, 1: HR

	source_img[0] = 60000;
	source_img[1] = b0 | b1 | b2;
	source_img[2] = getBitMask(0) | getBitMask(1) | getBitMask(2); // whether flag bit written
	source_img[3] = 60001;
	return source_img;
}

// 2024-06-26. jg kim. MPPC bias voltage 및 digital value 변환 코드
// 공정툴 쪽으로만 유지
// 2025-02-11. jg kim. 의미를 명확히 하기 위해 macro 사용
int CHardwareInfo::Mppc_Convert_VoltageToDigitalValue(int Voltage)
{
	int Bounded = 0; // 2024-07-15. jg kim. MPPC 값 bounding 추가
	// 2024-10-25. MPPC 전압 조정범위 조정 (53~56V -> 50~59V)
	// 2025-02-10 소숫점 표시 개선을 위해 double에서 unsigned int로 변경. 다루는 값은 1000을 곱하여 사용.
	
	if (Voltage >= MPPC_VOLTAGE_LOWER_BOUND && Voltage <= MPPC_VOLTAGE_UPPER_BOUND) // Digital value에서 환산한 값
		Bounded = Voltage;
	else if (Voltage < MPPC_VOLTAGE_LOWER_BOUND)
		Bounded = MPPC_VOLTAGE_LOWER_BOUND;
	else if (Voltage > MPPC_VOLTAGE_UPPER_BOUND)
		Bounded = MPPC_VOLTAGE_UPPER_BOUND;

	return int (MPPC_CONSTANT_VOLTAGE_CONVERSION - Bounded);
}

int CHardwareInfo::Mppc_Convert_DigitalValueToVoltage(int nDigitalValue)
{
	int nBound = 0;// 2024-07-15. jg kim. MPPC 값 bounding 추가
	// 2024-10-25. MPPC 전압 조정범위 조정 (53~56V -> 50~59V)
	// 2025-02-10 소숫점 표시 개선을 위해 double에서 unsigned int로 변경. 다루는 값은 1000을 곱하여 사용.
	if (nDigitalValue >= MPPC_DIGITAL_VALUE_LOWER_BOUND && nDigitalValue <= MPPC_DIGITAL_VALUE_UPPER_BOUND) // 통계에 의해 결정된 값
		nBound = nDigitalValue;
	else if (nDigitalValue < MPPC_DIGITAL_VALUE_LOWER_BOUND)
		nBound = MPPC_DIGITAL_VALUE_LOWER_BOUND;
	else if (nDigitalValue > MPPC_DIGITAL_VALUE_UPPER_BOUND)
		nBound = MPPC_DIGITAL_VALUE_UPPER_BOUND;

	return int(MPPC_CONSTANT_VOLTAGE_CONVERSION - nBound);
}
// 2024-07-15. jg kim. BLDC RPM, speed 계산 기능 추가
unsigned int CHardwareInfo::getBldcSpeed(double fRPM, unsigned int nClock)
{
	return unsigned int(round((nClock)*10000000 / (fRPM*4)));
}

double CHardwareInfo::getBldcRPM(unsigned int BldcSpeed, unsigned int nClock)
{
	return double(nClock)*1000000*TRAY_EJECT_TIME/ double(BldcSpeed);
}
