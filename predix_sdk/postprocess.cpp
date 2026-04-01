// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "postprocess.h"
//#ifdef max
//#undef max
//#undef min
//#endif
#include "scanner.h"
#include "common.h"
#include "global_data.h"
#include "../gnImageProcessing/inline_imgProc.h"
#include "../gnImageProcessing/ImageProcessingInterface.h"
#include "../gnImageProcessing/HardwareInfo.h"
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
#include "../include/Logger.h"
#include <memory>
#include <atltime.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


static int pp_by_dll_result = 0;
// 2026-03-04. jg kim. 영상 파일 이름을 전역 변수로 관리
CString g_strCurrentImageFileName;

extern stcalibration cur_calibrationdata;
bool g_is_no_calibration = false;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
// 2024-12-09. jg kim. 공정툴용 영상처리와 통합
void pre_postprocess_by_dll(int nMode)
{
	const char* LOG_FILE_NAME = "Image Processing Tool.log";
	// predixcr에 최적화되지 않아 현재 그냥 리턴함
	//return;	
	// 2024-05-02. 영상 보정/처리 과정의 로그 작성을 위함
	CHardwareInfo hwi;
	char buf[1024];

	// 2026-03-04. jg kim. 영상 파일 이름 로그 기록
	if (!g_strCurrentImageFileName.IsEmpty())
	{
		char szFileName[512] = {0};
		WideCharToMultiByte(CP_ACP, 0, g_strCurrentImageFileName, -1, szFileName, sizeof(szFileName), NULL, NULL);
		sprintf_s(buf, "%s\n", szFileName);
		writelog(buf, LOG_FILE_NAME);
		writelog(buf, "ImageProcessing.log");
	}

	auto pPdxData = GetPdxData();

	int  width = geo_x;
	int  height = geo_y;

	unsigned short* source_img = new unsigned short[width * height];
	// 2024-12-09. jg kim. 공정툴용 영상처리와 통합 후 사용하지는 않지만 ProcessAPI를 사용하는데 필요함
	unsigned short* bin = new unsigned short[width * height];
	unsigned short* result_img = nullptr;		// new unsigned short[width * height];
	unsigned short* out = new unsigned short[width * height];

	int result_width = 0;
	int result_height = 0;
	memcpy(source_img, pPdxData->map_raw.data(), sizeof(unsigned short) * width * height);


	// 2024. 02. 24. jg kim. 영상에 있는 calibration 계수를 decoding 하여 사용하도록 모드 추가
	// calibration coefficients를 구하고 그 결과를 장비에 저장하는데, 장비에서 변수를 0으로 초기화 하기 때문에
	// decoding한 calibration 계수가 0 이 아닌 값을 가지면 유효하다 판단함.
	// g_use_ini_calibration_coefs 모드 2 는 영상의 calibrtion 계수를 사용하는 모드임.
	// 2024-03-28. jg kim. 장비에서 보내주는 해상도 정보 활용하기 위함	
	// 2024-05-02. jg kim. 영상 보정/처리 과정의 로그 작성을 위함
	// 2024-03-28. jg kim. 장비에서 보내주는 정보를 활용하기 위함

	// 2025-02-14. jg kim. raw image에 삽입되어 있는 영상의 스캔 일시, image list상의 index를 디코딩

	// 2024-03-28. jg kim. 장비에서 보내주는 scan mode 정보 활용하기 위함
	//2026 - 01 - 13. jg kim.장비에서 넘겨주는 반사판 영역의 Laser On / Off x 좌표 및, 세로 줄 범위(시작 / 끝 x 좌표)
	stDecodedHardwareParam hwParam;

	if (!hwi.decodeGenorayTag(source_img, width, hwParam))
	{
		sprintf_s(buf, "Bit status. b2:%d, b1:%d, b0:%d\n", hwParam.Bs[2], hwParam.Bs[1], hwParam.Bs[0]);
		writelog(buf, LOG_FILE_NAME);
	}

	if (hwParam.bValidFwVer)
	{
		if (hwParam.nFwVer[4])
			sprintf_s(buf, "fwVer = %d.%d.%d.%dB%d\n", hwParam.nFwVer[0], hwParam.nFwVer[1], hwParam.nFwVer[2], hwParam.nFwVer[3], hwParam.nFwVer[4]);
		else
			sprintf_s(buf, "fwVer = %d.%d.%d.%d\n", hwParam.nFwVer[0], hwParam.nFwVer[1], hwParam.nFwVer[2], hwParam.nFwVer[3]);
	}
	else
	{
		sprintf_s(buf, "Invalid fwVer\n");
	}

	printf("%s", buf);
	writelog(buf, LOG_FILE_NAME);

	if (pPdxData->g_use_ini_calibration_coefs == 2)
	{
		memcpy(&(pPdxData->g_NewCalibrationData.data1), &hwParam.nMeans, sizeof(unsigned short) * MAX_CALI_IMAGE_COUNT);
		memcpy(&(pPdxData->g_NewCalibrationData.data2), &hwParam.fCoefficients, sizeof(float) * MAX_CALI_DATA_COUNT*(DEGREE_COUNT + 1));
	}
	// 2024-08-15. jg kim. 디버그 목적으로 코드 추가. 필요시에만 활성화 할 것.
	 /*sprintf_s(buf, "means\n");
	 printf("%s", buf); writelog(buf, LOG_FILE_NAME);

	 for (int i = 0; i < MAX_CALI_IMAGE_COUNT; i++)
	 {
		 sprintf_s(buf,"%d\n", pPdxData->g_NewCalibrationData.data1[i]);
		 printf("%s", buf); writelog(buf, LOG_FILE_NAME);
	 }

	sprintf_s(buf,"coeffs\n");
	printf("%s", buf); writelog(buf, LOG_FILE_NAME);
	for (int i = 0; i < MAX_CALI_IMAGE_COUNT; i++)
	{
		for (int j = 0; j < (DEGREE_COUNT + 1); j++)
		{
			sprintf_s(buf,"%e, ", pPdxData->g_NewCalibrationData.data2[i*(DEGREE_COUNT + 1) + j]);
			printf("%s", buf); writelog(buf, LOG_FILE_NAME);
		}
		sprintf_s(buf,"\n");
		printf("%s", buf); writelog(buf, LOG_FILE_NAME);
	}*/

	if (hwParam.bValidMacAddress)
		sprintf_s(buf, "Mac Address = %02X:%02X:%02X:%02X:%02X:%02X\n", hwParam.nMac[0], hwParam.nMac[1], hwParam.nMac[2], hwParam.nMac[3], hwParam.nMac[4], hwParam.nMac[5]);
	else
	{
		sprintf_s(buf, "Invalid Mac Address\n");
	}

	printf("%s", buf); 
	writelog(buf, LOG_FILE_NAME);	
	// 2024-05-02. jg kim. 장비에서 영상에 삽입한 태그를 먼저 decoding
	int nLowRes = -1;

	bool bLowRes = false;
	bool bLowXrayPower = false; 
	if (source_img[2]>=4&&source_img[2]<=7) // 2026-02-10. jg kim. 조건 Debug
	{
		sprintf(buf, "source_img[2]:%d. Valid tags. %s, %s, %s\n",source_img[2],
			hwParam.Bs[0] >= 0 ? (hwParam.Bs[0] ? "Normal X-ray" : "Low X-ray Power") : "Invalid Xray tag",
			hwParam.Bs[1] >= 0 ? (hwParam.Bs[1] ? "Door Closed"  : "Door Open") : "Invalid Door tag",
			hwParam.Bs[2] >= 0 ? (hwParam.Bs[2] ? "HR" : "SR")   : "Invalid Resolution tag"
		);
		writelog(buf, LOG_FILE_NAME);
	}
	else
	{		
		sprintf_s(buf,"Invalid tags\n");
		writelog(buf, LOG_FILE_NAME);
	}
	
	time_t image_time;
	if (hwParam.fdate > 0)
		image_time = (time_t)(hwParam.fdate * 86400. - 2209161600.);
	else
		image_time = (time_t)0;
	if (image_time > 0)
	{
		// 2026-01-27. jg kim. warning 유발 해소
		struct tm* p_t = gmtime(&image_time);
		sprintf_s(buf, "Scan date time :: %04d-%02d-%02d %02d:%02d:%02d\n", p_t->tm_year + 1900, p_t->tm_mon + 1, p_t->tm_mday, p_t->tm_hour, p_t->tm_min, p_t->tm_sec);
	}
	else
	{
		sprintf_s(buf, "Invalid Scan date time\n");
	}
	printf("%s", buf);
	writelog(buf, LOG_FILE_NAME);
	
	clock_t	start, end;
	if (hwParam.Bs[2] == -1)	// 2024-05-02. jg kim. tag에 유효한 해상도 정보가 없을 경우
	{
		start = clock();
		hwParam.Bs[2] = hwi.checkLowResolution(source_img, width, height) ? 0 : 1;
		end = clock();
		sprintf_s(buf, "CheckLowResolution again%lf ms. %s\n", double(end - start), hwParam.Bs[2] ? "HR" : "SR");
		writelog(buf, LOG_FILE_NAME);
	}

	bLowRes = hwParam.Bs[2] ? false : true;
	nLowRes = bLowRes;
	start = clock();
	// 2024-06-06. jg kim. 레드마인 18519, 18583 이슈에 대응하기 위하여 순서 변경
	pPdxData->g_en_Door = hwi._CheckDoorClosed(source_img, width, height, nLowRes) ? HW_DoorClosed : HW_DoorNotClosed;;
	end = clock();
	sprintf_s(buf, "CheckDoorClosed %lf ms\n", double(end - start));
	writelog(buf, LOG_FILE_NAME);
	hwParam.Bs[1] = int(pPdxData->g_en_Door) == 1 ? 1 : 0;
	pPdxData->nLastScannedRes = bLowRes ? eir_StRes : eir_HiRes;		// 한 종류의 값과 변수만 사용하도록 함.
	// 2024-05-02. jg kim. tag에 유효한 정보들이 없을 경우
	if (hwParam.Bs[0] < 1 || hwParam.Bs[1] < 0)
	{
		start = clock();
		bLowXrayPower = hwi.checkLowXrayPower(source_img, width, height, bLowRes);
		end = clock();
		sprintf_s(buf, "CheckLowXrayPower %lf ms\n", double(end - start));
		writelog(buf, LOG_FILE_NAME);
		
		hwParam.Bs[0] = bLowXrayPower ? 0 : 1;
		sprintf_s(buf, "Check HW status again. %s, %s, %s\n",
			hwParam.Bs[0] >= 0 ? (hwParam.Bs[0] ? "Normal X-ray" : "Low X-ray Power") : "Invalid Xray tag",
			hwParam.Bs[1] >= 0 ? (hwParam.Bs[1] ? "Door Closed" : "Door Open") : "Invalid Door tag",
			hwParam.Bs[2] >= 0 ? (hwParam.Bs[2] ? "HR" : "SR") : "Invalid Resolution tag"
		);
		writelog(buf, LOG_FILE_NAME);

	}	
	// 2024-06-06. jg kim. 레드마인 18519 이슈에 대응하기 위하여 순서 변경
	// 2026-02-09. jg kim. wobble 보정을 위한 반사판과 맞물려 부작용이 발생. 처리하지 않도록 수정.
	//if (b1 == 0) // PSP 투입구 커버가 열린 경우
	//{
	//	hwi.SuppressAmbientLight(source_img, width, height, nLowRes);
	//	sprintf(buf, "SuppressAmbientLight\n");
	//	hwi.writelog(buf_filename, buf);
	//}

	// 2024-04-22. jg kim. calibration coefficient, scan mode 유효성 검사결과 저장
	pPdxData->g_en_CalCoefficients = hwi._CheckValidCoefficients(pPdxData->g_NewCalibrationData.data1, pPdxData->g_NewCalibrationData.data2) ? Cal_ValidCoefficients : Cal_DefaultCoefficients;
	pPdxData->g_enScanMode = hwi._CheckValidIpIndex(hwParam.nIpIdx) ? HW_ValidScanMode : HW_InvalidScanMode;;

	sprintf(buf, "Image correction start \n");
	writelog(buf, LOG_FILE_NAME);

	sprintf(buf, "%s, %s, %s. Size%d, %s, %s\n",
		int(pPdxData->g_en_CalCoefficients) ? "Valid cal coeff" : "Invalid cal coeff",
		bLowRes?"SR":"HR",
		int(pPdxData->g_enScanMode) ? "Valid scan mode" : "Invalid scan mode",hwParam.nIpIdx,
		int(pPdxData->g_en_Door) ? "Door closed" : "Door open",
		bLowXrayPower ? "Low x-ray power" : "Normal x-ray");
	writelog(buf, LOG_FILE_NAME);

	// 2025-02-14. jg kim. raw image에 삽입되어 있는 영상의 스캔 일시, image list상의 index를 디코딩한 결과 및 해상도 결과를 current_pvIMAGE_INFO_list에 삽입
	pPdxData->current_pvIMAGE_INFO_list.cur_image_num = hwParam.cur_info_index;
	pPdxData->current_pvIMAGE_INFO_list.imageinfos[0].image_date = hwParam.fdate;
	pPdxData->current_pvIMAGE_INFO_list.imageinfos[0].image_resolution = bLowRes ? eir_StRes : eir_HiRes;

	// 2024-03-28. jg kim. 영상이 blank scan영상인지 판별 및 예외처리 개선.
	bool bResPre = false;
	if (bLowXrayPower)
	{
		//pPdxData->g_enNotification = Low_XRAY;  // 2024-06-14. jg kim. 코드 수정
		getPspSize(&result_width, &result_height, hwParam.nIpIdx);
		if (nLowRes == 1)
		{
			result_width /= 2;
			result_height /= 2;
		}
		result_img = new unsigned short[result_width* result_height];// scan mode 정보를 활용
		memset(result_img, 0, result_width*result_height);
	}
	else
	{
		ParametersSetting(pPdxData->g_NewCalibrationData.data2, pPdxData->g_NewCalibrationData.data1, MAX_CALI_IMAGE_COUNT);
		// 2024-12-09. jg kim. 공정툴용 영상처리와 통합
		bool bSaveCorrectionStep = false;
		switch (pPdxData->g_save_image_step)
		{
		case SaveImageCorrectionStep:
		case SaveBothProcessingStep:
			bSaveCorrectionStep = true;
			break;
		}
		SetMtfMode(pPdxData->bProcMTF);// 2026-01-12. jg kim. MTF 측정을 위한 영상보정 모드 추가
		SetFwWobblePos(hwParam.wobbleRelates[2], hwParam.wobbleRelates[3], hwParam.wobbleRelates[6], hwParam.wobbleRelates[7]); // 2026-01-12. jg kim. decoding 한 값으로 설정
		sprintf_s(buf, "wobbles %d, %d, %d, %d\n", hwParam.wobbleRelates[2], hwParam.wobbleRelates[3], hwParam.wobbleRelates[6], hwParam.wobbleRelates[7]);
		writelog(buf, LOG_FILE_NAME);
		start = clock();
		ProcessAPI(/*OUTPUT*/ &result_img, result_width, result_height, bResPre, bin, 
			/*INPUT*/source_img, width, height, hwParam.nIpIdx, bSaveCorrectionStep, nMode, bool(hwParam.Bs[1]));
		end = clock();
		sprintf_s(buf, "ProcessAPI %lf ms\n", double(end-start));
		writelog(buf, LOG_FILE_NAME);
		pPdxData->nCorrectWobbleResult = GetCorrectWobbleResult();// 2026-01-12. jg kim. Wobble 보정 결과 받아옴
	}

	// 2024-04-22. jg kim. 영상 보정결과 저장
	pPdxData->g_en_ImgCorrection = en_Error_Img_Correction(bResPre);

	sprintf(buf, "Image correction result: %s\n", bResPre ? "Success" : "Fail");
	writelog(buf, LOG_FILE_NAME);

	bool bResPost = false;
	int outW = 0;
	int outH = 0;

	// 2024-08-19. cy lee.  Test용 Console sw에선 g_ipostprocess가 1~4의 값이 아닌 경우 영상처리 미적용
	//						PortView에선 g_ipostprocess가 1이 아닌 경우 영상처리 미적용.
	// 2024-08-19. jg kim. 레드마인 이슈 20302에 대응하기 위함
	if (bResPre) // 2024-05-02. jg kim. 영상 보정이 정상적으로 되었을 경우
	//if (pPdxData->g_ipostprocess > 0 && pPdxData->g_ipostprocess < 5) 	// -> Test용 Console sw용 조건
	// 2024-08-15. jg kim. 기존 Enhnace mode만 적용할 것을 고려했으나 Enhance,  Enhance-SR, Natural, Natural-SR모드를 사용하기 위하여 변경
	{
		sprintf(buf, "Image processing start. Image w-h: %d-%d\n", result_width, result_height);
		//hwi.writelogHI(buf_filename, buf);
		writelog(buf, LOG_FILE_NAME);
		// 2024-08-19. jg kim. 레드마인 이슈 20276에 대응하기 위함
		// 2025-05-30. jg kim. MEDIUM 모드 구현
		if (pPdxData->g_ipostprocess > 0 && pPdxData->g_ipostprocess < 7)	 	// -> PortView용 조건
		{
			//KLOG3A(">>> PostProcessMode::Enhanced\n");
			// 2024-10-14. jg kim. 로그 작성 수정
			CStringA params = (pPdxData->g_ipp_type == 1 || pPdxData->g_ipp_type == 2) ? void2strA(">>> PostProcessMode::Enhance\n"):void2strA(">>> PostProcessMode::Natural\n");
			OutputDebugStringA(params + " \n");
			printf(params + " \n");


			// 2024-02-29. jg kim. paramMceIppi가 IMG_PROCESS_PARAM보다 나중에 만들었고 내용상 동일하기 때문에 IMG_PROCESS_PARAM을 사용하도록 수정
			// 2024-08-18. jg kim. HR, SR 모드는 영상 보정 단계에서 판별하고 ini 파일의 설정은 HARD, SOFT만 체크함.
			// 2024-08-19. cy lee. Test용 Console sw에선 g_ipp_type을 다음과 같이 사용
			//						0: none, 1: HR HARD, 2: HR SOFT, 3: SR HARD, 4: SR SOFT
			//						PortViewsw에선 g_ipp_type을 다음과 같이 사용
			//						0: SOFT, 1: MEDIUM, 2: HARD (아직 MEDIUM 설정이 별도로 없어 HARD 설정값 사용)
			// 2025-05-30. jg kim. MEDIUM 모드 구현
			if (pPdxData->g_ipp_type == 1 || pPdxData->g_ipp_type == 4) // HARD, 	 	// -> PortView용 조건
			{
				if (bLowRes)	// SR모드
				{
					PostProcessingParametersSetting(pPdxData->g_mceIppi_HARD_SR); // 4
					pPdxData->g_ipostprocess = 4;
				}
				else			// HR모드
				{
					PostProcessingParametersSetting(pPdxData->g_mceIppi_HARD_HR); // 1
					pPdxData->g_ipostprocess = 1;
				}
			}
			else if (pPdxData->g_ipp_type == 2 || pPdxData->g_ipp_type == 5)  // 2025-05-30. jg kim. MEDIUM 모드 구현
			{
				if (bLowRes)	// SR모드
				{
					PostProcessingParametersSetting(pPdxData->g_mceIppi_MEDIUM_SR); // 5
					pPdxData->g_ipostprocess = 5;
				}
				else			// HR모드
				{
					PostProcessingParametersSetting(pPdxData->g_mceIppi_MEDIUM_HR); // 2
					pPdxData->g_ipostprocess = 2;
				}
			}
			else //if (pPdxData->g_ipp_type==0) // SOFT, 	 	// -> PortView용 조건
			{ 
				if (bLowRes)	// SR모드
				{
					PostProcessingParametersSetting(pPdxData->g_mceIppi_SOFT_SR); // 6
					pPdxData->g_ipostprocess = 6;
				}
				else			// HR모드
				{
					PostProcessingParametersSetting(pPdxData->g_mceIppi_SOFT_HR); // 3
					pPdxData->g_ipostprocess = 3;
				}
			}			

			switch (pPdxData->g_save_image_step)
			{
			case SaveImageNone:
				PostProcess(out, outW, outH, bResPost, result_img, result_width, result_height, false);
				break;
			case SaveImageProcessingStep:
			case SaveBothProcessingStep:
			{
				//setLogFileNameIP(buf_filename); // 2024-05-02. jg kim. 영상처리 과정을 저장하는 경우 세부 로그도 작성 
				PostProcess(out, outW, outH, bResPost, result_img, result_width, result_height, true);
			}
			break;
			default:
				PostProcess(out, outW, outH, bResPost, result_img, result_width, result_height, false);
				break;
			}
		}
		else if (pPdxData->g_ipostprocess == 0) // 2024-08-23. jg kim. 공정툴에서는 영상보정(전처리)만 한 영상이 필요함
		{
			outW = result_width;
			outH = result_height;
			memcpy(out, result_img, sizeof(unsigned short)*result_width* result_height);
		}
	}
	else // 영상 보정에 실패했을 경우
	{
		getPspSize(&result_width, &result_height, hwParam.nIpIdx); // 2024-08-23. jg kim. 전에 구현했던것 같은데 현재는 의도한 대로 동작하지 않아 코드 추가
		outW = result_width;
		outH = result_height;
		result_img = new unsigned short[outW*outH];
		memset(result_img, 0, sizeof(unsigned short)*outW*outH);
		memcpy(out, result_img, sizeof(unsigned short)*result_width* result_height);
		bResPost = false;
	}
	// 2024-04-22. jg kim. 영상처리 결과 저장
	pPdxData->g_en_ImgProcessing = en_Error_Img_Processing(bResPost);
	sprintf(buf, "Image processing result:%s\n", bResPost ? "Success" : "Fail");
	//hwi.writelogHI(buf_filename, buf);
	writelog(buf, LOG_FILE_NAME);

	if (outW > 0 && outH > 0)
	{
		pPdxData->last_image_width = outW;
		pPdxData->last_image_height = outH;
		memmove(pPdxData->last_image_data, out, sizeof(unsigned short) * outW * outH);
	}
	else
	{
		//pPdxData->last_image_width = outW;
		//pPdxData->last_image_height = outH;
		//memmove(pPdxData->last_image_data, source_img, sizeof(unsigned short) * width * height);
		//clearBuf();
	}

	pp_by_dll_result = SUCCESS;
	//pp_by_dll_res = 0U;//밑에서 +1 하는 부분 오동작을 막기 위해 

	// 2026-01-27. jg kim. warning 유발 해소
	SAFE_DELETE_ARRAY(result_img);
	SAFE_DELETE_ARRAY(out); // 2024-10-14. jg kim. 메모리 해제 개선
	SAFE_DELETE_ARRAY(source_img);
	SAFE_DELETE_ARRAY(bin);
	//PreProcessing.dll 테스트 코드<끝>
}

void last_image_to_fin_by_dll(void)
{
	//predix cr  현재는  predix에 최적화되어있지 않아 그냥 리턴함
	auto pPdxData = GetPdxData();
	ASSERT(pPdxData);

	auto whSize = pPdxData->map_fin.w() * pPdxData->map_fin.h();
	ASSERT(whSize > 0);
	ASSERT(pPdxData->map_fin.size() > 0);
	for (int i = 0; i < whSize; i++)
		pPdxData->map_fin[i] = 0;

	ASSERT(pPdxData->last_image_data);

	if (pp_by_dll_result == SUCCESS)
	{//succeeded
		MYPLOGMA("last_image_to_fin_by_dll ok\n");
		MYPLOGDG(L"last image to fin from dll's image \n");
		for (int y = 0; y < pPdxData->last_image_height; y++)
		{
			for (int x = 0; x < pPdxData->last_image_width; x++)
				pPdxData->map_fin[(y * pPdxData->map_fin.w()) + x] = pPdxData->last_image_data[x + (y * pPdxData->last_image_width)];
		}
	}
}
