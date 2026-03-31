// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "../gnImageProcessing/PostProcessingInterface.h" // ApplyLinecorrect 사용을 위함
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
#include "../include/Logger.h"
#include "ImageProcessor.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <array>
#include <omp.h>
#include <vector>
#include <iostream>
#include <random>

#pragma warning(disable:4996)
#pragma warning(disable:4244)

using namespace cv;


#ifdef _DEBUG
#define _Check_Memory_Leak
#endif

#ifdef _Check_Memory_Leak

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h> 

#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)

#endif // _Check_Memory_Leak


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// macro constants
#define pow2(x) (x)*(x)
#define pow3(x) (x)*(x)*(x)
//#define	USE_DEBUG

// remove stripes
int g_nradius = 44;
int g_nmedRadius = 7;
int g_nmargin = 100;
double g_fcutoff = 40;
bool g_bwant_old_mode = false;
int g_nradius2 = 8;
double g_fstripe_gain = 0.13; // org value: 0.13
double g_fvary = g_fstripe_gain;
int g_nPreTerm = 9;
int g_nPostTerm = 50;
bool g_bProcMtf = false;// 2026-01-12. jg kim. MTF 모드 설정을 위해
int g_nCorrectWobbleResult;// 2026-01-12. jg kim. Wobble 보정 결과를 받기 위해
// 2026-03-10. jg kim. -1: 유효한 반사판이 없어 wobble 보정을 하지 않음, 0:Wobble 보정 실패, 1: Wobble 보정 성공
// 2026-01-13. jg kim. 장비에서 넘겨주는 반사판 영역의 Laser On/Off x 좌표 및, 세로 줄 범위(시작/끝 x 좌표)를 Wobble 보정에 활용하기 위함.
int g_nLaserOnPos;
int g_nLaserOffPos;
int g_nLineStartPos;
int g_nLineEndPos;

PspCalibrationParameters option_pp;
// 2024-04-26. jg kim. log 작성을 위해 추가
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
//char *file_name;
//extern "C" void __cdecl setLogFileName(char *name)
//{
//	file_name = name;
//}

extern "C" void __cdecl ParametersSetting(float* coefs, unsigned short* means, int nMeans)
{
	for (int i = 0; i < ((DEGREE_COUNT + 1) * MAX_CALI_IMAGE_COUNT); i++)
	{
		if (i < ((DEGREE_COUNT + 1) * nMeans))
			option_pp.coefs[i] = coefs[i];
		else
			option_pp.coefs[i] = 0;
	}
	for (int i = 0; i < (MAX_CALI_IMAGE_COUNT); i++)
	{
		if (i < nMeans)
			option_pp.means[i] = means[i];
		else
			option_pp.means[i] = 0;
	}
}

extern "C" void __cdecl SetMtfMode(bool bMTF)// 2026-01-12. jg kim. MTF 모드 설정을 위해
{
	g_bProcMtf = bMTF;
}

extern "C" DLL_DECLSPEC int __cdecl GetCorrectWobbleResult()// 2026-01-12. jg kim. Wobble 보정 결과를 받기 위해
{
	return g_nCorrectWobbleResult;
}

// 2026-01-13. jg kim. 장비에서 넘겨주는 반사판 영역의 Laser On/Off x 좌표 및, 세로 줄 범위(시작/끝 x 좌표)를 Wobble 보정에 활용하기 위함.
extern "C" DLL_DECLSPEC void __cdecl SetFwWobblePos(int nLaserOnPos, int nLaserOffPos, int nLineStartPos, int nLineEndPos)
{
	g_nLaserOnPos = nLaserOnPos;
	g_nLaserOffPos = nLaserOffPos;
	g_nLineStartPos = nLineStartPos;
	g_nLineEndPos = nLineEndPos;
}

// 2024-03-28. jg kim. 장비에서 보내주는 정보(스캔 모드) 활용 위함.
// 2024-04-22. jg kim. 영상 보정결과 리턴값 추가
int checkResult(cv::Mat out)
{
	int result = 0;
	// bounding
	if (out.rows > 0 && out.rows < 65535 &&
		out.cols > 0 && out.cols < 65535 &&
		out.data != nullptr &&
		out.size.dims() >= 0)
	{
		result = 1;
		// 2024-04-26. jg kim. binary mask를 제대로 만들지 못한 경우에도 return 1을 하여 수정.
		double min, max;
		cv::minMaxLoc(out, &min, &max);
		if (int(max) == 0)
			result = 0;
		else
			result = 1;
	}
	return result;
}

// 2024-03-28. jg kim. 장비에서 보내주는 정보(스캔 모드) 활용 위함.
extern "C" void __cdecl getPspSize(int *width, int *height, int index)
{
	unsigned short *pt = nullptr;
	int w = 0;
	int h = 0;
	ImageProcessor Imgproc(pt, w, h);
	Imgproc.GetIpSize(*width, *height, index);
}

// 2024-04-26. jg kim. log 작성을 위해 추가
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
const char* LOG_FILE_NAME = "ImageProcessing.log";
// 2024-06-25. jg kim. Cruxell 종속성 제거를 위해 구현. CCalibrationDlg에서 사용하기 위함
extern "C" DLL_DECLSPEC void __cdecl FindIPArea(unsigned short *input, int width, int height, 
	int &oWidth, int &oHeight, int &cx, int &cy, float &angle)
{
	ImageProcessor ip(input, width, height);
	CRX_RECT2 rt = ip._FindIPArea();
	oWidth = rt.rot.width;
	oHeight = rt.rot.height;
	cx = rt.rot.cx;
	cy = rt.rot.cy;
	angle = rt.rot.angle;
}

// 2024-07-08. jg kim. 영상 보정을 단계적으로 테스트하기 위해 외부 접근 함수 추가
// 2024-10-31. jg kim. 공정툴에서도 영상처리를 하는 경우에는 RemoveWormPattern 함수를 사용하도록 옵션 추가
// 2024-12-09. jg kim. 공정툴용 영상처리와 통합
extern "C" DLL_DECLSPEC void __cdecl ProcessAPI(unsigned short **result_img, int &result_width, int &result_height, bool &bResult, unsigned short *ImgBin,
	unsigned short *source_img, int source_width, int source_height, int nIpIndex, bool bSaveCorrectionStep, int nImageProcessMode, bool bDoorOpen)
{
	ImageProcessor processor(source_img, source_width, source_height);
	processor.SetSaveCorrectionStep(bSaveCorrectionStep);
	processor.SetIpIndex(nIpIndex);
	std::vector<cv::Mat> res;
	bool bImageProcess = true;
	if (nImageProcessMode == 1)
		bImageProcess = false;
	processor.SetImageProcess(bImageProcess);
	processor.SetImageProcessMode(nImageProcessMode);
	processor.SetDoorOpen(bDoorOpen); // 2026-02-09. jg kim. 문 열림 상태 설정. HardwareInfo class에서 이동
	res = processor.Run();

	cv::Mat out = res.at(0);
	bResult = bool(checkResult(out));
	if (bResult)
	{
		cv::Mat bin = res.at(1);
		result_width = out.cols;
		result_height = out.rows;
		cv::Mat output16; out.convertTo(output16, bin.type());
		*result_img = new unsigned short[result_width*result_height];
		memcpy(*result_img, (unsigned short*)output16.ptr(), sizeof(unsigned short)*out.cols*out.rows);
		if (nImageProcessMode == 0 || nImageProcessMode == 2)
			memcpy(ImgBin, (unsigned short*)bin.ptr(), sizeof(unsigned short)*bin.cols*bin.rows);
		else
			memcpy(ImgBin, (unsigned short*)bin.ptr(), sizeof(unsigned short)*out.cols*out.rows);
	}
	else
	{
		ASSERT(0);
		result_width = 0;
		result_height = 0;
	}
}

extern "C" DLL_DECLSPEC int __cdecl GetBldcShiftIndex(float **fpt_CornerInfo, int &nIpIndex, unsigned short *ImgBin, int width, int height)
{
	char buf[1024];
	cv::Mat bin = cv::Mat(cv::Size(width, height), CV_16U, ImgBin);
	ImageProcessor::stInfoCorner info;
	ImageProcessor processor(ImgBin, width, height);
	cv::Rect rt = processor.getBoundingRect(bin);
	processor.guessAndAnalyseIP(info, bin);
	int nBldcShiftPixels = 0;
	int nRef_L = LASER_ON_POSITION;
	int nRef_W = processor.ip[2].width;
	int nRef_R = nRef_L + nRef_W;

	// float fIpLengthError = float(IP_LENGTH_ERROR);
	// 왼쪽으로 이동
	// rt.x 가 nRef_L 보다 큰 만큼
	// 오른쪽으로 이동
	// rt.x == nRef_L 이고, nRef_R - (rt.x + rt.width)
	// shift left :: return positive number
	// shift right :: return negative number

	//printf_s("X/Width %d, %d. ", rt.x,  rt.width);

	float sumCornerScoreL = /*info.scoreCorner[0] +*/ info.scoreCorner[2];
	float sumCornerScoreR = (info.scoreCorner[1] + info.scoreCorner[3])/2.f;


	if (sumCornerScoreL >= sumCornerScoreR && info.countOverCornerThreshold <= 2 || rt.x > nRef_L &&  info.countOverCornerThreshold >= 3) // Left Edge. 왼쪽으로 이동해야 한다.
	{
		nBldcShiftPixels = rt.x - nRef_L;
		sprintf(buf, "Case 1. used edge = Left\n");
	}
	else if (sumCornerScoreL < sumCornerScoreR && info.countOverCornerThreshold <= 2 || rt.x < nRef_L &&  info.countOverCornerThreshold >= 3)
	{
		nBldcShiftPixels = rt.x + rt.width - nRef_R; //이동은 반대방향으로 해야 하기 때문에 반대로 계산함.
		sprintf(buf, "Case 2. used edge = Right\n");
	}
	else
	{
		sprintf(buf, "used edge = else. used corner = %d. corner counts = %d\n",int(info.e_usedCorner), info.countOverCornerThreshold);
	}
	writelog(buf, LOG_FILE_NAME);
	printf("%s\n",buf);
	*fpt_CornerInfo = new float[CORNERS];
	memcpy(*fpt_CornerInfo, info.scoreCorner, sizeof(float)*CORNERS);
	nIpIndex = info.n_guessedIndex;

	return nBldcShiftPixels;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
ImageProcessor::ImageProcessor(unsigned short* image, int width, int height)
	:m_bSaveCorrectionStep(false),
	m_bImageProcess(true),
	m_nLowResolution(-1),
	m_nIpIndex(-1)
{
	_input = cv::Mat(height, width, CV_16U, image);
	if(width*height>0) // 2024-03-28. jg kim. null pointer 및 width, height를 0,0 으로 클래스를 생성할 경우에 대응
		m_nLowResolution = image[(height / 2 + 20) * width + width - 2] < 15000;	
	// TIFF 해상도 설정 파라미터 초기화
	m_tiffParams.push_back(cv::IMWRITE_TIFF_RESUNIT);
	m_tiffParams.push_back(3); // 1=none, 2 = inches, 3 = centimeters
	m_tiffParams.push_back(cv::IMWRITE_TIFF_XDPI);
	m_tiffParams.push_back(400); // 400 pixels per 1 centimeters
	m_tiffParams.push_back(cv::IMWRITE_TIFF_YDPI);
	m_tiffParams.push_back(400);
	m_roiBackground = cv::Rect(10, 150, 90, height/2); // 공통된 기준으로 background의 ROI를 설정하기 위함.
	m_fBackgroundmin = 65535.0;
	m_fBackgroundmax = 0.0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
ImageProcessor::~ImageProcessor()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// 2024-12-09. jg kim. 공정툴용 영상처리와 통합
std::vector<cv::Mat> ImageProcessor::Run()
{

	std::vector<cv::Mat> coeffs = getCorrectionCoefficients();
	std::vector<cv::Mat> res;

	if (m_nImageProcessMode)
		//1: image correction_PE (Extract IP 하지 않음)
		//2: image correction_PE, image process (Extract IP)
		res = Preprocess_PE(coeffs);
	else
		//0: image correction, Extract IP (for PortView)
		res = Preprocess(coeffs);

	cv::Mat out = res.at(0);
	cv::Mat bin = res.at(1);
	if (m_nLowResolution == 1) // 2024-03-13. jg kim. 저해상도 영상 관련 이슈 (이슈 #17835) 
	{
		Mat temp = out.clone();
		resize(temp, out, cv::Size(int(temp.cols * 0.5), int(temp.rows * 0.5)), 0, 0, cv::INTER_LANCZOS4);
	}
	out.convertTo(out, CV_16U);
	res.clear();// 2024-12-16. jg kim. debug
	res.push_back(out);
	res.push_back(bin);
	return res;
}

#pragma region Preprocess

std::vector<cv::Mat> ImageProcessor::getCorrectionCoefficients()
{
	std::vector<cv::Mat> coeffs;
	cv::Mat mn = cv::Mat::zeros(MAX_CALI_IMAGE_COUNT + 1, 1, CV_32F);
	for (int i = 1; i < MAX_CALI_IMAGE_COUNT + 1; i++)
		mn.at<float>(i) = float(option_pp.means[i - 1]);

	coeffs.push_back(mn);

	cv::Mat coef = cv::Mat::zeros((DEGREE_COUNT + 1), 1, CV_32F);
	coeffs.push_back(coef);

	for (int r = 0; r < MAX_CALI_IMAGE_COUNT; r++)
	{
		int ofs = r * (DEGREE_COUNT + 1);
		cv::Mat coef2((DEGREE_COUNT + 1), 1, CV_32F, option_pp.coefs + ofs);
		coeffs.push_back(coef2);
	}

	return coeffs;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
 Raw image의 각 row를 x 좌표 별로 영역을 구별하면
0~149 : laser off 영역
150~1429 : laser on 영역. 1429 = 150 + 1280.
		이 영역에서는 초기에 continuous 한 laser 조사를 했지만
 		해상도가 충분히 나오지 않아 전자그룹 조정현 차장님이 
 		1/4 pixel 구간만 laser를 조사하고 
 		3/4 pixel 구간은 laser를 조사하지 않는 방식으로 스캔 방식을 변경함. (2023-12-15 경)
 		이로 인한 물리적인 효과는 convolution kernel이 작아아져 해상도 향상에 도움이 되는 것으로 추정됨.
 		실험 결과 해상도 05-502 차트스캔 시 기존 방식(continuous laser조사) 대비 3 lp/mm 가량 해상도 향상이 확인됨
1280 : Size 2 IP의 width (1240) + 여유 영역(40; 1mm)
 		Cruxell에서 좌/우 0.5mm (20 pixel)씩 여유 영역을 두고 IP를 설계했을 것으로 추정됨.
1430~1529 : laser off 영역
 		이 영역에서는 laser를 조사하지 않았지만, wobble 보정을 위해 반사판을 이 구간에 삽입, continuous laser 조사하는 방식으로 변경.
 		변경 후 세로 줄 패턴이 생성되고, 이 패턴을 이용하여 wobble 보정을 수행함(CorrectWobble 함수).
		(2025년 10월~11월 약 2개월간 알고리즘 개발 및 검증 진행)
 		* 여기서 말하는 wobble은 Main scan 방향 wobble임. Sub scan 방향 wobble 보정과 무관함.
1530~1535 : Tag 영역. Cruxell에서 RPM 정보를 삽입하기 위해 태그를 삽입했음.
*/
std::vector<cv::Mat> ImageProcessor::Preprocess(std::vector<cv::Mat> coeffs)
{
	// 2024-04-26. jg kim. log 작성을 위해 추가
	char* buf = new char[256];
	wchar_t* wString = new wchar_t[256];
	sprintf(buf, "Preprocess\n");	MultiByteToWideChar(CP_ACP, 0, buf, -1, wString, 256);	OutputDebugString(wString); printf("%s", buf);
	if (m_bSaveCorrectionStep)
		writelog(buf, LOG_FILE_NAME);
	// remove tag
	cv::Mat tagRemoved = RemoveTag(_input);
	sprintf(buf, "PRE_1 RemoveTag\n");
	writelog(buf, LOG_FILE_NAME);
	if(m_nLowResolution)
		SetTiffDPI(2); // 0: HR (400, 400), 1: LR (800, 800), 2: LR_intermediate (400, 800)
		// GenX-CR은 SR(Standard Resolution)과 HR(High Resolution) 모드가 있는데,
		// SR 모드에서 촬영된 영상은 LR(Low Resolution)으로 매칭하여 DPI를 800으로 설정하고, 
		// HR 모드에서 촬영된 영상은 HR로 간주하여 DPI를 400으로 설정함.
		// LR_intermediate는 GenX-CR의 SR 모드에서 촬영된 원본 영상은 
		// Main scan 방향으로는 800 DPI, Sub scan 방향으로는 400 DPI스캔되고 있는데, 
		// 이 영상의 DPI를 400으로 설정하여 보정한 후, 보정된 영상을 다시 800 DPI로 리사이즈하여 최종 결과로 사용하는 방식임.

	if (m_bSaveCorrectionStep)
		cv::imwrite("PRE_1. tag removed.tif", tagRemoved, m_tiffParams);

	tagRemoved = CorrectWobble(tagRemoved);
	// sprintf(buf, "PRE_1 CorrectWobble. RLaserOnPos =%d, RLaserOffPos =%d\n", g_nLaserOnPos, g_nLaserOffPos);
	// writelog(buf, LOG_FILE_NAME);

	if (m_bSaveCorrectionStep)
		cv::imwrite("PRE_1. wobble corrected.tif", tagRemoved, m_tiffParams);

	//Initialize(input);
	cv::Mat bin(tagRemoved.size(), CV_16U);
	std::vector<cv::Mat> res;
	bool bBin = BinarizeIPArea(bin, tagRemoved.clone());
	sprintf(buf, "PRE_2. BinarizeIPArea\n");
	writelog(buf, LOG_FILE_NAME);
	if (bBin)
	{
		// 2024-04-26. jg kim. log 작성을 위해 추가
		sprintf(buf, "Valid binary mask\n");	MultiByteToWideChar(CP_ACP, 0, buf, -1, wString, 256);	OutputDebugString(wString); printf("%s", buf);
		if (m_bSaveCorrectionStep)
			writelog(buf, LOG_FILE_NAME);
		if (m_bSaveCorrectionStep)
			cv::imwrite("PRE_2. bin.tif", bin, m_tiffParams);
		// 2024-03-28. jg kim. 이슈 #17906 (영상이 대각선으로 들어옴)에 대응하기 위해 추가 (저 선량 영역이 있는 영상의 binary mask 생성 개선)
		double min, max;
		//cv::Rect roi(TAG_POSITION + 1, 0, 100, _input.rows / 2);
		minMaxLoc(tagRemoved(m_roiBackground), &min, &max);
		m_fBackgroundmin = min;
		m_fBackgroundmax = max;
		// 2024-07-02. jg kim. IP의 BG 보다 살짝 어두운 영역도 활용하기 위해 변경
		cv::Mat th = GetBinaryImage(tagRemoved);
		// if (m_bSaveCorrectionStep)
		// {
		// 	writelog(buf, LOG_FILE_NAME);
		 	cv::imwrite("PRE_3-1. th.tif", th, m_tiffParams);
		// }
		RotatedRect minRect;
		bin = mergeMask(th, bin, minRect); // 2024-12-13. jg kim. 좌측 및 상단의 기구물 제거를 위해 구현
		// bin = th.clone();// 2024-04-09. jg kim. binary mask를 threshold image로 대체
		m_checkedBin=bin.clone();
		if (m_bSaveCorrectionStep)
		{
				sprintf(buf, "PRE_3. checked bin\n");
				writelog(buf, LOG_FILE_NAME);
				cv::imwrite("PRE_3-2. checked bin.tif", bin, m_tiffParams);
			}

		// 2024-09-24. jg kim. NED map을 위해 rtBounding을 사용하지 않음
		//Rect rect = getBoundingRect<unsigned short>((unsigned short*)bin.ptr(), bin.cols, bin.rows);// = getBoundingRect(bin);
		// 2024-04-16. jg kim. checked binary mask에서 구한 bounding rect이외의 영역에서 binary mask가 여전히(th) 있는 경우가 있어
		// 코드 추가 
		cv::Mat imgF;	// 이하 32-bit 사용
		tagRemoved.convertTo(imgF, CV_32F, 1);	// 2024-03-13. jg kim. 저해상도 영상 관련 이슈 (이슈 #17835) 

		// correct pixel gain
		// 2024-09-24. jg kim. 변수명을 성격에 맞게 변경
		cv::Mat NedPositive(imgF.size(), imgF.type());
		cv::Mat NedNegative(imgF.size(), imgF.type());

		// 2024-09-24. jg kim. NED map을 위해 rtBounding을 사용하지 않음
		GetNedMap(imgF, NedPositive, NedNegative);			// NED map 생성
		if (m_bSaveCorrectionStep)
		{
			sprintf(buf, "PRE_4. GetNedMap\n");
			writelog(buf, LOG_FILE_NAME);
			cv::imwrite("PRE_4. NedNegative.tif", NedNegative, m_tiffParams);
			cv::imwrite("PRE_4. NedPositive.tif", NedPositive, m_tiffParams);
		}
		cv::Mat result = CorrectPixelGains(imgF.clone(), coeffs);	// horizontal shading 보정
		if (m_bSaveCorrectionStep)
		{
			sprintf(buf, "PRE_4. CorrectPixelGains\n");
			writelog(buf, LOG_FILE_NAME);
		}
		g_bProcMtf =true;
		// NED 처리 디버깅을 위해 추가함. 
		// MTF 측정을 위한 영상 보정에는 적합하지 않아 MTF 처리용 영상처리시 이 섹션을 비활성화 시켜야 함.
		if (g_bProcMtf)// 2026-01-12. jg kim. MTF 모드 설정을 위해
		{
			int countsBin0 = PixelCounts(bin, 1000);
			int countsNED = PixelCounts(NedNegative.clone().mul(1000), 1000);
			if (countsNED < countsBin0)
			{
				cv::Mat temp; bin.convertTo(temp, CV_32F);
				divide(65535, temp, temp);
				cv::GaussianBlur(temp, temp, cv::Size(7, 7), 2);
				for (int r = 0; r < temp.rows; r++)
					for (int c = 0; c < temp.cols; c++)
					{
						float val = temp.at<float>(r, c);
						val = val > 1 ? 1 : val < 0 ? 0 : val;
						temp.at<float>(r, c) = val;
					}
				temp.copyTo(NedNegative);
			}
			cv::imwrite("PRE_4. NedNegative_forMTF.tif", NedNegative, m_tiffParams);
		}

		cv::Mat output = ProcessNedArea(imgF, result, NedNegative);
		cv::imwrite("PRE_4. ProcessNedArea_ned.tif", output, m_tiffParams); // 디버깅용 NED 영역 처리 영상 저장

		if (m_bSaveCorrectionStep)
			cv::imwrite("PRE_4. gain.tif", output, m_tiffParams);

		cv::Mat gainOrg = output.clone();
		// 2024-08-15. jg kim. 저해상도 모드 MirrorUniformity correction, WormPattern correction 개선을 위해 순서 변경
		output = CorrectMirrorUniformity(output, bin);
		if (m_bSaveCorrectionStep)
			cv::imwrite("PRE_5. MirrorCorrection.tif", output, m_tiffParams);

		if (m_nLowResolution == 1) // 저해상도(SR모드) 영상을 위한 처리
		{
			// 2024-08-23. jg kim. 레드마인이슈 20303에 대응 위함.
			// 2024-09-24. jg kim. 기존에 구현한 함수로 리팩토링. VerticalExpand 내부에서 clone() 하기 때문에 입력을 clone()하는 코드는 제거
			output = VerticalExpand(output);
			gainOrg = VerticalExpand(gainOrg);
			NedNegative = VerticalExpand(NedNegative);
			NedPositive = VerticalExpand(NedPositive);
			bin = VerticalExpand(bin);
		}
		// 2024-10-31. jg kim. RemoveWormPattern_New는 있어야 할 가로줄 피쳐를 보존하지만,
		// 대부분의 경우 두꺼운 worm pattern이 보정되지 않아 RemoveWormPattern 함수로 복원
		// 실제로는 HR이 아니지만 영상 크기가 HR이어서 HR로 설정함.
		SetTiffDPI(0);// 0: HR (400, 400), 1: LR (800, 800), 2: LR_intermediate (400, 800)
		output = RemoveWormPattern(output, g_nradius, g_nmedRadius, g_nmargin, g_fcutoff, g_bwant_old_mode, g_nradius2, g_fstripe_gain, g_fvary, false);
		sprintf(buf, "PRE_6. RemoveWormPattern\n");
		writelog(buf, LOG_FILE_NAME);

		if (m_bSaveCorrectionStep)
			cv::imwrite("PRE_6. RemoveWormPattern.tif", output, m_tiffParams);

		// 2024-04-17. jg kim.  회의 결론으로 테두리를 검정색으로 표시하지 않기로하여 변경
		output = LineCorrect(output, g_nPreTerm, g_nPostTerm,true);

		if (m_bSaveCorrectionStep && g_bProcMtf)// 2026-01-12. jg kim. MTF 측정을 위한 영상 별도 저장
			cv::imwrite("PRE_7. LineCorrect_MTF.tif", output, m_tiffParams);

		// 2024-10-14. jg kim. 영상 좌/우 엣지 주변에 기구물 흔적이 보이는 경우 처리하기 위함.
		output = BlendImage(gainOrg, imgF, output.clone(), NedPositive, NedNegative, bin);

		if (m_bSaveCorrectionStep)
			cv::imwrite("PRE_7. LineCorrect.tif", output, m_tiffParams);

		if (m_stInfoCorner.n_guessedIndex > CORNERS - 1) // 회전된 쪽을 사용
		{
			cv::Mat rotImage = cv::Mat::ones(output.size(), output.type())*max;
			GetRotatedImage(output, rotImage, minRect);
			sprintf(buf, "PRE_8. ImageRotation\n");
			writelog(buf, LOG_FILE_NAME);
			cv::Mat rotBin = GetRotatedImage(bin, minRect);

			if (m_bSaveCorrectionStep)
				cv::imwrite("PRE_8. ImageRotation.tif", rotImage, m_tiffParams);
			bin = rotBin.clone();
			output = rotImage.clone();
		}
		ImageProcessor::stInfoCorner info = getInfoCorner(); // m_stInfoCorner 값을 받아오는 것을 명시적으로 하기 위함.
		// 2024-04-09. jg kim. 품혁 이슈에 대응하기 위해 코드 변경	
		info.n_guessedIndex = info.n_guessedIndex % CORNERS;
		if (info.n_guessedIndex == -1)
			info.n_guessedIndex = GetIpIndex();

		cv::Rect rt = getBoundingRect(bin);
		float countsBin = float(PixelCounts(bin, 1000));
		float countsDummy = float(ip[info.n_guessedIndex].width * ip[info.n_guessedIndex].height) - ip[info.n_guessedIndex].rad * 2 * CV_PI;
		float ratioHor = float(rt.width) / float(ip[info.n_guessedIndex].width);
		float ratioVer = float(rt.height) / float(ip[info.n_guessedIndex].height);
		float ratioCounts = countsBin / countsDummy;
		float errorHor = ratioHor >= 1.f ? ratioHor - 1.f : 1.f - ratioHor;
		float errorVer = ratioVer >= 1.f ? ratioVer - 1.f : 1.f - ratioVer;
		float errorCounts = ratioCounts >= 1.f ? ratioCounts - 1.f : 1.f - ratioCounts;

		// 2025-04-09. jg kim. 촬영된 조건이 좋은 경우 NED 영역을 위해 binary mask를 크게 잡은 것을 다시 tight 하게 만들기 위해 처리 추가
		// 2025-04-11. jg kim. 레드마인 이슈 22684에 대응하기 위한 코드 변경
		if (errorHor <= 0.1f && errorVer <= 0.1f && errorCounts <= 0.1f)
		{
			cv::minMaxLoc(output, &min, &max);
			cv::Mat temp = (output-min) / (max-min) * 255;
			cv::Mat out8; temp.convertTo(out8, CV_8U);
			cv::threshold(out8, temp, 0, 255, cv::THRESH_OTSU);

			// 2025-04-11. jg kim. 레드마인 이슈 22684에 대응하기 위한 예외처리 추가
			float countsBin2 = float(PixelCounts(temp, 127));
			ratioCounts = countsBin2 / countsBin;
			if (ratioCounts >= 0.9f)
				rt = getBoundingRect(temp);
		}
		sprintf(buf, "PRE_8. Calculate real Pixel size\n");
		writelog(buf, LOG_FILE_NAME);

		// 2024-08-18. jg kim. CropImage에 mapNED를 전달

		cv::Mat out = ExtractIP(output, rt, info); // 2024-03-30. jg kim. 기존의 ExtractIP가 잘 동작하지 않아 새로 구현	

		sprintf(buf, "PRE_9. ExtractIP\n");
		writelog(buf, LOG_FILE_NAME);
		
		if (m_bSaveCorrectionStep)
			cv::imwrite("PRE_9. ExtractIP.tif", out, m_tiffParams);
		
		// 2025-04-08. jg kim. Pixel size 계산을 위한 코드 추가
		// 촬영된 조건이 좋아야 한다.
		// 좋다는 기준 : corner threshold를 넘는 갯수가 3개 이상
		// IP는 size 2 이어야 한다.
		 ratioHor = float(rt.width) / float(ip[2].width);
		 ratioVer = float(rt.height) / float(ip[2].height);
		 errorHor = (ratioHor >= 1.f ? ratioHor - 1.f : 1.f - ratioHor) / 10.f;
		 errorVer = (ratioVer >= 1.f ? ratioVer - 1.f : 1.f - ratioVer) / 10.f;
		if (info.countOverCornerThreshold > 3 && info.n_guessedIndex == 2 && errorHor < PixelSizeError && errorVer < PixelSizeError)
		{
			ratioHor = ratioHor >= 1.f ? 1.f + errorHor : 1.f - errorHor;
			ratioVer = ratioVer >= 1.f ? 1.f + errorVer : 1.f - errorVer;
			float pixelSizeHor = float(PixelSize) * ratioHor;
			float pixelSizeVer = float(PixelSize) * ratioVer;
			if (m_nLowResolution)
			{
				pixelSizeHor *= 2.f;
				pixelSizeVer *= 2.f;
			}
			sprintf(buf, "Pixel size (Hor/Ver) = %.2f um / %.2f um. Error is %.2f percent.\n",pixelSizeHor, pixelSizeVer, (errorHor > errorVer ? errorHor : errorVer)*100.f);
			MultiByteToWideChar(CP_ACP, 0, buf, -1, wString, 256);	OutputDebugString(wString); printf("%s", buf);
		}
		else
		{
			sprintf(buf, "Exceed pixel size error!!!\n");	MultiByteToWideChar(CP_ACP, 0, buf, -1, wString, 256);	OutputDebugString(wString); printf("%s", buf);
		}

		if (m_bSaveCorrectionStep)
			writelog(buf, LOG_FILE_NAME);

		SAFE_DELETE_ARRAY(buf);
		SAFE_DELETE_ARRAY(wString);

		res.push_back(out);
		res.push_back(bin);
		return res;
		// 2024-04-09. jg kim. IP index는 장비 정보를 활용할 수 있도록 변경.
		// 2024-06-10. jg kim. 함수 내부에서 background의 max값을 구하도록 변경.
	}
	else
	{
		// 2024-04-26. jg kim. log 작성을 위해 추가
		sprintf(buf, "Invalid Binary mask\n");	MultiByteToWideChar(CP_ACP, 0, buf, -1, wString, 256);	OutputDebugString(wString);
		if (m_bSaveCorrectionStep)
			writelog(buf, LOG_FILE_NAME);
		// 2024-04-26. jg kim. 반환할 영상의 크기를 IP 크기로 변경.
		SAFE_DELETE_ARRAY(buf);
		SAFE_DELETE_ARRAY(wString);
		cv::Mat out = cv::Mat::zeros(cv::Size(ip[m_nIpIndex].width, ip[m_nIpIndex].height), tagRemoved.type());
		res.push_back(out);
		res.push_back(bin);
		return res;
	}
}

cv::Mat ImageProcessor::BlendImage(cv::Mat gainOrg, cv::Mat imgF, cv::Mat gainCorrected, cv::Mat NedPositive, cv::Mat NedNegative, cv::Mat bin)
{
	//gainCorrected는 LineCorrect까지 완료된 영상임
	double min, max;
	cv::minMaxLoc(imgF(m_roiBackground), &min, &max); // background noise의 min, max 값

	cv::Mat output = cv::Mat::ones(imgF.size(),imgF.type())*max;
	cv::Rect rt = getBoundingRect(bin);
	for (int r = 0; r < output.rows; r++)
		for (int c = 0; c < output.cols; c++)
		{
			if (c >= rt.x && c < rt.x + rt.width - 1)
			{
				if (bin.at<ushort>(r,c) > 0)
				{
					float w = NedNegative.at<float>(r, c);
					// NED 인 경우 false. 대부분의 경우 영상이 있는 경우이며, 영상 내부에 NED가 있어도 채워짐(convex hull 처리 때문)
					output.at<float>(r, c) = gainCorrected.at<float>(r, c)*w + gainOrg.at<float>(r, c)*(1 - w);
					if (NedPositive.at<unsigned char>(r, c) > 0)
						// 실제 NED 인 경우. NED가 없는 영상의 경우, 전체가 0으로 나오기도 한다.
						output.at<float>(r, c) = gainOrg.at<float>(r, c);
				}
			}
		}
	return output;
}

std::vector<cv::Mat> ImageProcessor::Preprocess_PE(std::vector<cv::Mat> coeffs)
{
	UNREFERENCED_PARAMETER(coeffs);
	cv::Mat bin = cv::Mat::zeros(_input.size(), _input.type());
	cv::Mat tagRemoved = RemoveTag(_input);
	double min, max;
	cv::Rect roi(TAG_POSITION + 1, 0, 100, _input.rows / 2);
	minMaxLoc(tagRemoved(roi), &min, &max);
	cv::Mat output = cv::Mat::zeros(_input.size(), CV_32F);
	if (BinarizeIPArea(bin, tagRemoved.clone()))
	{
		RotatedRect minRect;
		//if (m_nImageProcessMode == 2)
		{
			cv::Mat th = GetBinaryImage(tagRemoved.clone());
			int nCountsTh = PixelCounts(th, 1000);
			int nCountsBin = PixelCounts(bin, 1000);

			if (nCountsBin < nCountsTh)
				bin = mergeMask(th, bin, minRect); // 2024-12-13. jg kim. 좌측 및 상단의 기구물 제거를 위해 구현
			else
				bin = th.clone();
		}

		cv::Mat imgF = Mat::zeros(tagRemoved.size(), CV_32F);	// 이하 32-bit 사용
		tagRemoved.convertTo(imgF, CV_32F, 1);

		// remove stripes
		int medRadius = 44;
		float fUseRatioWidth = 0.8f;

		imgF = getMaskedImage(imgF, bin);
		cv::Rect rt = getBoundingRect(bin);
		cv::Mat p = getCorrectionCoefficientsForSingleImage(imgF, rt, fUseRatioWidth);

		output = getHorizonGainCorrectedImage(imgF, p, rt);
		output = RemoveWormPattern(output, g_nradius, medRadius, g_nmargin, g_fcutoff, g_bwant_old_mode, g_nradius2, g_fstripe_gain, g_fvary, !m_bImageProcess);
		output = CorrectMirrorUniformity(output, bin);
		output = LineCorrect(output, g_nPreTerm, g_nPostTerm, true);
		if (m_nImageProcessMode == 2)
		{
			if (m_stInfoCorner.n_guessedIndex > CORNERS - 1) // 회전된 쪽을 사용
			{
				cv::Mat rotImage = cv::Mat::ones(output.size(), output.type())*max;
				GetRotatedImage(output, rotImage, minRect);
				cv::Mat rotBin = GetRotatedImage(bin, minRect);

				bin = rotBin.clone();
				output = rotImage.clone();
				rt = getBoundingRect(bin);
			}
			ImageProcessor::stInfoCorner info = getInfoCorner();
			// 2024-04-09. jg kim. 품혁 이슈에 대응하기 위해 코드 변경	
			info.n_guessedIndex = info.n_guessedIndex % CORNERS;
			if (info.n_guessedIndex == -1)
				info.n_guessedIndex = GetIpIndex();
			output = ExtractIP(output, rt, info);
			bin = (bin.clone())(rt);
		}
	}
	std::vector<cv::Mat> res;
	res.push_back(output);
	res.push_back(bin);
	return res;
}

int ImageProcessor::find_global_minimum(float *fpt_data, int nDataLength) {
	std::vector<float> data(fpt_data, fpt_data+nDataLength);
	int start_index = -1;
	int end_index = -1;

	// 1. 0이 아닌 영역 식별
	for (int i = 0; i < nDataLength; ++i) {
		if (data[i] != 0) {
			if (start_index == -1) {
				start_index = i;
			}
			end_index = i;
		}
	}

	if (start_index == -1) { // 모든 값이 0인 경우
		return -1;
	}

	std::vector<float> local_minima;
	// 2. 0이 아닌 영역 내의 Local Minimum 탐색
	// 기존
	//for (int i = start_index + 1; i < end_index; ++i) { // 경계 조건 주의
	//	if (data[i - 1] > data[i] && data[i] < data[i + 1]) {
	//		local_minima.push_back(i);
	//	}
	//}

	// 설정: 시작과 끝에서만 탐색할 범위 길이
	int range_limit = 30;  // 원하는 탐색 범위 (예: 50개 샘플)

	// 실제 범위 제한 (데이터 크기 초과 방지)
	int range_start = std::min(end_index, start_index + range_limit);
	int range_end = std::max(start_index, end_index - range_limit);

	// 1. 시작 구간 내 로컬 미니멈 탐색
	for (int i = start_index + 1; i < range_start - 1; ++i) {
		if (data[i - 1] > data[i] && data[i] < data[i + 1]) {
			local_minima.push_back(i);
		}
	}

	// 2. 끝 구간 내 로컬 미니멈 탐색
	for (int i = range_end + 1; i < end_index - 1; ++i) {
		if (data[i - 1] > data[i] && data[i] < data[i + 1]) {
			local_minima.push_back(i);
		}
	}

	if (local_minima.empty()) { // local minimum이 없는 경우 (단조 증가/감소)
		int min_val = data[start_index];
		int min_idx = start_index;

		for (int i = start_index + 1; i <= end_index; ++i) {
			if (data[i] < min_val) {
				min_val = data[i];
				min_idx = i;
			}
		}

		return min_idx;
	}

	// 3. Global Minimum 결정
	int global_min_index = local_minima[0];
	for (int local_minimum : local_minima) {
		if (data[local_minimum] < data[global_min_index]) {
			global_min_index = local_minimum;
		}
	}

	return global_min_index;
}

cv::Point2d  ImageProcessor::calculateVector(cv::Point2d point1, cv::Point2d point2)
{
	return cv::Point2d(point2.x - point1.x, point2.y - point1.y);
}

cv::Point2d  ImageProcessor::getArcAngle(cv::Rect rt, cv::Point2d center, eCornerType type)
{
	// 원호(arc)의 bounding rect인 rt로부터 LT, RT, LB, RB 좌표 설정
	std::vector< cv::Point> points = {
	cv::Point(rt.x, rt.y),// LT
	cv::Point(rt.x + rt.width - 1, rt.y),//RT
	cv::Point(rt.x, rt.y + rt.height - 1),//LB
	cv::Point(rt.x + rt.width - 1, rt.y + rt.height - 1)//RB
	};

	cv::Point start, end;
	switch (type) // 중심점 위치에 따라
	{
	case LeftTop://LT
	case RightBottom://RB
	{
		//RT-center-LB 순서로 각도 계산
		start = points[1]; // RT
		end = points[2]; // LB

	}
	break;
	case RightTop://RT
	case LeftBottom://LB
	{
		//RB-center-LT 순서로 각도 계산
		start = points[3]; // RB
		end = points[0]; // LT
	}
	break;
	}
	/*
	c++ atan2를 사용할 때 각도 측정의 기준은?
	C++에서 atan2 함수를 사용할 때, 각도 측정의 기준은 X축입니다. atan2(y, x)는 점 (x, y)와 원점 (0, 0)을 연결하는 직선이 X축과 이루는 각도를 반환합니다.
	이 함수는 두 인수를 사용하여 반시계방향으로 180도(-π)부터 시계방향으로 180도(π) 사이의 각도를 계산합니다. 양의 Y축 위의 점은 90도(π/2), 음의 Y축 위의 점은 -90도(-π/2)의 각도를 나타냅니다.
	기억하시기 좋게 예를 들어 볼게요:
	atan2(1, 0)의 경우, Y축 양의 방향이므로 90도(π/2)입니다.
	atan2(0, 1)의 경우, X축 양의 방향이므로 0도입니다.
	*/
	auto vector_start = calculateVector(center, start);
	double angle_start =  atan2(vector_start.y, vector_start.x);

	auto vector_end = calculateVector(center, end);
	double angle_end = atan2(vector_end.y, vector_end.x);

	return cv::Point2d(angle_start, angle_end);
}
// 타원의 파라미터를 추출하는 함수
void ImageProcessor::fitEllipseToContour(const std::vector<cv::Point>& points, cv::Point2f& center, cv::Size2f& axes, float& angle) {
	// 윤곽선 좌표를 사용하여 타원을 fitting
	cv::RotatedRect ellipse = cv::fitEllipse(points);
	// 타원의 파라미터 추출
	center = ellipse.center; // center point (mass center)
	axes = ellipse.size; //length of each side
	angle = ellipse.angle; //rotation angle in degrees	
}

//void  ImageProcessor::estimateCircle(const std::vector<cv::Point>& points, const cv::Rect rt, double& centerX, double& centerY, double& radius, double& startAngle, double& endAngle)
//{
//	int n = points.size();
//
//	cv::Mat A = cv::Mat::zeros(3, 3, CV_64F);
//	cv::Mat B = cv::Mat::zeros(3, 1, CV_64F);
//	cv::Mat X; // 방정식의 해
//
//	// Delogne–Kåsa method(fitting circle)를 위한 행렬 A 및 벡터 B 생성
//	for (const auto& point : points)
//	{
//		double x = point.x;
//		double y = point.y;
//		double x2 = x * x;
//		double y2 = y * y;
//
//		A.at<double>(0, 0) += x2;
//		A.at<double>(0, 1) += x * y;
//		A.at<double>(0, 2) += x;
//		A.at<double>(1, 0) += x * y;
//		A.at<double>(1, 1) += y2;
//		A.at<double>(1, 2) += y;
//		A.at<double>(2, 0) += x;
//		A.at<double>(2, 1) += y;
//		A.at<double>(2, 2) += 1;
//
//		B.at<double>(0) -= (x2 * x + x * y2);
//		B.at<double>(1) -= (y2 * y + y * x2);
//		B.at<double>(2) -= (x2 + y2);
//	}
//
//	// 방정식을 품
//	cv::solve(A, B, X, cv::DECOMP_SVD);
//
//	// 중심 좌표 및 반지름 계산
//	centerX = -0.5 * X.at<double>(0);
//	centerY = -0.5 * X.at<double>(1);
//
//	double sumR = 0;
//	for (const auto& point : points) {
//		sumR += std::sqrt(std::pow(point.x - centerX, 2) + std::pow(point.y - centerY, 2));
//	}
//
//	radius = sumR / (double)n; // 평균 반지름을 이용한 반지름 보정. 추정값은 참값과 +/- 1픽셀 정도 차이가 있을 수 있음
//
//	cv::Point2d arc_angle = getArcAngle(rt, cv::Point2d(centerX, centerY));
//	startAngle = std::min(arc_angle.x, arc_angle.y);
//	endAngle = std::max(arc_angle.x, arc_angle.y);
//}
cv::RotatedRect ImageProcessor::estimateCircle2(const std::vector<cv::Point2f> points)
{
	int iterations = 1000;
	float threshold = 2.0;

	const int minPoints = 3;  // Minimum points needed to define a circle
	if (points.size() < minPoints) {
		printf("Not enough points to fit a circle.\n");
	}

	int bestInliers = 0;
	Point2f bestCenter;
	float bestRadius = 0.0;

	std::mt19937 rng(static_cast<unsigned>(time(nullptr))); // 현재 시간을 시드로 사용
	std::uniform_int_distribution<int> dist(0, points.size() - 1);
	if (points.empty()) {
		printf("Error: No points available for distribution!\n");
	}

	for (int i = 0; i < iterations; ++i) {
		// Randomly select 3 points
		Point2f p1 = points[dist(rng)];
		Point2f p2 = points[dist(rng)];
		Point2f p3 = points[dist(rng)];

		// Check if the points are collinear
		float area = abs((p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y)) / 2.0);
		if (area < 1e-6) continue; // Skip collinear points

		// Calculate circle parameters (center and radius)
		Point2f tempCenter;
		float tempRadius;
		{
			// Midpoints and slopes
			Point2f mid1 = (p1 + p2) * 0.5;
			Point2f mid2 = (p2 + p3) * 0.5;
			Point2f dir1 = Point2f(-(p2.y - p1.y), p2.x - p1.x);
			Point2f dir2 = Point2f(-(p3.y - p2.y), p3.x - p2.x);

			// Solve intersection
			float a1 = dir1.x, b1 = -dir2.x, c1 = mid2.x - mid1.x;
			float a2 = dir1.y, b2 = -dir2.y, c2 = mid2.y - mid1.y;
			float det = a1 * b2 - a2 * b1;
			if (abs(det) < 1e-6) continue; // Skip if determinant is too small (parallel lines)

			float t1 = (c1 * b2 - c2 * b1) / det;
			tempCenter = mid1 + t1 * dir1;
			tempRadius = norm(tempCenter - p1);
		}

		// Count inliers
		int inliers = 0;
		for (const auto& pt : points) {
			float distance = abs(norm(tempCenter - pt) - tempRadius);
			if (distance < threshold) {
				++inliers;
			}
		}

		// Update best circle if current one is better
		if (inliers > bestInliers) {
			bestInliers = inliers;
			bestCenter = tempCenter;
			bestRadius = tempRadius;
		}
	}

	if (bestInliers == 0) {
		printf("RANSAC failed to find a valid circle.\n");
	}


	cv::RotatedRect rr;
	rr.angle = 0.f;// 0으로
	rr.center = bestCenter; // 원의 중심
	rr.size = cv::Size2f(bestRadius, bestRadius); // 반지름으로 할당
	return rr;
}

cv::RotatedRect ImageProcessor::estimateCircle(const std::vector<cv::Point> points)
{
	int n = points.size();

	cv::Mat A = cv::Mat::zeros(3, 3, CV_64F);
	cv::Mat B = cv::Mat::zeros(3, 1, CV_64F);
	cv::Mat X; // 방정식의 해

	// Delogne–Kåsa method(fitting circle)를 위한 행렬 A 및 벡터 B 생성
	for (const auto& point : points)
	{
		double x = point.x;
		double y = point.y;
		double x2 = x * x;
		double y2 = y * y;

		A.at<double>(0, 0) += x2;
		A.at<double>(0, 1) += x * y;
		A.at<double>(0, 2) += x;
		A.at<double>(1, 0) += x * y;
		A.at<double>(1, 1) += y2;
		A.at<double>(1, 2) += y;
		A.at<double>(2, 0) += x;
		A.at<double>(2, 1) += y;
		A.at<double>(2, 2) += 1;

		B.at<double>(0) -= (x2 * x + x * y2);
		B.at<double>(1) -= (y2 * y + y * x2);
		B.at<double>(2) -= (x2 + y2);
	}

	// 방정식을 품
	cv::solve(A, B, X, cv::DECOMP_SVD);

	// 중심 좌표 및 반지름 계산
	double centerX = -0.5 * X.at<double>(0);
	double centerY = -0.5 * X.at<double>(1);

	double sumR = 0;
	for (const auto& point : points) 
		sumR += std::sqrt(std::pow(point.x - centerX, 2) + std::pow(point.y - centerY, 2));

	double radius = sumR / (double)n; // 평균 반지름을 이용한 반지름 보정. 추정값은 참값과 +/- 1픽셀 정도 차이가 있을 수 있음

	cv::RotatedRect rr;
	rr.angle = 0.f;// 0으로
	rr.center = cv::Point2f(centerX,centerY); // 원의 중심
	rr.size = cv::Size2f(radius, radius); // 반지름으로 할당
	return rr;
}

void  ImageProcessor::getStartEndIndex(cv::Mat data, int &Start, int &End, float threshold)
{
	assert(data.cols == 1 || data.rows == 1);
	const float *ptr = data.ptr<float>();
	int dataSize = data.rows*data.cols;

	Start = End = 0;
	for (int idx = 0; idx < dataSize - 1; idx++)
	{
		float valueCurrent = ptr[idx];
		float valueNext = ptr[idx + 1];
		if (valueCurrent < threshold && valueNext  > threshold) Start = idx + 1;
		if (valueCurrent > threshold && valueNext < threshold) End = idx;
	}

	while (ptr[Start] == ptr[Start + 1] && Start <= End) Start++;
	while (ptr[End] == ptr[End - 1] && Start <= End) End--;
}


struct Circle {
	cv::Point2f center;
	float radius;
};

// Helper function to calculate distance between two points
inline float distance(const cv::Point2f& p1, const cv::Point2f& p2) {
	return std::sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

// Function to compute a circle given 3 points
Circle computeCircleFromThreePoints(const cv::Point2f& p1, const cv::Point2f& p2, const cv::Point2f& p3) {
	float x1 = p1.x, y1 = p1.y;
	float x2 = p2.x, y2 = p2.y;
	float x3 = p3.x, y3 = p3.y;

	float a = 2 * (x2 - x1);
	float b = 2 * (y2 - y1);
	float c = x2 * x2 - x1 * x1 + y2 * y2 - y1 * y1;

	float d = 2 * (x3 - x2);
	float e = 2 * (y3 - y2);
	float f = x3 * x3 - x2 * x2 + y3 * y3 - y2 * y2;

	float denominator = a * e - b * d;
	if (std::abs(denominator) < 1e-6) {
		throw std::runtime_error("Points are collinear, cannot compute a circle.");
	}

	float cx = (c * e - b * f) / denominator;
	float cy = (a * f - c * d) / denominator;
	float radius = distance(cv::Point2f(cx, cy), p1);

	return { cv::Point2f(cx, cy), radius };
}

// LMedS Circle Fitting function
Circle fitCircleLMedS(const std::vector<cv::Point2f>& points, float minRadius, float maxRadius, unsigned int iterations)
{
	int n = points.size();
	if (n < 3) {
		throw std::runtime_error("At least 3 points are required to fit a circle.");
	}

	std::vector<float> medians;
	std::vector<Circle> candidates;

	// Randomly sample 3 points to compute candidate circles
	for (size_t i = 0; i < iterations; ++i) {
		int idx1 = rand() % n;
		int idx2 = rand() % n;
		int idx3 = rand() % n;

		if (idx1 == idx2 || idx2 == idx3 || idx1 == idx3) continue;

		try {
			Circle candidate = computeCircleFromThreePoints(points[idx1], points[idx2], points[idx3]);

			// Check if the radius is within the valid range
			if (candidate.radius >= minRadius && candidate.radius <= maxRadius) {
				// Compute residuals for all points
				std::vector<float> residuals;
				for (const auto& p : points) {
					float dist = distance(candidate.center, p);
					residuals.push_back(std::abs(dist - candidate.radius));
				}

				// Calculate median residual
				std::nth_element(residuals.begin(), residuals.begin() + residuals.size() / 2, residuals.end());
				float medianResidual = residuals[residuals.size() / 2];

				medians.push_back(medianResidual);
				candidates.push_back(candidate);
			}
		}
		catch (const std::exception&) {
			// Skip invalid candidate (e.g., collinear points)
			continue;
		}
	}

	// Find the candidate circle with the smallest median residual
	if (medians.empty()) {
		printf("No valid circles found within the specified radius range.\n");
		return {cv::Point2f(-1.f, -1.f), -1.f};
	}

	auto minIt = std::min_element(medians.begin(), medians.end());
	size_t bestIndex = std::distance(medians.begin(), minIt);

	return candidates[bestIndex];
}

ImageProcessor::ArcInfo ImageProcessor::getArcInfoFromBin(cv::Mat bin, eCornerType type)
{
	cv::Rect rt = getBoundingRect2(bin);
	
	int L = LASER_ON_POSITION;
	int T = SCAN_START_POSITION;
	switch (type)// GenX-CR은 LeftTop 쪽에 기구물이 있어 ArcInfo를 구하지 않음
	{
	case LeftTop:
	{
		L = rt.x;
		T = rt.y;
	}
	break;
	case RightTop:
	{
		L = rt.x + rt.width - 1 - IP_CORNER_RADIUS;
		T = rt.y;
	}
		break;
	case LeftBottom:
	{
		L = rt.x;
		T = rt.y + rt.height - 1 - IP_CORNER_RADIUS;
	}
		break;
	case RightBottom:
	{
		L = rt.x + rt.width - 1 - IP_CORNER_RADIUS;
		T = rt.y + rt.height - 1 - IP_CORNER_RADIUS;
	}
	break;
	}

	double min, max;
	cv::minMaxLoc(bin, &min, &max);
	cv::Mat gray = cv::Mat::zeros(bin.size(), CV_8U);
	cv::Mat Cropped = cv::Mat::zeros(bin.size(), bin.type());
	cv::Rect roi(L, T, IP_CORNER_RADIUS, IP_CORNER_RADIUS);
	bin(roi).copyTo(Cropped(roi));
	for (int r = 0; r < bin.rows; r++)
		for (int c = 0; c < bin.cols; c++)
			gray.at<uchar>(r, c) = Cropped.at<ushort>(r, c) == ushort(max) ? 255 : 0;
	// 여기까지가 getArcImage를 얻기 위한 준비과정 임
	// bounding rect의 네 귀퉁이에서 IP_CORNER_RADIUS x IP_CORNER_RADIUS 영역을 잘라냄(이 안에는 arc가 있다 판단)

	cv::Mat arcImage = getArcImage(gray);
	std::vector<cv::Point> points;
	cv::findNonZero(arcImage, points);
	rt = cv::boundingRect(points);
	std::vector<cv::Point2f> pointsf;
	for (int i = 0; i < (int)points.size(); i++)
	{
		auto p = points.at(i);
		pointsf.push_back(cv::Point2f(p.x, p.y));
	}

	ArcInfo info;
	float r = (float)IP_CORNER_RADIUS;
	Circle circle = fitCircleLMedS(pointsf, r - 0.375, r + 0.375, 2000);
	if (circle.radius < 0)
		circle = fitCircleLMedS(pointsf, r - 0.5, r + 0.5, 2000);

	info.rr.center = circle.center;
	info.rr.size = cv::Size2f(circle.radius, circle.radius);
	info.arc_angle = getArcAngle(rt, info.rr.center, type);
	info.nPixels = (int)points.size();
	return info;
}

cv::Mat ImageProcessor::getArcImage(cv::Mat img_uchar)
{
	int threshold1 = 65;		// Upper threshold for the internal Canny edge detector
	cv::Mat canny;
	cv::Canny(img_uchar, canny, threshold1, threshold1 / 2);
	cv::Rect rt = getBoundingRect2(canny);

	// ROI를 사각형으로 생성해서 발생한 사각형의 edge를 제거하기 위함.
	for (int c = rt.x; c < rt.x + rt.width; c++)
	{
		canny.at<uchar>(rt.y, c) = 0;
		canny.at<uchar>(rt.y + rt.height - 1, c) = 0;
	}
	for (int r = rt.y; r < rt.y + rt.height; r++)
	{
		canny.at<uchar>(r, rt.x) = 0;
		canny.at<uchar>(r, rt.x + rt.width - 1) = 0;
	}

	// pixel 개수가 가장 많은 그룹을 원하는 arc로 봄
	// 연결 요소 레이블링. 사각형의 edge를 제거해도 남은 것들을 제거하기 위함
	Mat labels;
	int numComponents = cv::connectedComponents(canny, labels);

	std::vector<std::vector<cv::Point>> groups;
	int maxGroupIndex = -1;
	int maxGroupPixelCount = 0;
	for (int n = 0; n < numComponents; n++)
	{
		int label = n + 1;
		std::vector<cv::Point> group;
		for (int r = 0; r < canny.rows; r += 2)
		{
			for (int c = 0; c < canny.cols; c++)
				if (labels.at<int>(r, c) == label)
					group.push_back(cv::Point(c, r));
			for (int c = canny.cols - 1; c >= 0; c--)
				if (labels.at<int>(r + 1, c) == label)
					group.push_back(cv::Point(c, r + 1));
		}
		groups.push_back(group);
		if ((int)group.size() > maxGroupPixelCount)
		{
			maxGroupPixelCount = (int)group.size();
			maxGroupIndex = n;
		}
	}

	auto p = groups.at(maxGroupIndex);
	canny = cv::Mat::zeros(canny.size(), canny.type());
	for (const auto& point : p)
		canny.at<uchar>(point) = 255;
	//int N = 4; // Arc에서 x축이나 y축에서 연달아 같은 좌표가 나올때 제외하는 기준
	//// 같은 좌표가 N개 이상 있는 경우 원 추정의 정확도를 떨어뜨리므로 제외
	//std::map<int, int> yCount;
	//std::map<int, int> xCount;

	//// x, y좌표의 빈도수 계산
	//for (const auto& point : p) {
	//	yCount[point.y]++;
	//	xCount[point.x]++;
	//}

	//for (const auto& point : p) {
	//	if (yCount[point.y] < N) 
	//		canny.at<uchar>(point) = 255;
	//	if (xCount[point.x] >= N - 1)
	//		canny.at<uchar>(point) = 0;
	//}

	return canny;
}

cv::Mat ImageProcessor::getCleanBinaryMask(cv::Mat bin, int nBlockSize, float fEdgeThresh)
{
	//BLDC shift index가 맞지 않은 상태에서 scan할 경우 IP 주변에 잔상 같은 것이 보임
	// 이를 제거하기 위한 목적의 함수임.
	int orgType = bin.type();
	if (bin.type() != CV_32F)
	{
		cv::Mat temp;
		bin.convertTo(temp, CV_32F);
		bin = temp.clone();
	}

	// 영상 최상단 및 하단에 가로줄이 있어 지우고 시작함
	for (int r = 0; r < 2; r++)
		for (int c = 0; c < bin.cols; c++)
			bin.at<float>(r, c) = 0;

	for (int r = bin.rows-3; r < bin.rows; r++)
		for (int c = 0; c < bin.cols; c++)
			bin.at<float>(r, c) = 0;

	cv::Rect rt = getBoundingRect(bin);
	cv::Rect rtOrg = rt;
	cv::Mat bin8u = cv::Mat::zeros(bin.size(), CV_8U);
	for (int r = 0; r < bin.rows; r++)
		for (int c = 0; c < bin.cols; c++)
		{
			if (bin.at<float>(r, c) > 0)
				bin8u.at<uchar>(r, c) = 255;
		}

	auto se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(int(nBlockSize), int(nBlockSize)));
	cv::erode(bin8u, bin8u, se);
	// 2026-02-09 jg kim. 예외처리 추가
	int nonzeoroCount = cv::countNonZero(bin8u);
	bool bZeroCount = false;
	if (nonzeoroCount == 0)
	{
		bZeroCount = true;
		for (int r = 0; r < bin.rows; r++)
		for (int c = 0; c < bin.cols; c++)
		{
			if (bin.at<float>(r, c) > 0)
				bin8u.at<uchar>(r, c) = 255;
		}
	}
	if(bZeroCount==false)
	cv::dilate(bin8u, bin8u, se);
	bin8u.mul(255);
	bin8u.convertTo(bin, CV_32F);
	rt = getBoundingRect(bin);

	cv::Mat rowSum;
	cv::reduce(bin, rowSum, 0, cv::REDUCE_SUM, CV_32F);
	// 0 means that the matrix is reduced to a single row.
	// 1 means that the matrix is reduced to a single column.
	rowSum /= 255.f;

	int left = RAW_WIDTH;
	int right = 0;
	std::vector<cv::Point> pts;
	float* sum = (float*)rowSum.ptr();
	int i = 0;
	while (i < RAW_WIDTH)// profile에서 덩어리가 시작되는 index를 pt.x 로, 덩어리의 pixel count를 pt.y 로 입력하여 pts에 push_back
	{
		if (sum[i] > 0)
		{
			cv::Point pt(i,0);
			while (sum[i] > 0)
				pt.y = i++;
			pts.push_back(pt);
		}
		else
			i++;
	}

	if (pts.size() >= 2) // profile에서 덩어리가 2개 이상인 경우. Case 1
	{
		int maxIdx = 0;
		int length = 0;
		for (int j = 0; j<int(pts.size()); j++)
			if (pts.at(j).y - pts.at(j).x > length) // 덩어리의 너비가 최대인 덩어리를 찾음
			{
				length = pts.at(j).y - pts.at(j).x;
				maxIdx = j;
			}

		// 너비가 최대인 덩어리를 사용
		left = pts.at(maxIdx).x;
		right = pts.at(maxIdx).y;
	}	
	else // profile에서 덩어리가 1개인 경우. Case 2
	{
		left = pts.at(0).x;
		right = pts.at(0).y;
	}

	for (int j = 0; j < RAW_WIDTH; j++)
		if (j<left || j>right)
			sum[j] = 0;

	int globalMinIdx = find_global_minimum(sum, RAW_WIDTH);

	double ratio = 0.95; // (double(ip[2].height) - ip[2].rad) / double(ip[2].height); // 잡음이 있는 경우에 대응하기 위해 원호 하나만 포함시키는 것으로 함. 이 정도라도 global minimum은 충분히 찾는다고 생각함.
	double fmin, fmax;
	cv::minMaxLoc(rowSum, &fmin, &fmax);
	if (sum[globalMinIdx] < fmax * ratio) // 덩어리 2개가 붙은 경우
	{
		int center = (left + right) / 2;
		if (globalMinIdx >= center)	right = globalMinIdx;
		else						left  = globalMinIdx;
	}

	// clear peripheral
	for (int r = 0; r < bin.rows; r++)
		for (int c = 0; c < bin.cols; c++)
			if (c<left || c>right)
			{
				bin.at<float>(r, c) = 0.f;
				sum[c] = 0;
			}

	// 2025-11-10 jg kim. Binary mask의 주변부 중 상/하 방향은 고려하지 않도록 변경
	/*
	cv::imwrite("rowSum3.tif", rowSum);
	rt = getBoundingRect(bin);
	cv::Mat colSum;
	float radi = ip[2].rad;
	cv::reduce(bin(cv::Rect(rt.x,0,rt.width,rt.height+radi)), colSum, 1, cv::REDUCE_SUM, CV_32F); // 1 means that the matrix is reduced to a single column.
	colSum /= 255.f;
	cv::Mat colMat;
	cv::transpose(colSum, colMat);
	cv::Mat colDiff = colMat(cv::Rect(1, 0, colMat.cols - 1, 1)) - colMat(cv::Rect(0, 0, colMat.cols - 1, 1));

	cv::imwrite("colSum.tif", colMat);
	cv::imwrite("colDiff.tif",colDiff);
	int maxDiff = 0;
	int maxDiffIdx = -1;
	for (int r = colDiff.cols - 1; r > 0; r--)
		if (colDiff.at<float>( r) > maxDiff)
		{
			maxDiff = colDiff.at<float>( r);
			maxDiffIdx = r;
		}

	for (int r = 0; r <= maxDiffIdx; r++)
		for (int c = 0; c < bin.cols; c++)
			bin.at<float>(r, c) = 0;
	bin.mul(255);
	*/
 
	UNREFERENCED_PARAMETER(fEdgeThresh);
	
	if (bin.type() != orgType)
	{
		cv::Mat temp;
		bin.convertTo(temp, orgType);
		bin = temp.clone();
	}
	return bin.clone();
}

void ImageProcessor::GetRotatedImage(cv::Mat image, cv::Mat &rotImg, cv::RotatedRect minRect)
// 2024-03-30. jg kim. 기존의 ExtractIP가 잘 동작하지 않아 새로 구현
// 2024-04-01. jg kim. 회전 후 일부 영역이 채워지지 않아(원본 영상의 경계 이외의 영역에서 복사해 올 수 없기 때문에 rotImg를 미리 background의 max 값으로 setting(memset)함)
{	
	// 2024-12-13. jg kim. image의 자료형 체크
	int orgType = image.type();
	if (orgType != CV_32F)
	{
		cv::Mat temp = image.clone();
		temp.convertTo(image, CV_32F);
		temp = rotImg.clone();
		temp.convertTo(rotImg, CV_32F);
	}


	float deg = minRect.angle;
	/*if (minRect.size.width >= minRect.size.height)
		deg += 90;*/
	// 2024-06-10. jg kim. CheckBinarizedIPArea에서 같은 조건 검사, 과회전 제한을 하기 때문에 비활성화
	float rot_angle = deg * float(M_PI) / 180.f;
	float cx = minRect.center.x;
	float cy = minRect.center.y;

	float C = float(cos(rot_angle));
	float S = float(sin(rot_angle));
#pragma omp parallel for
	for (int row = 0; row < rotImg.rows; row++) 
	{
		for (int col = 0; col < rotImg.cols; col++) 
		{
			float x = col - cx;
			float y = row - cy;
			float rxf = x * C - y * S + cx;
			float ryf = x * S + y * C + cy;
			if (rxf >= 0 && rxf < rotImg.cols - 1 && ryf >= 0 && ryf < rotImg.rows - 1) 
			{
				int rx = (int)rxf;
				int ry = (int)ryf;
				float wx1 = rx + 1 - rxf;
				float wx2 = 1 - wx1;
				float wy1 = ry + 1 - ryf;
				float wy2 = 1 - wy1;
				rotImg.at<float>(row, col) =
					(image.at<float>(ry, rx) * wx1 + image.at<float>(ry, rx + 1) * wx2)*wy1 +
					(image.at<float>(ry + 1, rx) * wx1 + image.at<float>(ry + 1, rx + 1) * wx2)*wy2;
			}
		}
	}
	
	if (rotImg.type() != orgType)
	{
		cv::Mat temp = image.clone();
		temp.convertTo(image, orgType);
		temp = rotImg.clone();
		temp.convertTo(rotImg, orgType);
	}

}

cv::Mat ImageProcessor::GetRotatedImage(cv::Mat image, cv::RotatedRect minRect)
{
	int orgType = image.type();
	if (orgType != CV_32F)
	{
		cv::Mat temp = image.clone();
		temp.convertTo(image, CV_32F);
	}
	cv::Mat rotImg(image.size(), image.type());

	GetRotatedImage(image, rotImg, minRect);

	if (rotImg.type() != orgType)
	{
		cv::Mat temp = image.clone();
		temp.convertTo(image, orgType);
		temp = rotImg.clone();
		temp.convertTo(rotImg, orgType);
	}
	return rotImg;
}

// 2024-04-09. jg kim. threshold 이상의 pixel을 count
int ImageProcessor::PixelCounts(cv::Mat image, int threshold)
{
	int orgType = image.type();
	if (orgType != CV_16U)
	{
		cv::Mat temp = image.clone();
		temp.convertTo(image, CV_16U);
	}
	int cnts = 0;
	for (int row = 0; row < image.rows; row++)
		for (int col = 0; col < image.cols; col++)
			if (image.at<unsigned short>(row, col) > threshold)
				cnts++;
	return cnts;
}

template <typename T>
cv::Mat ImageProcessor::GetThresholdedImage(cv::Mat image, T threshold)
{
	Mat bin = Mat::zeros(image.size(), CV_16U);
	for (int row = 0; row < image.rows; row++)
		for (int col = 0; col < image.cols; col++)
			if (image.at<T>(row, col) > threshold)
				bin.at<unsigned short>(row, col) = 65535; // 2024-04-16. jg kim. debug
	// 2024-04-22. jg kim. thresholding 결과에서 잡티를 제거하기 위해
	Mat output; bin.convertTo(output, CV_8U);
	auto se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9));
	cv::erode(output, output, se);
	cv::erode(output, output, se);
	cv::dilate(output, output, se);
	cv::dilate(output, output, se);

	for (int row = 0; row < image.rows; row++)
		for (int col = 0; col < image.cols; col++)
			if (output.at<uchar>(row, col) ==0)
				bin.at<unsigned short>(row, col) = 0; // 2024-04-16. jg kim. debug
	return bin;
}

// 2024-10-28. jg kim. 기존 함수를 template으로 구현
template<typename T>
void ImageProcessor::RemoveTag(T * input, T * output, int width, int height)
{
	int tag_l = TAG_POSITION - 1;
	int tag_r = TAG_POSITION + 1;
	// 2024-02-23. jg kim. low x-ray power관련 tag가 추가되어 코드 추가
	// 실시간으로 전송되는 영상에는 tag가 포함되지 않으나 스캔이 완료되면 FW가 영상을 다시 열어 tag를 추가하기 때문에
	// Last image또는 image list에서 선택한 영상에는 포함됨.
	for (int n = 0; n < 4; n++)
	{
		output[n] = input[2 * width + n];// 2024-04-26. jg kim. low x-ray power, door open 등의 태그가 통합되어(4/29 FW 구현 예정) 이것으로 충분하나 
		output[2 * width + n] = input[2 * width + n];// 이전 FW가 적용된 장비에서 획득한 영상도 처리하기 위함.
	}

	for (int y = 0; y < height; y++)
	{
		output[y * width + tag_l] = input[y * width + TAG_POSITION]; // 2024-04-22. jg kim. calibration coefficients
		for (int x = 0; x < TAG_POSITION; x++)
			output[y * width + width - tag_r + x] = input[y * width + width - tag_r - 10 + x];// 2024-04-22. jg kim. BLDC relates
	}
}

template<typename T>
int ImageProcessor::findMinLoc(T * img, int nWidth, int y, int nMaxLoc, int nSearchRange, bool bLeft)
{
	double minValue = img[y*nWidth + nMaxLoc];
	int nMinLoc = nMaxLoc;
	if (bLeft)
	{
		for (int i = nMaxLoc; i >= nMaxLoc - nSearchRange; i--)
		{
			double value = img[y*nWidth + i];
			if (value < minValue)
			{
				nMinLoc = i;
				minValue = value;
			}
		}
	}
	else
	{
		for (int i = nMaxLoc; i < nMaxLoc + nSearchRange; i++)
		{
			double value = img[y*nWidth + i];
			if (value < minValue)
			{
				nMinLoc = i;
				minValue = value;
			}
		}
	}
	return nMinLoc;
}

template<typename T>
int ImageProcessor::findMinLoc_thres(T * img, int nWidth, int y, int nMaxLoc, int nSearchRange, double ratio_max, bool bLeft)
{
	double minValue = double(img[y*nWidth + nMaxLoc])*ratio_max;
	int nMinLoc = nMaxLoc;
	if (bLeft)
	{
		int i = nMaxLoc;
		while (i >= nMaxLoc - nSearchRange && img[y*nWidth + i] > minValue)
			i--;
		nMinLoc = i;
	}
	else
	{
		int i = nMaxLoc;
		while (i < nMaxLoc + nSearchRange && img[y*nWidth + i] > minValue)
			i++;
		nMinLoc = i;
	}
	return nMinLoc;
}


template<typename SrcT, typename DstT>
inline void ImageProcessor::resampleRowShifted(const SrcT* srcRow, DstT* dstRow, int w, double shift, int mode_interpolation)
{
	if (!srcRow || !dstRow || w <= 0)
		return;

	// --------------------------------------
	// ✔ assignPixel1D를 람다로 변경
	//    템플릿 특수화를 쓰던 기존 방식 대신
	//    타입 분기는 std::is_same<T, U>::value 로 처리
	//    (VS2017에서도 is_same<T,U>::value 는 정상 작동)
	// --------------------------------------
	auto assignPixel = [](DstT& dst, double v)
	{
		if (std::is_same<DstT, unsigned short>::value)
		{
			if (v < 0.0)        v = 0.0;
			else if (v > 65535.0) v = 65535.0;
			dst = static_cast<unsigned short>(v);
		}
		else
		{
			dst = static_cast<DstT>(v);
		}
	};
		
	for (int x = 0; x < w; ++x)
	{
		double fx = x + shift;
		int sx = static_cast<int>(fx);   // 기존 코드와 동일: 0 이상일 땐 floor와 동일

		// 공통 경계 체크 (선형 기준: sx, sx+1 필요)
		if (sx < 0 || sx >= w - 1) {
			// 밖이면 0
			assignPixel(dstRow[x], 0.0);
			continue;
		}

		double t = fx - sx;  // [0,1) 부근

		double v = 0.0;

		if (mode_interpolation == 0) {
			// INTERP_CUBIC
			// ----- 1D cubic interpolation (Catmull-Rom) -----
			// p0 = sx-1, p1 = sx, p2 = sx+1, p3 = sx+2
			int i0 = sx - 1;
			int i1 = sx;
			int i2 = sx + 1;
			int i3 = sx + 2;

			// cubic 보간에 필요한 4점이 모두 있지 않으면 선형으로 fallback
			if (i0 < 0 || i3 >= w) {
				double w1 = (sx + 1) - fx;
				double w2 = 1.0 - w1;
				v = static_cast<double>(srcRow[sx])     * w1 +
					static_cast<double>(srcRow[sx + 1]) * w2;
			}
			else {
				double p0 = static_cast<double>(srcRow[i0]);
				double p1 = static_cast<double>(srcRow[i1]);
				double p2 = static_cast<double>(srcRow[i2]);
				double p3 = static_cast<double>(srcRow[i3]);

				// Catmull-Rom Spline
				double a0 = -0.5 * p0 + 1.5 * p1 - 1.5 * p2 + 0.5 * p3;
				double a1 = p0 - 2.5 * p1 + 2.0 * p2 - 0.5 * p3;
				double a2 = -0.5 * p0 + 0.5 * p2;
				double a3 = p1;

				v = ((a0 * t + a1) * t + a2) * t + a3;
			}
		}
		else {
			// ----- 기존 linear interpolation -----
			double w1 = (sx + 1) - fx;
			double w2 = 1.0 - w1;
			v = static_cast<double>(srcRow[sx])     * w1 +
				static_cast<double>(srcRow[sx + 1]) * w2;
		}

		assignPixel(dstRow[x], v);
	}
}

template<typename T>
cv::Rect ImageProcessor::getBoundingRect(T * image, int width, int height)
{
	// 2025-11-11 jg kim. refactoring 하기 전 기존 코드
	/*
	int l = -1;// left
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int n = y * width + x;
			if (image[n] != 0) {
				l = x;	break;
			}
		}
		if (l > 0)			break;
	}
	int r = -1;// right
	for (int x = width - 1; x >= 0; x--) {
		for (int y = 0; y < height; y++) {
			int n = y * width + x;
			if (image[n] != 0) {
				r = x + 1;	break;
			}
		}
		if (r > 0)			break;
	}
	int t = -1;// top
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int n = y * width + x;
			if (image[n] != 0) {
				t = y;	break;
			}
		}
		if (t > 0)	break;
	}
	int b = -1;// bottom
	for (int y = height - 1; y >= 0; y--) {
		for (int x = 0; x < width; x++) {
			int n = y * width + x;
			if (image[n] != 0) {
				b = y + 1;	break;
			}
		}
		if (b > 0)	break;
	}
	if (b >= height)
		b = height - 1;

	return cv::Rect(l, t, r - l + 1, b - t + 1);	
	*/
	
	// 2025-11-11 jg kim. refactoring 함
	// nullptr 또는 비정상 입력 방지
	if (!image || width <= 0 || height <= 0)
		return cv::Rect();

	// 입력 버퍼를 cv::Mat으로 변환 (1채널, 크기: height x width)
	cv::Mat mask(height, width, cv::DataType<T>::type, const_cast<T*>(image));

	// 비제로(포어그라운드) 픽셀 좌표를 벡터로 추출
	std::vector<cv::Point> nonZeroPts;
	cv::findNonZero(mask, nonZeroPts);

	// 포어그라운드가 없으면 빈 사각형 반환
	if (nonZeroPts.empty())
		return cv::Rect();

	// OpenCV 내장 함수로 bounding box 계산
	return cv::boundingRect(nonZeroPts);
}

template<typename T>
void ImageProcessor::drawStripe(T * image, int width, int height, int coordinate, int nTick, int value, bool bVer)
{
	if (bVer) // vertical stripe
	{
		int L = coordinate - nTick/2;
		int R = coordinate + nTick;
		for (int r = 0; r < height; r++)
		{
			for (int c = L; c < R; c++)
			{
				int idx = r * width + c;
				image[idx] = value;
			}
		}
	}
	else // horizontal stripe
	{
		int T = coordinate - nTick / 2;
		int B = coordinate + nTick;
		for (int r = T; r < B; r++)
		{
			for (int c = 0; c < width; c++)
			{
				int idx = r * width + c;
				image[idx] = value;
			}
		}
	}	
}

cv::Mat ImageProcessor::ExtractIP(cv::Mat image, cv::Rect rt, ImageProcessor::stInfoCorner info)// 2024-03-30. jg kim. 기존의 ExtractIP가 잘 동작하지 않아 새로 구현
{
	// 2024-04-26. jg kim. log 작성을 위해 추가
	// char buf[256];
	// wchar_t wString[256];
	// sprintf(buf, "\n");	MultiByteToWideChar(CP_ACP, 0, buf, -1, wString, 256);	OutputDebugString(wString);
	// if (m_bSaveCorrectionStep)
	// 	writelog(buf, LOG_FILE_NAME);
	// 2026-02-20. jg kim. background 처리를 하지 않도록 수정
	// double max, min;
	// cv::Rect roi(TAG_POSITION + 1, 0, 100, _input.rows / 2);
	// cv::minMaxLoc(image(m_roiBackground), &min, &max);
	// 2024-09-26. jg kim. bounding 체크 하고나서 GetCropInfo 실행하도록 변경
	cv::Mat extracted = image(GetCropInfo(info, rt));

	// for (int row = 0; row < extracted.rows; row++)
	// 	for (int col = 0; col < extracted.cols; col++)
	// 	{
	// 		// 2024-08-18. jg kim. 엑스선이 일부 영역에만 조사되는 경우 등 영상이 IP의 width, height보다 작은 경우
	// 		// 빈 공간을 채워 넣어야 하는데, 기존에는 pixel 값이 bg보다 낮은 영역을 모두 빈 공간으로 판단했으나
	// 		// NED 영역에도 bg보다 낮은 pixel 값이 있을 수 있어 판단 기준을 'bg 보다 작은가'에서 '0'으로 변경
	// 		if (extracted.at<float>(row, col) <= m_fBackgroundmax)
	// 			extracted.at<float>(row, col) = m_fBackgroundmax;
	// 	}

	return extracted;
}

// 2024-04-09. jg kim. IP 추출을 위한 정보를 주는 함수
// dstX, dstY를 반환함
cv::Rect ImageProcessor::GetCropInfo(ImageProcessor::stInfoCorner info, cv::Rect rect)
{
	char buf[1024];
	wchar_t wString[1024];
	// 초기 변수 설정
	int ipIdx = info.n_guessedIndex; // guessAndAnalyseIP 함수에서 구한 IP index
	int cropW = ip[ipIdx].width; // 설계치 IP의 너비
	int cropH = ip[ipIdx].height;// 설계치 IP의 높이
	int w = rect.width;//현재 영상의 binary mask에서 구한 bounding box의 너비
	int h = rect.height;//현재 영상의 binary mask에서 구한 bounding box의 높이

	int l = rect.x;
	int r = l + w - 1;
	int t = rect.y;
	int b = t + h - 1;
	int refL = LASER_ON_POSITION;
	int refT = SCAN_START_POSITION;

	if (w % 2) w += 1;
	if (h % 2) h += 1;

	// 기본으로 diffW >= 0, diffH >= 0 조건으로 초기화
	// 설계 크기와 현재 크기의 차이 계산
	int diffW = w - cropW; // 너비 차이
	int diffH = h - cropH; // 높이 차이
	int ofsX = diffW / 2; // 너비 차이의 절반 (좌우로 균등하게 띄우기 위한 오프셋)
	int ofsY = diffH / 2; // 높이 차이의 절반 (상하로 균등하게 띄우기 위한 오프셋)
	int cropX = -1; // cropX, cropY는 추출할 IP의 왼쪽 위 좌표 (dstX, dstY)
	int cropY = -1;
	int cropR = -1;
	int cropB = -1;

	switch (info.countOverCornerThreshold) // 보이는 corner 갯수를 기준으로 처리
	{
	case 0:// corner가 보이지 않는 경우
	{
		cropX = refL; // LASER_ON_POSITION
		cropY = refT; // SCAN_START_POSITION
		sprintf(buf,"Corner,0,LT\n");
	}
		break;
	case 1: // corner가 하나 보이는 경우. 그 corner를 기준으로 cropX, cropY 계산
	{
		switch (info.e_usedCorner)
		{
		case e_LeftTop: // 보이는 corner가 왼쪽 위인 경우
		{
			cropX = l;
			cropY = t;
			sprintf(buf,"Corner,1,LT\n");
		}
		break;
		case e_RightTop: // 보이는 corner가 오른쪽 위인 경우
		{
			cropX = r - cropW;
			cropY = t;
			sprintf(buf,"Corner,1,RT\n");
		}
		break;
		case e_LeftBottom: // 보이는 corner가 왼쪽 아래인 경우
		{
			cropX = l;
			cropY = b - cropH;
			sprintf(buf,"Corner,1,LB\n");
		}
		break;
		case e_RightBottom: // 보이는 corner가 오른쪽 아래인 경우
		{
			cropX = r - cropW;
			cropY = b - cropH;
			sprintf(buf,"Corner,1,RB\n");
		}
		break;
		}
	}
	break;
	case 2: // corner가 두 개 보이는 경우
	{
		switch (info.e_usedCorner)
		{
		case e_LeftEdge: // left edge를 사용하는 경우.
		{
			if (info.scoreCorner[0] > info.scoreCorner[3])// 위쪽의 corner score가 더 높은 경우
				cropY = t;
			else// 아랫쪽의 corner score가 더 높은 경우
				cropY = b - cropH;

			if (diffH < 0) {
				cropX = l + ofsY; sprintf(buf, "Corner,2,LE.Case1\n");
			} else {
				cropX = l; sprintf(buf, "Corner,2,LE.Case2\n");
			}
		}
		break;
		case e_RightEdge:// right edge를 사용하는 경우.
		{
			if (info.scoreCorner[0] > info.scoreCorner[3])// 위쪽의 corner score가 더 높은 경우
				cropY = t;
			else// 아랫쪽의 corner score가 더 높은 경우
				cropY = b - cropH;

			if (diffH < 0) {
				cropX = r - cropW + ofsX; sprintf(buf, "Corner,2,RE.Case1\n");
			} else /*(diffH >= 0 && diffW >= 0)*/ {
				cropX = r - cropW;  sprintf(buf, "Corner,2,RE.Case2\n"); // 같은 간격으로 띄우기 위해
			}
		}
		break;
		case e_TopEdge:// top edge를 사용하는 경우.
		{
			if (info.scoreCorner[0] > info.scoreCorner[1])// 왼쪽의 corner score가 더 높은 경우
				cropX = l;
			else// 오른쪽의 corner score가 더 높은 경우
				cropX = r - cropW - ofsX;

			if (diffW < 0) {
				cropY = t + ofsX; sprintf(buf, "Corner,2,TE.Case1\n");
			} else {
				cropY = t; sprintf(buf, "Corner,2,TE.Case2\n");
			}
		}
		break;
		case e_BottomEdge:// bottom edge를 사용하는 경우.
		{
			if (info.scoreCorner[2] > info.scoreCorner[3])// 왼쪽의 corner score가 더 높은 경우
				cropX = l;
			else// 오른쪽의 corner score가 더 높은 경우
				cropX = r - cropW - ofsX;

			if (diffW < 0) {
				cropY = b - cropH - ofsX;/* 같은 간격으로 띄우기 위해*/ sprintf(buf, "Corner,2,BE.Case1\n");
			} else {
				cropY = b - cropH; sprintf(buf, "Corner,2,BE.Cas2\n");
			}
		}
		break;
		}
	}
	break;
	case 3: // corner가 세개 이상 보이는 경우
	case 4:
	{
		// L, T를 기준으로 계산
		cropX = l + ofsX;
		cropY = t + ofsY;
#ifdef EnableCode
		int ofs = 0;
		int nCase = -1;
		if (diffW >= 0 && diffH >= 0) // Case 1 계열
		{
			// diffW, diffH >= 0 (ofs >= 0) 인 경우
			// ofs를 빼 주면 더 보이고, 더하면 덜 보인다. (L, T)
			// ofs를 더하면 더 보이고, 빼 주면 덜 보인다. (R, B)

			// ofsX >= 0
			// ofsY >= 0
			if (info.corners_L >= info.corners_R && info.corners_T >= info.corners_B)
			{
				nCase = 11; // LT
				cropX = l + ofsX;
				cropY = t + ofsY;
			}
			else if (info.corners_L < info.corners_R && info.corners_T >= info.corners_B)
			{
				nCase = 12; // RT
				cropX = r - cropW - ofsX;
				cropY = t + ofsY;
			}
			else if (info.corners_L >= info.corners_R && info.corners_T < info.corners_B)
			{
				nCase = 13; // LB
				cropX = l + ofsX;
				cropY = b - cropH - ofsY;
			}
			else if (info.corners_L < info.corners_R && info.corners_T < info.corners_B)
			{
				nCase = 14; // RB
				cropX = r - cropW - ofsX;
				cropY = b - cropH - ofsY;
			}
		}		
		else if (diffW >= 0 && diffH < 0) // Case 2 계열
		{
			// diffW, diffH >= 0 (ofs >= 0) 인 경우
			// ofs를 빼 주면 더 보이고, 더하면 덜 보인다. (L, T)
			// ofs를 더하면 더 보이고, 빼 주면 덜 보인다. (R, B)

			// ofsX >=0 // L : width가 dumW 보다 더 크므로 offset을 더하여 차이만큼 적게 보이게 함.
			// ofsY < 0 // T : hieght가 dumH 보다 작으므모 offset을 빼 줘 차이만큼 더 보이게 보이게 함.
			if (info.corners_L >= info.corners_R && info.corners_T >= info.corners_B)
			{
				nCase = 21; // LT
				cropX = l + ofsX; 
				cropY = t + ofsY;
			}
			else if (info.corners_L < info.corners_R && info.corners_T >= info.corners_B)
			{
				nCase = 22; // RT
				cropX = r - cropW - ofsX;
				cropY = t + ofsY;
			}
			else if (info.corners_L >= info.corners_R && info.corners_T < info.corners_B)
			{
				nCase = 23; // LB
				cropX = l + ofsX;
				cropY = b - cropH - ofsX;
			}
			else if (info.corners_L < info.corners_R && info.corners_T < info.corners_B)
			{
				nCase = 24; // RB
				cropX = r - cropW - ofsX;
				cropY = b - cropH - ofsX;
			}
		}
		else if (diffW < 0 && diffH >= 0) // Case 3 계열
		{
			// diffW, diffH >= 0 (ofs >= 0) 인 경우
			// ofs를 빼 주면 더 보이고, 더하면 덜 보인다. (L, T)
			// ofs를 더하면 더 보이고, 빼 주면 덜 보인다. (R, B)
			// ofsX < 0
			// ofsY >=0
			ofs = abs(ofsX) < abs(ofsY) ? abs(ofsX) : abs(ofsY);
			if (info.corners_L >= info.corners_R && info.corners_T >= info.corners_B)
			{
				nCase = 31; // LT
				cropX = l + ofsX;
				cropY = t + ofsY;
			}
			else if (info.corners_L < info.corners_R && info.corners_T >= info.corners_B)
			{
				nCase = 32; // RT
				cropX = r - cropW - ofsX;
				cropY = t + ofsY;
			}
			else if (info.corners_L >= info.corners_R && info.corners_T < info.corners_B)
			{
				nCase = 33; // LB
				cropX = l + ofsX;
				cropY = b - cropH + ofsY;
			}
			else if (info.corners_L < info.corners_R && info.corners_T < info.corners_B)
			{
				nCase = 34; // RB
				cropX = r - cropW - ofsX;
				cropY = b - cropH + ofsY;
			}
		}
		else if (diffW < 0 && diffH < 0) // Case 4 계열
		{
			// diffW, diffH >= 0 (ofs >= 0) 인 경우
			// ofs를 빼 주면 더 보이고, 더하면 덜 보인다. (L, T)
			// ofs를 더하면 더 보이고, 빼 주면 덜 보인다. (R, B)

			// ofsX < 0
			// ofsY < 0
			ofs = abs(ofsX) < abs(ofsY) ? abs(ofsX) : abs(ofsY); // 여기는 ofsX, ofsY 둘 다 < 0 인 경우임.
			if (info.corners_L >= info.corners_R && info.corners_T >= info.corners_B)
			{
				nCase = 41; // LT
				cropX = l - ofs;
				cropY = t - ofs;
			}
			else if (info.corners_L < info.corners_R && info.corners_T >= info.corners_B)
			{
				nCase = 42; // RT
				cropX = r - cropW + ofs;
				cropY = t - ofs;
			}
			else if (info.corners_L >= info.corners_R && info.corners_T < info.corners_B)
			{
				nCase = 43; // LB
				cropX = l - ofs;
				cropY = b - cropH + ofs;

			}
			else if (info.corners_L < info.corners_R && info.corners_T < info.corners_B)
			{
				nCase = 44;
				cropX = r - cropW + ofs;
				cropY = b - cropH + ofs;
			}
		}
		else
			nCase = -99;
#endif

		sprintf(buf, "CropX/Y=%d,%d,Corner,%d,%s%s\n", cropX, cropY,info.countOverCornerThreshold, info.corners_L < info.corners_R ? "R" : "L", info.corners_T < info.corners_B ? "B" : "T"/*, nCase*/);
	}
	break;
	}

	MultiByteToWideChar(CP_ACP, 0, buf, -1, wString, 256);	OutputDebugString(wString);
	// 2024-09-26. jg kim. bounding 범위 체크
	cropY = cropY < 0 ? 0 : cropY;
	cropX = cropX < 0 ? 0 : cropX;
	cropR = cropX + cropW;
	cropB = cropY + cropH;
	cropW = cropR > RAW_WIDTH ? cropR - RAW_WIDTH : cropW;
	cropH = cropB > RAW_HEIGHT ? cropB - RAW_HEIGHT : cropH;
	return cv::Rect(cropX, cropY, cropW, cropH);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::RemoveTag(cv::Mat& input_)
{
	int tag_l = TAG_POSITION - 1;
	int tag_r = TAG_POSITION + 1;
	// 2024-02-23. jg kim. low x-ray power관련 tag가 추가되어 코드 추가
	// 실시간으로 전송되는 영상에는 tag가 포함되지 않으나 스캔이 완료되면 FW가 영상을 다시 열어 tag를 추가하기 때문에
	// Last image또는 image list에서 선택한 영상에는 포함됨.
	for (int n = 0; n < 5; n++) // 2025-03-26. jg kim. current, scan date 및 time을 저장하기 위해 추가된 영역을 반영하기 위해 4에서 5로 변경
	{
		input_.at<unsigned short>(0, n) = input_.at<unsigned short>(2, n); // 2024-04-26. jg kim. low x-ray power, door open 등의 태그가 통합되어(4/29 FW 구현 예정) 이것으로 충분하나 
		input_.at<unsigned short>(1, n) = input_.at<unsigned short>(2, n); // 이전 FW가 적용된 장비에서 획득한 영상도 처리하기 위함.
	}

	for (int y = 0; y < input_.rows; y++)
	{
		input_.at<unsigned short>(y, tag_l) = input_.at<unsigned short>(y, TAG_POSITION); // 2024-04-22. jg kim. calibration coefficients
		for (int x = 0; x < TAG_POSITION; x++)
			input_.at<unsigned short>(y, input_.cols - tag_r + x) = input_.at<unsigned short>(y, input_.cols - tag_r - 10 + x); // 2024-04-22. jg kim. BLDC relates
	}

	return input_.clone();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::CreateConvexHull(cv::Mat& input_)
{
	// 2026-02-09 jg kim. 예외처리 추가. 입력 이미지 유효성 검사
	if (input_.empty() || input_.rows == 0 || input_.cols == 0)
	{
		return input_.clone();
	}

	cv::Mat output = cv::Mat::zeros(input_.size(), CV_8U);

	std::vector<std::vector<cv::Point> > contour1;
	std::vector<cv::Vec4i> hierarchy1;

	// 2026-02-09 jg kim. 예외처리 추가
	try {
		cv::findContours(input_, // input
			contour1, hierarchy1, // output
			cv::RETR_EXTERNAL, // retrieve only the most external (top-level) contours
			cv::CHAIN_APPROX_SIMPLE); // contour approximation algorithm
			// compresses horizontal, vertical, and diagonal segments and leaves only their end points. For example, an up-right rectangular contour is encoded with 4 points
	}
	catch (const cv::Exception& e) {
		UNREFERENCED_PARAMETER(e);
		return input_.clone();
	}

	std::vector<std::vector<cv::Point>> validContours;
	for (auto& c : contour1)
	{
		if (c.size() > 100)
			validContours.emplace_back(c);
	}
	// 2026-02-09 jg kim. 예외처리 추가
	if (validContours.empty())
		return input_.clone();

	cv::drawContours(output, validContours, -1, cv::Scalar(255, 255, 255), -1, 16);	// , cv::FILLED, cv::LINE_AA);

	std::vector<cv::Point> finalContour;
	int largest_contour = 0;

	// contour가 2개 이상인 경우 병함
	if (validContours.size() > 1)
	{
		std::vector<cv::Point> center(validContours.size());
		for (unsigned int n = 0; n < validContours.size(); n++)
		{
			// 2026-02-09 jg kim. 예외처리 추가
			if (validContours[n].empty()) continue;  // 빈 contour 건너뛰기
			cv::Point c(0, 0);
			for (auto& pt : validContours[n])
			{
				c.x += pt.x;
				c.y += pt.y;
			}
			center[n].x = (int)(c.x / (float)validContours[n].size());
			center[n].y = (int)(c.y / (float)validContours[n].size());
		}

		// 연결
		for (unsigned int n = 0; n < validContours.size(); n++)
		{
			int i0 = n;
			int i1 = (n + 1) % validContours.size();

			cv::line(output, center[i0], center[i1], cv::Scalar(255, 255, 255), 3);
		}

		// 새 contour
		// 2026-02-09 jg kim. 예외처리 추가
		try {
			cv::findContours(output, contour1, hierarchy1, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);
		}
		catch (const cv::Exception& e) {
			UNREFERENCED_PARAMETER(e);
			return input_.clone();
		}
		
		if (contour1.empty())
		{
			return input_.clone();
		}
		
		auto size = contour1[0].size();
		for (unsigned long n = 1; n < contour1.size(); n++)
		{
			if (size < contour1[n].size())
			{
				largest_contour = n;
				size = contour1[n].size();
			}
		}
		// 2026-02-09 jg kim. 예외처리 추가		
		if (largest_contour >= 0 && largest_contour < (int)contour1.size())
		{
			cv::drawContours(output, contour1, largest_contour, cv::Scalar(255, 255, 255), -1, 16, hierarchy1);	// cv::FILLED, cv::LINE_AA, hierarchy1);
		}

		validContours = contour1;
	}

	// convex hull
	if (validContours.empty() || largest_contour < 0 || largest_contour >= (int)validContours.size()) // 2026-02-09 jg kim. 예외처리 추가
	{
		return input_.clone();
	}
	
	if (validContours[largest_contour].empty() || validContours[largest_contour].size() < 3) // 2026-02-09 jg kim. 예외처리 추가. convex hull을 계산하기 위해서는 contour에 최소 3개의 점이 필요함.
	{
		return input_.clone();
	}
	
	try {
		std::vector<cv::Point> hull;
		cv::convexHull(validContours[largest_contour], hull);
		if (!hull.empty())
		{
			cv::drawContours(output, std::vector<std::vector<cv::Point>>{hull}, 0, cv::Scalar(255, 255, 0), -1, 16);	// cv::FILLED, cv::LINE_AA);
		}
	}
	catch (const cv::Exception& e) { // 2026-02-09 jg kim. 예외처리 추가
		UNREFERENCED_PARAMETER(e);
		return input_.clone();
	}
	
	return output;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::SeededRegiongrowing(cv::Mat input_, std::queue<cv::Vec2i> q)
{
	cv::Mat mask = cv::Mat::zeros(input_.size(), CV_8U);
	mask.at<uchar>(0, 0) = 255; // left-top
	mask.at<uchar>(0, input_.cols - 1) = 255;// right-top
	mask.at<uchar>(input_.rows - 1, 0) = 255;// left-bottom
	mask.at<uchar>(input_.rows - 1, input_.cols - 1) = 255;// right-bottom
	while (!q.empty())
	{
		auto pt = q.front();
		q.pop();
		int row = pt[0];
		int col = pt[1];

		// 주어진 좌표의 4 point neighbor를 검사하여 pixel 값이 255이면 mask 해당 좌표에 255를 입력
		// pixel 값이 255인 좌표를 q에 push
		// 결국 네 귀퉁이와 연결된 pixel들만 mask에 포함됨.
		if (col > 0 && input_.at<uchar>(row, col - 1) == 255 && mask.at<uchar>(row, col - 1) == 0)
		{
			cv::Vec2i p(row, col - 1);
			q.push(p);
			mask.at<uchar>(p[0], p[1]) = 255;
		}
		if (col < input_.cols - 1 && input_.at<uchar>(row, col + 1) == 255 && mask.at<uchar>(row, col + 1) == 0)
		{
			cv::Vec2i p(row, col + 1);
			q.push(p);
			mask.at<uchar>(p[0], p[1]) = 255;
		}
		if (row > 0 && input_.at<uchar>(row - 1, col) == 255 && mask.at<uchar>(row - 1, col) == 0)
		{
			cv::Vec2i p(row - 1, col);
			q.push(p);
			mask.at<uchar>(p[0], p[1]) = 255;
		}
		if (row < input_.rows - 1 && input_.at<uchar>(row + 1, col) == 255 && mask.at<uchar>(row + 1, col) == 0)
		{
			cv::Vec2i p(row + 1, col);
			q.push(p);
			mask.at<uchar>(p[0], p[1]) = 255;
		}
	}
	return mask;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::CreateInitialBackground(cv::Mat& input_)
{
	cv::Mat output(input_.size(), CV_8U);
	// adaptive thresholding
	double minVal, maxVal;
	cv::minMaxIdx(input_, &minVal, &maxVal);
	cv::Mat temp(input_.size(), CV_8U);

	for (int y = 0; y < input_.rows; y++)
		for (int x = 0; x < input_.cols; x++)
			temp.at<uchar>(y, x) = (uchar)((double)(input_.at<ushort>(y, x) - minVal) / (maxVal - minVal) * 255.0);

	cv::adaptiveThreshold(temp, output, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 561, 5); // blockSize : 561
	// 2024-02-23 jg kim. adaptiveThreshold후 background와 영상의 영역이 분리되지 않는 경우에 대처하기 위해 추가
	// max 값보다 작은 영역에 있는 background를 제거하기 위함. 
	cv::Rect roi(TAG_POSITION + 1, 0, 100, _input.rows / 2);
	cv::minMaxIdx(input_(roi), &minVal, &maxVal);
	for (int y = 0; y < input_.rows; y++)
		for (int x = 0; x < input_.cols; x++)
			if (input_.at<ushort>(y, x) <= maxVal)
				output.at<uchar>(y, x) = 0;

	auto se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(25, 25));
	cv::erode(output, output, se); // erode를 한 번에서 두 번으로 증가
	cv::erode(output, output, se);

	std::queue<cv::Vec2i> q;
	q.push(cv::Vec2i(0, 0));// left-top
	q.push(cv::Vec2i(0, input_.cols - 1));// right-top
	q.push(cv::Vec2i(input_.rows - 1, 0));// left-bottom
	q.push(cv::Vec2i(input_.rows - 1, input_.cols - 1));// right-bottom

	cv::Mat mask = SeededRegiongrowing(output, q);    // trivial한 background 제거

	for (int y = 0; y < input_.rows; y++)
		for (int x = 0; x < input_.cols; x++)
			if (mask.at<uchar>(y, x) == 255) // update된 mask를 이용하여 output 외각부분 제거
				output.at<uchar>(y, x) = 0;
	se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(25, 25));//20220110 9, 9 -> 5, 5 ->25,25
	cv::dilate(output, output, se);
	cv::dilate(output, output, se);
	output = CreateConvexHull(output);	// 2024-12-13. jg kim. binary mask 내부에 비어있는 곳이 있어 convex hull을 한 번 더 함.
	return CreateConvexHull(output);
}

// 2024-12-13. jg kim. 기존의 CreateInitialBackground함수는 adaptive threshold 후에 
// 잡티를 제거하기 위해 erosion, dialtion을 했고, 그 결과 binary mask를 크게 축소시키는 문제가 있있었음.
// morphology를 사용하지 않고 잡티의 영향을 받지 않는 compact 한 binary mask를 만들기 위해 구현
cv::Mat ImageProcessor::CreateInitialBackground_New(cv::Mat & input_)
{
	cv::Mat output(input_.size(), CV_8U);
	// adaptive thresholding
	double minVal, maxVal;
	// 2026-02-09. jg kim. min, max 구하는 영역을 IP가 있을 영역으로 축소. wobble 보정용 반사판 영역을 제외하기 위함.
	// 2026-02-11. jg kim. binary mask가 실제보다 작게 생성되어(레드마인 24063) min, max 구하는 영역 조정.  
	int roi_X = m_roiBackground.x;
	int roi_Y = m_roiBackground.y;
	cv::minMaxIdx(input_(cv::Rect(roi_X,roi_Y,input_.cols-roi_X,input_.rows - roi_Y)), &minVal, &maxVal);
	// 주변광이 유입(Door open)된 경우에 대한 보완
	cv::GaussianBlur(input_, input_, cv::Size(5, 5), 0);
	cv::Mat Img8U(input_.size(), CV_8U);

	for (int y = 0; y < input_.rows; y++)
		for (int x = 0; x < input_.cols; x++)
			Img8U.at<uchar>(y, x) = (uchar)((double)(input_.at<ushort>(y, x) - minVal) / (maxVal - minVal) * 255.0);

	cv::adaptiveThreshold(Img8U, output, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, 561, 5); // blockSize : 561
	// 2024-02-23 jg kim. adaptiveThreshold후 background와 영상의 영역이 분리되지 않는 경우에 대처하기 위해 추가
	// max 값보다 작은 영역에 있는 background를 제거하기 위함.
	//cv::Rect roi(TAG_POSITION + 1, 0, 100, _input.rows / 2);
	
	cv::imwrite("PRE_adaptiveThreshold.tif", output);
	cv::minMaxIdx(input_(m_roiBackground), &minVal, &maxVal);
	for (int y = 0; y < input_.rows; y++)
		for (int x = 0; x < input_.cols; x++)
			if (input_.at<ushort>(y, x) <= maxVal)
				output.at<uchar>(y, x) = 0;
	
	// compact한 binary mask를 만들기 위함.
	ushort cntsRow[RAW_HEIGHT] = { 0, };
	ushort cntsCol[RAW_WIDTH] = { 0, };

	for (int y = 0; y < input_.rows; y++)
		for (int x = 0; x < input_.cols; x++)
			if (output.at<uchar>(y, x) > 0)
				cntsCol[x]++;

	for (int x = 0; x < input_.cols; x++)
		for (int y = 0; y < input_.rows; y++)
			if (output.at<uchar>(y, x) > 0)
				cntsRow[y]++;

	int nEdgeThreshold = 100;
	int L = 0;
	int R = RAW_WIDTH - 1;
	int T = 0;
	int B = RAW_HEIGHT - 1;
	
	while (cntsCol[L] < nEdgeThreshold && L < output.cols / 2)		L++;
	while (cntsCol[R] < nEdgeThreshold && R > output.cols / 2)		R--;		
	while (cntsRow[T] < nEdgeThreshold && T < output.rows / 2)		T++;
	while (cntsRow[B] < nEdgeThreshold && B > output.rows / 2)		B--;
	
	// 변수가 여러번 재정의됨. 
	cv::Rect roi2(L, T, R - L + 1, B - T + 1);

	cv::Mat temp = output.clone();
	output = cv::Mat::zeros(output.size(), output.type());
	temp(roi2).copyTo(output(roi2));

	// 2025-11-10 jg kim. Wobble 보정을 위한 반사판 추가로 인해 binary mask에 반사판이 보이는데, 이것을 제거하기 위해 추가한 코드
	float fEdgeThresh = 0.65f;
	int nKernelSize = 25;

	output = getCleanBinaryMask(output, nKernelSize, fEdgeThresh);
	cv::imwrite("PRE_adaptiveThreshold_cleaned.tif", output);
	return CreateConvexHull(output);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
double ImageProcessor::AverageLine8(unsigned short* input_, int width, int height, const ushort* mask, int x_start, int y_start, int y_end, int stride, int widthKernel)
{
	UNREFERENCED_PARAMETER(height);

	int mask_count = 0;
	double sum_data = 0; // ( size3 세로길이 + a ) / 8 * 64 * 65535 해도 int 표현범위 내라서 int배열씀

	for (int y = y_start; y <= y_end; y += stride)
	{
		for (int x = x_start; x < x_start + widthKernel; ++x)
		{
			if (x >= width)
				break;

			if (mask[y * width + x] != 0)
			{
				mask_count++;
				sum_data += input_[y * width + x];
			}
		}
	}
	return mask_count > 0 ? (double)((double)sum_data / (double)mask_count) : 0;
}


#pragma region Mirror Uniformity Correction

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//using 20211113
void ImageProcessor::kernel_set_avr_line8(unsigned short* input_, int width, int height, const ushort* mask, float* output,
	int x_start, int y_start, int y_end, double offset, int stride, double background, int widthKernel)
{
	UNREFERENCED_PARAMETER(height);
	UNREFERENCED_PARAMETER(background);

	for (int y = y_start; y <= y_end; y += stride)
	{
		for (int x = x_start; x < x_start + widthKernel; ++x) // 어차피 이진화 영역만 계산되고 이미지 우측 여유있어서 64커널로 다돌려도될듯
		{
			if (x >= width)
				break;
			if (mask[y * width + x] != 0)
			{
				double value = (double)(input_[y * width + x]);
				// if (value > background) // 2024-10-30. jg kim. 미러 패턴이 제대로 제거되지 않아 모든 케이스에 적용
				value *= offset;
				if (value >= 65535.0)
					value = 65535.0;
				output[y * width + x] = (float)(value);// in_ptr[y * img_width + x] * offset;
			}
		}
	}
}
// 2025-11-10 jg kim. 별도의 프로젝트에서 구현한 코드를 ImageProcessor클래스에 통합함
//cv::Mat ImageProcessor::CorrectWobble(cv::Mat img)
//{
//	int nSearchRange = 2;
//	cv::Mat _output=cv::Mat::zeros(img.size(), img.type());
//	cv::Mat imgF; img.convertTo(imgF, CV_32F);
//	int xs, xe;
//	getBandRegion(img, xs, xe);
//	m_nxs = xs;
//	m_nxe = xe;
//	double min_band, max_band;
//	cv::Rect roi_band(xs, 0, xe - xs, imgF.rows); // 반사판 영역을 ROI로 설정
//	cv::minMaxLoc(imgF(roi_band), &min_band, &max_band);
//	cv::Mat bin_Band = cv::Mat::zeros(img.size(), img.type());
//	bin_Band(roi_band).setTo(65535);
//
//	double min, max, mean, stddev;
//	cv::Rect roi(TAG_POSITION + 1, 0, 100, img.rows / 2);
//	cv::minMaxLoc(img(roi), &min, &max);
//	getMeanStdOfMat(img(roi), mean, stddev);
//
//	float *pt_imgF = (float*)imgF.ptr();
//	for (int r = 0; r < imgF.rows; r++) // 반사판 영역의 normalize
//	{
//		double min_local, max_local;
//		cv::minMaxLoc(imgF(cv::Rect(xs, r, xe - xs, 1)), &min_local, &max_local);
//		for (int c = xs; c < xe; c++)
//		{
//			float val = pt_imgF[r*imgF.cols + c];
//			pt_imgF[r*imgF.cols + c] = float((val - min) / (max_local - min)*max_band);
//		}
//	}
//	imgF = CorrectMirrorUniformity(imgF, bin_Band);// 반사판 영역의 미러 패턴 보정
//	imgF.convertTo(img, CV_16U);
//
//	int nHeight = m_nLowResolution ? img.rows / 2 : img.rows;
//	unsigned short *ptOut = (unsigned short*)_output.ptr();
//	CorrectWobble(ptOut, (unsigned short*)img.ptr(), img.cols, nHeight, nSearchRange);
//	return _output;
//}

void ImageProcessor::getIpHeight(cv::Mat img, int xs, int & ys, int & ye)
{
	double min, max, mean, stddev;
	cv::Rect roi(TAG_POSITION + 1, 0, 100, img.rows / 2);
	cv::minMaxLoc(img(roi), &min, &max);
	getMeanStdOfMat(img(roi), mean, stddev);
	double thres = max + stddev;

	cv::Mat bin;
	cv::threshold(img, bin, thres, 1, cv::THRESH_BINARY);

	cv::Mat colSum;
	cv::reduce(bin(cv::Rect(0, 0, xs, img.rows)), colSum, 1, cv::REDUCE_SUM, CV_32F); // 1 means that the matrix is reduced to a single column. x축 방향의 픽셀 수를 구하기 위함.
	ys = 0;
	ye = img.rows - 1;

	int* pt = colSum.ptr<int>();

	while (ys < img.rows && pt[ys] < 100) // x축 방향의 픽셀 수가 100보다 작은 경우 IP의 시작점이 아니라고 판단하고 ys를 증가시킴.
		ys++;

	while (ye > 0 && pt[ye] < 100)	// x축 방향의 픽셀 수가 100보다 작은 경우 IP의 끝점이 아니라고 판단하고 ye를 감소시킴.
		ye--;

	printf("ys =%d, ye=%d\n", ys, ye);
}

cv::Mat ImageProcessor::CorrectWobble(const cv::Mat input)
{
	// 파라미터/기본 설정
	const int nSearchRange = 2;
	CV_Assert(input.type() == CV_16U); // 가정: 16비트 그레이 이미지

	// 반사판 영역 (band) 구간 구하기
	cv::Rect roiBand = getBandRegion(input);
	int xs = roiBand.x;
	int xe = xs + roiBand.width;
	int ys = roiBand.y;
	int ye = ys + roiBand.height;
	// 2026-02-06. jg kim. wobble 보정을 위한 반사판 영역의 bounding
	xs = xs < 1430 ? 1430 : xs>1530 ? 1530 : xs;
	xe = xe < 1430 ? 1430 : xe>1530 ? 1530 : xe;

	// Wobble 보정 적용
	if (xe < xs) // 2026-02-03 jg kim. 반사판 영역을 찾지 못한 경우 대응. 보정하지 않지만 이후 영상보정에서 정보가 필요함.
	{
		char buf[1024];
		sprintf(buf, "Invalid vertical line for Wobble Correction\n");
		printf("%s", buf);
		writelog(buf, LOG_FILE_NAME);
		g_nCorrectWobbleResult = -1;
		xs = 1430;
		xe = 1530;
		g_nLaserOnPos = xs;
		g_nLaserOffPos = xe;
		return input.clone();
	}
	else
	{
		// 2026-02-06 jg kim. Binarymask 생성시 반사판 영역의 제거를 위함.
		g_nLaserOnPos = xs;
		g_nLaserOffPos = xe;
		return CorrectWobble(input, xs, xe, ys, ye, bool(m_nLowResolution), nSearchRange);
	}
}

void ImageProcessor::getMeanStdOfMat(cv::Mat img, double & mean, double & stdev)
{
	// 2024-12-17 jg kim. debug. 데이터 형 체크 추가
	cv::Mat data = img.clone();
	if (img.type() != CV_32F)
		img.convertTo(data, CV_32F);

	// 2024-02-25 jg kim. 급해서 일단 구현했음.
	mean = 0;
	for (int r = 0; r < img.rows; r++)
	{
		for (int c = 0; c < img.cols; c++)
		{
			mean += data.at<float>(r, c);
		}
	}
	mean /= float(img.rows * img.cols);

	stdev = 0; // std = sum( (x-m)^2 )
	for (int r = 0; r < img.rows; r++)
	{
		for (int c = 0; c < img.cols; c++)
		{
			float e = (float)mean - data.at<float>(r, c);
			stdev += e * e;
		}
	}
	stdev = sqrt(stdev / float(img.rows * img.cols - 1));
}

/*
	getBandRegion 반사판의 시작/끝 위치를 구하는 함수
	반사판 위치 xs, xe를 구하고, 구하는 방법이 FW가 써 준 위치를 해독한 것이면 1, SW적으로 계산한 것이면 0을 return 값으로 반환함
*/

bool ImageProcessor::getBandRegion(cv::Mat img, int & xs, int & xe)
{
	/*
		FW가 써 주는 반사판 시작(xs) 및 끝(xe) 위치
		xs, xe는 1430~1530 범위에 있어야 함.
		(2,0) => Right Laser On Position (pixel unit)  :: xs
		(2,1) => Right Laser Off Position (pixel unit) :: xe
	*/

	//unsigned short*ptImg = (unsigned short*)img.ptr();

	int bandStart = LASER_ON_POSITION + ip[2].width; // 가능한 반사판 시작 위치
	int bandEnd = img.cols - TAG_POSITION;	// 가능한 반사판 끝 위치
	
	//m_nFWxs = ptImg[img.cols * 2]; // FW가 써 준 반사판 시작 위치
	//m_nFWxe = ptImg[img.cols * 2 + 1]; // FW가 써 준 반사판 끝 위치

	// FW가 
	//if (xsFW >= bandStart && xsFW < bandEnd && xeFW >= bandStart && xeFW < bandEnd) // FW가 써 준 반사판 시작/끝 위치가 유효하면
	//{
	//	xs = xsFW;
	//	xe = xeFW;
	//	return true;
	//}
	//else // FW가 써 준 반사판 시작/끝 위치가 유효하지 않는 경우 영상을 분석하여 반사판 영역을 추출함
	{
		// raw image의 첫 줄(y=0)의 반사판 영역에서 noise + std(noise)보다 큰 영역 추출
		// FW가 써 주는 반사판 영역 보다는 적게 추출되는 경향이 있음
		double min, max, mean, stddev;
		cv::Rect roi(TAG_POSITION + 1, 0, 100, img.rows / 2);
		cv::minMaxLoc(img(roi), &min, &max);
		getMeanStdOfMat(img(roi), mean, stddev);
		double thres = max + stddev;
		xs = bandStart;
		xe = bandEnd;
		unsigned short value = 0;
		while (xs < bandEnd && value < thres)
		{
			value = img.at<unsigned short>(0, xs);
			xs++;
		}
		value = 0;
		while (xe > bandStart && value < thres)
		{
			value = img.at<unsigned short>(0, xe);
			xe--;
		}
		return false;
	}
}
cv::Rect ImageProcessor::getBandRegion(cv::Mat img)
{
	// 2026-02-03 jg kim. 초기값 수정
	int xs = 0, xe = img.cols - 1;
	int ys = 0, ye = img.rows - 1;
	getBandRegion(img, xs, xe);
	getIpHeight(img, xs, ys, ye);

	return cv::Rect(xs, ys, xe-xs, ye-ys);
}
// -----------------------------
// Hessian |Largest Eigenvalue| 영상 계산
// -----------------------------

cv::Mat ImageProcessor::computeHessianEigenImage(const cv::Mat src16u, double sigma)
{
	CV_Assert(!src16u.empty() && src16u.channels() == 1);

	// ---- 람다: 1D 컨볼루션 (수평/수직) ----
	auto convolve1D = [](const cv::Mat& src, cv::Mat& dst,
		const std::vector<double>& kernel, bool horizontal)
	{
		CV_Assert(!kernel.empty() && (kernel.size() % 2 == 1)); // 홀수 커널

		// 입력을 CV_64F로 확보
		cv::Mat src64;
		if (src.type() != CV_64F) src.convertTo(src64, CV_64F);
		else                      src64 = src;

		// 1D 커널 Mat 구성
		cv::Mat k;
		if (horizontal) {
			k = cv::Mat(1, static_cast<int>(kernel.size()), CV_64F);
			std::memcpy(k.ptr<double>(0), kernel.data(), sizeof(double) * kernel.size());
		}
		else {
			k = cv::Mat(static_cast<int>(kernel.size()), 1, CV_64F);
			std::memcpy(k.ptr<double>(0), kernel.data(), sizeof(double) * kernel.size());
		}

		// filter2D 적용 (경계: REPLICATE = clamp padding)
		cv::filter2D(src64, dst, CV_64F, k,
			cv::Point(-1, -1),  // 중심 자동
			0.0,                // bias 없음
			cv::BORDER_REPLICATE);
	};

	// ---- 람다: 가우시안 및 도함수 커널 생성 ----
	auto createGaussianKernels = [](std::vector<double>& G,
		std::vector<double>& Gp,
		std::vector<double>& Gpp,
		int ksize, double sigma)
	{
		G.resize(ksize); Gp.resize(ksize); Gpp.resize(ksize);
		int r = ksize / 2;
		double s2 = sigma * sigma;
		double s4 = s2 * s2;
		double norm = 1.0 / std::sqrt(2.0 * CV_PI * s2);

		for (int i = -r; i <= r; ++i) {
			double x = static_cast<double>(i);
			double e = std::exp(-(x * x) / (2.0 * s2));
			G[i + r] = norm * e;
			Gp[i + r] = -(x / s2) * G[i + r];            // 1차 미분
			Gpp[i + r] = ((x * x - s2) / s4) * G[i + r];  // 2차 미분
		}
	};

	// ---- 람다: 헤시안 성분 계산 (Ixx, Iyy, Ixy) ----
	auto computeHessian = [&](const cv::Mat& src16u_in,
		cv::Mat& Ixx, cv::Mat& Iyy, cv::Mat& Ixy,
		double sigma_in)
	{
		int radius = static_cast<int>(std::ceil(3 * sigma_in));
		int ksize = 2 * radius + 1;

		std::vector<double> G, Gp, Gpp;
		createGaussianKernels(G, Gp, Gpp, ksize, sigma_in);

		// 입력을 CV_64F로
		cv::Mat src64;
		if (src16u_in.type() != CV_64F) src16u_in.convertTo(src64, CV_64F);
		else                             src64 = src16u_in;

		cv::Mat tmp;

		// Ixx = Gpp(x) * G(y)
		convolve1D(src64, Ixx, Gpp, /*horizontal=*/true);

		// Iyy = G(x) * Gpp(y)
		convolve1D(src64, Iyy, Gpp,  /*horizontal=*/false);

		// Ixy = Gp(x) * Gp(y)
		convolve1D(src64, tmp, Gp,  /*horizontal=*/true);
		convolve1D(tmp, Ixy, Gp,   /*horizontal=*/false);
	};

	// 1) 헤시안 성분 계산 (CV_64F)
	cv::Mat Ixx, Iyy, Ixy;
	computeHessian(src16u, Ixx, Iyy, Ixy, sigma);

	// 2) 2x2 대칭 행렬 [[Ixx, Ixy], [Ixy, Iyy]]의 고유값
	//    λ = (Ixx+Iyy)/2 ± sqrt( ((Ixx-Iyy)/2)^2 + Ixy^2 )
	cv::Mat halfSum = (Ixx + Iyy) * 0.5;      // (Ixx + Iyy)/2
	cv::Mat halfDiff = (Ixx - Iyy) * 0.5;      // (Ixx - Iyy)/2
	cv::Mat rad;
	cv::sqrt(halfDiff.mul(halfDiff) + Ixy.mul(Ixy), rad);

	cv::Mat lambda1 = halfSum + rad;
	cv::Mat lambda2 = halfSum - rad;

	// 3) 최대 절대값 고유값: max(|λ1|, |λ2|)
	cv::Mat abs1 = cv::abs(lambda1);
	cv::Mat abs2 = cv::abs(lambda2);
	cv::Mat eigenImg(src16u.size(), CV_64F);
	cv::max(abs1, abs2, eigenImg);             // CV_64F

	return eigenImg;
}

double ImageProcessor::getBestMatchPositionNormalizedFit(
	double* fInputImg,
	int nWidth,
	int nHeight,
	double* fReferance,
	int nLengthReference,
	int y,
	int nMaxLoc,
	int nDataPoinits,
	double trustRangePx)
{
	UNREFERENCED_PARAMETER(nLengthReference);

	// 람다 함수들
	// 유틸: 유한수 확인
	auto finite_num = [](double v) -> bool {
		return std::isfinite(v);
	};

	// 포물선 꼭짓점 계산
	auto parabola_vertex_x = [&](const double* coeffs, double& outX) -> bool
	{
		const double a = coeffs[0];
		const double b = coeffs[1];
		const double eps = 1e-12;
		if (std::fabs(a) < eps) return false;
		outX = -b / (2.0 * a);
		return finite_num(outX);
	};

	// 2차 곡선 최소자승 피팅
	auto fit_2nd_order = [&](double* fDataX,
		double* fDataY,
		int nDataPoinits,
		double* fCoeffs)
	{
		int n = nDataPoinits;

		cv::Mat X(n, 3, CV_64F);
		cv::Mat y(n, 1, CV_64F);

		for (int i = 0; i < n; ++i) {
			double x = fDataX[i];
			double valY = fDataY[i];

			X.at<double>(i, 0) = x * x;
			X.at<double>(i, 1) = x;
			X.at<double>(i, 2) = 1.0;
			y.at<double>(i, 0) = valY;
		}

		cv::Mat coeffs;
		bool ok = cv::solve(X, y, coeffs, cv::DECOMP_NORMAL | cv::DECOMP_SVD);
		if (!ok) {
			fCoeffs[0] = fCoeffs[1] = fCoeffs[2] = 0.0;
			return;
		}

		fCoeffs[0] = coeffs.at<double>(0, 0);
		fCoeffs[1] = coeffs.at<double>(1, 0);
		fCoeffs[2] = coeffs.at<double>(2, 0);
	};

	// 안전한 중앙 인덱스 계산(홀/짝 모두 대응)
	auto half_points = [](int n) {
		return n / 2;
	};

	if (!fInputImg || !fReferance || nWidth <= 0 || nHeight <= 0 || y < 0 || y >= nHeight)
		return 0.0;

	if (!std::isfinite(trustRangePx) || trustRangePx < 0.0)
		trustRangePx = 2.0;

	if (nDataPoinits <= 2 || nWidth < nDataPoinits) {
		return std::clamp(static_cast<double>(nMaxLoc), 0.0, static_cast<double>(nWidth - 1));
	}

	const int halfN = half_points(nDataPoinits);
	int startX = std::clamp(nMaxLoc - halfN, 0, nWidth - nDataPoinits);

	// 1) 입력 구간의 x좌표 (원점 = nMaxLoc)
	std::vector<double> fDataX(nDataPoinits);
	for (int i = 0; i < nDataPoinits; ++i) {
		int xi = startX + i;
		fDataX[i] = static_cast<double>(xi - nMaxLoc);
	}

	// 2) Reference 피팅 (원점 = 중앙)
	std::vector<double> fDataX_ref(nDataPoinits);
	for (int i = 0; i < nDataPoinits; ++i)
		fDataX_ref[i] = static_cast<double>(i - halfN);

	double coeffRef[3] = { 0,0,0 };
	fit_2nd_order(fDataX_ref.data(), fReferance, nDataPoinits, coeffRef);

	double maxPosX_Ref = 0.0;
	bool okRef = parabola_vertex_x(coeffRef, maxPosX_Ref);
	if (!okRef) maxPosX_Ref = 0.0;

	// 3) 현재 입력 구간 포물선 피팅
	const double* pDataY = fInputImg + y * nWidth + startX;
	std::vector<double> dataY(pDataY, pDataY + nDataPoinits);

	double coeff[3] = { 0,0,0 };
	fit_2nd_order(fDataX.data(), dataY.data(), nDataPoinits, coeff);

	double maxPosX = 0.0;
	bool okCur = parabola_vertex_x(coeff, maxPosX);

	// fallback (3-point interpol.)
	if (!okCur || !std::isfinite(maxPosX) || std::fabs(maxPosX) > 5.0)
	{
		int nRange = int(nDataPoinits / 2);
		int i0 = std::max(1, nMaxLoc - nRange);
		int i1 = std::min(nWidth - nRange, nMaxLoc + nRange);

		int argmax = nMaxLoc;
		double best = -DBL_MAX;
		const double* row = fInputImg + y * nWidth;
		for (int xi = i0; xi <= i1; ++xi) {
			double v = row[xi];
			if (v > best) { best = v; argmax = xi; }
		}

		int xm = std::clamp(argmax, 1, nWidth - 2);
		double y_m1 = row[xm - 1];
		double y_0 = row[xm];
		double y_p1 = row[xm + 1];

		double denom = (y_m1 - 2.0 * y_0 + y_p1);
		double d = 0.0;
		if (std::fabs(denom) > 1e-12)
			d = 0.5 * (y_m1 - y_p1) / denom;

		d = std::clamp(d, -trustRangePx / 2, trustRangePx / 2);
		maxPosX = (xm + d) - nMaxLoc;
	}

	// 4) Reference와의 상대 변화량
	double delta = maxPosX - maxPosX_Ref;
	delta = std::clamp(delta, -trustRangePx, trustRangePx);

	double retValue = static_cast<double>(nMaxLoc) + delta;

	if (!finite_num(retValue))
		retValue = static_cast<double>(nMaxLoc);

	return std::clamp(retValue, 0.0, static_cast<double>(nWidth - 1));
}

std::pair<double, double> ImageProcessor::mean_std(const std::vector<double>& v)
{
	const std::size_t n = v.size();
	if (n <= 1) return { 0.0, 0.0 };

	const double m = std::accumulate(v.begin(), v.end(), 0.0) / static_cast<double>(n);

	double ss = 0.0;
	for (double x : v) {
		const double d = x - m;
		ss += d * d;
	}

	const double variance = ss / static_cast<double>(n - 1);
	return { m, variance };
}

std::vector<double> ImageProcessor::mean_std_min_max(const std::vector<double>& v)
{
	auto[fmean, fstd] = mean_std(v);
	auto[fmin, fmax] = std::minmax_element(v.begin(), v.end());
	return { fmean, fstd, *fmin, *fmax };
}

void ImageProcessor::printShiftStats(const std::vector<double>& shift, const char* label)
{
	auto stats = mean_std_min_max(shift);
	if (stats.size() < 4) return;
	double mean = stats[0], stdv = stats[1], minv = stats[2], maxv = stats[3];
	char buf[256];
	sprintf_s(buf, "%s) mean=%.3f, std=%.3f, min=%.3f, max=%.3f, range=%.2f\n",
		label, mean, stdv, minv, maxv, maxv - minv);
	writelog(buf, LOG_FILE_NAME);
	printf("%s", buf);
}

std::vector<double> ImageProcessor::makeNormalizedPatch(const std::vector<double>& src, int start, int len)
{
	std::vector<double> patch;

	if (src.empty() || len <= 0) {
		patch.assign(std::max(len, 1), 0.0);
		return patch;
	}

	int srcSize = static_cast<int>(src.size());
	start = std::clamp(start, 0, srcSize - 1);
	int end = start + len;
	end = std::clamp(end, start + 1, srcSize); // 최소 1개 이상

	patch.assign(src.begin() + start, src.begin() + end);

	double minVal = *std::min_element(patch.begin(), patch.end());
	double maxVal = *std::max_element(patch.begin(), patch.end());
	double denom = std::max(maxVal - minVal, 1e-12);

	for (auto& v : patch) {
		v = (v - minVal) / denom;
	}
	return patch;
}

int ImageProcessor::findMaxX(int y, int xs, int xe, int width, const std::vector<double>& mat)
{
	xs = std::clamp(xs, 0, width - 1);
	xe = std::clamp(xe, xs, width - 1);

	double maxVal = -DBL_MAX;
	int idx = xs;

	int rowOffset = y * width;
	for (int x = xs; x <= xe; ++x) {
		double v = mat[rowOffset + x];
		if (v > maxVal) {
			maxVal = v;
			idx = x;
		}
	}
	return idx;
}

int ImageProcessor::findLocalMaxAroundPrev(const std::vector<double>& mat, int y, int prevX, int searchRange, int width)
{
	int rowOffset = y * width;
	double maxVal = -DBL_MAX;
	int chosen = prevX;

	for (int k = -searchRange; k <= searchRange; ++k) {
		int nx = std::clamp(prevX + k, 0, width - 1);
		double v = mat[rowOffset + nx];
		if (v > maxVal) {
			maxVal = v;
			chosen = nx;
		}
	}
	return chosen;
}

std::pair<std::vector<double>, std::vector<double>> ImageProcessor::computeShiftVectors(const uint16_t* nInputImg, int nWidth, int nHeight, int xs, int xe, int nSearchRange, double sigma)
{
	std::vector<double> shift1(nHeight, 0.0);
	std::vector<double> shift2(nHeight, 0.0);
	// --- [0] 입력 검증 ---
	if (!nInputImg  || nWidth <= 1 || nHeight <= 1) {
		std::fprintf(stderr, "Invalid input.\n");
		return std::pair<std::vector<double>, std::vector<double>>{shift1, shift2};
	}

	xs -= 100;
	int widthNew = nWidth - xs;
	cv::Rect roi(xs,0,widthNew,nHeight);
	xs = 0; // ROI를 위해
	xe = widthNew;

	// --- 람다함수 정의 ---
	// y=0 기준점에서 최대값 index를 찾는 람다 함수 정의
	auto findMaxX = [&](int y, int xs, int xe, const std::vector<double>& mat) {
		double maxVal = -DBL_MAX;
		int idx = xs;
		for (int x = xs; x <= xe; ++x) {
			double v = mat[x + y * nWidth];
			if (v > maxVal) { maxVal = v; idx = x; }
		}
		return idx;
	};

	// 기준 패치 생성/정규화하는 람다 함수 정의
	auto makeNormalizedPatch = [&](const std::vector<double>& src, int start, int len) {
		std::vector<double> patch(src.begin() + start, src.begin() + start + len);
		double minVal = *std::min_element(patch.begin(), patch.end());
		double maxVal = *std::max_element(patch.begin(), patch.end());
		double denom = std::max(maxVal - minVal, 1e-12);
		for (auto& v : patch) v = (v - minVal) / denom;
		return patch;
	};

	// 현재 y좌표 -1 결과를 이용하여 최대값을 찾는 람다 함수 정의
	auto findLocalMaxAroundPrev = [&](const std::vector<double>& mat,
		int y, int prevX,
		int searchRange, int width) -> int {
		double maxVal = -DBL_MAX;
		int chosen = prevX;
		for (int k = -searchRange; k <= searchRange; ++k) {
			int nx = std::clamp(prevX + k, 0, width - 1);
			double v = mat[nx + y * width];
			if (v > maxVal) { maxVal = v; chosen = nx; }
		}
		return chosen;
	};

	const int radius = static_cast<int>(std::ceil(3 * sigma));
	const int ksize = makeOdd(2 * radius + 1);
	int refW = 5;
	int searchRangeX = nSearchRange;

	// --- [2] 영상 전처리 ---
	cv::Mat imgIn(cv::Size(nWidth, nHeight), CV_16U, const_cast<uint16_t*>(nInputImg));
	cv::Mat imgInROI = imgIn(roi);
	cv::Mat imgBlur;
	cv::Mat kx = cv::getGaussianKernel(ksize, sigma);
	cv::Mat ky = cv::Mat::ones(1, 1, CV_64F);
	cv::sepFilter2D(imgInROI, imgBlur, -1, kx, ky); // 2D Gaussian Blur -> 1D(x축) Gaussian Blur로 변경

	// --- [3] 헤시안 고유영상 생성 ---
	cv::Mat eigenMat = computeHessianEigenImage(imgBlur, sigma);
	std::vector<double> eigen((double*)eigenMat.ptr(),
		(double*)eigenMat.ptr() + widthNew * nHeight);

	// --- [4] 1차 정합 ---
	std::vector<int> maxLoc(nHeight, 0);
	std::vector<double> eigenWarp(widthNew * nHeight, 0.0);

	maxLoc[0] = findMaxX(0, xs, xe, eigen);
	auto reference = makeNormalizedPatch(eigen, maxLoc[0] - refW / 2, refW);
	double refBase = getBestMatchPositionNormalizedFit(eigen.data(), widthNew, nHeight,
		reference.data(), refW, 0, maxLoc[0], refW);

	for (int y = 0; y < nHeight; ++y) {
		if (y > 0)
			maxLoc[y] = findLocalMaxAroundPrev(eigen, y, maxLoc[y - 1], searchRangeX, widthNew);

		double bestLoc = getBestMatchPositionNormalizedFit(
			eigen.data(), widthNew, nHeight,
			reference.data(), refW,
			y, maxLoc[y], refW);

		shift1[y] = bestLoc - refBase;
		resampleRowShifted(&eigen[y * widthNew], &eigenWarp[y * widthNew], widthNew, shift1[y]);
	}
	auto[meanShift, fstd] = mean_std(shift1);
	UNREFERENCED_PARAMETER(fstd);
	// --- [5] 2차 정합 ---
	refW = 3;
	maxLoc[0] = findMaxX(0, xs, xe, eigenWarp);
	int expected0 = static_cast<int>(std::round(maxLoc[0] + meanShift));
	reference = makeNormalizedPatch(eigenWarp, expected0 - refW, refW);

	double bestLoc2_0 = getBestMatchPositionNormalizedFit(
		eigenWarp.data(), widthNew, nHeight,
		reference.data(), refW,
		0, expected0, refW);

	double refBase2 = bestLoc2_0 - meanShift;

	for (int y = 0; y < nHeight; ++y) {
		if (y > 0)
			maxLoc[y] = findLocalMaxAroundPrev(eigenWarp, y, maxLoc[y - 1], searchRangeX, widthNew);

		double center = maxLoc[y] + meanShift;
		double bestLoc2 = getBestMatchPositionNormalizedFit(
			eigenWarp.data(), widthNew, nHeight,
			reference.data(), refW,
			y, center, refW);

		shift2[y] = bestLoc2 - refBase2;
	}
	return std::pair<std::vector<double>, std::vector<double>>{shift1, shift2};
}

std::vector<std::vector<double>> ImageProcessor::computeShiftVectors(const uint16_t* nInputImg, int nWidth, int nHeight, int xs, int xe, int ys, int ye, int nSearchRange, double sigma)
{
	UNREFERENCED_PARAMETER(xe);
	std::vector<double> shift1(nHeight, 0.0);
	std::vector<double> shift2(nHeight, 0.0);
	std::vector<double> shift3(nHeight, 0.0);
	std::vector<std::vector<double>> ret;

	// --- [0] 입력 검증 ---
	if (!nInputImg || nWidth <= 1 || nHeight <= 1) {
		std::fprintf(stderr, "Invalid input.\n");
		ret.push_back(shift1);
		ret.push_back(shift2);
		ret.push_back(shift3);
		return ret;
	}

	// --- [1] ROI 설정 ---
	// 기존 코드: xs -= 100; widthNew = nWidth - xs;
	// 안전하게 클램프해서 ROI가 영상 안에만 있도록 수정
	int roiX = xs - 100;
	roiX = std::clamp(roiX, 0, nWidth - 1);
	int widthNew = nWidth - roiX;
	if (widthNew <= 1) {
		std::fprintf(stderr, "Invalid ROI width.\n");
		ret.push_back(shift1);
		ret.push_back(shift2);
		ret.push_back(shift3);
		return ret;
	}

	cv::Rect roi(roiX, 0, widthNew, nHeight);

	// 이후 연산에서는 ROI 좌표계를 사용 (0 ~ widthNew-1)
	int xsRoi = 0;
	int xeRoi = widthNew - 1;

	// 검색 범위도 widthNew 안에서만 의미 있게 제한
	int searchRangeX = std::max(1, std::min(nSearchRange, widthNew - 1));

	const int radius = static_cast<int>(std::ceil(3 * sigma));
	const int ksize = makeOdd(2 * radius + 1);
	int refW1 = 5; // 1차 정합 패치 폭
	int refW2 = 3; // 2,3차 정합 패치 폭

	// --- [2] 영상 전처리 ---
	cv::Mat imgIn(cv::Size(nWidth, nHeight), CV_16U, const_cast<uint16_t*>(nInputImg));
	cv::Mat imgInROI = imgIn(roi);

	cv::Mat imgBlur;
	// X 방향 Gaussian blur (ksizeX, sigmaX)
	cv::Mat kx = cv::getGaussianKernel(ksize, sigma, CV_64F);
	cv::Mat ky_identity = cv::Mat::ones(1, 1, CV_64F);   // y 방향은 그대로
	cv::Mat imgBlurX;

	cv::sepFilter2D(imgInROI, imgBlurX, -1, kx, ky_identity);

	// Y 방향 Gaussian blur (ksizeY, sigmaY)
	double sigmaY = 3;
	const int radiusY = static_cast<int>(std::ceil(3 * sigmaY));
	const int ksizeY = makeOdd(2 * radiusY + 1);
	cv::Mat ky = cv::getGaussianKernel(ksizeY, sigmaY, CV_64F);
	cv::Mat kx_identity = cv::Mat::ones(1, 1, CV_64F);   // x 방향은 그대로

	cv::sepFilter2D(imgBlurX, imgBlur, -1, kx_identity, ky);

	// --- [3] 헤시안 고유영상 생성 ---
	cv::Mat eigenMat = computeHessianEigenImage(imgBlur, sigma);
	std::vector<double> eigen(
		(double*)eigenMat.ptr<double>(),
		(double*)eigenMat.ptr<double>() + widthNew * nHeight);

	cv::Mat eigenMat32F;
	eigenMat.convertTo(eigenMat32F, CV_32F);
	/*cv::imwrite("Pre_Eigen.tif",eigenMat32F);

	cv::imwrite("Pre_Blur.tif", imgBlur);*/

	// --- [4] 1차 정합 ---
	std::vector<int> maxLoc(nHeight, 0);
	std::vector<double> eigenWarp(widthNew * nHeight, 0.0);

	// y=0에서 기준점 찾기
	maxLoc[0] = findMaxX(0, xsRoi, xeRoi, widthNew, eigen);

	// refW1는 홀수로 가정 (makeOdd 사용하므로)
	int halfRef1 = refW1 / 2;
	int refStart1 = maxLoc[0] - halfRef1;
	auto reference1 = makeNormalizedPatch(eigen, 0 * widthNew + refStart1, refW1);

	// 기준 y=0에서 base 위치
	double refBase1 = getBestMatchPositionNormalizedFit(
		eigen.data(), widthNew, nHeight,
		reference1.data(), refW1,
		0, maxLoc[0], refW1);

	for (int y = 0; y < nHeight; ++y) {
		if (y > 0) {
			maxLoc[y] = findLocalMaxAroundPrev(eigen, y, maxLoc[y - 1],
				searchRangeX, widthNew);
		}

		double center = static_cast<double>(maxLoc[y]);
		center = std::clamp(center, 0.0, static_cast<double>(widthNew - 1));

		double bestLoc = getBestMatchPositionNormalizedFit(
			eigen.data(), widthNew, nHeight,
			reference1.data(), refW1,
			y, center, refW1);

		shift1[y] = bestLoc - refBase1;

		// 1차 워핑 결과
		resampleRowShifted(&eigen[y * widthNew],
			&eigenWarp[y * widthNew],
			widthNew,
			shift1[y]);
	}

	auto[meanShift1, fstd1] = mean_std(shift1);
	UNREFERENCED_PARAMETER(fstd1);

	// --- [5] 2차 정합 ---
	std::vector<double> eigenWarp2(widthNew * nHeight, 0.0);

	// y=0에서 다시 최대값 (1차 warp 영상 기준)
	maxLoc[0] = findMaxX(0, xsRoi, xeRoi, widthNew, eigenWarp);

	int expected0_2 = static_cast<int>(std::round(maxLoc[0] + meanShift1));
	expected0_2 = std::clamp(expected0_2, 0, widthNew - 1);

	int refStart2_0 = expected0_2 - refW2 / 2;
	auto reference2 = makeNormalizedPatch(eigenWarp, 0 * widthNew + refStart2_0, refW2);

	double bestLoc2_0 = getBestMatchPositionNormalizedFit(
		eigenWarp.data(), widthNew, nHeight,
		reference2.data(), refW2,
		0, expected0_2, refW2);

	double refBase2 = bestLoc2_0 - meanShift1;

	for (int y = 0; y < nHeight; ++y) {
		if (y > 0) {
			maxLoc[y] = findLocalMaxAroundPrev(eigenWarp, y, maxLoc[y - 1],
				searchRangeX, widthNew);
		}

		double center2 = maxLoc[y] + meanShift1;
		center2 = std::clamp(center2, 0.0, static_cast<double>(widthNew - 1));

		double bestLoc2 = getBestMatchPositionNormalizedFit(
			eigenWarp.data(), widthNew, nHeight,
			reference2.data(), refW2,
			y, center2, refW2);

		shift2[y] = bestLoc2 - refBase2;

		// 1차 + 2차 워핑 결과
		resampleRowShifted(&eigen[y * widthNew],
			&eigenWarp2[y * widthNew],
			widthNew,
			shift1[y] + shift2[y]);
	}

	// --- [5.5] 1+2차 결과로 mean 다시 계산 ---
	std::vector<double> temp(nHeight, 0.0);
	for (int i = 0; i < nHeight; ++i) {
		temp[i] = shift1[i] + shift2[i];
	}

	auto[meanShift2, fstd2] = mean_std(temp);
	UNREFERENCED_PARAMETER(fstd2);

	// --- [6] 3차 정합 ---
	// 3차는 eigenWarp2(1+2차 누적 워프)에 대해, meanShift2를 기준으로 수행
	refW2 = 3; // 유지, 필요하면 따로 refW3 써도 됨

	maxLoc[0] = findMaxX(0, xsRoi, xeRoi, widthNew, eigenWarp2);

	int expected0_3 = static_cast<int>(std::round(maxLoc[0] + meanShift2));
	expected0_3 = std::clamp(expected0_3, 0, widthNew - 1);

	int refStart3_0 = expected0_3 - refW2 / 2;
	auto reference3 = makeNormalizedPatch(eigenWarp2, 0 * widthNew + refStart3_0, refW2);

	double bestLoc3_0 = getBestMatchPositionNormalizedFit(
		eigenWarp2.data(), widthNew, nHeight,
		reference3.data(), refW2,
		0, expected0_3, refW2);

	double refBase3 = bestLoc3_0 - meanShift2;

	for (int y = 0; y < nHeight; ++y) {
		if (y > 0) {
			maxLoc[y] = findLocalMaxAroundPrev(eigenWarp2, y, maxLoc[y - 1],
				searchRangeX, widthNew);
		}

		double center3 = maxLoc[y] + meanShift2;   // ⭐ 3차에서는 meanShift2 사용
		center3 = std::clamp(center3, 0.0, static_cast<double>(widthNew - 1));

		double bestLoc3 = getBestMatchPositionNormalizedFit(
			eigenWarp2.data(), widthNew, nHeight,
			reference3.data(), refW2,
			y, center3, refW2);

		shift3[y] = bestLoc3 - refBase3;
		// 필요하다면 eigenWarp3 를 만들어 (shift1+shift2+shift3) 누적 워핑도 가능
	}

	// --- [7] 결과 반환 ---
	std::vector<double> new_sub = refineShiftVector(shift3, ys, ye);
	for (int r = ys; r < ye; r++)
		shift3[r] = new_sub[r];

	ret.clear();
	ret.push_back(shift1);
	ret.push_back(shift2);
	ret.push_back(shift3);
	return ret;
}



// 벡터의 mean, std 계산
void ImageProcessor::calcMeanStd(const std::vector<double>& v, double& mean, double& stddev)
{
	if (v.empty()) {
		mean = 0.0;
		stddev = 0.0;
		return;
	}

	double sum = std::accumulate(v.begin(), v.end(), 0.0);
	mean = sum / static_cast<double>(v.size());

	double sq_sum = 0.0;
	for (double x : v) {
		double diff = x - mean;
		sq_sum += diff * diff;
	}
	stddev = std::sqrt(sq_sum / static_cast<double>(v.size()));
}

// mean ± k*std 기준으로 그룹화하되,
// std가 너무 작으면 전체 std의 일정 비율(min_std_scale)을 최소 std로 사용
std::vector<std::vector<double>> ImageProcessor::separateGroupsMeanStd(
	const std::vector<double>& data,
	double k,
	double min_std_scale,   // 예: 0.3 ~ 0.5 정도
	bool use_fixed_center = false,
	double fixed_mean = 0.0,
	double fixed_std = 0.0
)
{
	std::vector<std::vector<double>> groups;

	if (data.empty())
		return groups;

	// 전체 데이터 기준 global mean, std 계산
	double global_mean = 0.0, global_std = 0.0;
	calcMeanStd(data, global_mean, global_std);

	// 첫 값으로 첫 그룹 시작
	std::vector<double> current_group;
	current_group.push_back(data[0]);

	for (std::size_t i = 1; i < data.size(); ++i) {
		double value = data[i];

		// 현재 그룹의 mean, std
		double mean = 0.0, stddev = 0.0;

		if (use_fixed_center) {
			// 고정된 중심/표준편차 사용 (max group 통계 등)
			mean = fixed_mean;
			stddev = fixed_std;
		}
		else {
			// 기존 방식: 현재 그룹 통계 사용
			calcMeanStd(current_group, mean, stddev);
		}

		// std가 너무 작으면 global_std의 일부를 최소 폭으로 사용
		double effective_std = std::max(stddev, min_std_scale * global_std);

		double lower = mean - k * effective_std;
		double upper = mean + k * effective_std;

		if (value >= lower && value <= upper) {
			// 같은 그룹에 포함
			current_group.push_back(value);
		}
		else {
			// 새 그룹 시작
			groups.push_back(current_group);
			current_group.clear();
			current_group.push_back(value);
		}
	}

	// 마지막 그룹 추가
	if (!current_group.empty())
		groups.push_back(current_group);

	return groups;
}


std::vector<double> ImageProcessor::refineShiftVector(std::vector<double> shifts, int ys, int ye)
{
	double k = 5.0;              // mean ± k * std
	double min_std_scale = 0.4;  // 전체 std의 40%를 최소 std로 사용

	// 1) ys~ye 구간에서 1차 그룹핑
	std::vector<double> sub(shifts.begin() + ys, shifts.begin() + ye);
	auto groups1 = separateGroupsMeanStd(sub, k, min_std_scale);

	// 2) 가장 큰 그룹 찾기
	int maxIndex = -1;
	int maxSize = 0;

	for (std::size_t i = 0; i < groups1.size(); ++i) {
		const auto& g = groups1[i];
		if ((int)g.size() > maxSize) {
			maxSize = (int)g.size();
			maxIndex = (int)i;
		}
	}

	// 3) max group의 mean, std 구하기
	const auto& maxGroup = groups1[maxIndex];
	auto stats = mean_std_min_max(maxGroup); // [mean, std, min, max] 라고 가정
	double mean_max = stats[0];

	// 2) groups1 내 각 그룹의 평균을 mean_max 로 맞추기
	for (auto& g : groups1)
	{
		if (g.empty()) continue;

		// 현재 그룹의 평균
		auto stats_g = mean_std_min_max(g);
		double mean_g = stats_g[0];

		// 필요한 shift
		double shift_amount = mean_max - mean_g;

		// 그룹 전체를 shift
		for (double& v : g)
			v += shift_amount;
	}

	std::vector<double> new_sub;
	new_sub.reserve(sub.size());

	for (auto& g : groups1)
	{
		for (double v : g)
			new_sub.push_back(v);
	}

	// ★ new_sub 크기가 sub 크기보다 크거나 작지 않은지 확인
	if (new_sub.size() == sub.size()) {
		sub = new_sub;
	}
	else {
		// 이론적으로 크기가 반드시 같아야 할 때
		// 방어 코드
		size_t mn = std::min(sub.size(), new_sub.size());
		for (size_t i = 0; i < mn; ++i)
			sub[i] = new_sub[i];
	}

	std::vector<double> ret = shifts;
	for (int y = ys; y < ye; y++)
		ret[y] = sub[y];

	return ret;
}

cv::Mat ImageProcessor::CorrectWobble(cv::Mat InputImg, int xs, int xe, int ys, int ye, bool bLowResolution, int nSearchRange)
{
	// --- [0] 입력 검증 ---
	if (InputImg.cols <= 1 || InputImg.rows <= 1) {
		std::fprintf(stderr, "Invalid input.\n");
		return cv::Mat::zeros(InputImg.size(), InputImg.type());
	}
	int nWidth = InputImg.cols;
	int nHeight = bLowResolution ? InputImg.rows / 2 : InputImg.rows;

	// --- [1] 기본 설정 ---
	double sigma = 1.5;
	std::vector<double> shift1(nHeight, 0.0);
	std::vector<double> shift2(nHeight, 0.0);
	std::vector<double> shift3(nHeight, 0.0);
	cv::Mat OutputImg(InputImg.size(), InputImg.type());
	auto* nInputImg = InputImg.ptr<uint16_t>();
	auto* nOutputImg = OutputImg.ptr<uint16_t>();
	double product = 1;
	while (product > 0.01 && sigma <= 6)
	{
		std::fill(shift1.begin(), shift1.end(), 0.0);
		std::fill(shift2.begin(), shift2.end(), 0.0);
		std::fill(shift3.begin(), shift3.end(), 0.0);

		std::vector<std::vector<double>> ret = computeShiftVectors(nInputImg, nWidth, nHeight, xs, xe, ys, ye, nSearchRange, sigma);
		shift1 = ret[0];
		shift2 = ret[1];
		shift3 = ret[2];

		auto stats0 = mean_std_min_max(shift3);
		double stdv = stats0[1], minv = stats0[2], maxv = stats0[3];
		product = stdv * (maxv - minv);
		if (m_bSaveCorrectionStep)
		{
			char buf[256];
			sprintf_s(buf, "Used sigma = %.2f. Product = %.3f\n", sigma, product);
			writelog(buf, LOG_FILE_NAME);
			printf("%s", buf);
			printShiftStats(shift1, "Before");
			printShiftStats(shift3, "After");
		}
		sigma += 0.5;
	}

	if (product <= 0.01) // 보정이 잘되는 경우임.
	{
		// 원본 영상 워핑
		for (int y = 0; y < nHeight; ++y)
			resampleRowShifted(nInputImg + y * nWidth, nOutputImg + y * nWidth, nWidth, shift1[y] + shift2[y] + shift3[y]);
		g_nCorrectWobbleResult = 1;// 2026-01-12. jg kim. Wobble 보정 결과를 받기 위해
	}
	else // 보정이 안되는 경우임. 그럴 경우 원본을 반환
	{
		OutputImg = InputImg.clone();
		g_nCorrectWobbleResult = 0;
	}

	return OutputImg;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::CorrectMirrorUniformity(cv::Mat& input_, cv::Mat& bin, int nWidthKernel)
{
	cv::Mat inTemp; input_.convertTo(inTemp, CV_16U);
	auto* in_ptr = input_.ptr<float>(0);
	auto* in_ptrU = inTemp.ptr<ushort>(0);

	const ushort* mask = bin.ptr<ushort>(0);

	// mirror uniformity
	int width = input_.cols;
	int height = input_.rows;

	// calculate background(엑스레이가 조사된 영역) intensity
	double background = 0;
	int count = 0;
	for (int n = 0; n < input_.cols * input_.rows; n++) {
		if (mask[n] != 0) {// (엑스레이가 조사된 영역)
			background += in_ptr[n];
			count++;
		}
	}
	background /= (double)count; // 평균임

	cv::Mat output;
	input_.copyTo(output);
	auto* o_ptr = output.ptr<float>(0);

	Rect rt = getBoundingRect(bin);
	int l = rt.x;
	int t = rt.y;
	int r = rt.x + rt.width;// 2024-10-30. jg kim. debug
	int b = rt.y + rt.height;

	int widthKernel = nWidthKernel;
	const int stride = 8; // 기존 8. stride 값을 감소시킬 수록 mirror에서 기인한 가로줄 제거 효과가 커짐
	// 2024-07-10. jg kim. mirror correction이 제대로 되지 않는것을 발견하여 테스트 중
	for (int x = l; x < r; x += widthKernel) {
		// 1. 커널64(x) 기준으로 y축 8line간 평균 구하기(실데이터)
		std::vector<double> avg(stride, 0.0);
		for (int i = 0; i < stride; i++)
			avg[i] += AverageLine8(in_ptrU, width, height, mask, x, t + i, b, stride, widthKernel);

		// 2. 구한 평균값을 평활화 해주기 위한 계산(평균구하기)
		double avr_cal_avr_lin8 = 0;
		for (int i = 0; i < stride; ++i)
			avr_cal_avr_lin8 += avg[i];
		avr_cal_avr_lin8 /= stride;

		// 3. 구한 평균값을 평활화 해주기 위한 계산(offset 구하기)
		for (int i = 0; i < stride; ++i)
			avg[i] = avr_cal_avr_lin8 / avg[i];

		// 4. 구한 offset을 바탕으로 RAW에 보정작업
		for (int i = 0; i < stride; ++i)
			kernel_set_avr_line8(in_ptrU, width, height, mask, o_ptr, x, t + i, b, avg[i], stride, background, widthKernel);
	}

	return output;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Rect ImageProcessor::getBoundingRect(cv::Mat bin, int nMargin)
{
	/*std::vector<cv::Point> pts;
	auto* pt = bin.ptr();
	int C = bin.cols;
	int R = bin.rows;
	for(int r=0; r<R; r++)
		if(r > nMargin && r < R - nMargin)
		for (int c = 0; c < C; c++)
		{
			if (c > nMargin && c<C - nMargin )
			{
				int idx = r * C + c;
				if(pt[idx] > 0)
					pts.push_back(cv::Point(c,r));
			}
		}
	printf("pts = %d\n",pts.size());
	return cv::boundingRect(pts);*/

	cv::Mat image;
	if (bin.type() == CV_16U) // 2024-07-18. jg kim. data형 체크 추가
		image = bin.clone();
	else // 2024-08-05. jg kim. data type 체크 개선
		bin.convertTo(image, CV_16U);

	const ushort* mask = image.ptr<ushort>(0);

	// mirror uniformity
	int width = bin.cols;
	int height = bin.rows;

	int l = -1;// left
	for (int x = nMargin; x < width; x++) {
		for (int y = nMargin; y < height; y++) {
			int n = y * width + x;
			if (mask[n] != 0) {
				l = x;	break;
			}
		}
		if (l > 0)			break;
	}
	int r = -1;// right
	for (int x = width - 1- nMargin; x > nMargin; x--) {
		for (int y = nMargin; y < height; y++) {
			int n = y * width + x;
			if (mask[n] != 0) {
				r = x + 1;	break;
			}
		}
		if (r > 0)			break;
	}
	int t = -1;// top
	for (int y = nMargin; y < height; y++) {
		for (int x = nMargin; x < width; x++) {
			int n = y * width + x;
			if (mask[n] != 0) {
				t = y;	break;
			}
		}
		if (t > 0)	break;
	}
	int b = -1;// bottom
	for (int y = height - 1- nMargin; y > nMargin; y--) {
		for (int x = nMargin; x < width; x++) {
			int n = y * width + x;
			if (mask[n] != 0) {
				b = y + 1;	break;
			}
		}
		if (b > 0)	break;
	}
	if (b >= height)
		b = height - 1;

	return cv::Rect(l, t, r - l + 1, b - t + 1);
	// 2024-02-23. jg kim. 기존의 코드가 width, height 값을 return하지 않아 수정.
}

cv::Rect ImageProcessor::getBoundingRect2(cv::Mat bin)
{
	std::vector<cv::Point> points;
	cv::findNonZero(bin, points);
	return cv::boundingRect(points);
}

cv::Mat ImageProcessor::getCompactRegion(cv::Mat bin, float fThresRatio) 
// 2024-07-18. jg kim. NED image의 두꺼운 부분때문에 morphological operation이 잘 안되는 것에 대응하기 위함.
{
	int width = bin.cols;
	int height = bin.rows;
	int count = 0;
	int maxCount = 0;
	// 각 행별로 최대 픽셀 개수 찾기
	for (int r = 0; r < height; r++)
	{
		count = 0;
		for (int c = 0; c < width; c++)
		{
			if (bin.at<unsigned short>(r, c) > 0)
				count++;
		}
		if (count > maxCount)
			maxCount = count;
	}
	// 두꺼운 행 제거
	for (int r = 0; r < height; r++)
	{
		count = 0;
		for (int c = 0; c < width; c++)
		{
			if (bin.at<unsigned short>(r, c) > 0)
				count++;
		}

		if (count > maxCount * fThresRatio)//두꺼운 행 제거
			for (int c = 0; c < width; c++)
				bin.at<unsigned short>(r, c) = 0;
	}

	maxCount = 0;
	// 각 열별로 최대 픽셀 개수 찾기
	for (int c = 0; c < width; c++) 
	{
		count = 0;
		for (int r = 0; r < height; r++)
		{
			if (bin.at<unsigned short>(r, c) > 0)
				count++;
		}
		if (count > maxCount)
			maxCount = count;
	}
	// 두꺼운 열 제거
	for (int c = 0; c < width; c++)
	{
		count = 0;
		for (int r = 0; r < height; r++)
		{
			if (bin.at<unsigned short>(r, c) > 0)
				count++;
		}
		if (count > maxCount * fThresRatio)
			for (int r = 0; r < height; r++)
				bin.at<unsigned short>(r, c) = 0; // 두꺼운 열 제거
	}

	return bin.clone();
}

void circleFit(std::vector<cv::Point>& data, double & centerx, double & centery, double & radius)
{
	const int maxIter = 99;
	double mx = 0, my = 0;
	for (int i = (int)data.size(); i-- > 0;)
	{
		mx += data[i].x;
		my += data[i].y;
	}
	// center of mass;
	mx /= data.size();
	my /= data.size();
	// moment calculation;
	double Mxy, Mxx, Myy, Mzx, Mzy, Mzz; 
	Mxx = Myy = Mxy = Mzx = Mzy = Mzz = 0.;
	for (int i = (int)data.size(); i-- > 0;)
	{
		double xi = data[i].x - mx; // center of mass coordinate
		double yi = data[i].y - my; // center of mass coordinate
		double zi = xi * xi + yi * yi;
		Mxy += xi * yi;
		Mxx += xi * xi;
		Myy += yi * yi;
		Mzx += xi * zi;
		Mzy += yi * zi;
		Mzz += zi * zi;
	}
	Mxx /= data.size();
	Myy /= data.size();
	Mxy /= data.size();
	Mzx /= data.size();
	Mzy /= data.size();
	Mzz /= data.size();
	double Mz = Mxx + Myy;
	double Cxy = Mxx * Myy - Mxy * Mxy;
	double Vz = Mzz - Mz * Mz;
	// coefficients of characteristic polynomial;
	double C2 = 4 * Cxy - 3 * Mz * Mz - Mzz;
	double C1 = Vz * Mz + 4 * Cxy * Mz - Mzx * Mzx - Mzy * Mzy;
	double C0 = Mzx * (Mzx * Myy - Mzy * Mxy) + Mzy * (Mzy * Mxx - Mzx * Mxy) - Vz * Cxy;
	// Newton's method starting at lambda = 0
	double lambda = 0;
	double y = C0;
	// det(lambda = 0)
	for (int iter = 0; iter < maxIter; iter++) {
		double Dy = C1 + lambda * (2.* C2 + 16.*lambda * lambda);
		double lambdaNew = lambda - y / Dy;
		if ((lambdaNew == lambda))
			break;
		double ynew = C0 + lambdaNew * (C1 + lambdaNew * (C2 + 4 * lambdaNew * lambdaNew));
		if (fabs(ynew) >= fabs(y))
			break;
		lambda = lambdaNew; y = ynew;
	}
	double DEL = lambda * lambda - lambda * Mz + Cxy;
	double cx = (Mzx * (Myy - lambda) - Mzy * Mxy) / DEL / 2;
	double cy = (Mzy * (Mxx - lambda) - Mzx * Mxy) / DEL / 2;
	centerx = cx + mx;
	// recover origianl coordinate;
	centery = cy + my;
	radius = sqrt(cx * cx + cy * cy + Mz + 2 * lambda);
	//return fitError(data, centerx, centery, radius);
}

#pragma endregion Mirror Uniformity Correction

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

cv::Mat ImageProcessor::CorrectPixelGains(Mat img, std::vector<Mat> coeffs, cv::Mat &gainMap, cv::Mat &NedMap)
{
	Mat res = CorrectPixelGains(img, coeffs);	// horizontal shading 보정
	GetNedMap(img, gainMap, NedMap);			// NED map 생성

	return ProcessNedArea(img, res.clone(), NedMap);
}

/*
	ProcessNedArea
	NedMap을 img와 res을 섞는 weight으로 하여 영상을 섞음.
	img : gain correction (horizontal shading)하기 전 영상
	res : gain correction 한 영상
	NedMap : img, res를 섞는 weight. 값이 높으면 res에 가중치를 더 주고, 낮으면 img에 가중치를 더 줌
*/

cv::Mat ImageProcessor::ProcessNedArea(cv::Mat img, cv::Mat res, cv::Mat NedMap)
{
#pragma omp parallel for
	for (int x = 0; x < img.cols; x++)
		for (int y = 0; y < img.rows; y++)
		{
			float w = NedMap.at<float>(y, x);
			res.at<float>(y, x) = res.at<float>(y, x)*w + img.at<float>(y, x)*(1 - w); // gain correction 된 영상과, NED 영역을 부드럽게 섞음
		}
	return res;
}

cv::Mat ImageProcessor::CorrectPixelGains(cv::Mat img, std::vector<cv::Mat> coeffs)
{
// Debug output: coeffs 내용을 means와 coefs로 나누어 출력
/*
char buf[1024];
for (int i = 0; i < coeffs[0].rows; i++) {
	sprintf_s(buf, "means[%d] = %f\n", i, coeffs[0].at<float>(i));
	writelog(buf, LOG_FILE_NAME);
}

for (int r = 2; r < coeffs.size(); r++) {
	sprintf_s(buf, "r = %d\n", r);
	writelog(buf, LOG_FILE_NAME);
	for (int d = 0; d < coeffs[r].rows; d++) {
		sprintf_s(buf, "  coef[%d] = %e\n", d, coeffs[r].at<float>(d));
		writelog(buf, LOG_FILE_NAME);
	}
}
*/
/*
	coeffs[0]: means 데이터 출력
	means[1]~means[5] 까지만 사용
	means[1]~means[5]: 각 조사시간별 영상의 ROI에서의 평균 밝기
	조사시간은 
	2026-01-17이전은 0.05, 0.16, 0.32, 0.63, 1.25s
	2026-01-17부터 0.05, 0.1, 0.2, 0.4, 0.63s로 변경 (MPPC의 gain을 증가시켜 전반적인 밝기가 증가하였기 때문에 calibration 시 조사시간을 줄임)
	
	coeffs[2~11]: coefs 데이터 출력 (coeffs[1]은 placeholder이므로 skip)
	2026-01-17 기준
	각 조사시간 별 획득한 영상의 ROI에서 SubScan 방향(raw image의 y축)으로 평균 밝기를 구함.
	그러면 ROI width 개수(Main scan 방향. raw image의 x축)의 평균 밝기가 구해짐.
	Main scan 방향(x축)의 위치에 대한 polynomial fitting(4차식)을 통해 얻은 계수들.
	
	Main scan 방향 프로파일을 보면 위로 볼록한 포물선 형태이고 이를 4차식으로 fitting 하였음.
	Main scan 방향 프로파일은 조사시간이 증가함에 따라 점점 더 볼록한 형태가 됨. 그래서 조사시간별 fitting 계수를 구함.
	이는 신호를 획득하는 장치인 MPPC의 특성에 기인한 것으로 MPPC에 입사되는 광량이 증가할수록 MPPC의 출력신호가 선형적으로 증가하지 않고 포화되는 경향이 있음.
	
	조사시간 별 영상의 polynomial 계수들(4차식이므로 5개 계수)
	coeffs[2] : 0.05s 
	coeffs[3] : 0.1s
	coeffs[4] : 0.2s
	coeffs[5] : 0.4s
	coeffs[6] : 0.63s
*/
	
	Mat M; coeffs.at(0).convertTo(M, CV_64F);  // 2024-07-22. jg kim. NED영역에서 세로줄 발생 디버깅
	// M : 각 조사시간별 ROI 평균 밝기 (x-ray calibration시 계산한 값)
	int count = static_cast<int>(coeffs.size() - 1);

	Mat res = img.clone();
#pragma omp parallel for
	for (int x = 100; x < img.cols - 100; x++)
	{
		Mat Y(Size(1, count), CV_64F);  // 2024-07-22. jg kim. NED영역에서 세로줄 발생 디버깅
		for (int c = 0; c < count; c++)
		{
			cv::Mat temp; coeffs.at(c + 1).convertTo(temp, CV_64F);
			Y.at<double>(c) = get_fitted_value(temp, double(x)); 
			// Y : 주어진 x 좌표에서 polynomial fitting으로 구한 조사시간 별 밝기
		}

		Mat p1 = polyfit(M, Y, DEGREE_COUNT); // p1: 조사시간별 ROI 평균밝기 → x좌표에서의 조사시간 별 밝기
		Mat p2 = polyfit(Y, M, DEGREE_COUNT); // p2: x좌표에서의 조사시간 별 밝기 → 조사시간별 ROI 평균밝기
		// p1, p2 : Main scan 방향(x축) 각 픽셀 위치에서 감도 차이를 보정하기 위한 양방향 mapping 관계 구축
		for (int y = 0; y < img.rows; y++)
		{
			double value = img.at<float>(y, x);  // 2024-07-22. jg kim. NED영역에서 세로줄 발생 디버깅
			double g = getGain(p1, p2, value); // 주어진 x, y 좌표의 픽셀 밝기의 horizontal shading 보정 계수 g를 계산
			res.at<float>(y, x) = value * g; // 보정된 밝기 = 원래 밝기 * 보정 계수
		}
	}
	for (int x = 100; x < img.cols - 100; x++)
	{
		Mat Y(Size(1, count), CV_64F);
		for (int c = 0; c < count; c++)
		{
			cv::Mat temp; coeffs.at(c + 1).convertTo(temp, CV_64F);
			Y.at<double>(c) = get_fitted_value(temp, double(x)); 
			// Y : 주어진 x 좌표에서 polynomial fitting으로 구한 조사시간 별 밝기
		}

		Mat p1 = polyfit(M, Y, DEGREE_COUNT);
		// p1: 조사시간별 ROI 평균밝기 → x좌표에서의 조사시간 별 밝기
		Mat p2 = polyfit(Y, M, DEGREE_COUNT);
		// p2: x좌표에서의 조사시간 별 밝기 → 조사시간별 ROI 평균밝기
		// p1, p2 : Main scan 방향(x축) 각 픽셀 위치에서 감도 차이를 보정하기 위한 양방향 mapping 관계 구축
		for (int y = 0; y < img.rows; y++)
		{
			double value = img.at<float>(y, x);
			double g = getGain(p1, p2, value);
			// 주어진 x, y 좌표의 픽셀 밝기의 horizontal shading 보정 계수 g를 계산
			res.at<float>(y, x) = value * g;
			// 보정된 밝기 = 원래 밝기 * 보정 계수
		}
	}
	/*
	이 알고리즘은 "Main scan 방향의 위로 볼록한 프로파일"과 "MPPC 포화 특성"을 고려하여, 
	Main scan 방향(x축) 각 픽셀 위치에서 조사시간-밝기 관계의 비선형성을 polynomial fitting으로 modeling하고,
	이를 통해 horizontal shading을 보정함.
	*/
	return res;
}

void ImageProcessor::GetNedMap(cv::Mat img, cv::Mat & gainMap, cv::Mat & NedMap)
{
	//Rect roi(TAG_POSITION + 1, 0, 100, img.rows / 2);
	// 2024-04-09. jg kim. scan 후반부에 ambient light 영상을 받는 장비가 있어 background의 min, max를 구하는 영역을 축소시킴.
	
	// background의 통계값(min, max, mean, stddev) 계산
	float mean, stddev;
	getMeanStddev(img(m_roiBackground), mean, stddev); // m_roiBackground : (10, 150, 90, height/2)
	double min, max;
	minMaxLoc(img(m_roiBackground), &min, &max);

	// 2025-11-11. jg kim 반사판이 추가되는 경우 background의 통계값 계산에 영향을 받아 비활성화 시킴. Door open과 마찬가지
	/*
	roi = Rect(img.cols - 101, 0, 100, img.rows / 2);
	// 2024-04-09. jg kim. scan 후반부에 ambient light 영상을 받는 장비가 있어 background의 min, max를 구하는 영역을 축소시킴.
	float rmean, rstddev;
	getMeanStddev(img(roi), rmean, rstddev);
	double rmin, rmax;
	minMaxLoc(img(roi), &rmin, &rmax);

	min = min < rmin ? min : rmin;
	max = max > rmax ? max : rmax;
	stddev = stddev > rstddev ? stddev : rstddev;
	*/

	cv::Mat NED_Negative = cv::Mat::zeros(img.size(), CV_16U);
	cv::Mat NED_Positive = cv::Mat::zeros(img.size(), CV_16U);
	// 2024-09-24. jg kim. NED map을 위해 rtBounding을 사용하지 않음
	int L = 0;
	int T = 0;
	int R = img.cols;
	int B = img.rows;
#pragma omp parallel for
	for (int x = L; x < R; x++)
	{
		for (int y = T; y < B; y++)
		{
			float value = img.at<float>(y, x);
			if (value < max *1.01f) 
			// pixel value가 max(background의) * 1.01 값보다 작은 경우 NED 영역으로 판단하여 NED_Positive는 255, NED_Negative는 0으로 설정. 그렇지 않은 경우 반대로 설정.
			{
				NED_Negative.at<unsigned short>(y, x) = 0; // NED 영역이 아닌 부분
				NED_Positive.at<unsigned short>(y, x) = 255; // NED 영역
			}
			else
			{
				NED_Negative.at<unsigned short>(y, x) = 255; // NED 영역이 아닌 부분
				NED_Positive.at<unsigned short>(y, x) = 0; // NED 영역
			}
		}
	}

	NED_Positive = getCompactRegion(NED_Positive, 0.8f); // 2024-07-18. jg kim. NED image의 두꺼운 부분때문에 morphological operation이 잘 안되는 것에 대응하기 위함.
	cv::medianBlur(NED_Positive, NED_Positive, 3);
	Rect rect = getBoundingRect(NED_Positive);
	// Ned Positive에서 NED 영역으로 판단된 부분의 bounding rect를 구함
	L = rect.x;
	if (L < 0)
		L = 0;
	T = rect.y;
	if (T < 0)
		T = 0;
	R = L + rect.width;
	B = T + rect.height;

	// 2024-02-23. jg kim. getBoundingRect의 return값 중 width, height를 return 하도록 수정하여 original code가 맞도록 수정
	R = R > img.cols ? img.cols : R;
	B = B > img.rows ? img.rows : B;

	int nSE = 25;
	int rSE = int(nSE / 2);

	// Ned Positive에 대해 morphological operation을 수행하여 NED 영역으로 판단된 부분의 외곽부분을 제거
	cv::Mat out8u; NED_Positive.convertTo(out8u, CV_8U);
	auto se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(nSE, nSE));
	cv::erode(out8u, out8u, se);
	cv::dilate(out8u, out8u, se);

	// Seeded Region Growing을 이용하여 NED 영역으로 판단된 부분의 외곽(raw image의 LT, RT, LB, RB임)부분을 제거
	std::queue<cv::Vec2i> seed_LT;	seed_LT.push(cv::Vec2i(T + rSE, L + rSE));// left-top
	std::queue<cv::Vec2i> seed_RT;	seed_RT.push(cv::Vec2i(T + rSE, R - 1 - rSE));// right-top
	std::queue<cv::Vec2i> seed_LB;	seed_LB.push(cv::Vec2i(B - 1 - rSE, L + rSE));// left-bottom
	std::queue<cv::Vec2i> seed_RB;	seed_RB.push(cv::Vec2i(B - 1 - rSE, R - 1 - rSE));// right-bottom

	std::vector< std::queue<cv::Vec2i> > seeds;
	seeds.push_back(seed_LT);
	seeds.push_back(seed_RT);
	seeds.push_back(seed_LB);
	seeds.push_back(seed_RB);

	std::vector< cv::Mat> masks;

	for (int i = 0; i < 4; i++)// out8u(NED_Positive; morphological process 했음) 결과 
	{
		masks.push_back(SeededRegiongrowing(out8u, seeds.at(i))); // LT/RT/LB/RB corner seed를 바탕으로 SeededRegiongrowing한 결과를 masks(vector<Mat>)에 저장
	}

	for (int y = 0; y < out8u.rows; y++) // masks의 각각의 결과를 mask에 병합
		for (int x = 0; x < out8u.cols; x++)
		{
			for (int j = 0; j < 4; j++)
			{
				if (masks.at(j).at<uchar>(y, x) > 0)// masks를 이용하여 output 외각부분 제거
					out8u.at<uchar>(y, x) = 0;
			}
		}

	cv::Mat nedneg8; NED_Negative.convertTo(nedneg8, CV_8U);

	cv::Mat convex_ = CreateConvexHull(nedneg8);
	convex_.convertTo(convex_, CV_32F);
	GaussianBlur(convex_, convex_, Size(19, 19), 3, 3, BORDER_REFLECT);
	// NED negative의 convex hull에 대해 GaussianBlur를 수행하여 NED 영역으로 판단된 부분의 외곽부분을 부드럽게 처리

	float* pt = convex_.ptr<float>(0);
	for (int n = 0; n < convex_.rows*convex_.cols; n++)
		pt[n] /= 255;

	// convex_ :: NED negative (NED 영역은 0, 아닌 영역은 1개념)를 convex hull로 처리. 내부의 빈 자리가 없음.

	NedMap = convex_.clone(); 
	// NED negative. 영상 영역과 NED 영역의 경계를 부드럽게 처리하기 위한 weight map. NED 영역은 0, 아닌 영역은 1개념.
	// convex hull로 처리하여 내부의 빈 자리가 없음.
	gainMap = out8u.clone();
	// NED positive. NED 영역으로 판단된 부분

}

void ImageProcessor::getMeanStddev(cv::Mat img, float &mean, float &stddev)
{
	// 2024-12-17 jg kim. debug. 데이터 형 체크 추가
	cv::Mat data = img.clone();
	if (img.type() != CV_32F)
		img.convertTo(data, CV_32F);

	// 2024-02-25 jg kim. 급해서 일단 구현했음.
	mean = 0;
	for (int r = 0; r < img.rows; r++)
	{
		for (int c = 0; c < img.cols; c++)
		{
			mean += data.at<float>(r, c);
		}
	}
	mean /= float(img.rows * img.cols);

	stddev = 0; // std = sum( (x-m)^2 )
	for (int r = 0; r < img.rows; r++)
	{
		for (int c = 0; c < img.cols; c++)
		{
			float e = mean - data.at<float>(r, c);
			stddev += e * e;
		}
	}
	stddev = sqrt(stddev / float(img.rows * img.cols - 1));
}

void ImageProcessor::getMeanStddev(std::vector<float> vtData, float & mean, float & stddev)
{
	// 2024-07-10 jg kim. 영상이 아닌 data 배열에 대한 평균, 표준편차 구하는 버전 구현.
	mean = 0;
	for(int i=0; i<(int)vtData.size(); i++)
		mean += vtData.at(i);

	mean /= float(vtData.size());

	stddev = 0; // std = sum( (x-m)^2 )
	for (int i = 0; i<(int)vtData.size(); i++)
	{
		float e = mean - vtData.at(i);
		stddev += e * e;
	}
	stddev = sqrt(stddev / float(vtData.size()));
}

std::vector<float> ImageProcessor::getAspectRatios(std::vector<cv::Mat> imgs)
{
	std::vector<float> res;
	for (int i = 0; i < (int)imgs.size(); i++)
	{
		cv::Mat bin = imgs.at(i);
		cv::Rect rt = getBoundingRect(bin);
		float w = float(rt.width);
		float h = float(rt.height);
		printf("getBoundingRect. l/t/w/h = %d, %d, %d, %d,\n",rt.x, rt.y, rt.width, rt.height);
		res.push_back(h/w);
	}
	return res;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::LineCorrect(Mat input_, int preTerm, int postTerm, bool vertical)
{
	int width = input_.cols;
	int height = input_.rows;

	std::vector<float> tempF(width * height);
#pragma omp parallel for
	for (int n = 0; n < width * height; n++)
		tempF[n] = input_.ptr<float>(0)[n];

	// genoray 기법
	cv::Mat output(height, width, CV_32FC1);
	/*CMceIppi*/::ApplyLinecorrect(output.ptr<float>(0), tempF.data(), width, height, preTerm, postTerm, vertical);
	return output;
}


#pragma region Stripe Removal
struct INDEX_VAL
{
	int x;
	float val;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//using20211113
template<int DEGREE>
auto Regression(INDEX_VAL* data, int dataSize)
{
	constexpr size_t d = DEGREE;
	constexpr size_t d1 = d + 1;
	constexpr size_t d2 = d + 2;
	constexpr size_t dd = d * 2 + 1;

	std::array<double, d1> coefs{ 0 };
	std::array<double, dd> xset{ 0 };
	std::array<double, d1> yset{ 0 };
	std::array<std::array<double, d2>, d1> table{ 0 };

	double sum[dd] = { 0 };
	sum[0] = dataSize;
	for (int n = 0; n < dataSize; n++)
	{
		double v = data[n].x;
		double v2 = v;
		sum[1] += v;
		for (int y = 2; y < dd; y++)
		{
			v2 *= v;
			sum[y] += v2;
		}
	}
	for (int y = 0; y < dd; y++)
	{
		xset[y] = sum[y];
		sum[y] = 0.0;	// for later use
	}

	for (int y = 0; y < d1; y++)
	{
		for (int x = 0; x < d1; x++)
		{
			table[y][x] = xset[y + x];
		}
	}

	for (int n = 0; n < dataSize; n++)
	{
		double x = data[n].x;
		double v = data[n].val;
		//double x2 = x;
		sum[0] += v;

		for (int y = 1; y < dd; y++)
		{
			v *= x;
			sum[y] += v;
		}
	}
	for (int y = 0; y < d1; y++)
	{
		yset[y] = sum[y];
		//		sum[y] = 0.0;	// for later use
	}
	for (int n = 0; n < d1; n++)
	{
		table[n][d1] = yset[n];
	}

	for (int n = 0; n < d1; n++)
	{
		for (int y = n + 1; y < d1; y++)
		{
			if (table[n][n] < table[y][n])
			{
				for (int x = 0; x < d2; x++)
				{
					std::swap(table[n][x], table[y][x]);
				}
			}
		}
	}

	for (int n = 0; n < d; n++)
	{
		for (int y = n + 1; y < d1; y++)
		{
			const double t = table[y][n] / table[n][n];
			for (int x = 0; x < d2; x++)
			{
				table[y][x] -= t * table[n][x];
			}
		}
	}

	for (int y = d; y >= 0; --y)
	{
		if (table[y][y] != 0.0)
		{
			coefs[y] = table[y][d1];
			for (int x = 0; x < d1; x++)
			{
				if (x != y)
					coefs[y] -= table[y][x] * coefs[x];
			}
			coefs[y] /= table[y][y];
		}
		else
			coefs[y] = 0.0;
	}

	return coefs;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
template<typename T>
void ApplyMedianFilter(T* output, T* input, int width, int height, int radius)
{
	auto get_med_func = [](float* data, int size) {	// fast version for small sized array
		for (int i = 0; i < size / 2; i++)
		{
			for (int j = 0; j < size - 1; j++)
			{
				if (data[j] > data[j + 1])
					std::swap(data[j], data[j + 1]);
			}
		}

		return data[size / 2];
	};

	int len = radius * 2 + 1;
	//int size = len * len;

#pragma omp parallel for
	for (int x = 0; x < width; x++)
	{
		std::vector<float> medians(len);
		std::vector<float> temp(len);

		// y == 0
		for (int y0 = 0 - radius, j = 0; y0 <= 0 + radius; y0++, j++)
		{
			int y1 = y0;
			if (y0 < 0)	y1 = 0;

			for (int x0 = x - radius, i = 0; x0 <= x + radius; x0++, i++)
			{
				int x1 = std::clamp(x0, 0, width - 1);
				float v = input[y1 * width + x1];
				temp[i] = v;
			}
			auto med = get_med_func(temp.data(), len);
			medians[j] = med;
		}

		std::vector<float> temp2 = medians;
		std::nth_element(temp2.begin(), temp2.begin() + len / 2, temp2.end());
		output[x] = temp2[len / 2];// get_med_func(temp2.data(), len);

		// 0<y<height-1
		for (int y = 1; y < height; y++)
		{
			int y0 = y + radius;
			int y1 = y0 < height ? y0 : height - 1;

			for (int x0 = x - radius, i = 0; x0 <= x + radius; x0++, i++)
			{
				int x1 = std::clamp(x0, 0, width - 1);
				float v = input[y1 * width + x1];
				temp[i] = v;
			}
			std::nth_element(temp.begin(), temp.begin() + len / 2, temp.end());
			medians[(y + len - 1) % len] = temp[len / 2];// get_med_func(temp.data(), len);

			temp2 = medians;
			std::nth_element(temp2.begin(), temp2.begin() + len / 2, temp2.end());
			output[y * width + x] = temp2[len / 2];// get_med_func(temp2.data(), len);
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
std::vector<float> BlurVerticalDirection(float* input, int width, int height, int radius, int medRadius, bool want_old_mode)
{
	std::vector<float> tempData(width * height);
	memcpy(tempData.data(), input, sizeof(float) * width * height);

	//constexpr float threshold = 0.1f;
	constexpr float base = 10.0;

#pragma omp parallel for
	for (int y = 0; y < height; y++)
	{
		double me, lo, hi;
		int count = 0;
		double sum = 0;

		for (int x = 0; x < width; x++)
		{
			int y0 = std::max(y - radius, 0);
			int y1 = std::min(y + radius, height - 1);

			double sum2 = 0;
			for (int y2 = y0; y2 <= y1; y2++)
			{
				double b = input[y2 * width + x];
				sum2 += b;
			}
			double avg = sum2 / (double)(y1 - y0 + 1);
			sum2 = 0;
			for (int y2 = y0; y2 <= y1; y2++)
			{
				double b = input[y2 * width + x] - avg;
				sum2 += b * b;
			}
			double var = sum2 / (double)(y1 - y0 + 1);
			double stdev = sqrt(var);


			double t = 0;
			if (want_old_mode)
			{
				t = 4000.0 / stdev * 0.16;
				t = std::clamp(t, 0.1, 0.3);
			}
			else
			{
				t = 400.0 / stdev * 0.16;
				t = std::clamp(t, 0.1, 0.2);
			}
			int pos = y * width + x;
			me = input[pos];
			lo = me * (1.0 - t) - base;
			hi = me * (1.0 + t) + base;
			count = 0;
			sum = 0.0;

			for (int y2 = y0; y2 <= y1; y2++)
			{
				double b = input[y2 * width + x];
				sum2 += b;
				if (b >= lo && b <= hi)
				{
					sum += b;
					count++;
				}
			}

			tempData[pos] = (float)(sum / (double)count);
		}
	}

	std::vector<float> output(width * height);
	ApplyMedianFilter(output.data(), tempData.data(), width, height, medRadius);
	return output;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// using20211113
std::vector<float> CalculateRatiosOfEachLine(float* ref, float* original, int width, int height, int margin, double cutoff, int nradius, double fstripe_gain, double fvary)
{// ref : blurred image, original : ApplyLinecorrect 결과, cutoff = 40
	std::vector<float> tempData(width * height);
	const int radius = nradius;					const int len = radius * 2 + 1;
	const double stripe_gain = fstripe_gain;	const double vary = fvary;
#pragma omp parallel for 
	// avg, std를 구한 후 tempData
	// tempData[pos] = (std_dev / avg < vary) ? ref/original : 0;
	for (int y = 0; y < height; y++) {
		std::vector<double> buffer(len);
		for (int x = 0; x < width; x++) {
			int pos = y * width + x;
			float v = ref[pos] / original[pos];
			if (original[pos] > cutoff && v >= 1.0 - stripe_gain && v <= 1.0 + stripe_gain) { // stripe_gain 0.13
				int count = 0; double avg = 0; // avg
				for (int offset = -width * radius; offset <= width * radius; offset += width) {
					if (pos + offset >= 0 && pos + offset < width * height) {
						double k2 = original[pos + offset];// k[offset];
						if (k2 > cutoff && abs(k2 - original[pos]) / original[pos] < 0.04) {
							avg += k2;		buffer[count++] = k2;
						}
					}
				}
				if (count > 0) {
					avg /= (float)count;					 double std_dev = 0;// std dev
					for (int n = 0; n < count; n++) {
						double e = buffer[n] - avg;
						std_dev += e * e;
					}
					std_dev = sqrt(std_dev / (double)(count - 1));
					double checker = std_dev / avg;
					tempData[pos] = (checker < vary) ? v : 0;
				}
				else {
					tempData[pos] = v;
				}
			}
		}
	}

	constexpr int degree = 4;// 3->4 20210319
	double d = 0.1;
	double lower = 1 - d;	 double upper = 1 + d;//20210319
	std::vector<float> ratio(width * height, 1.0);	// output
#pragma omp parallel for
	for (int y = 0; y < height; y++) {
		std::vector<INDEX_VAL> xy(width - margin * 2);// margin =100. 좌/우 100 픽셀을 제외한 가로 한 줄의 image
		int xySize = 0;		//int counter = 0;
		for (int x = margin; x < width - margin; x++) {
			int pos = y * width + x;	float r = ref[pos];			float s = tempData[pos];
			if (r > cutoff && s >= lower && s <= upper) { // cutoff = 40
				xy[xySize++] = { x, s };
			}
		}
		auto coefs = Regression<degree>(xy.data(), xySize); // fitting 계수를 구하고
		for (auto& coef : coefs) { if (std::isnan(coef)) coef = 0; }

		for (int x = 0; x < width; x++) {
			const int pos = y * width + x;	//float r = ref[pos];		//float s = tempData[pos]; 
			float value = (float)coefs[0];	float x0 = (float)x;
			for (int i = 1; i <= degree; i++) { // 해당 x좌표에서의 fitting 된 값을 계산
				value += (float)(coefs[i] * x0);
				x0 *= x;
			}
			ratio[pos] = std::clamp(value, (float)lower, (float)upper);
		}
	}
	return ratio;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::RemoveWormPattern(cv::Mat input_, int radius, int medRadius, int margin, double cutoff,
	bool want_old_mode, int nradius, double fstripe_gain, double fvary, bool bPE)
{
	int width = input_.cols;
	int height = input_.rows;

	auto blurred = BlurVerticalDirection(input_.ptr<float>(0), width, height, radius, medRadius, want_old_mode);
	auto ratio = CalculateRatiosOfEachLine(blurred.data(), input_.ptr<float>(0), width, height, margin, cutoff, nradius, fstripe_gain, fvary);
	cv::Mat matRatio(height, width, CV_32FC1);
	for (int n = 0; n < width * height; n++)
		matRatio.ptr<float>(0)[n] = ratio[n];
	if (bPE)
	{
		cv::Mat blurredRatioMat(height, width, CV_32FC1);
		int sigma = 8;
		cv::GaussianBlur(matRatio, blurredRatioMat, cv::Size(sigma * 8 + 1, sigma * 8 + 1), sigma); // kernel size >= 8 *sigma + 1
		matRatio = blurredRatioMat.clone();
	}

	cv::Mat output= input_.mul(matRatio);
	return output.clone();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 


#pragma endregion Stripe Removal

#pragma endregion Preprocess

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::polyfit(const cv::Mat& src_x, const cv::Mat& src_y, int order)
{
	CV_Assert((src_x.rows > 0) && (src_y.rows > 0) && (src_x.cols == 1) && (src_y.cols == 1) && (order >= 1));
	cv::Mat X = Mat::zeros(src_x.rows, order + 1, CV_64F);  // 2024-07-22. jg kim. NED영역에서 세로줄 발생 디버깅
	cv::Mat copy;
	for (int i = 0; i <= order; i++)
	{
		copy = src_x.clone();
		cv::pow(copy, i, copy);
		cv::Mat M1 = X.col(i);
		copy.col(0).copyTo(M1);
	}
	cv::Mat X_t, X_inv;
	cv::transpose(X, X_t);
	cv::invert((X_t * X), X_inv);
	return (X_inv * X_t) * src_y;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
double ImageProcessor::getGain(cv::Mat p1, cv::Mat p2, double level)  // 2024-07-22. jg kim. NED영역에서 세로줄 발생 디버깅
{
	return  get_fitted_value(p2, level) / get_fitted_value(p1, get_fitted_value(p2, level));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
double ImageProcessor::get_fitted_value(cv::Mat p, double valueInput)  // 2024-07-22. jg kim. NED영역에서 세로줄 발생 디버깅
{
	double* pP = (double*)p.data;
	return pP[4] * pow(valueInput, 4) + pP[3] * pow(valueInput, 3) + pP[2] * pow(valueInput, 2) + pP[1] * valueInput + pP[0];
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
int ImageProcessor::BinarizeIPArea(cv::Mat& output, cv::Mat input_)
{
	bool ret = false;
	auto* out_ptr = output.ptr<ushort>(0);
	// 2026-02-06. jg kim. wobble 보정을 위한 반사판 영역 처리
	for (int y = 0; y < input_.rows; y++)
		for (int x = g_nLaserOnPos; x < input_.cols; x++)
			input_.at<ushort>(y, x)=0;

	cv::Mat ipArea = CreateInitialBackground_New(input_);
	if (ipArea.cols == 0 || ipArea.rows == 0)
		ret = false;

	for (int n = 0; n < ipArea.cols * ipArea.rows; n++)
		out_ptr[n] = ipArea.ptr<uchar>(0)[n] > 0 ? 65535 : 0;

	// 2024-04-26. jg kim. door open이면서 low x-ray power에 걸리지 않는 예외 처리 추가
	int counts = 0;
	for (int y = 0; y < ipArea.rows; y++)
	{
		for (int x = 6; x < 140; x++)
		{
			ushort value = output.at<ushort>(y, x);
			if (value )
				counts++;
		}
		// 2026-02-09. jg kim. 반사판 영역 제외
		// for (int x = ipArea.cols - 106; x < ipArea.cols - 6; x++)
		// {
		// 	ushort value = output.at<ushort>(y, x);
		// 	if (value)
		// 		counts++;
		// }
	}
	if (counts > 10)
		ret = false;
	else
		ret = true;

	counts = 0;
	for (int y = 0; y < ipArea.rows; y++)
	{
		for (int x = 150; x < g_nLaserOnPos; x++)// 2026-02-09. jg kim. 반사판 영역 제외
		{
			ushort value = output.at<ushort>(y, x);
			if (value)
				counts++;
		}
	}
	// char buf[1024];
	// sprintf(buf, "IP binary pixel counts: %d\n", counts);
	// writelog(buf, LOG_FILE_NAME);
	// printf_s(buf);
	
	if (counts < 3000) // 2024-04-26. jg kim. SR 모드 Size0 의 1%에 해당하는 pixel 갯수. binary mask에서 IP 영역이 너무 작으면 invalid 처리
		ret = false;
	else
		ret = true;

	return ret;
}

void ImageProcessor::guessAndAnalyseIP(ImageProcessor::stInfoCorner &info, cv::Mat bin, cv::Mat Rot_bin, float thresholdCorner, bool bRotate)
{
	// 2024-04-16. jg kim. 오차범위를 변수화
	float error = float(IP_LENGTH_ERROR); // 오차 범위를 기존 2%에서 5%로 완화
	float error2 = error * 2.f;
	std::vector<ImageProcessor::stInfoCorner> infos;
	// 2024-04-16. jg kim.
	// getCornerInfos 함수는 bin 영상에서 각 IP index별 corner 정보를 구하여 infos 벡터에 저장함.
	// infos 벡터의에는 회전하지 않은 경우와 회전한 경우(bRotate)의 corner 정보가 모두 저장됨.
	getCornerInfos(bin, error, infos, thresholdCorner);
	if (bRotate)
		getCornerInfos(Rot_bin, error, infos, thresholdCorner);

	int maxIndex = -1;
	float maxproduct = 0;
	int nBinaryIPCount = cv::countNonZero(bin);
	
	// 2026-02-03. jg kim. Edge score 계산을 위한 Sobel filtering (FW IpExtraction 함수 차용)
	cv::Mat bin8u;
	bin.convertTo(bin8u, CV_8U, 1.0/255.0);
	cv::Mat sobelX, sobelY, edgeImg;
	cv::Sobel(bin8u, sobelX, CV_32F, 1, 0, 3);
	cv::Sobel(bin8u, sobelY, CV_32F, 0, 1, 3);
	cv::magnitude(sobelX, sobelY, edgeImg);
	edgeImg.convertTo(edgeImg, CV_8U);
	
	// bRotate가 true인 경우 회전된 binary mask에서도 edge image 생성
	cv::Mat rotEdgeImg;
	if (bRotate) {
		cv::Mat rotBin8u;
		Rot_bin.convertTo(rotBin8u, CV_8U, 1.0/255.0);
		cv::Mat rotSobelX, rotSobelY;
		cv::Sobel(rotBin8u, rotSobelX, CV_32F, 1, 0, 3);
		cv::Sobel(rotBin8u, rotSobelY, CV_32F, 0, 1, 3);
		cv::magnitude(rotSobelX, rotSobelY, rotEdgeImg);
		rotEdgeImg.convertTo(rotEdgeImg, CV_8U);
	}

	for (int i = 0; i < int(infos.size()); i++)
	{
		ImageProcessor::stInfoCorner temp_Info = infos.at(i);
		// product : (corner 인지 판별하는 thresholdCorner를 넘는)corner 갯수 * (threshold 를 넘는 corner의)평균 corner score * (1 - abs(err_rW)) * (1 - abs(err_rH))
		//         : product가 최대인 조합이 가능성이 가장 높은 IP index가 됨.
		//		   : err_rW, err_rH는 오차율(error)을 넘으면 2배를 곱하여 product가 오차율을 넘지 않는 경우보다 훨씬 작아지도록 조정함.
		float err_rW = temp_Info.errRatio[0] > error2 ? error2 : temp_Info.errRatio[0];
		float err_rH = temp_Info.errRatio[1] > error2 ? error2 : temp_Info.errRatio[1];
		float wHor = err_rW < error ?  (1.f - err_rW) :  (1.f - err_rW * 2);
		float wVer = err_rH < error ?  (1.f - err_rH) :  (1.f - err_rH * 2);
		float product = float(temp_Info.countOverCornerThreshold)*temp_Info.meanScoreCornerOverCornerThreshold * wHor * wVer;

		double fDummyPixelCounts = (double(ip[i%4].width*ip[i%4].height)-double(ip[i%4].rad*ip[i%4].rad)*(4-CV_PI));
		// 2026-02-03. jg kim. ratio는 IP가 온전히 보이는 경우 설계치에 의한 IP pixel 갯수와 binary 영상에서 IP pixel 갯수의 비율이 1에 가까워짐.
		double ratio = ((fDummyPixelCounts) / double(nBinaryIPCount));
		ratio = ratio < 0.95 ? 0.95 : (ratio>1.05 ? 1.05 : ratio); // 너무 큰 오차 방지

		// Edge score 계산 (IpExtraction 아이디어 적용)
		int ipIdx = i % 4;
		int rightEdgeScore = 0, bottomEdgeScore = 0;
		
		// 회전 여부에 따른 적절한 edge image 선택
		bool isRotatedCase = bRotate && (i >= 4); // infos의 첫 4개는 원본, 다음 4개는 회전된 경우
		cv::Mat& currentEdgeImg = isRotatedCase ? rotEdgeImg : edgeImg;
		
		int leftMost = 150, topMost = 65;

		// search area를 고려한 edge position 계산
		int searchArea = 50;
		int rightX_min = std::max(0, leftMost + ip[ipIdx].width - searchArea);
		int rightX_max = std::min(currentEdgeImg.cols - 1, leftMost + ip[ipIdx].width + searchArea);
		int bottomY_min = std::max(0, topMost + ip[ipIdx].height - searchArea);
		int bottomY_max = std::min(currentEdgeImg.rows - 1, topMost + ip[ipIdx].height + searchArea);
		
		// 우측 edge score 계산 (search area에서 최대값 찾기)
		for (int x = rightX_min; x <= rightX_max; x++) {
			int tempScore = 0;
			for (int y = topMost; y < std::min(topMost + ip[ipIdx].height, currentEdgeImg.rows); y++) {
				if (currentEdgeImg.at<uchar>(y, x) > 0) tempScore++;
			}
			rightEdgeScore = std::max(rightEdgeScore, tempScore);
		}
		
		// 하단 edge score 계산 (search area에서 최대값 찾기)
		for (int y = bottomY_min; y <= bottomY_max; y++) {
			int tempScore = 0;
			for (int x = leftMost; x < std::min(leftMost + ip[ipIdx].width, currentEdgeImg.cols); x++) {
				if (currentEdgeImg.at<uchar>(y, x) > 0) tempScore++;
			}
			bottomEdgeScore = std::max(bottomEdgeScore, tempScore);
		}
		int edgeScore = rightEdgeScore + bottomEdgeScore;
		

		float enhancedProduct = (product*double(temp_Info.countOverCornerThreshold)/2) * (1.0f + edgeScore*2)/ratio;
		// if (m_bSaveCorrectionStep)
		// {
		// 	sprintf_s(buf, "Index = %d, product = %.1f. countOverCornerThreshold =%d, ratio = %.1f, right = %d, bottom = %d, enhancedProduct = %.1f\n", i, product,temp_Info.countOverCornerThreshold, ratio, rightEdgeScore, bottomEdgeScore, enhancedProduct);
		// 	printf("%s", buf);
		// 	writelog(buf, LOG_FILE_NAME);
		// }
		// product에 edge score를 가중치로 추가 (IpExtraction 아이디어)
		if (enhancedProduct > maxproduct)
		{
			maxproduct = enhancedProduct;
			maxIndex = i;			
		}
	}

	// if (m_bSaveCorrectionStep)
	// {
	// 	sprintf_s(buf, "maxIndex = %d\n", maxIndex);
	// 	printf("%s", buf);
	// 	writelog(buf, LOG_FILE_NAME);
	// }
	if (maxIndex == -1)
	{
		ImageProcessor::stInfoCorner dummy;
		info = dummy;
	}
	else
		info = infos.at(maxIndex);

	info.n_guessedIndex = maxIndex;
	info.n_maxIndex = maxIndex;
	
	// if (minRatio < 0.1)
	// 	info.n_guessedIndex = maxIndex;

	// if (info.countOverCornerThreshold < 2 && minRatio > 0.1) // corner가 2개 이상 보이지 않으면 추정할 수 없다.
	// 														 // 2026-02-03. jg kim. 설계상 IP 크기별 ratio는 최소 0.15 가량이다.
	// 														 // 따라서 minRatio가 0.1보다 작으면 정확하다 생각할 수 있다.

	// 	info.n_guessedIndex = -1;
}

void ImageProcessor::guessAndAnalyseIP(ImageProcessor::stInfoCorner & info, cv::Mat bin)
{
	RotatedRect rr = getMinRect(bin);
	cv::Mat rotBin = GetRotatedImage(bin, rr);
	float thresholdCorner = float(IP_CORNER_THRESHOLD);
	bool bRotate = abs(rr.angle) > 0.01f ? true : false;
	guessAndAnalyseIP(info, bin, rotBin, thresholdCorner, bRotate);
}

void ImageProcessor::getCornerInfos(cv::Mat bin, float error, std::vector<ImageProcessor::stInfoCorner> &infos, float cornerThres)
{// 2024-04-26. jg kim. log 작성을 위해 추가
	for (int i = 0; i < IP_INDICES; i++)
		infos.push_back(getCornerInfo(bin, i, error, cornerThres));
}

std::vector<cv::Rect> ImageProcessor::getSubMaskRois(cv::Rect rtBound, int radi)
{
	std::vector<cv::Rect> rois;
	rois.push_back(cv::Rect(rtBound.x, rtBound.y, radi, radi));
	rois.push_back(cv::Rect(rtBound.x + rtBound.width - radi, rtBound.y, radi, radi));
	rois.push_back(cv::Rect(rtBound.x, rtBound.y + rtBound.height - radi, radi, radi));
	rois.push_back(cv::Rect(rtBound.x + rtBound.width - radi, rtBound.y + rtBound.height - radi, radi, radi));

	return rois;
}

// 2024-12-13. jg kim. 기존의 함수를 개선
float ImageProcessor::getCornerScore(cv::Mat subMask, int nCornerIndex)
{
	// 정사각형의 한변을 반지름으로 하는 1/4원과 나머지 부분의 면적비는 대략 0.785 : 0.215 임.
	int countZero = int((1 - CV_PI / 4) * subMask.rows * subMask.cols);// IP의 corner를 제외한 나머지 부분의 pixel 갯수 (정사각형 전체 픽셀의 21.5% 가량)
	int countNonZero = int((CV_PI / 4)  * subMask.rows * subMask.cols);// 원의 1/4. IP의 corner의  pixel 갯수 (정사각형 전체 픽셀의 78.5% 가량)
	int countZeroOrg = countZero;
	int countNonZeroOrg = countNonZero;
	cv::Point center;
	int radius = subMask.rows;
	for (int y = 0; y < subMask.rows; y++)
		for (int x = 0; x < subMask.cols; x++)
		{
			unsigned short value = subMask.at<unsigned short>(y, x);
			center = getCornerCenter(subMask, nCornerIndex);
			float distance = sqrt(pow2(center.x - x) + pow2(center.y - y));
			if (distance <= radius) // IP의 코너(원의 1/4)
			{
				if (value ==0)		countNonZero--;
			}
			else // 사각형에서 원의 1/4을 제외한 나머지 부분 
			{
				if (value > 0)	countZero--; 
			}
		}
	float score = 0;
	if (countNonZero <= 0 || countZero <= 0)// nonzero나 zero인 영역의 pixel 갯수가 0보다 작거나 같다는 것은 확실하게 IP의 corner가 정상적이지 않다는 것임.
											// 이런 corner는 사용하지 않도록 하지 않기 위해 score를 0으로 함.
		score = 0;
	else
	{
		float ratioNonZero = float(countNonZero) / float(countNonZeroOrg);
		float ratioZero = float(countZero) / float(countZeroOrg);
		score = (ratioNonZero + ratioZero) / 2;
	}
	
	return score;
}

std::vector<cv::Mat> ImageProcessor::getSubMasks(cv::Mat Mask, std::vector<cv::Rect> rois)
{
	std::vector<cv::Mat> subMasks;

	for (int i = 0; i < CORNERS; i++)
		subMasks.push_back(Mask(rois.at(i)));

	return subMasks;
}

// 2024-07-01. jg kim. guessIpIdx_New에 있던 코드를 함수로 추출함

ImageProcessor::stInfoCorner ImageProcessor::getCornerInfo(cv::Mat bin, int nIpIndex, float error, float cornerThres)
{
	ImageProcessor::stInfoCorner info;
	Rect rect = getBoundingRect(bin);
	if (rect.width < 100 && rect.height < 100) // 2024-10-14. jg kim. 예외처리 추가	
		memset(info.scoreCorner, 0, sizeof(float) * CORNERS);
	else
	{
		cv::Rect rtImg(0, 0, bin.cols, bin.rows);
		int dumW = ip[nIpIndex].width;
		int dumH = ip[nIpIndex].height;		
		int refL = LASER_ON_POSITION;
		int refT = SCAN_START_POSITION;
		int radi = nIpIndex==2 ? int(ip[nIpIndex].rad+0.5f) : int(ip[nIpIndex].rad);
		int W = rect.width;
		int H = rect.height;

		float errW = abs(1 - float(W) / float(dumW));
		float errH = abs(1 - float(H) / float(dumH));
		info.errRatio[0] = errW;
		info.errRatio[1] = errH;

		int L = rect.x;
		int T = rect.y;
		int R = rect.x + W;
		int B = rect.y + H;

		int x = 0;
		int y = 0;
		int i = 0;
		int counts_T = PixelCounts(bin(checkRoiBound(rtImg, cv::Rect(refL,			  refT,			   dumW,	 dumH / 2))), 1000); // 1000 : mask와 background를 구별하기 위한 threshold.
		int counts_B = PixelCounts(bin(checkRoiBound(rtImg, cv::Rect(refL,			  refT + dumH / 2, dumW,	 dumH / 2))), 1000);
		int counts_L = PixelCounts(bin(checkRoiBound(rtImg, cv::Rect(refL,			  refT,			   dumW / 2, dumH))), 1000);
		int counts_R = PixelCounts(bin(checkRoiBound(rtImg, cv::Rect(refL + dumW / 2, refT,			   dumW / 2, dumH))), 1000);

		if (errW <= error && errH <= error) // Case 1. 최상의 경우
		{
			x = L;			y = T;					info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
			x = R - radi;	y = T;					info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
			x = L;			y = B - radi;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
			x = R - radi;	y = B - radi;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
		}
		else if (errW <= error && errH > error) // Case 2.  가로로 IP가 온전히 보이지만 상/하 방향으로는 최소 어느 한쪽이 온전히 보이지 않는 경우
		{// 상/하 어느쪽을 중심으로 corner score를 계산할지 결정
			if (counts_T >= counts_B) // top을 중심으로 corner score 계산
			{
				x = L;			y = T;					info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = R - radi;	y = T;					info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				// bottom은 corner score가 top에 비해 아주 작게 나오도록 dumH를 더하여 찾는다.
				x = L;			y = T + dumH - radi;	info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = R - radi;	y = T + dumH - radi;	info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
			}
			else // bottom을 중심으로 corner score 계산 
			{
				// top은 corner score가 bottom에 비해 아주 작게 나오도록 dumH를 더하여 찾는다.
				x = L;		  y = B - dumH;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = R - radi; y = B - dumH;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = L;		  y = B - radi;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = R - radi; y = B - radi;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
			}
		}
		else if (errW > error && errH <= error) // Case 3.  세로로 IP가 온전히 보이지만 좌/우 방향으로는 최소 어느 한쪽이 온전히 보이지 않는 경우
		{ // 좌/우 어느쪽을 선택할지 결정
			if (counts_L >= counts_R) // left
			{
				x = L;					y = T;				info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = L + dumW - radi;	y = T;				info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = L;					y = B - radi;		info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = L + dumW - radi;	y = B - radi;		info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
			}
			else // right
			{
				x = R - dumW;			y = T;				info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = R - radi;			y = T;				info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = R - dumW;			y = B - radi;		info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				x = R - radi;			y = B - radi;		info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
			}
		}
		else // Case 4.  errW > error && errH > error
		{
			// LT, RT, LB, RB 어느 한 corner를 중심으로 계산
			if (counts_L >= counts_R) // left 중심
			{
				if (counts_T >= counts_B) // top 중심
				{
					x = L;					y = T;					info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = L + dumW - radi;	y = T;					info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = L;					y = T + dumH - radi;	info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = L + dumW - radi;	y = T + dumH - radi;	info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				}
				else // bottom 중심
				{
					// top은 corner score가 bottom에 비해 아주 작게 나오도록 dumH를 더하여 찾는다.
					x = L;					y = B - dumH;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = R - radi;			y = B - dumH;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = L;					y = B - radi;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = R - radi;			y = B - radi;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				}
			}
			else // right 중심
			{
				if (counts_T >= counts_B) // top 중심
				{
					x = R - dumW;			y = T;					info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = R - radi;			y = T;					info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = R - dumW;			y = T + dumH - radi;	info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = R - radi;			y = T + dumH - radi;	info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				}
				else // bottom 중심
				{
					// top은 corner score가 bottom에 비해 아주 작게 나오도록 dumH를 더하여 찾는다.
					x = R - dumW;			y = B - dumH;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = R - radi;			y = B - dumH;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = R - dumW;			y = B - radi;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
					x = R - radi;			y = B - radi;			info.scoreCorner[i] = getCornerScore(bin(checkRoiBound(rtImg, Rect(x, y, radi, radi))), i); i++;
				}
			}
		}
	
		info.corners_L = int(info.scoreCorner[int(e_LeftTop)] > cornerThres) + int(info.scoreCorner[int(e_LeftBottom)] > cornerThres);
		info.corners_R = int(info.scoreCorner[int(e_RightTop)] > cornerThres) + int(info.scoreCorner[int(e_RightBottom)] > cornerThres);
		info.corners_T = int(info.scoreCorner[int(e_LeftTop)] > cornerThres) + int(info.scoreCorner[int(e_RightTop)] > cornerThres);
		info.corners_B = int(info.scoreCorner[int(e_LeftBottom)] > cornerThres) + int(info.scoreCorner[int(e_RightBottom)] > cornerThres);

		info.countOverCornerThreshold = 
			  int(info.scoreCorner[int(e_LeftTop)] > cornerThres) + int(info.scoreCorner[int(e_RightTop)] > cornerThres) 
			+ int(info.scoreCorner[int(e_LeftBottom)] > cornerThres) + int(info.scoreCorner[int(e_RightBottom)] > cornerThres);

		switch (info.countOverCornerThreshold)
		{
		case 0:
		{
			info.e_usedCorner = e_NoCorner;
		}
			break;
		case 1:
		{
			for (int j = 0; j < CORNERS; j++)
				if (info.scoreCorner[j] > cornerThres)
					info.e_usedCorner = usedCorner(j);
		}
		break;
		case 2:
		{
			if (info.corners_L >= 2)
				info.e_usedCorner = e_LeftEdge;
			else if (info.corners_R >= 2)
				info.e_usedCorner = e_RightEdge;
			else if (info.corners_T >= 2)
				info.e_usedCorner = e_TopEdge;
			else
				info.e_usedCorner = e_BottomEdge;
		}
		break;
		case 3:
		{
			;// 별도로 정의하지 않음.
		}
		break;
		case 4:
		{
			info.e_usedCorner = e_AllCorner;
		}
		break;
		}

		info.meanScoreCornerOverCornerThreshold = 0.f;
		for (int j = 0; j < CORNERS; j++)
			if (info.scoreCorner[j] > cornerThres)
				info.meanScoreCornerOverCornerThreshold += info.scoreCorner[j];

		if (info.countOverCornerThreshold > 0)
			info.meanScoreCornerOverCornerThreshold /= float(info.countOverCornerThreshold);
		else
			info.meanScoreCornerOverCornerThreshold = 0.f;
	}
	return info;
}

// 2024-04-09. jg kim. 품혁 이슈에 대응하기 위해 새로 구현
cv::Point ImageProcessor::getCornerCenter(cv::Mat subMask, int nCornerIndex)
{
	int cx = 0;
	int cy = 0;
	switch (nCornerIndex)
	{
	case 0:
	{ // left top
		cx = subMask.cols - 1;
		cy = subMask.rows - 1;
	}
	break;
	case 1:
	{// right top
		cx = 0;
		cy = subMask.rows - 1;
	}
	break;
	case 2:
	{// left bottom
		cx = subMask.cols - 1;
		cy = 0;
	}
	break;
	case 3:
	{// right bottom
		cx = 0;
		cy = 0;
	}
	break;
	}

	return cv::Point(cx, cy);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::CheckRemoveTagedImage(cv::Mat image, int nROI)
{
	float fmean = (float)mean(image(Rect(0, 0, nROI, nROI))).val[0];
	for (int y = 0; y < image.rows; y++) { // 스캔 후반부에 background level 이 올라가 IP 영역 추출 정확도가 떨어지는 현상에 대응하기 위함
		float temp_mean = (float)cv::mean(image(Rect(0, y, nROI, 1))).val[0];
		for (int x = 0; x < image.cols; x++)
			if (image.at<unsigned short>(y, x) < temp_mean + 160 || x > image.cols - 106)
				image.at<unsigned short>(y, x) = unsigned short(fmean);
	}
	return image;
}
/*
 Raw image의 각 row를 x 좌표 별로 영역을 구별하면
0~149 : laser off 영역
150~1429 : laser on 영역. 1429 = 150 + 1280.
		이 영역에서는 초기에 continuous 한 laser 조사를 했지만
 		해상도가 충분히 나오지 않아 전자그룹 조정현 차장님이 
 		1/4 pixel 구간만 laser를 조사하고 
 		3/4 pixel 구간은 laser를 조사하지 않는 방식으로 스캔 방식을 변경함. (2023-12-15 경)
 		이로 인한 물리적인 효과는 convolution kernel이 작아아져 해상도 향상에 도움이 되는 것으로 추정됨.
 		실험 결과 해상도 05-502 차트스캔 시 기존 방식(continuous laser조사) 대비 3 lp/mm 가량 해상도 향상이 확인됨
1280 : Size 2 IP의 width (1240) + 여유 영역(40; 1mm)
 		Cruxell에서 좌/우 0.5mm (20 pixel)씩 여유 영역을 두고 IP를 설계했을 것으로 추정됨.
1430~1529 : laser off 영역
 		이 영역에서는 laser를 조사하지 않았지만, wobble 보정을 위해 반사판을 이 구간에 삽입, continuous laser 조사하는 방식으로 변경.
 		변경 후 세로 줄 패턴이 생성되고, 이 패턴을 이용하여 wobble 보정을 수행함(CorrectWobble 함수).
		(2025년 10월~11월 약 2개월간 알고리즘 개발 및 검증 진행)
 		* 여기서 말하는 wobble은 Main scan 방향 wobble임. Sub scan 방향 wobble 보정과 무관함.
1530~1535 : Tag 영역. Cruxell에서 RPM 정보를 삽입하기 위해 태그를 삽입했음.
*/
// 2024-07-01. jg kim. Binary image 생성 시 BG 보다 어두운 부분 검출하는 기능 구현
cv::Mat ImageProcessor::GetBinaryImage(cv::Mat image)
{
	// 2024-12-17 jg kim. debug. 데이터 형 체크 추가
	if (image.type() != CV_16U)
	{
		cv::Mat temp = image.clone();
		temp.convertTo(image, CV_16U);
	}

	cv::Mat blur;
	GaussianBlur(image, blur, Size(5, 5), 2);	

	double fmin;
	double fmax=0;
	float fmean, fstddev;
	int refL = 100; // left laser off 범위
	int refR = blur.cols - 106; // right laser off 범위.
	// Coding 당시에는 laser off 범위였지만 현재는 wobble 보정에 활용하기 위해 continuous laser 조사하는 영역으로 변경됨. (2025-10-11)
	int W = 100;
	cv::Mat bin = cv::Mat::zeros(blur.size(), blur.type());
	for (int r = 0; r < blur.rows; r++)// 2024-12-16. jg kim. debug
	{
		int H = 100;
		if (r + H > blur.rows - 1) // 마지막 부분에서 100x100 영역이 blur 이미지의 범위를 벗어나는 경우, H를 조정하여 범위 내에서 영역을 추출하도록 함.
			H = blur.rows - 1 - r;
		cv::Mat subImg = blur(Rect(10, r, W, H)); // blur image의 100x100 영역을 추출

		cv::minMaxLoc(subImg, &fmin, &fmax); // subImg의 최소값과 최대값 계산
		getMeanStddev(subImg, fmean, fstddev); // subImg의 평균과 표준편차 계산
		for (int c = refL; c < refR; c++) // blur 이미지의 각 row에서 left laser off 범위와 right laser off 범위를 제외한 영역에서 픽셀 값을 검사하여 binary image 생성
		{
			float value = float(blur.at<unsigned short>(r, c));
			if (value < fmin - fstddev * 2 && m_nImageProcessMode < 1)
			// m_nImageProcessMode < 1 인 경우: PortView에서 영상처리 할 때.
			// BG 보다 어두운 부분 검출하여 binary image를 보완하고자 함.
				bin.at<unsigned short>(r, c) = 65535;
			if (value > fmax + fstddev * 2)
				bin.at<unsigned short>(r, c) = 65535; // 일반적인 thresholding
		}
	}

	float fEdgeThresh = 0.65f;
	int nKernelSize = 25;
	//if (m_nImageProcessMode == 1) //1: image correction_PE (Extract IP 하지 않음)
	{
		cv::Mat th_org = bin.clone();
		bin = getCleanBinaryMask(bin, nKernelSize, fEdgeThresh); // remove pheripheral structure and keep central structure.
	}
	// 2024-04-22. jg kim. thresholding 결과에서 잡티를 제거하기 위해
	Mat outputNew; bin.convertTo(outputNew, CV_8U);
	int nonZeroCounts=0;
	nonZeroCounts = cv::countNonZero(outputNew);
	auto se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9));
	cv::erode(outputNew, outputNew, se);
	nonZeroCounts = cv::countNonZero(outputNew);
	if (nonZeroCounts == 0)
		bin.convertTo(outputNew, CV_8U);
	else
	{
		cv::erode(outputNew, outputNew, se);
		nonZeroCounts = cv::countNonZero(outputNew);
		if (nonZeroCounts == 0)
			bin.convertTo(outputNew, CV_8U);
		else
		{
			// erode 2번 후 nonzero가 남아있는 경우: dilate 2번 수행
			cv::dilate(outputNew, outputNew, se);
			cv::dilate(outputNew, outputNew, se);
		}
	}
	outputNew = CreateConvexHull(outputNew);
	outputNew.convertTo(bin, CV_16U);

	for (int r = 0; r < blur.rows; r++)
	{
		for (int c = refL; c < refR; c++)
		{
			auto value = bin.at<unsigned short>(r, c);
			if (value == 255)
				bin.at<unsigned short>(r, c) = 65535;
		}
	}
	return bin;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat ImageProcessor::VerticalExpand(cv::Mat image)
{
	cv::Mat temp;
	cv::resize(image, temp, cv::Size(image.cols, image.rows * 2), 0, 0, cv::INTER_LANCZOS4);
	return temp(cv::Rect(0, 0, image.cols, image.rows)).clone();
}

cv::Mat ImageProcessor::VerticalShrink(cv::Mat image)
{
	cv::Mat temp;
	cv::resize(image, temp, cv::Size(image.cols, image.rows * 0.5), 0, 0, cv::INTER_LANCZOS4);
	cv::Mat temp2 = cv::Mat::zeros(image.size(), image.type());
	temp.copyTo(temp2(cv::Rect(0, 0, temp.cols, temp.rows)) );
	return temp2.clone();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
Mat ImageProcessor::ZeropadImage(Mat image, int nTimes)
{
	cv::Mat paddedImage = cv::Mat::zeros(image.rows * nTimes, image.cols * nTimes, image.type());	
	image.copyTo(paddedImage(cv::Rect(image.cols / nTimes, image.rows / nTimes, image.cols, image.rows))); // 최종 영상을 paddedImage center에 복사
	return paddedImage;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
Mat ImageProcessor::DepadImage(Mat image, int nTimes)
{
	return image(cv::Rect(image.cols / (nTimes) / 2, image.rows / (nTimes) / 2, image.cols / (nTimes), image.rows / (nTimes))).clone();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
Mat ImageProcessor::DrawIP(int ipIndex)
{
	int ip_width = ip[ipIndex].width;
	int ip_height = ip[ipIndex].height;
	int ip_rad = (int)(ip[ipIndex].rad);
	return DrawIP(ip_width, ip_height, ip_rad);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
Mat ImageProcessor::DrawIP(int ip_width, int ip_height, int ip_rad)
{
	// CV_16UC1
	Mat output;
	Mat mask = Mat::zeros(Size(ip_width, ip_height), CV_8UC1);
	mask.copyTo(output);
	// p1 -- p2
	// |      |
	// p4 -- p3
	// 좌상단 마진 10px 씩 줬음
	// 1. IP 모양대로 Line 그리기

	Point tl(0, 0); //FLOODFILL_OFFSET
	Point p1(tl.x + 1, tl.y + 1);
	Point p2(tl.x + ip_width - 2, tl.y + 1);
	Point p3(tl.x + ip_width - 2, tl.y + ip_height - 2);
	Point p4(tl.x + 1, tl.y + ip_height - 2);

	line(mask, Point(p1.x + ip_rad, p1.y + 0), Point(p2.x - ip_rad, p2.y + 0), Scalar(255), 1, 4);	//LineTypes::LINE_4);
	line(mask, Point(p2.x + 0, p2.y + ip_rad), Point(p3.x + 0, p3.y - ip_rad), Scalar(255), 1, 4);	//LineTypes::LINE_4);
	line(mask, Point(p4.x + ip_rad, p4.y + 0), Point(p3.x - ip_rad, p3.y + 0), Scalar(255), 1, 4);	//LineTypes::LINE_4);
	line(mask, Point(p1.x + 0, p1.y + ip_rad), Point(p4.x + 0, p4.y - ip_rad), Scalar(255), 1, 4);	//LineTypes::LINE_4);

	ellipse(mask, p1 + Point(ip_rad, ip_rad), Size(ip_rad, ip_rad), 180., 0., 90., Scalar(255));
	ellipse(mask, p2 + Point(-ip_rad, ip_rad), Size(ip_rad, ip_rad), 270., 0., 90., Scalar(255));
	ellipse(mask, p3 + Point(-ip_rad, -ip_rad), Size(ip_rad, ip_rad), 0., 0., 90., Scalar(255));
	ellipse(mask, p4 + Point(ip_rad, -ip_rad), Size(ip_rad, ip_rad), 90., 0., 90., Scalar(255));

	// 2. 채우기
	cv::Point fillFrom(p1.x, p1.y);
	cv::floodFill(mask, fillFrom, Scalar(255));

	// 3. 배경 Black / IP 마스크영역 White
	mask.convertTo(output, CV_16UC1, 256);
	bitwise_not(output, output);
	return output;
}

cv::Mat ImageProcessor::getCorrectionCoefficientsForSingleImage(cv::Mat imgF, cv::Rect rt, float fUseRatioWidth)
{
	int newX = rt.x + rt.width / 2 - rt.width*fUseRatioWidth / 2;
	int newW = rt.width * fUseRatioWidth;
	// 2024-08-05. jg kim. polyfit ASSERT 에러 대응
	double *meanY = new double[newW];
	double *X = new double[newW];
	memset(meanY, 0, sizeof(double)*newW);
	memset(X, 0, sizeof(double)*newW);
	for (int c = 0; c < newW; c++)
	{
		int x = c + newX;
		X[c] = double(x);
		int cnt = 0;
		for (int r = 0; r < imgF.rows; r++)
			if (imgF.at<float>(r, x) > 0)
			{
				meanY[c] += double(imgF.at<float>(r, x));
				cnt++;
			}

		if (cnt > 0)
			meanY[c] /= float(cnt);
	}
	cv::Mat p = polyfit(cv::Mat(newW, 1, CV_64F, X), cv::Mat(newW, 1, CV_64F, meanY), DEGREE_COUNT);
	SAFE_DELETE_ARRAY(meanY);
	SAFE_DELETE_ARRAY(X);
	return p;
}

cv::Mat ImageProcessor::getHorizonGainCorrectedImage(cv::Mat img, cv::Mat p, cv::Rect rt)
{
	cv::Mat test = cv::Mat::zeros(img.rows, img.cols, CV_32F);
	for (int r = 0; r < img.rows; r++)
		for (int c = 0; c < img.cols; c++)
			test.at<float>(r, c) = get_fitted_value(p, c);

	//sprintf_s(buf, "_fitted_%d.tif", i);		cv::imwrite(buf, test);
	double min, max;
	cv::minMaxLoc(test/*(rt)*/, &min, &max); // 2024-07-12 jg kim. 디버깅
	cv::Mat gain = cv::Mat::zeros(img.size(), CV_32F);
	for (int c = rt.x; c < rt.x + rt.width; c++)
	{
		for (int r = rt.y; r < rt.y + rt.height; r++)
		{
			float scale = max / test.at<float>(r, c);
			if (scale <= max / min)
			{
				float value = img.at<float>(r, c) * scale;
				value = value < 0 ? 0 : (value > 65535 ? 65535 : value);
				gain.at<float>(r, c) = value;
			}
			else
			{
				gain.at<float>(r, c) = img.at<float>(r, c);
			}
		}
	}
	////sprintf_s(buf,"_scaled_%d.tif", i);		cv::imwrite(buf, gain);
	return gain;
}

cv::Mat ImageProcessor::getMaskedImage(cv::Mat img, cv::Mat mask)
{
	cv::Mat out = img.clone();
	for (int r = 0; r < img.rows; r++)
		for (int c = 0; c < img.cols; c++)
			out.at<float>(r, c) = mask.at<ushort>(r, c) == 0 ? 0 : img.at<float>(r, c);
	return out;
}

cv::Rect ImageProcessor::checkRoiBound(cv::Rect rtImg, cv::Rect roi)
{
	int W = rtImg.width;
	int H = rtImg.height;
	int x = roi.x;
	int y = roi.y;
	int w = roi.width;
	int h = roi.height;

	x = x < 0 ? 0 : x;
	y = y < 0 ? 0 : y;
	w = x + w > W ? W - x : w;
	h = y + h > H ? H - y : h;

	w = w < 0 ? 0 : w;
	h = h < 0 ? 0 : h;

	return cv::Rect(x, y, w, h);
}

ImageProcessor::stInfoCorner ImageProcessor::getInfoCorner()
{
	return m_stInfoCorner;
}

void ImageProcessor::setInfoCorner(ImageProcessor::stInfoCorner info)
{
	m_stInfoCorner = info;
}

void ImageProcessor::SuppressAmbientLight(unsigned short *pRawImage, int nWidth, int nHeight, int nLowRes)
{
	int sy = 0;
	int ey = nHeight;
	if (nLowRes == 1) // low resolution 처리
		ey = nHeight / 2;

	// 2024-06-06. jg kim. 하드코딩 했던 값들을 변수화 함. 
	// 레드마인 18519 이슈에 대응하기 위하여 skip width, measure width 값 변경
	int nMeasureWidth = m_roiBackground.width; // pixel
	int nMeasureHeight = m_roiBackground.height;// pixel
	int nSkipWidth = m_roiBackground.x; 	// pixel
	int nSkipHeight = m_roiBackground.y; 	// pixel
	int max_Background = 0;
	for (int y = nSkipHeight; y < nSkipWidth + nMeasureHeight; y++) // raw image의 좌측 상단에서 local max를 구하고
	{
		for (int x = nSkipWidth; x < nSkipWidth + nMeasureWidth; x++)
		{
			if (pRawImage[y * nWidth + x] > max_Background)
				max_Background = pRawImage[y * nWidth + x];
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
		for (int x = 0; x < 1410; x++)
		{
			if (pRawImage[y * nWidth + x] <= unsigned short(float(max)*1.05f)) // 각 line에서 max * 1.05f보다 작은 값은 local max 값으로 변경한다.
				pRawImage[y * nWidth + x] = unsigned short(max_Background);
		}
	}
}

void ImageProcessor::SetSaveCorrectionStep(bool bSaveStep)
{
	m_bSaveCorrectionStep = bSaveStep;
}
void ImageProcessor::SetImageProcess(bool bImageProcess)
{
	m_bImageProcess = bImageProcess;
}

void ImageProcessor::SetTiffDPI(int nResolutionMode)
{
	int xdpi = 400;
	int ydpi = 400;
	switch (nResolutionMode) // 0: HR (400, 400), 1: LR (800, 800), 2: LR_intermediate (400, 800)
	{
	case 0:
		xdpi = 400;
		ydpi = 400;	
		break;
	case 1:
		xdpi = 800;
		ydpi = 800;
		break;
	case 2:
		xdpi = 400;
		ydpi = 800;
		break;
	default:
		break;
	}
	SetTiffDPI(xdpi, ydpi);
}

void ImageProcessor::SetTiffDPI(int xdpi, int ydpi)
{
	// m_tiffParams 구조: [RESUNIT, 2, XDPI, xdpi_value, YDPI, ydpi_value]
	// XDPI는 인덱스 3, YDPI는 인덱스 5에 위치
	if (m_tiffParams.size() >= 6) {
		m_tiffParams[3] = xdpi; // XDPI 값 변경
		m_tiffParams[5] = ydpi; // YDPI 값 변경
	}
}

// 2024-03-28. jg kim. blank scan일 경우 장비에서 보내주는 scan mode를 활용하여 영상 크기를 반환
void ImageProcessor::GetIpSize(int & width, int & height, int nIndex)
{
	width = ip[nIndex].width;
	height = ip[nIndex].height;
}
// 2024-03-28. jg kim. blank scan일 경우 장비에서 보내주는 scan mode를 활용하기 위함
void ImageProcessor::SetIpIndex(int nIpIndex)
{
	m_nIpIndex = nIpIndex;
}
int ImageProcessor::GetIpIndex()
{
	return m_nIpIndex;
}
// 2024-06-25. jg kim. Cruxell 종속성 제거를 위해 구현
CRX_RECT2 ImageProcessor::_FindIPArea()
{
	// remove tag
	cv::Mat input = _input;
	cv::Mat tagRemoved = RemoveTag(input);
	tagRemoved = CorrectWobble(tagRemoved);	// 2025-11-07. jg kim. wobble correction 추가
	cv::Mat ipArea = CreateInitialBackground_New(tagRemoved); // 2025-11-07. jg kim. wobble correction을 위한 반사판 처리 추가

	// 외곽선 검출을 위한 변수
	std::vector<std::vector<cv::Point> > contour2;
	std::vector<cv::Vec4i> hierarchy1;

	// 1. 외곽선 검출 (contours)
	cv::findContours(ipArea, contour2, hierarchy1, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

	CRX_RECT2 rect = { 0, input.cols - 1 ,0, input.rows - 1 };

	double c_max = 0;
	int c_index = 0;
	int cur_index = 0;
	if (contour2.size() < 1)
	{
		// countour 검출 안된 ERROR처리
//			info_pp.result = ER_DLL_FAIL_FIND_COUNTOUR;
			//return ER_DLL_FAIL_FIND_COUNTOUR;
		return rect;
	}
	else
	{
		// 2. 검출된 외곽선 도형중 가장 큰 도형을 IP로 선택
		for (auto i = contour2.begin(); i != contour2.end(); i++)
		{
			if (cv::contourArea(*i) > c_max)
			{
				c_max = cv::contourArea(*i);
				c_index = cur_index;
			}
			cur_index++;
		}

		//drawContours(temp, contour2, c_index, cv::Scalar(255), cv::FILLED, cv::LineTypes::LINE_AA);
		auto BoundingRect_ip = cv::boundingRect(contour2[c_index]);

		rect.box.left = BoundingRect_ip.x;
		rect.box.right = rect.box.left + BoundingRect_ip.width - 1;
		rect.box.top = BoundingRect_ip.y;
		rect.box.bottom = rect.box.top + BoundingRect_ip.height - 1;

		auto RotatedRect_ip = cv::minAreaRect(contour2[c_index]);
		rect.rot.angle = RotatedRect_ip.angle + 90;
		if (RotatedRect_ip.size.width < RotatedRect_ip.size.height)
			rect.rot.angle = rect.rot.angle - 90;
		rect.rot.cx = (int)(RotatedRect_ip.center.x);
		rect.rot.cy = (int)(RotatedRect_ip.center.y);
		rect.rot.width = (int)(RotatedRect_ip.size.width);
		rect.rot.height = (int)(RotatedRect_ip.size.height);

	}
	return rect;
}

void ImageProcessor::SetImageProcessMode(int nImageProcessMode)
{
	m_nImageProcessMode = nImageProcessMode;
}

void ImageProcessor::SetDoorOpen(bool bOpen)
{
	m_bDoorOpen = bOpen;
}

// 두 점 사이의 거리 계산
double distance(cv::Point p1, cv::Point p2) {
	return sqrt(pow(p1.x - p2.x, 2) + pow(p1.y - p2.y, 2));
}

// 직선의 기울기 계산
double calculate_slope(cv::Point p1, cv::Point p2) {
	if (p2.x - p1.x == 0) {
		return INFINITY; // 수직선
	}
	return static_cast<double>(p2.y - p1.y) / (p2.x - p1.x);
}

// 라디안을 각도로 변환
double radians_to_degrees(double radians) {
	return radians * 180.0 / CV_PI;
}


// 2024-12-13. jg kim. 좌측 및 상단의 기구물을 제거하기 위해 구현
cv::Mat ImageProcessor::mergeMask(cv::Mat th, cv::Mat bin, cv::RotatedRect &rr)
{
	// 1. low resolution 전처리
	cv::Mat merged = cv::Mat::zeros(bin.size(), bin.type());
	if (m_nLowResolution)
	{
		th = VerticalExpand(th.clone());
		bin = VerticalExpand(bin.clone());
	}

	RotatedRect rrTh = getMinRect2(th);
	RotatedRect rrBin = getMinRect2(bin);
	// 2026-02-03. jg kim. 레드마인 이슈 24028 대응용
	// adaptive threshold로 구한 rrBin.angle의 값을 사용하도록 변경
	float angle =  /*PixelCounts(th, 1) > PixelCounts(bin, 1) ? rrTh.angle : */rrBin.angle;

	// 2. 회전 정보 통합. CreateInitialBackground_New에서 구한 bin으로부터 구한 rrBin의 angle을 사용 // 2026-02-03. jg kim. 레드마인 이슈 24028 대응용
	rrTh.angle = rrBin.angle = angle;

	// 3. 이미지 회전 및 SubMaskROI (IP 네 귀퉁이를 지정할) 설정
	cv::Mat rotBin = GetRotatedImage(bin, rrBin);
	cv::Mat rotTh = GetRotatedImage(th, rrTh);

	int radi = int(round(ip[m_nIpIndex].rad));
	std::vector<cv::Rect> rtsBin = getSubMaskRois(getBoundingRect(rotBin), radi);
	std::vector<cv::Rect> rtsTh = getSubMaskRois(getBoundingRect(rotTh), radi);
	
	// 4. 두 binary mask의 corner score 계산.
	float scoreBin[CORNERS] = {0, };
	float scoreTh[CORNERS] = {0, };

	for (int i = 0; i < CORNERS; i++)
	{
		scoreBin[i] = getCornerScore(bin(rtsBin.at(i)), i);
		scoreTh[i] = getCornerScore(rotTh(rtsTh.at(i)), i);
	}

	//5. 마스크 병합 및 모폴로지 연산(잡티 제거)
	cv::bitwise_or(rotTh, rotBin, rotTh); // 두 mask를 합치고
	cv::Mat bin8U; rotTh.convertTo(bin8U, CV_8U);

	auto se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(9, 9));
	cv::erode(bin8U, bin8U, se);
	cv::dilate(bin8U, bin8U, se);
	bin8U.convertTo(rotTh, CV_16U);
	rotTh *= 255;

	//6. 좌상단 코너 기구물 제거
	//상단 기구물 제거: rtsBin.at(0).y 위쪽 전체를 0으로 설정
	//좌측 기구물 제거: rtsBin.at(0).x 왼쪽 전체를 0으로 설정
	//코너 보정: 반지름 내에서 거리 계산으로 둥근 모서리 재생성
	if (scoreBin[0] > 0.9f || scoreTh[0] > 0.85f && abs(rtsTh.at(0).x - rtsBin.at(0).x) < 70 && abs(rtsTh.at(0).y - rtsBin.at(0).y) < 70)
		// 좌측 상단의 threshold 기반 binary mask는(th) 기구물의 영향이 클 수록 score가 낮게 나옴. 
		// 크럭셀 방식의 binary mask는 (bin) 기구물 제거는 잘 되나 mask가 주변부 보다 level이 낮은 영역을 mask에 포함시키지 못함.
		// 두 mask의 y 좌표의 차이는 기구물에 의한 것이고, 최대 차이를 20 pixel로 설정함.
	{
		for (int r = 0; r < rtsBin.at(0).y; r++) // mask 상단의 기구물 제거 목적
			for (int c = 0; c < th.cols; c++)
				rotTh.at<ushort>(r, c) = 0;

		for (int r = rtsBin.at(0).y; r < rtsBin.at(0).y + radi; r++) // 좌측 상단의 corner는 크럭셀 방식의 binary mask로 치환
			for (int c = rtsBin.at(0).x; c < rtsBin.at(0).x + radi; c++)
			{
				float dist = sqrt(pow2(rtsBin.at(0).y + radi-1 -r)+pow2(rtsBin.at(0).x + radi-1-c));
				if (dist > radi)
					rotTh.at<ushort>(r, c) = 0;
			}

		for (int r = 0; r < bin.rows; r++) // mask 좌측의 기구물 제거 목적
			for (int c = 0; c < rtsBin.at(0).x; c++)
				rotTh.at<ushort>(r, c) = 0;
	}
	// 7. 우상단 코너 기구물 제거
	// 좌상단과 동일한 방식으로, 단 우측 상단의 경우 좌측 상단보다 기구물의 영향이 덜할 것으로 예상되어
	// score 기준을 조금 완화함.
	if (scoreBin[1] > 0.9f || scoreTh[1] > 0.85f && abs(rtsTh.at(1).y - rtsBin.at(1).y) < 70)
		// 우측 상단의 threshold 기반 binary mask는(th) 기구물의 영향이 클 수록 score가 낮게 나옴. 
		// 크럭셀 방식의 binary mask는 (bin) 기구물 제거는 잘 되나 mask가 주변부 보다 level이 낮은 영역을 mask에 포함시키지 못함.
		// 두 mask의 y 좌표의 차이는 기구물에 의한 것이고, 최대 차이를 20 pixel로 설정함.
	{
		for (int r = 0; r < rtsBin.at(1).y; r++) // mask 상단의 기구물 제거 목적
			for (int c = 0; c < th.cols; c++)
				rotTh.at<ushort>(r, c) = 0;

		if ( abs( (rtsTh.at(1).x + rtsTh.at(1).width) - (rtsBin.at(1).x + rtsBin.at(1).width)) < 70 ) 
		{
			for (int r = rtsBin.at(1).y; r < rtsBin.at(1).y + radi; r++) // 우측 상단의 corner는 크럭셀 방식의 binary mask로 치환
				for (int c = rtsBin.at(1).x; c < rtsBin.at(1).x + radi; c++)
				{
					if (sqrt(pow2(rtsBin.at(1).y + radi - 1 - r) + pow2(rtsBin.at(1).x - c)) > radi)
						rotTh.at<ushort>(r, c) = 0;
				}

			for (int r = 0; r < bin.rows; r++) // 우측의 번지는 것 제거 목적
				for (int c = rtsBin.at(1).x + radi; c < bin.cols; c++)
					rotTh.at<ushort>(r, c) = 0;
		}		
	}

	// 8. 역회전 및 최종 처리
	// 역회전
	// 원래 방향으로 복원. (회전은 0.01도 이상인 경우에만 수행)
	// 이후 보정 단계에서는 회전하지 않은 영상에 보정을 하고, 회전은 Extract IP 직전 단계에서만 
	// 하기 때문에 영상 보정에 사용하는 mask는 회전된 상태로 유지.
	// IP 분석
	// 회전된 mask, 회전되지 않은 mask 각각에 대해서 IP 분석을 수행하여
	// IP corner의 위치(LT, RT, LB, RB) 및 score를 구하고,
	// corner threshold를 넘는 corner의 갯수, 평균 score 등을 계산.
	// 이런 지표를 바탕으로 product를 계산, 최대 product를 보이는 IP index를 최종 IP index로 선택.
	// 정보 저장
	// 계산된 정보를 m_stInfoCorner에 저장.
	// 해상도 복원
	// low resolution인 경우 원래 해상도로 복원
	rrTh.angle = -angle;
	th = cv::Mat::zeros(rotTh.size(), rotTh.type());
	GetRotatedImage(rotTh, th, rrTh);
	
	bool bRotate = abs(rrTh.angle) > 0.01f ? true : false; // 0.01도 이하는 회전하지 않음. 

	ImageProcessor::stInfoCorner info;
	guessAndAnalyseIP(info, th, rotTh, float(IP_CORNER_THRESHOLD), bRotate);

	if (info.n_maxIndex < 0)
	{
		info.n_guessedIndex = m_nIpIndex;
	}

	setInfoCorner(info); // m_stInfoCorner 값을 쓰는 것을 명시적으로 하기 위함.
	rr = getMinRect2(th);
	if (m_nLowResolution)
		th = VerticalShrink(th.clone());

	return th.clone();
}

cv::RotatedRect ImageProcessor::getMinRect(cv::Mat bin)
{
	std::vector<cv::Point> pixels;
	// 2024-09-24. jg kim. 저해상도 image의 경우 세로로 늘려줘야 함. 그렇지 않을 경우 각도 계산이 잘못됨.
	// 2024-09-24. jg kim. 함수가 투명하지 않아 저해상도 image는 미리 세로로 늘려서 입력받도록 방침을 바꿈
	cv::findNonZero(bin, pixels);
	cv::RotatedRect minRect = cv::minAreaRect(pixels);// contour에서 회전각, 회전된 Rect 정보 계산 // 2024-03-30. jg kim. 이슈 #17906 에 대흥하기 위해 변경
	cv::Rect rt = getBoundingRect(bin);

	double ro_angle = minRect.angle;
	float ratio = 0.05f;
	float ratioW = float(rt.width) / float(minRect.size.width);
	float ratioH = float(rt.height) / float(minRect.size.height);
	// 2024-09-24. jg kim. 1 - ratio보다 작은 경우는 제외
	if (minRect.size.width >= minRect.size.height && ratioW > (1 + ratio) ||  ratioH > (1 + ratio))
	{
		ro_angle += 90;
		minRect.angle = ro_angle;
		cv::Size2f sz = minRect.size;
		minRect.size = cv::Size2f(sz.height, sz.width);
	}
	double max_rot = 20; // 6 -> 20
	ro_angle = ro_angle > max_rot ? 0 : (ro_angle < -max_rot ? 0 : ro_angle);
	minRect.angle = ro_angle;

	return minRect;
}

cv::RotatedRect ImageProcessor::getMinRect2(cv::Mat bin)
{
	std::vector<cv::Point> pixels;
	// 2024-09-24. jg kim. 저해상도 image의 경우 세로로 늘려줘야 함. 그렇지 않을 경우 각도 계산이 잘못됨.
	// 2024-09-24. jg kim. 함수가 투명하지 않아 저해상도 image는 미리 세로로 늘려서 입력받도록 방침을 바꿈
	cv::findNonZero(bin, pixels);
	cv::RotatedRect minRect = cv::minAreaRect(pixels);// contour에서 회전각, 회전된 Rect 정보 계산 // 2024-03-30. jg kim. 이슈 #17906 에 대흥하기 위해 변경
	
	cv::Mat rotBin = GetRotatedImage(bin, minRect);
	cv::Rect rt = getBoundingRect(rotBin);

	if (rt.width > rt.height && abs(minRect.angle) > 60)  // 2025-05-29. jg kim. 엑스레이가 온전히 조사되지 않은 경우를 걸러내기 위해 조건 추가
	{
		minRect.angle -= 90;
		minRect.size.width = rt.height;
		minRect.size.height = rt.width;
	}

	return minRect;
}


cv::RotatedRect ImageProcessor::getMinRect_test(cv::Mat bin)
{
	cv::Mat thuchar; bin.convertTo(thuchar, CV_8U);
	cv::Mat canny;
	int threshold1 = 65;
	cv::Canny(thuchar, canny, threshold1, threshold1 / 2);
	std::vector<Vec4i> lines;

	HoughLinesP(canny, lines, 1, CV_PI / 180, /*threshold*/50, /*min length*/100, /*max line gap*/100); // 파라미터 조정 필요

	cv::Mat result_hough;
	cv::cvtColor(thuchar, result_hough, COLOR_GRAY2BGR);
	if (!lines.empty()) {
		for (const auto& line_4i : lines) {
			cv::line(result_hough, Point(line_4i[0], line_4i[1]), Point(line_4i[2], line_4i[3]), Scalar(0, 0, 255), 2);
		}
	}

	cv::Mat th_rot = cv::Mat::zeros(bin.size(), bin.type());
	cv::Rect th_rect = getBoundingRect2(bin);
	double max_length = 0;
	double dominant_slope = 0;

	for (const auto& line_4i : lines) {
		Point p1(line_4i[0], line_4i[1]);
		Point p2(line_4i[2], line_4i[3]);
		double length = distance(p1, p2);

		if (length > max_length) {
			max_length = length;
			dominant_slope = calculate_slope(p1, p2);
		}
	}
	double rotation_angle_radians = atan(dominant_slope);

	std::vector<cv::Point2f> contour;
	if (!lines.empty()) {
		for (const auto& line_4i : lines) {
			for (int i = 0; i < 2; i++)
				contour.push_back(cv::Point2f(line_4i[2 * i], line_4i[2 * i + 1]));
		}
	}

	double cx = 0;
	double cy = 0;

	for (const auto& p : contour) {
		cx += p.x;
		cy += p.y;
	}
	cx /= double(contour.size());
	cy /= double(contour.size());

	cv::RotatedRect rr_thres;
	rr_thres.angle = radians_to_degrees(rotation_angle_radians);
	rr_thres.center = cv::Point2f(cx, cy);
	th_rot = GetRotatedImage(bin, rr_thres);
	th_rect = getBoundingRect2(th_rot);

	if (th_rect.width > th_rect.height )
	{
		rr_thres.angle -= 90;
	}
	th_rot = cv::Mat::zeros(bin.size(), bin.type());
	th_rot = GetRotatedImage(bin, rr_thres);
	th_rect = getBoundingRect2(th_rot);
	rr_thres.size = cv::Size2f(th_rect.width, th_rect.height);

	return rr_thres;
}
