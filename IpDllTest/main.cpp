#pragma once
#include "pch.h"
#include <atltime.h>
#include <atlconv.h>
#include "opencv2/opencv.hpp"
#include "../include/inlineFunctions.h"
#include "../gnImageProcessing/inline_imgProc.h"
#include "../gnImageProcessing/PostProcessingInterface.h"
#include "../gnScanInformation/HardwareInfo.h"
#include "../predix_sdk/common_type.h"
#include "../predix_sdk/CiniFile.h"
#include "../predix_sdk/global_data.h"
#include "../predix_sdk/postprocess.h"
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
#include "../include/Logger.h"


#pragma warning(disable:4305)

#define RawImageWidth 1536
#define RawImageHeight 2360


CString ExtractOnlyFileName(CString FileName)
{
	CString Path;
	int i = -1;
	i = FileName.ReverseFind('\\');
	if (i == -1)
		return CString("");
	if (i >= 0)
		Path = FileName.Right(FileName.GetLength() - i - 1);
	return Path;
}

bool ends_with(char const *str, char const *suffix)
{
	size_t l0 = strlen(str);
	size_t l1 = strlen(suffix);

	if (l0 < l1)
		return false;

	return strcmp(str + l0 - l1, suffix) == 0;
}

cv::Mat loadRawImage(char* name, int width, int height);

IMG_PROCESS_PARAM getDefaultImageProcessingParameters();

// 2024-04-22. jg kim. Calibration coefficient의 유효성 검사
en_Error_Cal_Coefficients CheckValidCoefficients(unsigned short* pMeans, float* pCoefficients)
{
	CHardwareInfo hwi;
	return hwi._CheckValidCoefficients(pMeans, pCoefficients) ? Cal_ValidCoefficients : Cal_DefaultCoefficients;
}

// 2024-04-22. jg kim. PSP 투입구가 열려 있는지 검사
en_Error_Door CheckDoorClosed(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes)
{
	CHardwareInfo hwi;
	return hwi._CheckDoorClosed(pRawImage, nWidth, nHeight, nLowRes) ? HW_DoorClosed : HW_DoorNotClosed;
}


// 2024-04-22. jg kim. 장비에서 보내오는 스캔모드 정보의 유효성을 검사
en_Error_ScanMode CheckValidIpIndex(int & nIpIdx)
{
	CHardwareInfo hwi;
	return hwi._CheckValidIpIndex(nIpIdx) ? HW_ValidScanMode : HW_InvalidScanMode;
}

cv::Mat loadRawImage(char * name, int width, int height)
{
	cv::Mat img = cv::Mat::zeros(cv::Size(width, height), CV_16U);
	FILE *fh;
	fopen_s(&fh, name, "rb");
	fread((unsigned short*)img.ptr(), sizeof(unsigned short), width*height, fh);
	fclose(fh);
	return img.clone();
}


IMG_PROCESS_PARAM getDefaultImageProcessingParameters()
{
	IMG_PROCESS_PARAM param;
	param.mp.fScaling = 1.f;
	param.mp.fExponential = 0.7f;
	param.mp.fMaxDifference = 120.f;
	param.mp.fc0 = 1.f;
	param.mp.fEdgeEnhancement = 2.f;
	param.mp.nEdgeLayerNumb = 3;
	param.mp.fLatitudeReduction = 2.f;
	param.mp.nLatitudeLayerNumb = 2;
	param.nHistogramLeftoffset = 100;
	param.nHistogramRightoffset = 100;
	param.nHistogramTopoffset = 100;
	param.nHistogramBottomoffset = 100;
	param.nMetalMaskingThreshold = 0;
	param.nMetalMaskingDilationCount = 0;
	param.fPyramidW2Offset = 16.f;
	param.nPyramidLayer = 6;
	param.fGamma = 1.55f;
	param.nPreBlur = Median3x3;
	param.nPostBlur = noneBlur;
	param.bInvert = true;
	param.cImageFilterInfo.bApply = true;
	param.cImageFilterInfo.nAmount = 200;
	param.cImageFilterInfo.dRadius = 1.5f;
	param.cImageFilterInfo.nThreshold = 0;
	param.bUseBLFilter1 = true;
	param.fSigmaD1 = 2.3f;
	param.fSigmaR1 = 1000.f;
	param.bUseBLFilter2 = true;
	param.fSigmaD2 = 1.f;
	param.fSigmaR2 = 300.f;
	return param;
}
void load_ini_datas(CINIFile* piniFile)
{
	OutputDebugString(_T("load_ini_datas() entered.\n"));

	if (piniFile == NULL)
	{
		printf("NULL\n");
		return;
	}

	auto pGlobalData = GetPdxData();
	ASSERT(pGlobalData);

	CString value;

	pGlobalData->g_ipostprocess = PP_DEFAULT;

	//! kiwa72(2023.04.23 11h) - 기존 접속한 장비의 이름과 IP주소 읽기
	value = piniFile->GetValueL(_T("NtoN_INFO"), _T("DEVICE_NAME"));
	if (value == _T("none") || value == _T(""))
	{
		CString temp = _T("none");
		piniFile->SetValue(_T("NtoN_INFO"), _T("DEVICE_NAME"), temp);
		pGlobalData->g_sConnectDeviceName = temp;
	}
	else
	{
		pGlobalData->g_sConnectDeviceName = value;
	}

	value = piniFile->GetValueL(_T("NtoN_INFO"), _T("DEVICE_ADDR"));
	if (value == _T("none") || value == _T("0.0.0.0"))
	{
		CString temp = _T("0.0.0.0");
		piniFile->SetValue(_T("NtoN_INFO"), _T("DEVICE_ADDR"), temp);
		pGlobalData->g_sConnectDeviceAddr = temp;
	}
	else
	{
		pGlobalData->g_sConnectDeviceAddr = value;
	}
	//......

	value = piniFile->GetValueL(_T("POSTPROCESSF"), _T("process_type"));
	if (value == _T("none"))
	{
		CString temp;
		temp.Format(_T("%d"), PPF_EDGE_ENHANCE_DEFAULT); // 2024-02-28. jg kim. default 값 변경
		piniFile->SetValue(_T("POSTPROCESSF"), _T("process_type"), temp);
		pGlobalData->g_ipp_type = PPF_EDGE_ENHANCE_DEFAULT; // 2024-02-28. jg kim. default 값 변경
	}
	else
	{
		try
		{
			long lvalue = _ttoi(value);
			if (lvalue >= 0 || lvalue<= 6)
				pGlobalData->g_ipp_type = lvalue;
			else
				pGlobalData->g_ipp_type = 0; // 2024-02-28. jg kim. default 값 변경
		}
		catch (...)
		{
			pGlobalData->g_ipp_type = PPF_EDGE_ENHANCE_DEFAULT; // 2024-02-28. jg kim. default 값 변경
		}
	}
	//<<pp_type

	//>>calibration

	value = piniFile->GetValueL(_T("calibration"), _T("use_ini"));
	if (value == _T("none"))
	{
		CString temp;
		temp.Format(_T("%d"), 2); // 2024-02-28. jg kim. default 값 변경 (장비의 calibration coefficient 사용)
		piniFile->SetValue(_T("calibration"), _T("use_ini"), temp);
		pGlobalData->g_use_ini_calibration_coefs = 0;
	}
	else
	{
		try
		{
			long lvalue = _ttoi(value);
			pGlobalData->g_use_ini_calibration_coefs = lvalue;
		}
		catch (...)
		{
			pGlobalData->g_use_ini_calibration_coefs = 0;
		}
	}

	if (pGlobalData->g_use_ini_calibration_coefs == 1)
	{
		wchar_t parambuf[32] = { 0, };
		for (int i = 0; i < MAX_CALI_IMAGE_COUNT; i++)// 2023-11-22. jg kim.  predix.ini에서 Genoray calibration parameter읽음. 기존의 코드 대체
		{
			wsprintf(parambuf, L"mean%02d", i);

			value = piniFile->GetValueL(L"calibration", parambuf);
			if (value == "none")
			{
				pGlobalData->g_NewCalibrationData.data1[i] = 0;
				piniFile->SetValue(L"calibration", parambuf, L"0");
			}
			else
			{
				try
				{
					pGlobalData->g_NewCalibrationData.data1[i] = (unsigned short)_wtoi(value);
				}
				catch (...)
				{
					pGlobalData->g_NewCalibrationData.data1[i] = 0;
				}
			}
		}

		for (int i = 0; i < (DEGREE_COUNT + 1) * MAX_CALI_IMAGE_COUNT; i++)
		{
			wsprintf(parambuf, L"coef%02d", i);
			value = piniFile->GetValueL(L"calibration", parambuf);
			if (value == "none")
			{
				pGlobalData->g_NewCalibrationData.data2[i] = 0.;
				piniFile->SetValue(L"calibration", parambuf, L"0");
			}
			else
			{
				//MYPLOGMW("coef%02d  %s\n", i, value )
				try
				{
					pGlobalData->g_NewCalibrationData.data2[i] = (float)_wtof(value);
					//MYPLOGMA("ok coef%02d  %f\n", i, pGlobalData->g_fcoefs[i]);
				}
				catch (...)
				{
					pGlobalData->g_NewCalibrationData.data2[i] = 0;
					//MYPLOGMA("error coef%02d  %f\n", i, pGlobalData->g_fcoefs[i]);
				}
			}
		}
	}
	else
	{
		memset(pGlobalData->g_NewCalibrationData.data1, 0, sizeof(unsigned short) * MAX_CALI_IMAGE_COUNT);
		memset(pGlobalData->g_NewCalibrationData.data2, 0, sizeof(float) * (DEGREE_COUNT + 1) * MAX_CALI_IMAGE_COUNT);
	}

	/* post processing parameter*/
	// 2024-08-15. jg kim. 기존 Natural, Enhance모드에서 HARD, SORT, HARD_SR, SOFT_SR 이름 변경 및 저해상도용 파라미터 추가
	// 2024-08-18. jg kim. 김효근 차장, 부민철 과장의 파라미터 튜닝 결과 반영
	// 2024-08-19. jg kim. key 값이 ini 파일에 없을 경우 default값을 반환하도록 수정
	CString processType;
	processType = L"OX_GENXCR_IPPI_HARD";
	value = piniFile->GetValueL(processType, _T("DESCRIPTION"));  // 2024-02-28. jg kim. default 영상처리 파라미터 생성
	if (value == _T("none"))
	{
		piniFile->SetValue(processType, _T("DESCRIPTION"), _T("2"));
		piniFile->SetValue(processType, _T("IMAGE_TYPE"), _T("0"));
		piniFile->SetValue(processType, _T("a"), _T("1"));
		piniFile->SetValue(processType, _T("p"), _T("0.7"));
		piniFile->SetValue(processType, _T("M"), _T("120"));
		piniFile->SetValue(processType, _T("C0"), _T("1"));
		piniFile->SetValue(processType, _T("fe"), _T("2"));
		piniFile->SetValue(processType, _T("ne"), _T("3"));
		piniFile->SetValue(processType, _T("fl"), _T("2"));
		piniFile->SetValue(processType, _T("nl"), _T("2"));
		piniFile->SetValue(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		piniFile->SetValue(processType, _T("Pyramid_W2_Offset"), _T("10"));
		piniFile->SetValue(processType, _T("Pyramid_Layer"), _T("4"));
		piniFile->SetValue(processType, _T("G"), _T("1.55"));
		piniFile->SetValue(processType, _T("preblur"), _T("Median3x3"));
		piniFile->SetValue(processType, _T("postblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("invert"), _T("1"));
		piniFile->SetValue(processType, _T("APPLY_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("FILTER_AMOUNT"), _T("200"));
		piniFile->SetValue(processType, _T("FILTER_RADIUS"), _T("1.5"));
		piniFile->SetValue(processType, _T("FILTER_THRESHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("APPLY_FILTER3"), _T("1"));
		piniFile->SetValue(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		piniFile->SetValue(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER1"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_D"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
	}
	else
	{
		value = piniFile->GetValueL(processType, _T("a"), _T("1"));
		pGlobalData->g_mceIppi_HARD_HR.mp.fScaling = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("p"), _T("0.7"));
		pGlobalData->g_mceIppi_HARD_HR.mp.fExponential = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("M"), _T("120"));
		pGlobalData->g_mceIppi_HARD_HR.mp.fMaxDifference = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("C0"), _T("1"));
		pGlobalData->g_mceIppi_HARD_HR.mp.fc0 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("fe"), _T("2"));
		pGlobalData->g_mceIppi_HARD_HR.mp.fEdgeEnhancement = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("ne"), _T("3"));
		pGlobalData->g_mceIppi_HARD_HR.mp.nEdgeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("fl"), _T("2"));
		pGlobalData->g_mceIppi_HARD_HR.mp.fLatitudeReduction = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("nl"), _T("2"));
		pGlobalData->g_mceIppi_HARD_HR.mp.nLatitudeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_HARD_HR.nHistogramLeftoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_HARD_HR.nHistogramRightoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_HARD_HR.nHistogramTopoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_HARD_HR.nHistogramBottomoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		pGlobalData->g_mceIppi_HARD_HR.nMetalMaskingThreshold = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		pGlobalData->g_mceIppi_HARD_HR.nMetalMaskingDilationCount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("Pyramid_W2_Offset"), _T("10"));
		pGlobalData->g_mceIppi_HARD_HR.fPyramidW2Offset = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("Pyramid_Layer"), _T("4"));
		pGlobalData->g_mceIppi_HARD_HR.nPyramidLayer = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("G"), _T("1.55"));
		pGlobalData->g_mceIppi_HARD_HR.fGamma = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("preblur"), _T("Median3x3"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_HARD_HR.nPreBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_HARD_HR.nPreBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_HARD_HR.nPreBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_HARD_HR.nPreBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_HARD_HR.nPreBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_HARD_HR.nPreBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_HARD_HR.nPreBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("postblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_HARD_HR.nPostBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_HARD_HR.nPostBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_HARD_HR.nPostBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_HARD_HR.nPostBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_HARD_HR.nPostBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_HARD_HR.nPostBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_HARD_HR.nPostBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("invert"), _T("1"));
		pGlobalData->g_mceIppi_HARD_HR.bInvert = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("APPLY_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_HARD_HR.cImageFilterInfo.bApply = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_AMOUNT"), _T("200"));
		pGlobalData->g_mceIppi_HARD_HR.cImageFilterInfo.nAmount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_RADIUS"), _T("1.5"));
		pGlobalData->g_mceIppi_HARD_HR.cImageFilterInfo.dRadius = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("FILTER_THRESHOLD"), _T("0"));
		pGlobalData->g_mceIppi_HARD_HR.cImageFilterInfo.nThreshold = _ttoi(value);

		value = piniFile->GetValueL(processType, _T("APPLY_FILTER3"), _T("1")); // 2024-08-18. jg kim. 읽는 부분이 제대로 구현되어 있지 않아 수정
		pGlobalData->g_mceIppi_HARD_HR.bApplyCLAHE = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		pGlobalData->g_mceIppi_HARD_HR.nCLAHEBlockSize = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		pGlobalData->g_mceIppi_HARD_HR.fCLAHEClipLimit = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER1"), _T("1"));
		pGlobalData->g_mceIppi_HARD_HR.bUseBLFilter1 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_D"), _T("1"));
		pGlobalData->g_mceIppi_HARD_HR.fSigmaD1 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		pGlobalData->g_mceIppi_HARD_HR.fSigmaR1 = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_HARD_HR.bUseBLFilter2 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		pGlobalData->g_mceIppi_HARD_HR.fSigmaD2 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
		pGlobalData->g_mceIppi_HARD_HR.fSigmaR2 = float(_ttof(value));
	}

	processType = L"OX_GENXCR_IPPI_MEDIUM"; // 2025-05-30. jg kim. MEDIUM 모드 구현
	value = piniFile->GetValueL(processType, _T("DESCRIPTION"));  // 2024-02-28. jg kim. default 영상처리 파라미터 생성
	if (value == _T("none"))
	{
		piniFile->SetValue(processType, _T("DESCRIPTION"), _T("2"));
		piniFile->SetValue(processType, _T("IMAGE_TYPE"), _T("0"));
		piniFile->SetValue(processType, _T("a"), _T("1"));
		piniFile->SetValue(processType, _T("p"), _T("0.8"));
		piniFile->SetValue(processType, _T("M"), _T("120"));
		piniFile->SetValue(processType, _T("C0"), _T("1"));
		piniFile->SetValue(processType, _T("fe"), _T("2"));
		piniFile->SetValue(processType, _T("ne"), _T("3"));
		piniFile->SetValue(processType, _T("fl"), _T("2"));
		piniFile->SetValue(processType, _T("nl"), _T("2"));
		piniFile->SetValue(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		piniFile->SetValue(processType, _T("Pyramid_W2_Offset"), _T("10"));
		piniFile->SetValue(processType, _T("Pyramid_Layer"), _T("4"));
		piniFile->SetValue(processType, _T("G"), _T("1.55"));
		piniFile->SetValue(processType, _T("preblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("postblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("invert"), _T("1"));
		piniFile->SetValue(processType, _T("APPLY_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("FILTER_AMOUNT"), _T("200"));
		piniFile->SetValue(processType, _T("FILTER_RADIUS"), _T("1.5"));
		piniFile->SetValue(processType, _T("FILTER_THRESHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("APPLY_FILTER3"), _T("1"));
		piniFile->SetValue(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		piniFile->SetValue(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER1"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_D"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
	}
	else
	{
		value = piniFile->GetValueL(processType, _T("a"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_HR.mp.fScaling = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("p"), _T("0.8"));
		pGlobalData->g_mceIppi_MEDIUM_HR.mp.fExponential = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("M"), _T("120"));
		pGlobalData->g_mceIppi_MEDIUM_HR.mp.fMaxDifference = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("C0"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_HR.mp.fc0 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("fe"), _T("2"));
		pGlobalData->g_mceIppi_MEDIUM_HR.mp.fEdgeEnhancement = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("ne"), _T("3"));
		pGlobalData->g_mceIppi_MEDIUM_HR.mp.nEdgeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("fl"), _T("2"));
		pGlobalData->g_mceIppi_MEDIUM_HR.mp.fLatitudeReduction = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("nl"), _T("2"));
		pGlobalData->g_mceIppi_MEDIUM_HR.mp.nLatitudeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_MEDIUM_HR.nHistogramLeftoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_MEDIUM_HR.nHistogramRightoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_MEDIUM_HR.nHistogramTopoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_MEDIUM_HR.nHistogramBottomoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_HR.nMetalMaskingThreshold = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_HR.nMetalMaskingDilationCount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("Pyramid_W2_Offset"), _T("10"));
		pGlobalData->g_mceIppi_MEDIUM_HR.fPyramidW2Offset = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("Pyramid_Layer"), _T("4"));
		pGlobalData->g_mceIppi_MEDIUM_HR.nPyramidLayer = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("G"), _T("1.55"));
		pGlobalData->g_mceIppi_MEDIUM_HR.fGamma = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("preblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPreBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPreBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPreBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPreBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPreBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPreBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPreBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("postblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPostBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPostBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPostBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPostBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPostBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPostBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_MEDIUM_HR.nPostBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("invert"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_HR.bInvert = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("APPLY_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_HR.cImageFilterInfo.bApply = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_AMOUNT"), _T("200"));
		pGlobalData->g_mceIppi_MEDIUM_HR.cImageFilterInfo.nAmount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_RADIUS"), _T("1.5"));
		pGlobalData->g_mceIppi_MEDIUM_HR.cImageFilterInfo.dRadius = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("FILTER_THRESHOLD"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_HR.cImageFilterInfo.nThreshold = _ttoi(value);

		value = piniFile->GetValueL(processType, _T("APPLY_FILTER3"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_HR.bApplyCLAHE = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		pGlobalData->g_mceIppi_MEDIUM_HR.nCLAHEBlockSize = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		pGlobalData->g_mceIppi_MEDIUM_HR.fCLAHEClipLimit = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER1"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_HR.bUseBLFilter1 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_D"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_HR.fSigmaD1 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		pGlobalData->g_mceIppi_MEDIUM_HR.fSigmaR1 = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_HR.bUseBLFilter2 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_HR.fSigmaD2 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
		pGlobalData->g_mceIppi_MEDIUM_HR.fSigmaR2 = float(_ttof(value));
	}

	processType = L"OX_GENXCR_IPPI_SOFT";
	value = piniFile->GetValueL(processType, _T("DESCRIPTION"));
	if (value == _T("none"))
	{
		piniFile->SetValue(processType, _T("DESCRIPTION"), _T("2"));
		piniFile->SetValue(processType, _T("IMAGE_TYPE"), _T("0"));
		piniFile->SetValue(processType, _T("a"), _T("1"));
		piniFile->SetValue(processType, _T("p"), _T("0.9"));
		piniFile->SetValue(processType, _T("M"), _T("120"));
		piniFile->SetValue(processType, _T("C0"), _T("1"));
		piniFile->SetValue(processType, _T("fe"), _T("2"));
		piniFile->SetValue(processType, _T("ne"), _T("3"));
		piniFile->SetValue(processType, _T("fl"), _T("2"));
		piniFile->SetValue(processType, _T("nl"), _T("2"));
		piniFile->SetValue(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("100"));
		piniFile->SetValue(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		piniFile->SetValue(processType, _T("Pyramid_W2_Offset"), _T("10"));
		piniFile->SetValue(processType, _T("Pyramid_Layer"), _T("4"));
		piniFile->SetValue(processType, _T("G"), _T("1.55"));
		piniFile->SetValue(processType, _T("preblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("postblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("invert"), _T("1"));
		piniFile->SetValue(processType, _T("APPLY_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("FILTER_AMOUNT"), _T("200"));
		piniFile->SetValue(processType, _T("FILTER_RADIUS"), _T("1.5"));
		piniFile->SetValue(processType, _T("FILTER_THRESHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("APPLY_FILTER3"), _T("1"));
		piniFile->SetValue(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		piniFile->SetValue(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER1"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_D"), _T("0.5"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
	}
	else
	{
		value = piniFile->GetValueL(processType, _T("a"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_HR.mp.fScaling = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("p"), _T("0.9"));
		pGlobalData->g_mceIppi_SOFT_HR.mp.fExponential = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("M"), _T("120"));
		pGlobalData->g_mceIppi_SOFT_HR.mp.fMaxDifference = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("C0"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_HR.mp.fc0 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("fe"), _T("2"));
		pGlobalData->g_mceIppi_SOFT_HR.mp.fEdgeEnhancement = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("ne"), _T("3"));
		pGlobalData->g_mceIppi_SOFT_HR.mp.nEdgeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("fl"), _T("2"));
		pGlobalData->g_mceIppi_SOFT_HR.mp.fLatitudeReduction = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("nl"), _T("2"));
		pGlobalData->g_mceIppi_SOFT_HR.mp.nLatitudeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_SOFT_HR.nHistogramLeftoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_SOFT_HR.nHistogramRightoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_SOFT_HR.nHistogramTopoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("100"));
		pGlobalData->g_mceIppi_SOFT_HR.nHistogramBottomoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_HR.nMetalMaskingThreshold = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_HR.nMetalMaskingDilationCount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("Pyramid_W2_Offset"), _T("10"));
		pGlobalData->g_mceIppi_SOFT_HR.fPyramidW2Offset = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("Pyramid_Layer"), _T("4"));
		pGlobalData->g_mceIppi_SOFT_HR.nPyramidLayer = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("G"), _T("1.55"));
		pGlobalData->g_mceIppi_SOFT_HR.fGamma = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("preblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_SOFT_HR.nPreBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_SOFT_HR.nPreBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_SOFT_HR.nPreBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_SOFT_HR.nPreBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_SOFT_HR.nPreBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_SOFT_HR.nPreBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_SOFT_HR.nPreBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("postblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_SOFT_HR.nPostBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_SOFT_HR.nPostBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_SOFT_HR.nPostBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_SOFT_HR.nPostBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_SOFT_HR.nPostBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_SOFT_HR.nPostBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_SOFT_HR.nPostBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("invert"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_HR.bInvert = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("APPLY_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_HR.cImageFilterInfo.bApply = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_AMOUNT"), _T("200"));
		pGlobalData->g_mceIppi_SOFT_HR.cImageFilterInfo.nAmount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_RADIUS"), _T("1.5"));
		pGlobalData->g_mceIppi_SOFT_HR.cImageFilterInfo.dRadius = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("FILTER_THRESHOLD"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_HR.cImageFilterInfo.nThreshold = _ttoi(value);

		value = piniFile->GetValueL(processType, _T("APPLY_FILTER3"), _T("1")); // 2024-08-18. jg kim. 읽는 부분이 제대로 구현되어 있지 않아 수정
		pGlobalData->g_mceIppi_SOFT_HR.bApplyCLAHE = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		pGlobalData->g_mceIppi_SOFT_HR.nCLAHEBlockSize = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		pGlobalData->g_mceIppi_SOFT_HR.fCLAHEClipLimit = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER1"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_HR.bUseBLFilter1 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_D"), _T("0.5"));
		pGlobalData->g_mceIppi_SOFT_HR.fSigmaD1 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		pGlobalData->g_mceIppi_SOFT_HR.fSigmaR1 = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_HR.bUseBLFilter2 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_HR.fSigmaD2 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
		pGlobalData->g_mceIppi_SOFT_HR.fSigmaR2 = float(_ttof(value));
	}

	processType = L"OX_GENXCR_IPPI_HARD_SR";
	value = piniFile->GetValueL(processType, _T("DESCRIPTION"));
	if (value == _T("none"))
	{
		piniFile->SetValue(processType, _T("DESCRIPTION"), _T("2"));
		piniFile->SetValue(processType, _T("IMAGE_TYPE"), _T("0"));
		piniFile->SetValue(processType, _T("a"), _T("1"));
		piniFile->SetValue(processType, _T("p"), _T("0.7"));
		piniFile->SetValue(processType, _T("M"), _T("120"));
		piniFile->SetValue(processType, _T("C0"), _T("1"));
		piniFile->SetValue(processType, _T("fe"), _T("2"));
		piniFile->SetValue(processType, _T("ne"), _T("3"));
		piniFile->SetValue(processType, _T("fl"), _T("2"));
		piniFile->SetValue(processType, _T("nl"), _T("2"));
		piniFile->SetValue(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		piniFile->SetValue(processType, _T("Pyramid_W2_Offset"), _T("10"));
		piniFile->SetValue(processType, _T("Pyramid_Layer"), _T("3"));
		piniFile->SetValue(processType, _T("G"), _T("1.55"));
		piniFile->SetValue(processType, _T("preblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("postblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("invert"), _T("1"));
		piniFile->SetValue(processType, _T("APPLY_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("FILTER_AMOUNT"), _T("200"));
		piniFile->SetValue(processType, _T("FILTER_RADIUS"), _T("1.5"));
		piniFile->SetValue(processType, _T("FILTER_THRESHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("APPLY_FILTER3"), _T("1"));
		piniFile->SetValue(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		piniFile->SetValue(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER1"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_D"), _T("0.5"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
	}
	else
	{
		value = piniFile->GetValueL(processType, _T("a"), _T("1"));
		pGlobalData->g_mceIppi_HARD_SR.mp.fScaling = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("p"), _T("0.7"));
		pGlobalData->g_mceIppi_HARD_SR.mp.fExponential = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("M"), _T("120"));
		pGlobalData->g_mceIppi_HARD_SR.mp.fMaxDifference = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("C0"), _T("1"));
		pGlobalData->g_mceIppi_HARD_SR.mp.fc0 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("fe"), _T("2"));
		pGlobalData->g_mceIppi_HARD_SR.mp.fEdgeEnhancement = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("ne"), _T("3"));
		pGlobalData->g_mceIppi_HARD_SR.mp.nEdgeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("fl"), _T("2"));
		pGlobalData->g_mceIppi_HARD_SR.mp.fLatitudeReduction = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("nl"), _T("2"));
		pGlobalData->g_mceIppi_HARD_SR.mp.nLatitudeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_HARD_SR.nHistogramLeftoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_HARD_SR.nHistogramRightoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_HARD_SR.nHistogramTopoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_HARD_SR.nHistogramBottomoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		pGlobalData->g_mceIppi_HARD_SR.nMetalMaskingThreshold = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		pGlobalData->g_mceIppi_HARD_SR.nMetalMaskingDilationCount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("Pyramid_W2_Offset"), _T("10"));
		pGlobalData->g_mceIppi_HARD_SR.fPyramidW2Offset = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("Pyramid_Layer"), _T("3"));
		pGlobalData->g_mceIppi_HARD_SR.nPyramidLayer = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("G"), _T("1.55"));
		pGlobalData->g_mceIppi_HARD_SR.fGamma = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("preblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_HARD_SR.nPreBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_HARD_SR.nPreBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_HARD_SR.nPreBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_HARD_SR.nPreBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_HARD_SR.nPreBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_HARD_SR.nPreBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_HARD_SR.nPreBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("postblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_HARD_SR.nPostBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_HARD_SR.nPostBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_HARD_SR.nPostBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_HARD_SR.nPostBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_HARD_SR.nPostBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_HARD_SR.nPostBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_HARD_SR.nPostBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("invert"), _T("1"));
		pGlobalData->g_mceIppi_HARD_SR.bInvert = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("APPLY_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_HARD_SR.cImageFilterInfo.bApply = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_AMOUNT"), _T("200"));
		pGlobalData->g_mceIppi_HARD_SR.cImageFilterInfo.nAmount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_RADIUS"), _T("1.5"));
		pGlobalData->g_mceIppi_HARD_SR.cImageFilterInfo.dRadius = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("FILTER_THRESHOLD"), _T("0"));
		pGlobalData->g_mceIppi_HARD_SR.cImageFilterInfo.nThreshold = _ttoi(value);

		value = piniFile->GetValueL(processType, _T("APPLY_FILTER3"), _T("1")); // 2024-08-18. jg kim. 읽는 부분이 제대로 구현되어 있지 않아 수정
		pGlobalData->g_mceIppi_HARD_SR.bApplyCLAHE = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		pGlobalData->g_mceIppi_HARD_SR.nCLAHEBlockSize = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		pGlobalData->g_mceIppi_HARD_SR.fCLAHEClipLimit = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER1"), _T("1"));
		pGlobalData->g_mceIppi_HARD_SR.bUseBLFilter1 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_D"), _T("0.5"));
		pGlobalData->g_mceIppi_HARD_SR.fSigmaD1 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		pGlobalData->g_mceIppi_HARD_SR.fSigmaR1 = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_HARD_SR.bUseBLFilter2 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		pGlobalData->g_mceIppi_HARD_SR.fSigmaD2 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
		pGlobalData->g_mceIppi_HARD_SR.fSigmaR2 = float(_ttof(value));
	}

	processType = L"OX_GENXCR_IPPI_MEDIUM_SR"; // 2025-05-30. jg kim. MEDIUM 모드 구현
	value = piniFile->GetValueL(processType, _T("DESCRIPTION"));
	if (value == _T("none"))
	{
		piniFile->SetValue(processType, _T("DESCRIPTION"), _T("2"));
		piniFile->SetValue(processType, _T("IMAGE_TYPE"), _T("0"));
		piniFile->SetValue(processType, _T("a"), _T("1"));
		piniFile->SetValue(processType, _T("p"), _T("0.8"));
		piniFile->SetValue(processType, _T("M"), _T("120"));
		piniFile->SetValue(processType, _T("C0"), _T("1"));
		piniFile->SetValue(processType, _T("fe"), _T("2"));
		piniFile->SetValue(processType, _T("ne"), _T("3"));
		piniFile->SetValue(processType, _T("fl"), _T("2"));
		piniFile->SetValue(processType, _T("nl"), _T("2"));
		piniFile->SetValue(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		piniFile->SetValue(processType, _T("Pyramid_W2_Offset"), _T("10"));
		piniFile->SetValue(processType, _T("Pyramid_Layer"), _T("3"));
		piniFile->SetValue(processType, _T("G"), _T("1.55"));
		piniFile->SetValue(processType, _T("preblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("postblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("invert"), _T("1"));
		piniFile->SetValue(processType, _T("APPLY_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("FILTER_AMOUNT"), _T("200"));
		piniFile->SetValue(processType, _T("FILTER_RADIUS"), _T("1.5"));
		piniFile->SetValue(processType, _T("FILTER_THRESHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("APPLY_FILTER3"), _T("1"));
		piniFile->SetValue(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		piniFile->SetValue(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER1"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_D"), _T("0.5"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
	}
	else
	{
		value = piniFile->GetValueL(processType, _T("a"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_SR.mp.fScaling = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("p"), _T("0.8"));
		pGlobalData->g_mceIppi_MEDIUM_SR.mp.fExponential = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("M"), _T("120"));
		pGlobalData->g_mceIppi_MEDIUM_SR.mp.fMaxDifference = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("C0"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_SR.mp.fc0 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("fe"), _T("2"));
		pGlobalData->g_mceIppi_MEDIUM_SR.mp.fEdgeEnhancement = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("ne"), _T("3"));
		pGlobalData->g_mceIppi_MEDIUM_SR.mp.nEdgeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("fl"), _T("2"));
		pGlobalData->g_mceIppi_MEDIUM_SR.mp.fLatitudeReduction = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("nl"), _T("2"));
		pGlobalData->g_mceIppi_MEDIUM_SR.mp.nLatitudeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_MEDIUM_SR.nHistogramLeftoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_MEDIUM_SR.nHistogramRightoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_MEDIUM_SR.nHistogramTopoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_MEDIUM_SR.nHistogramBottomoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_SR.nMetalMaskingThreshold = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_SR.nMetalMaskingDilationCount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("Pyramid_W2_Offset"), _T("10"));
		pGlobalData->g_mceIppi_MEDIUM_SR.fPyramidW2Offset = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("Pyramid_Layer"), _T("3"));
		pGlobalData->g_mceIppi_MEDIUM_SR.nPyramidLayer = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("G"), _T("1.55"));
		pGlobalData->g_mceIppi_MEDIUM_SR.fGamma = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("preblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPreBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPreBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPreBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPreBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPreBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPreBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPreBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("postblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPostBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPostBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPostBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPostBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPostBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPostBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_MEDIUM_SR.nPostBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("invert"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_SR.bInvert = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("APPLY_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_SR.cImageFilterInfo.bApply = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_AMOUNT"), _T("200"));
		pGlobalData->g_mceIppi_MEDIUM_SR.cImageFilterInfo.nAmount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_RADIUS"), _T("1.5"));
		pGlobalData->g_mceIppi_MEDIUM_SR.cImageFilterInfo.dRadius = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("FILTER_THRESHOLD"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_SR.cImageFilterInfo.nThreshold = _ttoi(value);

		value = piniFile->GetValueL(processType, _T("APPLY_FILTER3"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_SR.bApplyCLAHE = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		pGlobalData->g_mceIppi_MEDIUM_SR.nCLAHEBlockSize = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		pGlobalData->g_mceIppi_MEDIUM_SR.fCLAHEClipLimit = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER1"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_SR.bUseBLFilter1 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_D"), _T("0.5"));
		pGlobalData->g_mceIppi_MEDIUM_SR.fSigmaD1 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		pGlobalData->g_mceIppi_MEDIUM_SR.fSigmaR1 = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_MEDIUM_SR.bUseBLFilter2 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		pGlobalData->g_mceIppi_MEDIUM_SR.fSigmaD2 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
		pGlobalData->g_mceIppi_MEDIUM_SR.fSigmaR2 = float(_ttof(value));
	}

	processType = L"OX_GENXCR_IPPI_SOFT_SR";
	value = piniFile->GetValueL(processType, _T("DESCRIPTION"));
	if (value == _T("none"))
	{
		piniFile->SetValue(processType, _T("DESCRIPTION"), _T("2"));
		piniFile->SetValue(processType, _T("IMAGE_TYPE"), _T("0"));
		piniFile->SetValue(processType, _T("a"), _T("1"));
		piniFile->SetValue(processType, _T("p"), _T("0.9"));
		piniFile->SetValue(processType, _T("M"), _T("120"));
		piniFile->SetValue(processType, _T("C0"), _T("1"));
		piniFile->SetValue(processType, _T("fe"), _T("2"));
		piniFile->SetValue(processType, _T("ne"), _T("3"));
		piniFile->SetValue(processType, _T("fl"), _T("2"));
		piniFile->SetValue(processType, _T("nl"), _T("2"));
		piniFile->SetValue(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("50"));
		piniFile->SetValue(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		piniFile->SetValue(processType, _T("Pyramid_W2_Offset"), _T("10"));
		piniFile->SetValue(processType, _T("Pyramid_Layer"), _T("3"));
		piniFile->SetValue(processType, _T("G"), _T("1.55"));
		piniFile->SetValue(processType, _T("preblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("postblur"), _T("noneBlur"));
		piniFile->SetValue(processType, _T("invert"), _T("1"));
		piniFile->SetValue(processType, _T("APPLY_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("FILTER_AMOUNT"), _T("200"));
		piniFile->SetValue(processType, _T("FILTER_RADIUS"), _T("1.5"));
		piniFile->SetValue(processType, _T("FILTER_THRESHOLD"), _T("0"));
		piniFile->SetValue(processType, _T("APPLY_FILTER3"), _T("1"));
		piniFile->SetValue(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		piniFile->SetValue(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER1"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_D"), _T("0.5"));
		piniFile->SetValue(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		piniFile->SetValue(processType, _T("USE_BL_FILTER2"), _T("0"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		piniFile->SetValue(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
	}
	else
	{
		value = piniFile->GetValueL(processType, _T("a"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_SR.mp.fScaling = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("p"), _T("0.9"));
		pGlobalData->g_mceIppi_SOFT_SR.mp.fExponential = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("M"), _T("120"));
		pGlobalData->g_mceIppi_SOFT_SR.mp.fMaxDifference = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("C0"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_SR.mp.fc0 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("fe"), _T("2"));
		pGlobalData->g_mceIppi_SOFT_SR.mp.fEdgeEnhancement = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("ne"), _T("3"));
		pGlobalData->g_mceIppi_SOFT_SR.mp.nEdgeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("fl"), _T("2"));
		pGlobalData->g_mceIppi_SOFT_SR.mp.fLatitudeReduction = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("nl"), _T("2"));
		pGlobalData->g_mceIppi_SOFT_SR.mp.nLatitudeLayerNumb = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_LEFT_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_SOFT_SR.nHistogramLeftoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_RIGHT_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_SOFT_SR.nHistogramRightoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_TOP_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_SOFT_SR.nHistogramTopoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("HISTOGRAM_BOTTOM_OFFSET"), _T("50"));
		pGlobalData->g_mceIppi_SOFT_SR.nHistogramBottomoffset = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_THREASHOLD"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_SR.nMetalMaskingThreshold = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("METAL_MASKING_DILATION_COUNT"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_SR.nMetalMaskingDilationCount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("Pyramid_W2_Offset"), _T("10"));
		pGlobalData->g_mceIppi_SOFT_SR.fPyramidW2Offset = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("Pyramid_Layer"), _T("3"));
		pGlobalData->g_mceIppi_SOFT_SR.nPyramidLayer = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("G"), _T("1.55"));
		pGlobalData->g_mceIppi_SOFT_SR.fGamma = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("preblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_SOFT_SR.nPreBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_SOFT_SR.nPreBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_SOFT_SR.nPreBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_SOFT_SR.nPreBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_SOFT_SR.nPreBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_SOFT_SR.nPreBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_SOFT_SR.nPreBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("postblur"), _T("noneBlur"));
		if (!value.CompareNoCase(_T("NONE")))
			pGlobalData->g_mceIppi_SOFT_SR.nPostBlur = noneBlur;
		else if (!value.CompareNoCase(_T("Median3x3")))
			pGlobalData->g_mceIppi_SOFT_SR.nPostBlur = Median3x3;
		else if (!value.CompareNoCase(_T("Median5x5")))
			pGlobalData->g_mceIppi_SOFT_SR.nPostBlur = Median5x5;
		else if (!value.CompareNoCase(_T("Gauss3x3")))
			pGlobalData->g_mceIppi_SOFT_SR.nPostBlur = Gauss3x3;
		else if (!value.CompareNoCase(_T("Gauss5x5")))
			pGlobalData->g_mceIppi_SOFT_SR.nPostBlur = Gauss5x5;
		else if (!value.CompareNoCase(_T("Wiener3x3")))
			pGlobalData->g_mceIppi_SOFT_SR.nPostBlur = Wiener3x3;
		else if (!value.CompareNoCase(_T("Wiener5x5")))
			pGlobalData->g_mceIppi_SOFT_SR.nPostBlur = Wiener5x5;

		value = piniFile->GetValueL(processType, _T("invert"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_SR.bInvert = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("APPLY_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_SR.cImageFilterInfo.bApply = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_AMOUNT"), _T("200"));
		pGlobalData->g_mceIppi_SOFT_SR.cImageFilterInfo.nAmount = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("FILTER_RADIUS"), _T("1.5"));
		pGlobalData->g_mceIppi_SOFT_SR.cImageFilterInfo.dRadius = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("FILTER_THRESHOLD"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_SR.cImageFilterInfo.nThreshold = _ttoi(value);

		value = piniFile->GetValueL(processType, _T("APPLY_FILTER3"), _T("1")); // 2024-08-18. jg kim. 읽는 부분이 제대로 구현되어 있지 않아 수정
		pGlobalData->g_mceIppi_SOFT_SR.bApplyCLAHE = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_BlockSize"), _T("512"));
		pGlobalData->g_mceIppi_SOFT_SR.nCLAHEBlockSize = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("CL_FILTER_ClipLimit"), _T("1.2"));
		pGlobalData->g_mceIppi_SOFT_SR.fCLAHEClipLimit = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER1"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_SR.bUseBLFilter1 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_D"), _T("0.5"));
		pGlobalData->g_mceIppi_SOFT_SR.fSigmaD1 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER1_SIGMA_R"), _T("1000"));
		pGlobalData->g_mceIppi_SOFT_SR.fSigmaR1 = float(_ttof(value));

		value = piniFile->GetValueL(processType, _T("USE_BL_FILTER2"), _T("0"));
		pGlobalData->g_mceIppi_SOFT_SR.bUseBLFilter2 = _ttoi(value);
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_D"), _T("1"));
		pGlobalData->g_mceIppi_SOFT_SR.fSigmaD2 = float(_ttof(value));
		value = piniFile->GetValueL(processType, _T("BL_FILTER2_SIGMA_R"), _T("300"));
		pGlobalData->g_mceIppi_SOFT_HR.fSigmaR2 = float(_ttof(value));
	}



	/* post processing parameter*/

	//<<
	//>>blur_radius
	pGlobalData->g_iblur_radius = PPF_BLUR_RADIUS_DEFAULT;
	//<<blur_radius
	//>>edge_enhance
	pGlobalData->g_iedge_enhance = PPF_EDGE_ENHANCE_DEFAULT;//20211215
	//<<edge_enhance

	//>>brightness_offset
	pGlobalData->g_ibrightness_offset = PPF_BRIGHTNESS_OFFSET_DEFAULT;
	//<<brightness_offset

	//>>save_image_step
	value = piniFile->GetValueL(_T("POSTPROCESSF"), _T("save_image_step"));
	if (value == _T("none"))
	{
		CString temp;
		temp.Format(_T("%d"), 0);
		piniFile->SetValue(_T("POSTPROCESSF"), _T("save_image_step"), temp);
	}
	else
	{
		switch (_ttoi(value))
		{
		case 0:
			pGlobalData->g_save_image_step = SaveImageNone;
			break;
		case 1:
			pGlobalData->g_save_image_step = SaveImageCorrectionStep;
			break;
		case 2:
			pGlobalData->g_save_image_step = SaveImageProcessingStep;
			break;
		case 3:
			pGlobalData->g_save_image_step = SaveBothProcessingStep;
			break;
		default:
			pGlobalData->g_save_image_step = SaveImageNone;
			break;
		}
	}
	//>>save_image_step

#ifdef DEVELTEST
	OutputDebugString(_T("load_ini_datas() leave.\n"));
#endif
}

//#define debugging
int IpDllTest(int argc, char** argv);


int main(int argc, char** argv)
{
	IpDllTest(argc, argv);
	system("pause");
	return 0;
}

int IpDllTest(int argc, char** argv)
{
	const char* LOG_FILE_NAME = "ImageProcessing.log";

#ifdef debugging
	argc = 2;
#endif // debugging

	if (argc > 1)
	{
		global_data::InitInstance();
		TCHAR szspecialPath[1024] = { 0 };
		TCHAR path[1024];
		_stprintf_s(path, _T("%s\\"), (LPCTSTR)GetCurrentModPath(false));
		_stprintf_s(szspecialPath, _T("%spredix.ini"),path);

		auto pPdxData = GetPdxData();
		ASSERT(pPdxData->g_piniFile == nullptr);
		pPdxData->g_piniFile = new CINIFile(szspecialPath);
		load_ini_datas(pPdxData->g_piniFile);
			   
		printf("use ini = %d. save image step = %d. postprocess = %d\n", pPdxData->g_use_ini_calibration_coefs, pPdxData->g_save_image_step, pPdxData->g_ipostprocess);
		enum_save_image_step imgStep = pPdxData->g_save_image_step;
		char buf[1024];
		for (int i = 1; i < argc; i++)
		{
			memset(buf, 0, 1024);
			char *name = argv[i];
			sprintf_s(buf, "%d/%d th\t %s\n", i, argc - 1, name);
			writelog(buf,LOG_FILE_NAME);
			cv::Mat img;
			if (ends_with(name, ".raw"))
				img = loadRawImage(name, RawImageWidth, RawImageHeight);
			else if (ends_with(name, ".tif"))
				img = cv::imread(name, cv::IMREAD_UNCHANGED);
			memcpy(pPdxData->map_raw.data(), (unsigned short*)img.ptr(), sizeof(unsigned short) * img.cols * img.rows);

			// 2026-03-04. jg kim. 한글 파일명 처리를 위해 MultiByteToWideChar 사용
			wchar_t wszFullPath[MAX_PATH] = {0};
			MultiByteToWideChar(CP_ACP, 0, name, -1, wszFullPath, MAX_PATH);
			CString strFileName = ExtractOnlyFileName(CString(wszFullPath));
			g_strCurrentImageFileName = strFileName;
			pre_postprocess_by_dll(); // 2024-07-11. jg kim. argument는 의미 없음.
			// 2024-08-18. jg kim. HR, SR모드는 영상 보정을 거쳐야 알 수 있어서 코드 실행 순서를 변경함.

			/*sprintf_s(buf, "rename PRE_7. LineCorrect.tif %2d_PRE_7. LineCorrect.tif",i);
			CString oldFile("PRE_7. LineCorrect.tif");
			CString newFile;
			newFile.Format(L"PRE_7. LineCorrect_%s.tif", strFileName);
			CFile::Rename(oldFile, newFile);*/
			switch (pPdxData->g_ipostprocess)
			{
			case 0: // image correction only
			{
				sprintf_s(buf, "Image correction only.\n"); printf("%s", buf); writelog(buf,LOG_FILE_NAME);
				sprintf_s(buf, "out_ImgCorrection_%s.tif", CStringA(strFileName).GetString());
			}
			break;
			case 1: // image correction + image processing (HARD)
			{
				sprintf_s(buf, "Image correction and image processing (HARD).\n"); printf("%s", buf); writelog(buf,LOG_FILE_NAME);
				sprintf_s(buf, "out_ImgProcessing_%s_HARD.tif", CStringA(strFileName).GetString());
			}
			break;
			case 4: // image correction + image processing (HARD_SR)
			{
				sprintf_s(buf, "Image correction and image processing (HARD_SR).\n"); printf("%s", buf); writelog(buf,LOG_FILE_NAME);
				sprintf_s(buf, "out_ImgProcessing_%s_HARD_SR.tif", CStringA(strFileName).GetString());
			}
			break;
			case 2: // image correction + image processing (MEDIUM)  // 2025-05-30. jg kim. MEDIUM 모드 구현
			{
				sprintf_s(buf, "Image correction and image processing (MEDIUM).\n"); printf("%s", buf); writelog(buf,LOG_FILE_NAME);
				sprintf_s(buf, "out_ImgProcessing_%s_MEDIUM.tif", CStringA(strFileName).GetString());
			}
			break;
			case 5: // image correction + image processing (MEDIUM_SR)
			{
				sprintf_s(buf, "Image correction and image processing (MEDIUM_SR).\n"); printf("%s", buf); writelog(buf,LOG_FILE_NAME);
				sprintf_s(buf, "out_ImgProcessing_%s_MEDIUM_SR.tif", CStringA(strFileName).GetString());
			}
			break;
			case 3: // image correction + image processing (SOFT)
			{
				sprintf_s(buf, "Image correction and image processing (SOFT).\n"); printf("%s", buf); writelog(buf,LOG_FILE_NAME);
				sprintf_s(buf, "out_ImgProcessing_%s_SOFT.tif", CStringA(strFileName).GetString());
			}
			break;
			case 6: // image correction + image processing (SOFT_SR)
			{
				sprintf_s(buf, "Image correction and image processing (SOFT_SR).\n"); printf("%s", buf); writelog(buf,LOG_FILE_NAME);
				sprintf_s(buf, "out_ImgProcessing_%s_SOFT_SR.tif", CStringA(strFileName).GetString());
			}
			break;
			}
			cv::imwrite(buf, cv::Mat(cv::Size(pPdxData->last_image_width, pPdxData->last_image_height), CV_16U, pPdxData->last_image_data));

			char buf2[1024];
			sprintf_s(buf2, "%s\n", buf);
			writelog(buf2,LOG_FILE_NAME);
			if  ((pPdxData->g_ipostprocess == 0 && pPdxData->g_en_ImgCorrection) /* 영상 보정만 하는 경우 */
				||
			 (pPdxData->g_ipostprocess && pPdxData->g_en_ImgCorrection && pPdxData->g_en_ImgProcessing)) /* 둘 다 하는 경우*/
			{
				cv::Mat out(cv::Size(pPdxData->last_image_width, pPdxData->last_image_height), CV_16U, pPdxData->last_image_data);
				cv::imwrite(buf, out); 
				char buf2[1024];
				// sprintf_s(buf, "PRE_1. tag removed.tif");
				// sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				// MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);

				sprintf_s(buf, "PRE_1. wobble corrected.tif");
				sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				
				// sprintf_s(buf, "PRE_1 edgeImg2.tif");
				// sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				// MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				
				// sprintf_s(buf, "PRE_2. bin.tif");
				// sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				// MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				
				// sprintf_s(buf, "PRE_3. th.tif");
				// sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				// MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				
				// sprintf_s(buf, "PRE_3. checked bin.tif");
				// sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				// MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);

				sprintf_s(buf, "PRE_9. ExtractIP.tif");
				sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				
				// sprintf_s(buf, "PRE_4. NedNegative.tif");
				// sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				// MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				
				// sprintf_s(buf, "PRE_4. NedPositive.tif");
				// sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				// MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				
				sprintf_s(buf, "PRE_7. LineCorrect.tif");
				sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				/*
				sprintf_s(buf, "PRE_3. checked bin.tif");
				sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(),buf);
				MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);

				sprintf_s(buf, "Pre_Blur.tif");
				sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);

				sprintf_s(buf, "Pre_Eigen.tif");
				sprintf_s(buf2, "%s_%s", CStringA(strFileName).GetString(), buf);
				MoveFileEx(CString(buf), CString(buf2), MOVEFILE_REPLACE_EXISTING);
				*/
			}
		}
	}

	return 0;

}
