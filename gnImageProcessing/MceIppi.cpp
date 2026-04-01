// Refactored by Makeit, on 2024/02/16.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "MceIppi.h"
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
#include "../include/Logger.h"
#include <math.h>
#include <omp.h>
#include <filesystem>



//#define debugSave

#ifdef _DEBUG
#define _Check_Memory_Leak
#endif

#ifdef _Check_Memory_Leak

#define _CRTDBG_MAP_ALLOC  
#include <stdlib.h>  
#include <crtdbg.h> 

#define new new(_CLIENT_BLOCK, __FILE__, __LINE__)
#define malloc(s) _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)

#else
#include <cstdlib>
#endif // _Check_Memory_Leak



// macro constants
#define NOISE_VALUE 100
#define RAW_IMAGE_WIDTH 1536
#define RAW_IMAGE_HEIGHT 2360

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) delete[] p; p = NULL;}
#endif



IMG_PROCESS_PARAM g_params;
extern "C" void __cdecl PostProcessingParametersSetting(IMG_PROCESS_PARAM params)
{
	g_params = params;
}
// 2024-04-26. jg kim. log 작성을 위해 추가
// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
static const char* LOG_FILE_NAME = "ImageProcessing.log";

// 2024-04-22. jg kim. 영상처리 리턴값 추가
extern "C" void __cdecl PostProcess(unsigned short* result_img, int& result_width, int& result_height, bool &bResult,
	unsigned short* source_img, int source_width, int source_height, bool bSaveImageProcessingStep)
{
#ifdef _Check_Memory_Leak
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
	CMceIppi ippi;
	ippi.SetImage(source_img, source_width, source_height);
	ippi.SetSaveImageProcessingStep(bSaveImageProcessingStep);
	bResult = ippi.ProcessMceIppi(g_params);
	ippi.getProcessedImage(result_img, result_width, result_height);

#ifdef _Check_Memory_Leak
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CMceIppi::CMceIppi()
	: m_p32f_Org_Buf(nullptr)
	, m_SB_p32f_Org_Buf(0)
	, m_nImgWidth(0)
	, m_nImgHeight(0)
	, m_nHistogramLeftoffset(284)
	, m_nHistogramRightoffset(284)
	, m_nHistogramTopoffset(284)
	, m_nHistogramBottomoffset(284)
	, m_pMaskImage(NULL)
	, m_p32f_Buf(NULL)
	, m_SB_p32f_Buf(0)
	, m_p8u_MaskImage(NULL)
	, m_SB_p8u_MaskImage(0)
	, m_nTotalPyrLevel(0)
	, m_pPyrWidthArray(NULL)
	, m_pPyrHeightArray(NULL)
	, m_p32f_LaplacianPyrImageArray(NULL)
	, m_pSB_m_p32f_LaplacianPyrImageArray(NULL)
	, m_fBaseLevelValue(0.f)
	, m_fSrcMin(0.f)
	, m_fSrcMax(0.f)
	, org_roiImg{0}
	, org_roiImgHisto{0}
	//, m_nPyramidLayer(1)
	//, m_fPyramidW2Offset(0)
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CMceIppi::~CMceIppi()
{
	BufferDelete();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// delete class member buffer
void CMceIppi::BufferDelete()
{
	SAFE_DELETE_ARRAY(m_pMaskImage);

	if (m_p32f_Org_Buf)
		ippiFree(m_p32f_Org_Buf);
	m_p32f_Org_Buf = NULL;

	if (m_p32f_Buf)
		ippiFree(m_p32f_Buf);
	m_p32f_Buf = NULL;

	if (m_p8u_MaskImage)
		ippiFree(m_p8u_MaskImage);
	m_p8u_MaskImage = NULL;

	if (m_p32f_LaplacianPyrImageArray)
		ippiFree(m_p32f_LaplacianPyrImageArray);
	m_p32f_LaplacianPyrImageArray = NULL;

	SAFE_DELETE_ARRAY(m_pPyrWidthArray);
	SAFE_DELETE_ARRAY(m_pPyrHeightArray);
	SAFE_DELETE_ARRAY(m_pSB_m_p32f_LaplacianPyrImageArray);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::MetalThreshold(float fThreshold, int nErodeCount)
{
	return MakeMetalMask(m_p32f_Buf, m_SB_p32f_Buf, fThreshold, nErodeCount, m_p8u_MaskImage, m_SB_p8u_MaskImage);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyMedian(int nKernelSize)
{
	BOOL rtn = FALSE;

	Ipp32f* p32f_TmpBuf = NULL;
	int SB_p32f_TmpBuf;
	p32f_TmpBuf = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &SB_p32f_TmpBuf);

	if (!BlurProcess(nKernelSize, m_p32f_Buf, m_SB_p32f_Buf, p32f_TmpBuf, SB_p32f_TmpBuf))
		goto RETURN_FALSE;

	ippiCopy_32f_C1R(p32f_TmpBuf, SB_p32f_TmpBuf, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);

	rtn = TRUE;

RETURN_FALSE:;

	if (p32f_TmpBuf)
		ippiFree(p32f_TmpBuf);

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyBilateral(float fSigmaD1, float fSigmaR1)
{
	Ipp32f* p32f_TmpBuf = NULL;
	int SB_p32f_TmpBuf;
	p32f_TmpBuf = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &SB_p32f_TmpBuf);

	ApplyBilateralFilter(fSigmaD1, fSigmaR1, m_p32f_Buf, m_SB_p32f_Buf, p32f_TmpBuf, SB_p32f_TmpBuf);
	ippiCopy_32f_C1R(p32f_TmpBuf, SB_p32f_TmpBuf, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);

	if (p32f_TmpBuf)
		ippiFree(p32f_TmpBuf);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyLog(void)
{
	BOOL rtn = FALSE;

	IppStatus	status;

	//////////////////// m_p32f_Buf + 1 -> log 적용하기전 pixel값이 0인부분 제거하기 위해
	////////2014.03.17 jinchangshou////////////////////////////////////////////////////////////////
	status = ippiAddC_32f_C1IR(1, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);

	//rtn = ApplyLogCurve(m_nImgWidth, m_nImgHeight, m_p32f_Buf, m_SB_p32f_Buf); 이거뿐이라 변경; 
	status = ippiLn_32f_C1IR(m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);

	rtn = TRUE;

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyInvert(void)
{
	BOOL rtn = FALSE;

	ScaleTo14bit();

	float dMaxValue = 0;

	IppStatus	status;
	status = ippiMax_32f_C1R(m_p32f_Buf, m_SB_p32f_Buf, org_roiImg, &dMaxValue);
	if (status < 0)
	{
		goto RETURN_FALSE;
	}

	status = ippiMulC_32f_C1IR(-1, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);
	if (status < 0)
	{
		goto RETURN_FALSE;
	}

	status = ippiAddC_32f_C1IR((float)dMaxValue, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);
	if (status < 0)
	{
		goto RETURN_FALSE;
	}

	rtn = TRUE;
RETURN_FALSE:;

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyMusica(float a, float M, float p, float c0, float fe, int ne, float fl, int nl)
{
	// 아래 논문을 참조하여 구현
	// https://www.dgzfp.de/Portals/24/PDFs/BBonline/bb_67-CD/bb67_v16.pdf
	// MUSICA : MUlti Scale Image Contrast Amplification
	if (!GeneratePyramid())
		return false;

	if (!ApplyPyramidCoefficient(a, M, p, c0, fe, ne, fl, nl))
		return false;

	if (!ReconstructPyrImage())
		return false;

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyUnsharpMask(int nAmount, double dRadius, int nThreshold)
{
	BOOL bResult = TRUE;

	//INT nAllocBytes = 2;

	//------------------------------------------------------------------------
	// parameters

	nAmount = min(2000, max(50, nAmount));
	dRadius = min(3, max(0.5, dRadius));
	//nThreshold = min(255,max(0,nThreshold));
	nThreshold = min(65535, max(0, nThreshold));

	double fAmount = nAmount / 100.;

	//------------------------------------------------------------------------
	float* pTempBuff = new float[m_nImgWidth * m_nImgHeight];
	for (int ny = 0; ny < m_nImgHeight; ny++)
		memcpy(pTempBuff + ny * m_nImgWidth, m_p32f_Buf + ny * m_SB_p32f_Buf / 4, m_nImgWidth * sizeof(float));

	float Vprev = 0, Vpost = 0;
	float Vtemp;

	UNREFERENCED_PARAMETER(Vpost);

	//-----------------------------------------------------------
	// GAUSSIAN BLUR FILTERING
	//http://www.gamedev.net/topic/307417-how-to-write-gaussian-blur-filter-in-c-tool/

	int mWidth = m_nImgWidth;
	int mHeight = m_nImgHeight;
	float* mData = pTempBuff;

	//number of bytes per pixel
	float* temp = new float[mWidth * mHeight];

	double sigma2 = dRadius * dRadius;
	int    size = 5;//good approximation of filter

	// internal used variables ...
	double sum = 0;
	int    y;

	//filter normalize
	double* pfactor = new double[size * 2 + 1];
#pragma omp parallel for reduction(+:sum)
	for (int i = 0; i <= size * 2; i++)
	{
		pfactor[i] = exp(-(i - size) * (i - size) / (2 * sigma2));
		sum += pfactor[i];
	}

#pragma omp parallel for 
	for (int i = 0; i <= size * 2; i++)
	{
		pfactor[i] /= sum;
	}

	//blurs x components
#pragma omp parallel for
	for (y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			//process a pixel
			double pixel = 0;

			//accumulate colors
			for (int i = max(0, x - size); i <= min(mWidth - 1, x + size); i++)
				pixel += pfactor[i - (x - size)] * mData[i + y * mWidth];

			//copy a pixel
			temp[x + y * mWidth] = (float)pixel;
		}
	}

	//blurs y components
#pragma omp parallel for 
	for (y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			//process a pixel
			double pixel = 0;

			//accumulate colors
			for (int i = max(0, y - size); i <= min(mHeight - 1, y + size); i++)
				pixel += pfactor[i - (y - size)] * temp[x + i * mWidth];

			//copy a pixel
			mData[x + y * mWidth] = (float)pixel;
		}
	}
	SAFE_DELETE_ARRAY(pfactor);
	SAFE_DELETE_ARRAY(temp);

	//-------------------------------------------------------------------------------------------
	// UNSHARP MASK FILTERING

	int pos1, pos2;
	float Vdiff;
#pragma omp parallel for private(pos1, pos2, Vprev, Vtemp, Vdiff)
	for (y = size; y < mHeight - size; y++) {
		for (int x = size; x < mWidth - size; x++) {
			pos1 = y * mWidth + x;
			pos2 = y * m_SB_p32f_Buf / 4 + x;

			Vprev = m_p32f_Buf[pos2];
			Vtemp = pTempBuff[pos1];
			Vdiff = Vprev - Vtemp;

			m_p32f_Buf[pos2] = (float)max(0, min(65535, fAmount * Vdiff + Vprev));

		}
	}

	SAFE_DELETE_ARRAY(pTempBuff);

	return bResult;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyGaussianBlur(Ipp32f* pImage, int nAmount, double dRadius, int nThreshold)
{
	BOOL bResult = TRUE;

	//INT nAllocBytes = 2;

	//------------------------------------------------------------------------
	// parameters

	nAmount = min(2000, max(50, nAmount));
	dRadius = min(3, max(0.5, dRadius));
	//nThreshold = min(255,max(0,nThreshold));
	nThreshold = min(65535, max(0, nThreshold));

	//double fAmount = nAmount / 100.;

	//------------------------------------------------------------------------
	float* pTempBuff = new float[m_nImgWidth * m_nImgHeight];
	for(int y=0; y<m_nImgHeight; y++) 
	// 2024-03-13. jg kim. 메모리 복사를 잘못하고 있었으나 지금까지는 문제되지 않았다.
	// 저해상도 영상 관련 이슈 (이슈 #17835) 처리하다 문제 발견
		memcpy(pTempBuff+y*m_nImgWidth, pImage+y* m_SB_p32f_Buf/4, m_nImgWidth * sizeof(float));

	//float Vprev = 0, Vpost = 0;

	//-----------------------------------------------------------
	// GAUSSIAN BLUR FILTERING
	//http://www.gamedev.net/topic/307417-how-to-write-gaussian-blur-filter-in-c-tool/

	int mWidth = m_nImgWidth;
	int mHeight = m_nImgHeight;
	float* mData = pTempBuff;

	//nombre d'octets par pixel
	float* temp = new float[mWidth * mHeight];

	double sigma2 = dRadius * dRadius;
	int size = 5;//good approximation of filter

	// internal used variables ...
	double sum = 0;
	int y;

	//filter normalize
	double* pfactor = new double[size * 2 + 1];
#pragma omp parallel for reduction(+:sum)
	for (int i = 0; i <= size * 2; i++)
	{
		pfactor[i] = exp(-(i - size) * (i - size) / (2 * sigma2));
		sum += pfactor[i];
	}

#pragma omp parallel for 
	for (int i = 0; i <= size * 2; i++)
	{
		pfactor[i] /= sum;
	}

	//blurs x components
#pragma omp parallel for
	for (y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			//process a pixel
			double pixel = 0;

			//accumulate colors
			for (int i = max(0, x - size); i <= min(mWidth - 1, x + size); i++)
				pixel += pfactor[i - (x - size)] * mData[i + y * mWidth];

			//copy a pixel
			temp[x + y * mWidth] = (float)pixel;
		}
	}
	//blurs y components
#pragma omp parallel for 
	for (y = 0; y < mHeight; y++)
	{
		for (int x = 0; x < mWidth; x++)
		{
			//process a pixel
			double pixel = 0;
			//accumulate colors
			for (int i = max(0, y - size); i <= min(mHeight - 1, y + size); i++)
				pixel += pfactor[i - (y - size)] * temp[x + i * mWidth];
			//copy a pixel
			mData[x + y * mWidth] = (float)pixel;
		}
	}
	for ( y = 0; y < m_nImgHeight; y++)
		memcpy(pImage + y * m_SB_p32f_Buf/4, pTempBuff + y * m_nImgWidth, m_nImgWidth * sizeof(float));
	SAFE_DELETE_ARRAY(pTempBuff);
	SAFE_DELETE_ARRAY(pfactor);
	SAFE_DELETE_ARRAY(temp);
	return bResult;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyGaussianBlur(void)
{
	//BOOL			ApplyGaussianBlur(Ipp32f*			img, int nAmount = 100, double dRadius = 1.5, int nThreshold = 0);
	ApplyGaussianBlur(m_p32f_Buf, 100, 1.5, 0);
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::SetWindow(int nPyramidLayer, float fPyramidW2Offset)
{
	// mask 영상 downsample하여 w2값 잡기
	// LCY-20151217-MOD: W1도 여기서 잡도록 수정.
	// fSrcMax를 (fSrcMax-fSrcMin)의 m_IPP.fPyramidW2Offset만큼 offset 주어 threshold값으로 사용.
	GetMinMaxUsingDownsampledImage(m_p32f_Buf, m_SB_p32f_Buf, m_p8u_MaskImage, m_SB_p8u_MaskImage, nPyramidLayer, 2, &m_fSrcMin, &m_fSrcMax);

	m_fSrcMax = m_fSrcMax + (m_fSrcMax - m_fSrcMin) * fPyramidW2Offset / 100;
	ippiThreshold_LTValGTVal_32f_C1IR(m_p32f_Buf, m_SB_p32f_Buf, org_roiImg, m_fSrcMin, m_fSrcMin, m_fSrcMax, m_fSrcMax);
	//float fmin = m_nMin - float(m_nMax) / 2;// input :: center
	//float fmax = m_nMin + float(m_nMax) / 2;// input :: width
	//ippiThreshold_LTValGTVal_32f_C1IR(m_p32f_Buf, m_SB_p32f_Buf, org_roiImg, fmin, fmin, fmax, fmax);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyGamma(float fGamma)
{
	//float fmin = m_nMin - float(m_nMax) / 2;// input :: center
	//float fmax = m_nMin + float(m_nMax) / 2;// input :: width
	//return 	ApplyGammaCurve(m_p32f_Buf, m_SB_p32f_Buf, fGamma, fmin, fmax);
	return 	ApplyGammaCurve(m_p32f_Buf, m_SB_p32f_Buf, fGamma, m_fSrcMin, m_fSrcMax);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyLinecorrect(void)
{
	int nPreTerm = 9;
	int nPostTerm = 50;

	float* pfTempImg = new float[m_nImgWidth * m_nImgHeight];

	for (int itery = 0; itery < m_nImgHeight; itery++)
		memcpy(pfTempImg + itery * m_nImgWidth, (char*)m_p32f_Buf + itery * m_SB_p32f_Buf, m_nImgWidth * sizeof(m_p32f_Buf[0]));

	Correct_line_cpu(pfTempImg, pfTempImg, m_nImgWidth, m_nImgHeight, nPreTerm, nPostTerm, &org_roiImg);

	for (int itery = 0; itery < m_nImgHeight; itery++)
		memcpy((char*)m_p32f_Buf + itery * m_SB_p32f_Buf, pfTempImg + itery * m_nImgWidth, m_nImgWidth * sizeof(m_p32f_Buf[0]));

	SAFE_DELETE_ARRAY(pfTempImg);

	return true; // 2024-04-26. jg kim. debug
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::ApplyLinecorrect(float* Output_img, float* Input_img, int nWidth, int nHeight, int nPreTerm, int nPostTerm, bool bVertical)
{
	UNREFERENCED_PARAMETER(bVertical);

	int nSB_p32f_Buf = 0;
	auto p32f_Buf = ippiMalloc_32f_C1(nWidth, nHeight, &nSB_p32f_Buf);

	for (int itery = 0; itery < nHeight; itery++)
		memcpy(Output_img + itery * nWidth, (char*)p32f_Buf + itery * nSB_p32f_Buf, nWidth * sizeof(p32f_Buf[0]));

	Correct_line_cpu(Output_img, Input_img, nWidth, nHeight, nPreTerm, nPostTerm);

	for (int itery = 0; itery < nHeight; itery++)
		memcpy((char*)p32f_Buf + itery * nSB_p32f_Buf, Input_img + itery * nWidth, nWidth * sizeof(p32f_Buf[0]));

	ippiFree(p32f_Buf);
}

// Export wrapper for ApplyLinecorrect
extern "C" void __cdecl ApplyLinecorrect(float* Output_img, float* Input_img, int nWidth, int nHeight, int nPreTerm, int nPostTerm, bool bVertical)
{
	CMceIppi::ApplyLinecorrect(Output_img, Input_img, nWidth, nHeight, nPreTerm, nPostTerm, bVertical);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::MakeMetalMask(Ipp32f* p32f_Buf, int SB_p32f_Buf, float fRefValue, int nErodeCount, Ipp8u* p8u_MaskImage_, int SB_p8u_MaskImage_)
{
	//IppiSize roiImgHisto = { nWidth, nHeight }; // 중복 삭제

	Ipp32f* p32f_bufTemp;
	int SB_p32f_bufTemp;
	p32f_bufTemp = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &SB_p32f_bufTemp);

	Ipp8u* p8u_MaskTemp = NULL;
	int SB_p8u_MaskTemp;
	p8u_MaskTemp = ippiMalloc_8u_C1(m_nImgWidth, m_nImgHeight, &SB_p8u_MaskTemp);
	ippiSet_8u_C1R(1, p8u_MaskTemp, SB_p8u_MaskTemp, org_roiImg);

	ippiThreshold_LTValGTVal_32f_C1R(p32f_Buf,
		SB_p32f_Buf,
		p32f_bufTemp,
		SB_p32f_bufTemp,
		org_roiImg,
		fRefValue, 0,
		fRefValue, 1);

	//IppiSize roi = { m_nImgWidth, m_nImgHeight }; // 중복 삭제
	ippiConvert_32f8u_C1R(p32f_bufTemp, SB_p32f_bufTemp, p8u_MaskTemp, SB_p8u_MaskTemp, org_roiImg, ippRndNear);

	//////////////////////
	IppiSize roiImg = { m_nImgWidth - 2, m_nImgHeight - 2 };

	for (int iter = 0; iter < nErodeCount; iter++)
	{
		ippiErode3x3_8u_C1IR(p8u_MaskTemp + SB_p8u_MaskTemp + 1,		// LCY-161018-2-MOD: Erode로 변경. 잘못하용하고 있었음.
			SB_p8u_MaskTemp,
			roiImg);
	}

	ippiThreshold_GTVal_8u_C1IR(p8u_MaskTemp,
		SB_p8u_MaskTemp,
		org_roiImg,
		1, 1);

	// LCY-20171124-MOD: mask 영역이 너무 작으면, 원본 mask 그대로 사용하도록 수정하여, 영상 전체 level이 낮을 경우 발새하던 영상 하얗게 나오는 문제 수정
	Ipp64f fMean;
	ippiMean_8u_C1R(p8u_MaskTemp, SB_p8u_MaskTemp, org_roiImg, &fMean);
	if (fMean > 0.2) // 0.8일때가 좀더 나았음
		ippiMul_8u_C1IRSfs(p8u_MaskTemp, SB_p8u_MaskTemp, p8u_MaskImage_, SB_p8u_MaskImage_, org_roiImg, 0);


	ippiFree(p32f_bufTemp);
	ippiFree(p8u_MaskTemp);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::BlurProcess(int nKernelSize, Ipp32f* p32f_SrcBuf, int SB_p32f_SrcBuf, Ipp32f* p32f_DesBuf, int SB_p32f_DesBuf)
{
	BOOL rtn = FALSE;

	rtn = ApplySelectiveMedianFilter(nKernelSize, p32f_SrcBuf, SB_p32f_SrcBuf, p32f_DesBuf, SB_p32f_DesBuf);
	rtn = ApplySelectiveMedianFilter(nKernelSize, p32f_DesBuf, SB_p32f_DesBuf, p32f_SrcBuf, SB_p32f_SrcBuf);
	rtn = ApplySelectiveMedianFilter(nKernelSize, p32f_SrcBuf, SB_p32f_SrcBuf, p32f_DesBuf, SB_p32f_DesBuf); // 왜 3번이나? 검토 필요

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplySelectiveMedianFilter(int nKernelSize, Ipp32f* p32f_SrcBuf, int SB_p32f_SrcBuf, Ipp32f* p32f_DesBuf, int SB_p32f_DesBuf)
{
	BOOL rtn = FALSE;

	IppiSize kernel = { nKernelSize, nKernelSize };
	IppiPoint anchor = { (nKernelSize - 1) / 2, (nKernelSize - 1) / 2 };
	IppStatus status = (IppStatus)0;

	Ipp8u* p8u_BufMin = NULL;
	int SB_p8u_BufMin;
	p8u_BufMin = ippiMalloc_8u_C1(m_nImgWidth, m_nImgHeight, &SB_p8u_BufMin);
	ippiSet_8u_C1R(0, p8u_BufMin, SB_p8u_BufMin, org_roiImg);

	Ipp8u* p8u_BufMax = NULL;
	int SB_p8u_BufMax;
	p8u_BufMax = ippiMalloc_8u_C1(m_nImgWidth, m_nImgHeight, &SB_p8u_BufMax);
	ippiSet_8u_C1R(0, p8u_BufMax, SB_p8u_BufMax, org_roiImg);

	SelectiveNoiseCount(m_nImgWidth, m_nImgHeight, nKernelSize, p32f_SrcBuf, SB_p32f_SrcBuf, p8u_BufMin, SB_p8u_BufMin, p8u_BufMax, SB_p8u_BufMax);

	Ipp8u* p8u_BufNoiseMap = NULL;
	int SB_p8u_BufNoiseMap;
	p8u_BufNoiseMap = ippiMalloc_8u_C1(m_nImgWidth, m_nImgHeight, &SB_p8u_BufNoiseMap);
	ippiSet_8u_C1R(0, p8u_BufNoiseMap, SB_p8u_BufNoiseMap, org_roiImg);

	CreateNoiseMap(m_nImgWidth, m_nImgHeight, p8u_BufMin, SB_p8u_BufMin, p8u_BufMax, SB_p8u_BufMax, p8u_BufNoiseMap, SB_p8u_BufNoiseMap, 4);

	Ipp32f* p32f_TmpBuf;
	int SB_p32f_TmpBuf;
	p32f_TmpBuf = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &SB_p32f_TmpBuf);

	TestMedian(nKernelSize, p32f_SrcBuf, SB_p32f_SrcBuf, p32f_TmpBuf, SB_p32f_TmpBuf, p8u_BufNoiseMap, SB_p8u_BufNoiseMap); // 검토 필요

	ippiCopy_32f_C1R(p32f_SrcBuf, SB_p32f_SrcBuf, p32f_DesBuf, SB_p32f_DesBuf, org_roiImg);
	ippiCopy_32f_C1MR(p32f_TmpBuf, SB_p32f_TmpBuf, p32f_DesBuf, SB_p32f_DesBuf, org_roiImg, p8u_BufNoiseMap, SB_p8u_BufNoiseMap);

	if (status < 0)
	{
		goto RETURN_FALSE;
	}

	rtn = TRUE;

RETURN_FALSE:;

	if (p8u_BufMin)
		ippiFree(p8u_BufMin);

	if (p8u_BufMax)
		ippiFree(p8u_BufMax);

	if (p8u_BufNoiseMap)
		ippiFree(p8u_BufNoiseMap);

	if (p32f_TmpBuf)
		ippiFree(p32f_TmpBuf);

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::SelectiveNoiseCount(int nWidth, int nHeight, int nKernelSize, Ipp32f* p32f_BufSrc, int SB_p32f_BufSrc,
	Ipp8u* p8u_BufMin, int SB_p8u_BufMin, Ipp8u* p8u_BufMax, int SB_p8u_BufMax)
{
	BOOL rtn = FALSE;
	int nRadius = (nKernelSize - 1) / 2;

	memset(p8u_BufMin, 0, SB_p8u_BufMin * nHeight);
	memset(p8u_BufMax, 0, SB_p8u_BufMax * nHeight);

	int ny;
	int nWidthIpp = SB_p32f_BufSrc / 4;
#pragma omp parallel for 
	for (ny = nRadius; ny < nHeight - nRadius; ny++)
	{
		for (int nx = nRadius; nx < nWidth - nRadius; nx++)
		{
			int Zmin = 100000000, Zmax = -100000000, min_x = 0, min_y = 0, max_x = 0, max_y = 0;
			for (int itery = ny - nRadius; itery <= ny + nRadius; itery++)
			{
				for (int iterx = nx - nRadius; iterx <= nx + nRadius; iterx++)
				{
					unsigned short temp_value = (unsigned short)*(p32f_BufSrc + nWidthIpp * itery + iterx);
					if (Zmin > temp_value)
					{
						Zmin = temp_value;
						min_x = iterx;
						min_y = itery;
					}
					if (Zmax < temp_value)
					{
						Zmax = temp_value;
						max_x = iterx;
						max_y = itery;
					}
				}
			}

			(*(p8u_BufMin + SB_p8u_BufMin * min_y + min_x))++;
			(*(p8u_BufMax + SB_p8u_BufMax * max_y + max_x))++;
		}
	}

	rtn = TRUE;

	//RETURN_FALSE:;
	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::CreateNoiseMap(int nWidth, int nHeight, Ipp8u* p8u_BufMin, int SB_p8u_BufMin,
	Ipp8u* p8u_BufMax, int SB_p8u_BufMax, Ipp8u* p8u_BufNoise, int SB_p8u_BufNoise, int nThresholdNoiseCount)
{
	BOOL rtn = FALSE;

	int ny;
#pragma omp parallel for 
	for (ny = 0; ny < nHeight; ny++)
	{
		for (int nx = 0; nx < nWidth; nx++)
		{
			if (p8u_BufMin[ny * SB_p8u_BufMin + nx] >= nThresholdNoiseCount || p8u_BufMax[ny * SB_p8u_BufMax + nx] >= nThresholdNoiseCount ||
				ny == 0 || nx == 0 || ny == nHeight - 1 || nx == nWidth - 1)
				p8u_BufNoise[ny * SB_p8u_BufNoise + nx] = 1;
		}
	}

	rtn = TRUE;

	//RETURN_FALSE:;
	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
template <typename T>
inline int  SelectionSort(T* pData, long nLen)
{
	int i, j;
	float temp;
	for (i = 1; i < nLen; i++)
	{
		temp = pData[i];
		j = i - 1;

		while (j > -1 && pData[j] > temp)
		{
			pData[j + 1] = pData[j];
			j--;
		}
		pData[j + 1] = temp;
	}
	return 1;   //arrays are passed to functions by address; nothing is returned
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::TestMedian(int nKernelSize, Ipp32f* p32f_SrcBuf, int SB_p32f_SrcBuf, Ipp32f* p32f_DesBuf, int SB_p32f_DesBuf,
	Ipp8u* p8u_Mask, int SB_p8u_Mask)
{
	BOOL rtn = FALSE;
	int nRadius = (nKernelSize - 1) / 2;

	int ny;
#pragma omp parallel for 
	for (ny = 0; ny < m_nImgHeight; ny++)
	{
		for (int nx = 0; nx < m_nImgWidth; nx++)
		{
			float Data[100];
			if (p8u_Mask != NULL)
			{
				if (p8u_Mask[ny * SB_p8u_Mask + nx])
				{
					int BufLen = 0;
					for (int itery = ny - nRadius; itery <= ny + nRadius; itery++)
					{
						for (int iterx = nx - nRadius; iterx <= nx + nRadius; iterx++)
						{
							if ((iterx >= 0) && (iterx < m_nImgWidth) && (itery >= 0) && (itery < m_nImgHeight))
							{
								Data[BufLen] = p32f_SrcBuf[itery * SB_p32f_SrcBuf / 4 + iterx];
								BufLen++;
							}
						}
					}
					SelectionSort(Data, BufLen);
					BufLen = BufLen / 2;
					*(p32f_DesBuf + SB_p32f_DesBuf / 4 * ny + nx) = Data[BufLen];
				}
			}
			else
			{
				int BufLen = 0;
				for (int itery = ny - nRadius; itery <= ny + nRadius; itery++)
				{
					for (int iterx = nx - nRadius; iterx <= nx + nRadius; iterx++)
					{
						if ((iterx >= 0) && (iterx < m_nImgWidth) && (itery >= 0) && (itery < m_nImgHeight))
						{
							Data[BufLen] = p32f_SrcBuf[itery * SB_p32f_SrcBuf / 4 + iterx];
							BufLen++;
						}
					}
				}
				SelectionSort(Data, BufLen);
				BufLen = BufLen / 2;
				*(p32f_DesBuf + SB_p32f_DesBuf / 4 * ny + nx) = Data[BufLen];
			}
		}
	}
	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyBilateralFilter(float fSigmaD, float fSigmaR, Ipp32f* p32f_SrcBuf, int SB_p32f_SrcBuf,
	Ipp32f* p32f_DesBuf, int SB_p32f_DesBuf)
{
	UNREFERENCED_PARAMETER(SB_p32f_DesBuf);

	BOOL rtn = FALSE;

	int nKernelDiameter = (int)2 * (int)ceil(2 * fSigmaD) + 1;	  // fSigma=1이면, raduis=2 (5x5)// 0.5이하는 (3x3)
	int nRadius = nKernelDiameter / 2;
	float f1Divby2xSqureSigmaD = 1.f / (2.0f * fSigmaD * fSigmaD);
	float f2Divby2xSqureSigmaR = 1.f / (2.0f * fSigmaR * fSigmaR);

	int ny;
#pragma omp parallel for 
	for (ny = 0; ny < m_nImgHeight; ny++)
	{
		for (int nx = 0; nx < m_nImgWidth; nx++)
		{
			if ((ny < nRadius) || (ny >= (m_nImgHeight - nRadius)) || (nx < nRadius) || (nx >= (m_nImgWidth - nRadius)))
			{
				*(p32f_DesBuf + SB_p32f_SrcBuf / 4 * ny + nx) = *(p32f_SrcBuf + SB_p32f_SrcBuf / 4 * ny + nx);
			}
			else
			{
				float ls = *(p32f_SrcBuf + SB_p32f_SrcBuf / 4 * ny + nx), lp = 0;
				float ks = 0, sum = 0;

				for (int iy = ny - nRadius; iy <= ny + nRadius; iy++)
				{
					int dy = iy - ny;
					for (int ix = nx - nRadius; ix <= nx + nRadius; ix++)
					{
						lp = *(p32f_SrcBuf + SB_p32f_SrcBuf / 4 * iy + ix);
						int dx = ix - nx;

						int d = (dx * dx + dy * dy);
						float ld = ls - lp;

						float fps = -d * f1Divby2xSqureSigmaD;
						float gps = -(ld * ld) * f2Divby2xSqureSigmaR;
						float dWeight = (float)exp(fps + gps);

						sum += (lp * dWeight);
						ks += dWeight;
					}
				}
				*(p32f_DesBuf + SB_p32f_SrcBuf / 4 * ny + nx) = sum / ks;
			}
		}
	}

	rtn = TRUE;

	//RETURN_FALSE:;
	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::GeneratePyramid(void)
{
	BOOL rtn = FALSE;

	int nPadSize;
	if (m_nImgWidth > m_nImgHeight)
		nPadSize = m_nImgWidth;
	else
		nPadSize = m_nImgHeight;

	IppiSize roiImg = { m_nImgWidth, m_nImgHeight };

	m_nTotalPyrLevel = (int)ceil(log((float)nPadSize) / log((float)2)) + 1;	// pyramid 단계 계산, 해상도가 2000이면 pPyrLevel = 12, 원본 포함 12단계.
	// Pyramid 영상 해상도
	if (m_nTotalPyrLevel < 0)
		//goto RETURN_FALSE;
		return rtn;

	SAFE_DELETE_ARRAY(m_pPyrWidthArray);
	SAFE_DELETE_ARRAY(m_pPyrHeightArray);

	m_pPyrWidthArray = new int[m_nTotalPyrLevel];
	m_pPyrHeightArray = new int[m_nTotalPyrLevel];
	m_pPyrWidthArray[0] = m_nImgWidth;
	m_pPyrHeightArray[0] = m_nImgHeight;
	for (int level = 1; level < m_nTotalPyrLevel; level++)
	{
		m_pPyrWidthArray[level] = (m_pPyrWidthArray[level - 1] + 1) / 2;
		m_pPyrHeightArray[level] = (m_pPyrHeightArray[level - 1] + 1) / 2;
	}

	// Gaussian pyramid buffer
	Ipp32f** p32f_GaussPyrImageArray = new Ipp32f * [m_nTotalPyrLevel];
	int* pSB_p32f_GaussPyrImageArray = new int[m_nTotalPyrLevel];
	p32f_GaussPyrImageArray[0] = ippiMalloc_32f_C1(m_pPyrWidthArray[0], m_pPyrHeightArray[0], &pSB_p32f_GaussPyrImageArray[0]);
	ippiCopy_32f_C1R(m_p32f_Buf, m_SB_p32f_Buf, p32f_GaussPyrImageArray[0], pSB_p32f_GaussPyrImageArray[0], roiImg);

	// Laplacian pyramid buffer	
	SAFE_DELETE_ARRAY(m_pSB_m_p32f_LaplacianPyrImageArray);

	if (m_p32f_LaplacianPyrImageArray)
	{
		for (int level = 0; level < m_nTotalPyrLevel - 1; level++)
		{
			ippiFree(m_p32f_LaplacianPyrImageArray[level]);
			m_p32f_LaplacianPyrImageArray[level] = NULL;
		}
		delete[] m_p32f_LaplacianPyrImageArray;
	}
	m_p32f_LaplacianPyrImageArray = new Ipp32f * [m_nTotalPyrLevel - 1];	// 마지막 1x1 픽셀 단계에는 Laplacian pyramid가 생성되지 않으므로 nTotalLevel-1 까지 생성
	m_pSB_m_p32f_LaplacianPyrImageArray = new int[m_nTotalPyrLevel - 1];

	// Gaussan pyramid level 1부터 pPyrLevel까지 생성
	// Laplacian pyramid level 0부터 pPyrLevel-1까지 생성
	IppStatus status;
	for (int level = 1; level < m_nTotalPyrLevel; level++)
	{
		//{ Gaussian pyramid 생성
		p32f_GaussPyrImageArray[level] = ippiMalloc_32f_C1(m_pPyrWidthArray[level], m_pPyrHeightArray[level], &pSB_p32f_GaussPyrImageArray[level]);
		int nDownBufSize;
		status = ippiPyrDownGetBufSize_Gauss5x5(m_pPyrWidthArray[level - 1], ipp32f, 1, &nDownBufSize);

		Ipp8u* pBufDown = new Ipp8u[nDownBufSize];

		IppiSize roiSrcImg = { m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1] };
		status = ippiPyrDown_Gauss5x5_32f_C1R(p32f_GaussPyrImageArray[level - 1], pSB_p32f_GaussPyrImageArray[level - 1],
			p32f_GaussPyrImageArray[level], pSB_p32f_GaussPyrImageArray[level], roiSrcImg, pBufDown);

		//{ Laplacian pyramid 생성을 위해 Gaussian pyramid를 upsample하여 p32f_TempPyrImage에 임시 저장
		int nUpBufSize;
		status = ippiPyrUpGetBufSize_Gauss5x5(m_pPyrWidthArray[level], ipp32f, 1, &nUpBufSize);

		Ipp8u* pBufUp = new Ipp8u[nUpBufSize];

		Ipp32f* p32f_TempPyrImage;
		p32f_TempPyrImage = NULL;
		int SB_p32f_TempPyrImage;

		IppiSize roiDesImg = { m_pPyrWidthArray[level], m_pPyrHeightArray[level] };
		p32f_TempPyrImage = ippiMalloc_32f_C1(m_pPyrWidthArray[level] * 2, m_pPyrHeightArray[level] * 2, &SB_p32f_TempPyrImage);
		status = ippiPyrUp_Gauss5x5_32f_C1R(p32f_GaussPyrImageArray[level], pSB_p32f_GaussPyrImageArray[level], p32f_TempPyrImage, SB_p32f_TempPyrImage, roiDesImg, pBufUp);

		//{ Upsample한 영상과 해당 level의 Gaussian pyramid 영상의 차를 구하여 Laplacian pyramid 생성
		int nTrayCount = 0;
		while (1)
		{
			m_p32f_LaplacianPyrImageArray[level - 1] = ippiMalloc_32f_C1(m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1],
				&m_pSB_m_p32f_LaplacianPyrImageArray[level - 1]);

			if (m_p32f_LaplacianPyrImageArray[level - 1] != NULL)
				break;
			else
			{
				if (nTrayCount >= 5)
				{
					return FALSE;
				}
				nTrayCount++;
			}
		}


		status = ippiSub_32f_C1R(p32f_TempPyrImage, SB_p32f_TempPyrImage,
			p32f_GaussPyrImageArray[level - 1], pSB_p32f_GaussPyrImageArray[level - 1],
			m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], roiSrcImg);

		ippiFree(p32f_TempPyrImage);

		SAFE_DELETE_ARRAY(pBufUp);
		SAFE_DELETE_ARRAY(pBufDown);
	}
	m_fBaseLevelValue = p32f_GaussPyrImageArray[m_nTotalPyrLevel - 1][0];

	SAFE_DELETE_ARRAY(pSB_p32f_GaussPyrImageArray);

	if (p32f_GaussPyrImageArray)
	{
		for (int level = 0; level < m_nTotalPyrLevel; level++)
		{
			ippiFree(p32f_GaussPyrImageArray[level]);
		}
		SAFE_DELETE_ARRAY(p32f_GaussPyrImageArray);
	}

	rtn = TRUE;

	//RETURN_FALSE:;
	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::CalcPower(Ipp32f* p32f_Buf, int SB_p32f_Buf, float fPower, IppiSize roiSize)
{
	ippiLn_32f_C1IR(p32f_Buf, SB_p32f_Buf, roiSize);
	ippiMulC_32f_C1IR(fPower, p32f_Buf, SB_p32f_Buf, roiSize);
	ippiExp_32f_C1IR(p32f_Buf, SB_p32f_Buf, roiSize);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyPyramidCoefficient(float a, float M, float p, float c0, float fe, int ne, float fl, int nl)
{
	BOOL rtn = FALSE; // SPIE Vol. 2167 Image Processing (1994) / 551
	float ae, al;
	float xc0 = c0 / M; // 5.0 / 80.0 => 0.0625
	float fTemp = (float)pow(xc0, p) / xc0;
	float fCoefTemp = M * a;
	int L = m_nTotalPyrLevel;
	for (int level = L - 1; level > 0; level--) {
		float p2 = p;
		if (level < ne + 1)	ae = (float)pow(fe, 1 - (level - 1) / (float)ne); // 논문의 식 4.8. level은 논문의 k (scale index)
		else				ae = 1;
		if (level < L - nl)	al = 1;
		else				al = (float)pow(fl, (L - nl - level - 1) / (float)nl); // 논문의 식 4.9
		//if ((level == 1) || (level == 2))  { ae = p2 = 1; }//1~2번 layer 강조 안하기위해서?
		if (level == 1) { ae = p2 = 1; }//1~2번 layer 강조 안하기위해서?

		IppiSize roiImg = { m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1] };
		ippiDivC_32f_C1IR(M, m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], roiImg);

		Ipp32f* p32f_PyrPlus;		int pSB_p32f_PyrPlus;	p32f_PyrPlus = ippiMalloc_32f_C1(m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1], &pSB_p32f_PyrPlus);		ippiSet_32f_C1R(0, p32f_PyrPlus, pSB_p32f_PyrPlus, roiImg);
		Ipp8u* p8u_PyrPlusMask;	int pSB_p8u_PyrPlusMask; p8u_PyrPlusMask = ippiMalloc_8u_C1(m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1], &pSB_p8u_PyrPlusMask); ippiSet_8u_C1R(0, p8u_PyrPlusMask, pSB_p8u_PyrPlusMask, roiImg);
		Ipp32f* p32f_PyrMinus;		int pSB_p32f_PyrMinus;	p32f_PyrMinus = ippiMalloc_32f_C1(m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1], &pSB_p32f_PyrMinus);	ippiSet_32f_C1R(0, p32f_PyrMinus, pSB_p32f_PyrMinus, roiImg);
		Ipp8u* p8u_PyrMinusMask;	int pSB_p8u_PyrMinusMask; p8u_PyrMinusMask = ippiMalloc_8u_C1(m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1], &pSB_p8u_PyrMinusMask); ippiSet_8u_C1R(0, p8u_PyrMinusMask, pSB_p8u_PyrMinusMask, roiImg);

		ippiThreshold_LTVal_32f_C1R(m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], p32f_PyrPlus, pSB_p32f_PyrPlus, roiImg, xc0, 0); // pixel = pixel <= threshold ? 0 : pixel
		ippiCompareC_32f_C1R(p32f_PyrPlus, pSB_p32f_PyrPlus, xc0 / 10, p8u_PyrPlusMask, pSB_p8u_PyrPlusMask, roiImg, ippCmpGreaterEq);

		ippiThreshold_GTVal_32f_C1R(m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], p32f_PyrMinus, pSB_p32f_PyrMinus, roiImg, -xc0, 0); // pixel = pixel >= threshold ? 0 : pixel
		ippiCompareC_32f_C1R(p32f_PyrMinus, pSB_p32f_PyrMinus, -xc0 / 10, p8u_PyrMinusMask, pSB_p8u_PyrMinusMask, roiImg, ippCmpLessEq);

		ippiThreshold_LTValGTVal_32f_C1IR(m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], roiImg, -xc0, 0, xc0, 0);

		CalcPower(p32f_PyrPlus, pSB_p32f_PyrPlus, p2, roiImg);

		ippiAbs_32f_C1IR(p32f_PyrMinus, pSB_p32f_PyrMinus, roiImg);
		CalcPower(p32f_PyrMinus, pSB_p32f_PyrMinus, p2, roiImg);
		ippiMulC_32f_C1IR(-1, p32f_PyrMinus, pSB_p32f_PyrMinus, roiImg);

		ippiMulC_32f_C1IR(fTemp, m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], roiImg);  // m_p32f_LaplacianPyrImageArray *= fTemp

		ippiCopy_32f_C1MR(p32f_PyrPlus, pSB_p32f_PyrPlus, m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], roiImg, p8u_PyrPlusMask, pSB_p8u_PyrPlusMask);
		ippiCopy_32f_C1MR(p32f_PyrMinus, pSB_p32f_PyrMinus, m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], roiImg, p8u_PyrMinusMask, pSB_p8u_PyrMinusMask);

		ippiMulC_32f_C1IR(fCoefTemp * ae * al, m_p32f_LaplacianPyrImageArray[level - 1], m_pSB_m_p32f_LaplacianPyrImageArray[level - 1], roiImg);

		ippiFree(p32f_PyrPlus);
		ippiFree(p8u_PyrPlusMask);
		ippiFree(p32f_PyrMinus);
		ippiFree(p8u_PyrMinusMask);
	}
	rtn = TRUE;
	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ReconstructPyrImage(void)
{
	BOOL rtn = FALSE;

	Ipp32f** p32f_TempPyrImageArray = new Ipp32f * [m_nTotalPyrLevel];
	int* pSB_p32f_TempPyrImageArray = new int[m_nTotalPyrLevel];
	p32f_TempPyrImageArray[m_nTotalPyrLevel - 1] = ippiMalloc_32f_C1(m_pPyrWidthArray[m_nTotalPyrLevel - 1] * 2, m_pPyrHeightArray[m_nTotalPyrLevel - 1] * 2, &pSB_p32f_TempPyrImageArray[m_nTotalPyrLevel - 1]);
	p32f_TempPyrImageArray[m_nTotalPyrLevel - 1][0] = m_fBaseLevelValue;
	for (int level = m_nTotalPyrLevel - 1; level > 0; level--)
	{
		p32f_TempPyrImageArray[level - 1] = ippiMalloc_32f_C1(m_pPyrWidthArray[level] * 2, m_pPyrHeightArray[level] * 2, &pSB_p32f_TempPyrImageArray[level - 1]);

		int nUpBufSize;
		ippiPyrUpGetBufSize_Gauss5x5(m_pPyrWidthArray[level], ipp32f, 1, &nUpBufSize);
		Ipp8u* pBufUp = new Ipp8u[nUpBufSize];

		IppiSize roiLowerLevelImg = { m_pPyrWidthArray[level], m_pPyrHeightArray[level] };
		IppiSize roiUpperLevelImg = { m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1] };

		ippiPyrUp_Gauss5x5_32f_C1R(p32f_TempPyrImageArray[level],
			pSB_p32f_TempPyrImageArray[level],
			p32f_TempPyrImageArray[level - 1],
			pSB_p32f_TempPyrImageArray[level - 1],
			roiLowerLevelImg,
			pBufUp);

		ippiAdd_32f_C1R(m_p32f_LaplacianPyrImageArray[level - 1],
			m_pSB_m_p32f_LaplacianPyrImageArray[level - 1],
			p32f_TempPyrImageArray[level - 1],
			pSB_p32f_TempPyrImageArray[level - 1],
			p32f_TempPyrImageArray[level - 1],
			pSB_p32f_TempPyrImageArray[level - 1],
			roiUpperLevelImg);

		SAFE_DELETE_ARRAY(pBufUp);
	}

	IppiSize roiImg = { m_nImgWidth, m_nImgHeight };
	ippiCopy_32f_C1R(p32f_TempPyrImageArray[0], pSB_p32f_TempPyrImageArray[0], m_p32f_Buf, m_SB_p32f_Buf, roiImg);

	SAFE_DELETE_ARRAY(pSB_p32f_TempPyrImageArray);

	if (p32f_TempPyrImageArray)
	{
		for (int level = 0; level < m_nTotalPyrLevel - 1; level++)
		{
			ippiFree(p32f_TempPyrImageArray[level]);
		}
		SAFE_DELETE_ARRAY(p32f_TempPyrImageArray);
	}

	SAFE_DELETE_ARRAY(m_pSB_m_p32f_LaplacianPyrImageArray);

	if (m_p32f_LaplacianPyrImageArray)
	{
		for (int level = 0; level < m_nTotalPyrLevel - 1; level++)
		{
			ippiFree(m_p32f_LaplacianPyrImageArray[level]);
			m_p32f_LaplacianPyrImageArray[level] = NULL;
		}
		SAFE_DELETE_ARRAY(m_p32f_LaplacianPyrImageArray);
	}

	rtn = TRUE;

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::GetMinMaxUsingDownsampledImage(Ipp32f* p32f_Image, int SB_p32f_Image, Ipp8u* p8u_MaskImage_, int SB_p8u_MaskImage_,
	int nLayerW2, int nLayerW1, float* pMin, float* pMax)
{
	BOOL rtn = FALSE;

	Ipp32f* p32f_MaskImage = NULL;
	int SB_p32f_MaskImage;
	p32f_MaskImage = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &SB_p32f_MaskImage);
	ippiConvert_8u32f_C1R(p8u_MaskImage_, SB_p8u_MaskImage_, p32f_MaskImage, SB_p32f_MaskImage, org_roiImg);

	int nLayer = max(nLayerW2 + 1, 3);

	int nPadSize;
	if (m_nImgWidth > m_nImgHeight)
		nPadSize = m_nImgWidth;
	else
		nPadSize = m_nImgHeight;
	m_nTotalPyrLevel = (int)ceil(log((float)nPadSize) / log((float)2)) + 1;

	if (m_nTotalPyrLevel < 0)
		//goto RETURN_FALSE;
		return rtn;

	//Gaussian pyramid
	SAFE_DELETE_ARRAY(m_pPyrWidthArray);
	SAFE_DELETE_ARRAY(m_pPyrHeightArray);

	m_pPyrWidthArray = new int[m_nTotalPyrLevel];
	m_pPyrHeightArray = new int[m_nTotalPyrLevel];
	m_pPyrWidthArray[0] = m_nImgWidth;
	m_pPyrHeightArray[0] = m_nImgHeight;
	for (int level = 1; level < m_nTotalPyrLevel; level++)
	{
		m_pPyrWidthArray[level] = (m_pPyrWidthArray[level - 1] + 1) / 2;
		m_pPyrHeightArray[level] = (m_pPyrHeightArray[level - 1] + 1) / 2;
	}

	// Gaussian pyramid buffer
	Ipp32f** p32f_GaussPyrImageArray = new Ipp32f * [nLayer];
	int* pSB_p32f_GaussPyrImageArray = new int[nLayer];
	p32f_GaussPyrImageArray[0] = ippiMalloc_32f_C1(m_pPyrWidthArray[0], m_pPyrHeightArray[0], &pSB_p32f_GaussPyrImageArray[0]); // 

	//{ LCY-151210-MOD: masking 안한 영상 사용하던 버그 수정
	ippiMul_32f_C1R(p32f_Image, SB_p32f_Image, p32f_MaskImage, SB_p32f_MaskImage, p32f_GaussPyrImageArray[0], pSB_p32f_GaussPyrImageArray[0], org_roiImg);


	Ipp8u* p8u_MaskImageInv = NULL;
	int SB_p8u_MaskImageInv;
	p8u_MaskImageInv = ippiMalloc_8u_C1(m_nImgWidth, m_nImgHeight, &SB_p8u_MaskImageInv);
	ippiMulC_8u_C1RSfs(p8u_MaskImage_, SB_p8u_MaskImage_, 255, p8u_MaskImageInv, SB_p8u_MaskImageInv, org_roiImg, 0);
	ippiNot_8u_C1IR(p8u_MaskImageInv, SB_p8u_MaskImageInv, org_roiImg);

	Ipp64f fMean;
	ippiMean_32f_C1MR(p32f_Image, SB_p32f_Image, p8u_MaskImage_, SB_p8u_MaskImage_, org_roiImg, &fMean);
	ippiSet_32f_C1MR((Ipp32f)fMean, p32f_GaussPyrImageArray[0], pSB_p32f_GaussPyrImageArray[0], org_roiImg, p8u_MaskImageInv, SB_p8u_MaskImageInv);

	IppStatus	status;
	for (int level = 1; level < nLayer; level++)
	{
		//{ Gaussian pyramid 생성
		p32f_GaussPyrImageArray[level] = ippiMalloc_32f_C1(m_pPyrWidthArray[level], m_pPyrHeightArray[level], &pSB_p32f_GaussPyrImageArray[level]);
		int nDownBufSize;
		status = ippiPyrDownGetBufSize_Gauss5x5(m_pPyrWidthArray[level - 1], ipp32f, 1, &nDownBufSize);

		Ipp8u* pBufDown = new Ipp8u[nDownBufSize];
		IppiSize roiSrcImg = { m_pPyrWidthArray[level - 1], m_pPyrHeightArray[level - 1] };
		status = ippiPyrDown_Gauss5x5_32f_C1R(p32f_GaussPyrImageArray[level - 1], pSB_p32f_GaussPyrImageArray[level - 1],
			p32f_GaussPyrImageArray[level], pSB_p32f_GaussPyrImageArray[level], roiSrcImg, pBufDown);
		//}

		SAFE_DELETE_ARRAY(pBufDown);
	}

	float fSrcMin, fSrcMax;
	IppiSize  roiTemp = { m_pPyrWidthArray[nLayerW2], m_pPyrHeightArray[nLayerW2] };

	ippiMax_32f_C1R(p32f_GaussPyrImageArray[nLayerW2], pSB_p32f_GaussPyrImageArray[nLayerW2], roiTemp, &fSrcMax);

	roiTemp.width = m_pPyrWidthArray[nLayerW1]; roiTemp.height = m_pPyrHeightArray[nLayerW1];
	ippiMin_32f_C1R(p32f_GaussPyrImageArray[nLayerW1], pSB_p32f_GaussPyrImageArray[nLayerW1], roiTemp, &fSrcMin);

	SAFE_DELETE_ARRAY(pSB_p32f_GaussPyrImageArray);

	if (p32f_GaussPyrImageArray)
	{
		for (int level = 0; level < nLayer; level++)
		{
			ippiFree(p32f_GaussPyrImageArray[level]);
		}
		SAFE_DELETE_ARRAY(p32f_GaussPyrImageArray);
	}

	if (p32f_MaskImage)
		ippiFree(p32f_MaskImage);

	if (p8u_MaskImageInv)
		ippiFree(p8u_MaskImageInv);

	*pMin = fSrcMin;
	*pMax = fSrcMax;

	rtn = TRUE;

	//RETURN_FALSE:;

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ApplyGammaCurve(Ipp32f* p32f_Buf, int SB_p32f_Buf, float fGamma, float fInW1, float fInW2, float fOutW1, float fOutW2)
{
	//
	//	  (x-fInW1)  fGamma										// LCY-120927-4: 
	// ( ------------ )   x  (fOutW2-fOutW1)  +  fOutW1			//	=> x-fInW1 부분에서 fInW1이 int 형이었어서 음수가 나오는 버그 발생!!!
	//  (fInW2-fInW1)
	//
	// 

	BOOL rtn = FALSE;

	if (fInW2 == fInW1)		// nInW1과 nInW2가 같으면 ippiDivC_32f_C1IR에서 제수가 0이 되어버림.
		rtn = FALSE;

	fOutW1 = fInW1;
	fOutW2 = fInW2;   // 같은 값 사용중임

	IppStatus status;

	status = ippiSubC_32f_C1IR(fInW1, p32f_Buf, SB_p32f_Buf, org_roiImg);
	if (status < 0)
	{
		rtn = FALSE;
	}
	status = ippiDivC_32f_C1IR((float)(fInW2 - fInW1), p32f_Buf, SB_p32f_Buf, org_roiImg);
	if (status < 0)
	{
		rtn = FALSE;
	}

	CalcPower(p32f_Buf, SB_p32f_Buf, fGamma, org_roiImg);


	status = ippiMulC_32f_C1IR((fOutW2 - fOutW1), p32f_Buf, SB_p32f_Buf, org_roiImg);
	if (status < 0)
	{
		rtn = FALSE;
	}
	status = ippiAddC_32f_C1IR(fOutW1, p32f_Buf, SB_p32f_Buf, org_roiImg);
	if (status < 0)
	{
		rtn = FALSE;
	}

	rtn = TRUE;

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ScaleTo14bit(void)
{
	return ScaleTo1xbit();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::ScaleTo1xbit(int n)
{
	float value = 4095.f;
	switch (n)
	{
	case 2:
		value = 4095;
		break;
	case 4:
		value = 16383;
		break;
	case 6:
		value = 65535;
		break;
	}

	BOOL rtn = FALSE;

	IppStatus status;

	// pixel값이 0인 위치에 대해 mask를 생성하고 dilation하여 mask를 키움.
	Ipp8u* p8u_bufMask;	int SB_p8u_bufMask;	p8u_bufMask = ippiMalloc_8u_C1(m_nImgWidth, m_nImgHeight, &SB_p8u_bufMask);
	ippiSet_8u_C1R(0, p8u_bufMask, SB_p8u_bufMask, org_roiImg);
	ippiCompareC_32f_C1R(m_p32f_Buf, m_SB_p32f_Buf, 1, p8u_bufMask, SB_p8u_bufMask, org_roiImg, ippCmpLess);

	Ipp8u* p8u_bufMaskTmp;	int SB_p8u_bufMaskTmp;	p8u_bufMaskTmp = ippiMalloc_8u_C1(m_nImgWidth, m_nImgHeight, &SB_p8u_bufMaskTmp);
	ippiCopy_8u_C1R(p8u_bufMask, SB_p8u_bufMask, p8u_bufMaskTmp, SB_p8u_bufMaskTmp, org_roiImg);

	// 굳이 왜 temp를 쓰는거지?
	// dilation 10번 수행
	IppiSize roiDilation = { m_nImgWidth - 2, m_nImgHeight - 2 };
	for (int iter = 0; iter < 10; iter++)
		ippiDilate3x3_8u_C1IR(p8u_bufMaskTmp + SB_p8u_bufMaskTmp + 1, SB_p8u_bufMaskTmp, roiDilation);

	//{ LCY-20151217-MOD: Log적용시 ROI 영역내의 min, max값 이용하도록 수정, normilize이후 min, max 점위 밖 threshold 처리.
	Ipp32f* p32f_bufImgTmp;	int SB_p32f_bufImgTmp;
	p32f_bufImgTmp = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &SB_p32f_bufImgTmp);
	ippiCopy_32f_C1R(m_p32f_Buf, m_SB_p32f_Buf, p32f_bufImgTmp, SB_p32f_bufImgTmp, org_roiImg);

	// mask 영역 밖의 부분에서 최소값을 찾기 위해 mask 영역의 값을 10으로 set. (ln 적용 이후이기 때문에 10이란 값이 작은 값이 아님. ln이전 약 22026.4)
	// 값바꿔도 다 똑같은듯? 0 5 10 100 1000 동일함
	ippiSet_32f_C1MR(10, p32f_bufImgTmp, SB_p32f_bufImgTmp, org_roiImg, p8u_bufMaskTmp, SB_p8u_bufMaskTmp); // mask 없어서 동작 x


	float fMaxLevel = 0;
	float fMinLevel = 0;

	status = ippiMin_32f_C1R(m_p32f_Buf + m_nHistogramLeftoffset + m_nHistogramTopoffset * m_SB_p32f_Buf / 4, m_SB_p32f_Buf, org_roiImgHisto, &fMinLevel);

	status = ippiSubC_32f_C1IR(fMinLevel, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg); // 왜필요한거지??

	Ipp32f* p32f_ResizebufImg;	int SB_p32f_ResizebufImg;
	int nResizeWidth = m_nImgWidth / 2;
	int nResizeHeight = m_nImgHeight / 2;
	p32f_ResizebufImg = ippiMalloc_32f_C1(nResizeWidth, nResizeHeight, &SB_p32f_ResizebufImg);

	Resize(m_p32f_Buf, m_nImgWidth, m_nImgHeight, p32f_ResizebufImg, nResizeWidth, nResizeHeight);

	int nPreTerm = 3;
	IppiSize roimed = { nResizeWidth - (nPreTerm - 1), nResizeHeight - (nPreTerm - 1) };
	IppiSize mask_med = { nPreTerm, nPreTerm };
	IppiPoint anchor_med = { nPreTerm / 2, nPreTerm / 2 };

	status = ippiFilterMedian_32f_C1R(p32f_ResizebufImg + anchor_med.x + anchor_med.y * (SB_p32f_ResizebufImg / 4),
		SB_p32f_ResizebufImg,
		p32f_ResizebufImg + anchor_med.x + anchor_med.y * (SB_p32f_ResizebufImg / 4),
		SB_p32f_ResizebufImg, roimed, mask_med, anchor_med);

	//status = ippiMax_32f_C1R(m_p32f_Buf + m_nHistogramLeftoffset + m_nHistogramTopoffset*m_SB_p32f_Buf / 4, m_SB_p32f_Buf, org_roiImgHisto, &fMaxLevel);
	status = ippiMax_32f_C1R(p32f_ResizebufImg + nPreTerm - 1 + (nPreTerm - 1) * SB_p32f_ResizebufImg / 4, SB_p32f_ResizebufImg, roimed, &fMaxLevel);

	if (fMaxLevel > 0.000001)
	{
		//{ normalize
		status = ippiDivC_32f_C1IR(fMaxLevel, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);

		status = ippiMulC_32f_C1IR(value, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);
	}

	ippiThreshold_LTVal_32f_C1IR(m_p32f_Buf, m_SB_p32f_Buf, org_roiImg, 0, 0);
	ippiThreshold_GTVal_32f_C1IR(m_p32f_Buf, m_SB_p32f_Buf, org_roiImg, value, value);

	ippiFree(p8u_bufMask);
	ippiFree(p8u_bufMaskTmp);
	ippiFree(p32f_bufImgTmp);

	rtn = TRUE;

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::Resize(Ipp32f* scr, int nSrcWidth, int nSrcHeight,
	Ipp32f* dst, int nDesWidth, int nDesHeight)
{
	UNREFERENCED_PARAMETER(nSrcHeight);

	BOOL rtn = FALSE;

	for (int itery = 0; itery < nDesHeight; itery++)
	{
		for (int iterx = 0; iterx < nDesWidth; iterx++)
		{
			float fTempValue = *(scr + 2 * iterx + 2 * itery * nSrcWidth);
			*(dst + iterx + itery * nDesWidth) = fTempValue;
		}
	}

	//#ifdef  USE_FILESAVE
	//	float* pResultRawBuf = new float[nDesWidth*nDesHeight];
	//
	//	for (int itery = 0; itery < nDesHeight; itery++)
	//		memcpy(pResultRawBuf + itery*nDesWidth, (char*)dst + itery*SB_dst, nDesWidth * sizeof(dst[0]));
	//
	//	QString file = "D:\\t\\Resize.tif";
	//	QByteArray tempFile = file.toUtf8();
	//
	//	FILE* fp;
	//	fp = fopen(tempFile.constData(), "wb");
	//	if (fp == NULL) return false;
	//	fwrite(pResultRawBuf, nDesWidth*nDesHeight, sizeof(float), fp);
	//	fclose(fp);
	//
	//	if (pResultRawBuf)
	//		delete[] pResultRawBuf;
	//#endif
	rtn = TRUE;

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::FillMaskAreaGeX_CR(unsigned short* pRawBuf)
{
	UNREFERENCED_PARAMETER(pRawBuf);
	SAFE_DELETE_ARRAY(m_pMaskImage);
	m_pMaskImage = new char[m_nImgWidth * m_nImgHeight];
	memset(m_pMaskImage, 0, sizeof(char) * m_nImgWidth * m_nImgHeight);
	// 2024-03-13. jg kim. IP테두리가 희게 보이는 이슈(# 17807) 대응. 이전에 구현하였으나 누락되어 이번에 반영
	int nCrop = 0;
	for (int y = nCrop; y < m_nImgHeight - nCrop; y++)
	{
		for (int x = nCrop; x < m_nImgWidth - nCrop; x++)
		{
			int idx = y * m_nImgWidth + x;
			if (pRawBuf[idx] > 1000) {// 2024-11-20. jg kim. threshold를 사용하여  마스크를 만드는 방법은 위험하기 때문에 개선 요망
				m_pMaskImage[idx] = 1;
			}
		}
	}
	return 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::RestoreMaskArea(int nMaskValue)
{
	UNREFERENCED_PARAMETER(nMaskValue);

	BOOL rtn = FALSE;

	if (m_pMaskImage)
	{
		int l, t, w, h;
		 // 2024-04-09. jg kim. 품혁 이슈 대응을 위해 변경
		// 2024-04-17. jg kim.  회의 결론으로 테두리를 검정색으로 표시하지 않기로하여 변경
		//getBoundingRect(l, t, w, h);
		l = 0; t = 0; w = m_nImgWidth; h = m_nImgHeight;
		//int index = guessIpIdx(l, t, w, h);
		//l+=2;
		//t+=2;
		//w-=4;
		//h-=4;
		//
		//int ip_rad = int(ip[index].rad);//280
		//// 2024-03-13. jg kim. 저해상도 영상 관련 이슈 (이슈 #17835) 대응.
		//// 그 동안 ippi 메모리 할당, 복사를 잘못해 왔으나 공교롭게 문제가 되지 않았었다.
		//if (m_bLowResolution)
		//	ip_rad /= 2;
		//int powR = int(ip_rad) * int(ip_rad);

		Ipp32f* pMask = NULL;
		int SB_p32f_MaskImage;
		pMask = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &SB_p32f_MaskImage);

		//int cx, cy;
		for (int y = t; y < t+h; y++)
		{
			for (int x = l; x < l+w; x++)
				*(pMask + y * m_SB_p32f_Buf / 4 + x) = 1;

			//if (y <t+ip_rad) {
			//	cy = ip_rad+t;
			//	for (int x = l; x < l+ip_rad; x++) {// L,T
			//		cx = l+ip_rad;
			//		if ((cx - x)*(cx - x) + (cy - y)*(cy - y) > powR)
			//			*(pMask + y * m_SB_p32f_Buf / 4 + x) = 0;
			//	}
			//	for (int x = l+w - ip_rad; x<m_nImgWidth; x++) { // R,T
			//		cx = l+w - ip_rad;
			//		if ((cx - x)*(cx - x) + (cy - y)*(cy - y) > powR)
			//			*(pMask + y * m_SB_p32f_Buf / 4 + x ) = 0;
			//	}
			//} else if (y > t+h - ip_rad - 1) {// L,B
			//	cy = t+h - ip_rad;
			//	for (int x = l; x < l+ip_rad; x++) {
			//		cx = l+ip_rad;
			//		if ((cx - x)*(cx - x) + (cy - y)*(cy - y) > powR)
			//			*(pMask + y * m_SB_p32f_Buf / 4 + x) = 0;
			//	}
			//	for (int x = l+w - ip_rad; x<m_nImgWidth; x++) { // R,B
			//		cx = l+w - ip_rad;
			//		if ((cx - x)*(cx - x) + (cy - y)*(cy - y) > powR)
			//			*(pMask + y * m_SB_p32f_Buf / 4 + x ) = 0;
			//	}
			//}
		}
		

		ApplyGaussianBlur(pMask);

		for (int y = 0; y < m_nImgHeight; y++)
		{
			for (int x = 0; x < m_nImgWidth; x++)
			{
				int n = y * m_SB_p32f_Buf / 4 + x ;
				// 2024-03-28. jg kim. Blanck scan을 할 경우 버퍼를 0으로 채워 리턴 하는데,
				// 이런 경우 분자를 0으로 나누는 경우 발생 (NAN 생성).
				// 분모가 0인 경우에 대한 예외처리 추가. 
				if(m_fSrcMax - m_fSrcMin == 0) 
					m_p32f_Buf[n] = 0;
				else
					m_p32f_Buf[n] = (m_p32f_Buf[n] - m_fSrcMin) / (m_fSrcMax - m_fSrcMin)  * pMask[n] * 65535;
			}
		}


		SAFE_DELETE_ARRAY(m_pMaskImage);
		ippiFree(pMask);
		pMask = NULL;
		rtn = TRUE;
	}

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
int CMceIppi::guessIpIdx(int left, int top, int width, int height)
{
	int shortSide = width - left + 1;
	int longSide = height - top + 1;

	int ipIdx = -1;
	float ratio = 0;
	for (int i = 0; i < 4; i++) {
		// ipSize의 width, height를 변경하며 r 값을 update하여 r이 최대가 되게 하는 ipIdx를 구한다.
		// 그 ipIdx가 촬영된 IP index 이다.
		int W = ip[i].width;
		int H = ip[i].height;
		float rw = W < shortSide ? (float)W / (float)shortSide : (float)shortSide / (float)W;
		float rh = H < longSide ? (float)H / (float)longSide : (float)longSide / (float)H;
		// W와 shortSide, H와 longSide와의 비율 rw, rh를 각각 구한다. 이 때 작은 값을 큰 값으로 나누어 항상 비율이 1 이하가 되도록 한다.
		//float r = rw > rh ? rw : rh; // rw, rh의 최대값 r을 구한다.
		float r = (rw + rh) / 2;
		if (r > ratio) {
			ratio = r;
			ipIdx = i;
		}
	}
	return ipIdx;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::getBoundingRect(int & l, int & t, int & width, int & height)
{
	// mirror uniformity
	width = m_nImgWidth;
	height = m_nImgHeight;

	l = -1;// left
	for (int x = 0; x < width; x++)
	{
		for (int y = 0; y < height; y++)
		{
			int n = y * width + x;
			if (m_pMaskImage[n] != 0)
			{
				l = x;	break;

			}
		}
		if (l > 0)
			break;
	}

	int r = -1;// right
	for (int x = width - 1; x > l; x--)
	{
		for (int y = 0; y < height; y++)
		{
			int n = y * width + x;
			if (m_pMaskImage[n] != 0)
			{
				r = x + 1;	break;
			}
		}
		if (r > 0)
			break;
	}

	t = -1;// top
	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int n = y * width + x;
			if (m_pMaskImage[n] != 0)
			{
				t = y;	break;
			}
		}
		if (t > 0)
			break;
	}

	int b = -1;// bottom
	for (int y = height - 1; y > t; y--)
	{
		for (int x = 0; x < width; x++)
		{
			int n = y * width + x;
			if (m_pMaskImage[n] != 0)
			{
				b = y + 1;	break;
			}
		}
		if (b > 0)
			break;
	}

	if (b >= height)
		b = height - 1;
	width = r - l;
	height = b - t;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::Convert16sRawToIpp32f(unsigned short* pRawBuf)
{
	BOOL rtn = FALSE;

	Ipp16u* p16u_OrgImage;
	p16u_OrgImage = NULL;
	int SB_p16u_OrgImage;
	p16u_OrgImage = ippiMalloc_16u_C1(m_nImgWidth, m_nImgHeight, &SB_p16u_OrgImage);

	for (int y = 0; y < m_nImgHeight; y++)
	{
		memcpy(((char*)p16u_OrgImage + y * (SB_p16u_OrgImage)), pRawBuf + m_nImgWidth * y, m_nImgWidth * sizeof(unsigned short));
	}

	int nWidth = m_nImgWidth;
	int nHeight = m_nImgHeight;

	int nWidthOffset = 4, nHeightOffset = 4;
	IppiSize roiImgHisto = { nWidth - 2 * nWidthOffset, nHeight - 2 * nHeightOffset };

	if (m_p32f_Org_Buf)
		ippiFree(m_p32f_Org_Buf);
	m_p32f_Org_Buf = NULL;
	m_p32f_Org_Buf = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &m_SB_p32f_Org_Buf);

	{
		IppStatus status;
		status = ippiConvert_16u32f_C1R(p16u_OrgImage, SB_p16u_OrgImage, m_p32f_Org_Buf, m_SB_p32f_Org_Buf, org_roiImg);
		if (status < 0)
		{
			goto RETURN_FALSE;
		}
	}

	if (p16u_OrgImage)
		ippiFree(p16u_OrgImage); 	// 더이상 사용하지 않으므로 해제.

	rtn = TRUE;

RETURN_FALSE:;

	return rtn;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::SaveIpp32f(char* file)
{
	if (!m_p32f_Buf)
	{
		assert(0);
		return false;
	}

	float* pResultRawBuf = new float[m_nImgWidth * m_nImgHeight];
	assert(pResultRawBuf != nullptr);

	for (int itery = 0; itery < m_nImgHeight; itery++)
		memcpy(pResultRawBuf + itery * m_nImgWidth, (char*)m_p32f_Buf + itery * m_SB_p32f_Buf, m_nImgWidth * sizeof(m_p32f_Buf[0]));

	FILE* fh = nullptr;
	auto nErr = fopen_s(&fh, file, "wb");
	if ( nErr == 0 && fh != nullptr )
	{
		// 파일의 절대 경로를 확인
		//std::filesystem::path absolutePath = std::filesystem::canonical(file);
		//OutputDebugStringA(absolutePath.string().c_str());

		fwrite(pResultRawBuf, sizeof(float), m_nImgWidth * m_nImgHeight, fh);
		fclose(fh);
	}
	else
	{
		assert(fh);
	}

	SAFE_DELETE_ARRAY(pResultRawBuf);

	return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::CopyOrgIpp32buftoIpp32buf(void)
{
	if (m_p32f_Buf)
		ippiFree(m_p32f_Buf);
	m_p32f_Buf = NULL;
	m_p32f_Buf = ippiMalloc_32f_C1(m_nImgWidth, m_nImgHeight, &m_SB_p32f_Buf);

	ippiCopy_32f_C1R(m_p32f_Org_Buf, m_SB_p32f_Org_Buf, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::SetHistogramOffset(int left, int top, int right, int bottom)
{
	m_nHistogramLeftoffset = left;
	m_nHistogramRightoffset = right;
	m_nHistogramTopoffset = top;
	m_nHistogramBottomoffset = bottom;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::SetImage(unsigned short* image, int Width, int Height)
{
	// 2024-03-13. jg kim. 저해상도 영상 관련 이슈 (이슈 #17835) 
	m_bLowResolution = false;
	int count = Width * Height;
	if (count == ip[0].width *ip[0].height / 4)
		m_bLowResolution = true;
	else if (count == ip[1].width *ip[1].height / 4)
		m_bLowResolution = true;
	else if (count == ip[2].width *ip[2].height / 4)
		m_bLowResolution = true;
	else if (count == ip[3].width *ip[3].height / 4)
		m_bLowResolution = true;
	else if (count == ip[0].width *ip[0].height)
		m_bLowResolution = false;
	else if (count == ip[1].width *ip[1].height)
		m_bLowResolution = false;
	else if (count == ip[2].width *ip[2].height)
		m_bLowResolution = false;
	else if (count == ip[3].width *ip[3].height)
		m_bLowResolution = false;
	else if(count == RAW_IMAGE_WIDTH * RAW_IMAGE_HEIGHT)
	{
		for (int i = 0; i < Width*Height; i++)
			image[i] = 0;
	}

	m_nImgWidth = Width;
	m_nImgHeight = Height;
	org_roiImg = { m_nImgWidth, m_nImgHeight };	// img 전체 영역
	SetHistogramOffset(g_params.nHistogramLeftoffset, g_params.nHistogramTopoffset, g_params.nHistogramRightoffset, g_params.nHistogramBottomoffset);
	// 2024-03-04. jg kim. 이게 누락되어 초기값(284,284,284,284)만 사용되었음
	org_roiImgHisto = { m_nImgWidth - (m_nHistogramLeftoffset + m_nHistogramRightoffset), m_nImgHeight - (m_nHistogramTopoffset + m_nHistogramBottomoffset) };

	FillMaskAreaGeX_CR(image);

	if (m_p8u_MaskImage) // m_pMaskImage
		ippiFree(m_p8u_MaskImage);
	m_p8u_MaskImage = NULL;
	m_p8u_MaskImage = ippiMalloc_8u_C1(m_nImgWidth, m_nImgHeight, &m_SB_p8u_MaskImage);

	ippiSet_8u_C1R(0, m_p8u_MaskImage, m_SB_p8u_MaskImage, org_roiImg);
	ippiSet_8u_C1R(1, m_p8u_MaskImage + m_nHistogramLeftoffset + m_nHistogramTopoffset * m_SB_p8u_MaskImage / sizeof(Ipp8u), m_SB_p8u_MaskImage, org_roiImgHisto);

	Convert16sRawToIpp32f(image);
	CopyOrgIpp32buftoIpp32buf();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::getProcessedImage(unsigned short* image, int& Width, int& Height)
{
	//m_p32f_Buf
	Ipp16u* p16u_buf;
	int SB_p16u_buf;
	p16u_buf = ippiMalloc_16u_C1(m_nImgWidth, m_nImgHeight, &SB_p16u_buf);
	ippiConvert_32f16u_C1R(m_p32f_Buf, m_SB_p32f_Buf, p16u_buf, SB_p16u_buf, org_roiImg, ippRndNear);


	for (int itery = 0; itery < m_nImgHeight; itery++)
		memcpy(image + itery * m_nImgWidth, (char*)p16u_buf + itery * SB_p16u_buf, m_nImgWidth * sizeof(p16u_buf[0]));
	Width = m_nImgWidth;
	Height = m_nImgHeight;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 

BOOL CMceIppi::ProcessMceIppi(IMG_PROCESS_PARAM ipp)
{
	// LCY-240818. 설정을 별도로 사용하기 때문에 해당 조건 삭제
	//// 2024-03-13. jg kim. 저해상도 영상 관련 이슈 (이슈 #17835) 
	//if (m_bLowResolution)
	//{
	//	m_nHistogramLeftoffset = ipp.nHistogramLeftoffset / 2;
	//	m_nHistogramRightoffset = ipp.nHistogramRightoffset / 2;
	//	m_nHistogramTopoffset = ipp.nHistogramTopoffset / 2;
	//	m_nHistogramBottomoffset = ipp.nHistogramBottomoffset / 2;
	//}
	//else
	//{
	//	m_nHistogramLeftoffset = ipp.nHistogramLeftoffset;
	//	m_nHistogramRightoffset = ipp.nHistogramRightoffset;
	//	m_nHistogramTopoffset = ipp.nHistogramTopoffset;
	//	m_nHistogramBottomoffset = ipp.nHistogramBottomoffset;
	//}

	char name[256];// 2024-04-26. jg kim. log 작성을 위해 추가
	char buf[256];
	bool res = false;
	char strPrefix[32] = "POST";
	if (m_bSaveImageProcessingStep)
	{
		sprintf_s(buf, "%s_01converted_%d-%d\n", strPrefix, m_nImgWidth, m_nImgHeight);
		sprintf_s(name, "%s_01_converted_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
		SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
	}

	// LCY-20240815-ADD: 영상 최소값(IP 외부의 가장 어두운 부분)을 획득하고, 여기에 100 더한 값을 기준으로 mask 생성
	float fSrcMin;
	ippiMin_32f_C1R(m_p32f_Buf, m_SB_p32f_Buf, org_roiImg, &fSrcMin);	
	fSrcMin = fSrcMin + 100;
	MetalThreshold(fSrcMin, 1);

	// LCY-20240815-MOD: IP 외곽 경계선 부분에 튀는 값이 있을 수 있어, 2개 낮은 layer에서 밝기 검출. 영상처리에 의한 halo artifact 같은 것이 없는 상태이므로, 밝기 offset 설정값 줄 필요 없음
	SetWindow(2, 0);	
	
	CopyOrgIpp32buftoIpp32buf();
	for (int n = 0; n < m_nImgWidth * m_nImgHeight; n++)
	{
		if (m_p32f_Buf[n] > m_fSrcMax)
			m_p32f_Buf[n] = m_fSrcMax;
	} // m_fSrcMax 보다 큰 튀는 pixel value처리

	if (ipp.bInvert)
	{
		res = ApplyInvert();

		if (m_bSaveImageProcessingStep)
		{
			sprintf_s(buf, "%s_02_inverted_%d-%d_%s\n", strPrefix, m_nImgWidth, m_nImgHeight,res?"Success":"Fail");
			sprintf_s(name, "%s_02_inverted_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
			SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
		}
	}

	if (ipp.nPreBlur == Median3x3)
	{
		res = ApplyMedian(3);
	}
	else if (ipp.nPreBlur == Median5x5)
	{
		res = ApplyMedian(5);
	}
	if (m_bSaveImageProcessingStep)
	{
		sprintf_s(buf, "%s_03_Blur_Median3x3_%d-%d_%s\n", strPrefix, m_nImgWidth, m_nImgHeight, res ? "Success" : "Fail");
		sprintf_s(name, "%s_03_Blur_Median3x3_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
		SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
	}

	res = ApplyBilateral(ipp.fSigmaD1, ipp.fSigmaR1);
	if (m_bSaveImageProcessingStep)
	{
		sprintf_s(buf, "%s_04_Bilateral_SigmaD1-%.3f, SigmaR1-%.3f_%d-%d_%s\n", strPrefix, ipp.fSigmaD1, ipp.fSigmaR1, m_nImgWidth, m_nImgHeight, res ? "Success" : "Fail");
		sprintf_s(name, "%s_04_Bilateral_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
		SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
	}

	// LCY-240815-DEL: GenX-CR은 전처리 단게에서 가로줄을 제거하기 때문에, 후처리 단계에서 가로줄 제거를 수행할 필요 없음. 
	// 				   차후 공용화를 위해 설정값으로 처리하도록 수정 필요
	//res = ApplyLinecorrect();
	//if (m_bSaveImageProcessingStep)
	//{
	//	sprintf_s(buf, "%s_05_Linecorrect_%d-%d_%s\n", strPrefix, m_nImgWidth, m_nImgHeight, res ? "Success" : "Fail");
	//	sprintf_s(name, "%s_05_Linecorrect_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
	//	SaveIpp32f(name);
	//	if (file_name_IP)// 2024-04-26. jg kim. log 작성을 위해 추가
	//		writelogIP(file_name_IP, buf);
	//}

	if (ipp.bApplyCLAHE) // 2024-08-18. jg kim. 기존에 무조건 실행하도록 구현되어 있어서 수정
	{
		_CLAHE(65535, ipp.nCLAHEBlockSize, 16383, ipp.fCLAHEClipLimit, EXPAND_CENTER);
		if (m_bSaveImageProcessingStep)
		{
			sprintf_s(buf, "%s_06_CLAHE_MaxIntensity-%d, BlockSize-%d, HistogramBins-%d, ClipLimit-%.3f, ExpandMode-%s_%d-%d\n", strPrefix,
				65535, ipp.nCLAHEBlockSize, 16383, ipp.fCLAHEClipLimit, "EXPAND_CENTER", m_nImgWidth, m_nImgHeight);
			sprintf_s(name, "%s_06_CLAHE_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
			SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
		}
	}	

	Mce_PARAM mp = ipp.mp;
	res = ApplyMusica(mp.fScaling, mp.fMaxDifference, mp.fExponential, mp.fc0,
		mp.fEdgeEnhancement, mp.nEdgeLayerNumb, mp.fLatitudeReduction, mp.nLatitudeLayerNumb);
	//// a, M, p, c0, fe, ne, fl, nl
	if (m_bSaveImageProcessingStep)
	{
		sprintf_s(buf, "%s_07_MCE_a-%.3f, m-%.3f, p-%.3f, c0-%.3f, fe-%.3f, ne-%d, fl-%.3f, nl-%d_%d-%d_%s\n", strPrefix,
			mp.fScaling, mp.fMaxDifference, mp.fExponential, mp.fc0,
			mp.fEdgeEnhancement, mp.nEdgeLayerNumb, mp.fLatitudeReduction, mp.nLatitudeLayerNumb, m_nImgWidth, m_nImgHeight
			, res ? "Success" : "Fail");
		sprintf_s(name, "%s_07_MCE_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
		SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
	}

	if (ipp.cImageFilterInfo.bApply) // 2024-03-04. jg kim. Unsharpmask 옵션 처리
	{
		ApplyUnsharpMask(ipp.cImageFilterInfo.nAmount, ipp.cImageFilterInfo.dRadius, ipp.cImageFilterInfo.nThreshold);
		if (m_bSaveImageProcessingStep)
		{
			sprintf_s(name, "%s_08_ApplyUnsharpMask_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
			SaveIpp32f(name);
		}
	}
	// 2024-02-26. jg kim. 함수가 잘못 들어가 비활성화 시킴.
	// 2024-02-27. jg kim. 특정 영상의 경우 gamma correction을 하면 NAN이 나와 복원.
	// 2024-02-27. jg kim. 하드코딩 되어 있었던 코드 수정.
	res = SetWindow(ipp.nPyramidLayer, ipp.fPyramidW2Offset);
	if (m_bSaveImageProcessingStep)
	{
		sprintf_s(buf, "%s_08_PyramidWindow Pyramid Layer-%d, Pyramid W2 Offset-%.3f_%d-%d_%s\n", strPrefix,
			ipp.nPyramidLayer, ipp.fPyramidW2Offset, m_nImgWidth, m_nImgHeight, res ? "Success" : "Fail");
		sprintf_s(name, "%s_08_PyramidWindow_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
		SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
	}
	sprintf_s(buf, "%s_09_Gamma_G-%.3f_%.3f-%.3f\n", strPrefix, ipp.fGamma, m_fSrcMin, m_fSrcMax);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);

	res = ApplyGamma(ipp.fGamma);
	if (m_bSaveImageProcessingStep)
	{
		sprintf_s(buf, "%s_09_Gamma_G-%.3f_%d-%d_%s\n", strPrefix, ipp.fGamma, m_nImgWidth, m_nImgHeight, res ? "Success" : "Fail");
		sprintf_s(name, "%s_09_Gamma_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
		SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
	}

	// LCY-20240815-MOD: ApplyGamma 수행전에 이미 downsample한 것에서 밝기를 잡았기 때문에, 본 단계에선 downsample에서 밝기 잡을 필요없이 그대로 잡아도 무방
	res = SetWindow(0, 0);
	//ippiMinMax_32f_C1R(m_p32f_Buf + m_nHistogramLeftoffset + m_nHistogramTopoffset * m_SB_p32f_Buf / 4, m_SB_p32f_Buf, org_roiImgHisto, &m_fSrcMin, &m_fSrcMax);


	sprintf_s(buf, "%s_10_SetWindow_%.3f-%.3f\n", strPrefix, m_fSrcMin, m_fSrcMax);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);

	if (m_fSrcMin < 0)
	{
		ippiSubC_32f_C1IR(m_fSrcMin, m_p32f_Buf, m_SB_p32f_Buf, org_roiImg);
		m_fSrcMax -= m_fSrcMin;
		m_fSrcMin = 0;
	}

	res = RestoreMaskArea(unsigned int(m_fSrcMin));
	if (m_bSaveImageProcessingStep)
	{
		sprintf_s(buf, "%s_10_RestoreMaskArea_Src min-%.3f_%d-%d_%s\n", strPrefix, m_fSrcMin, m_nImgWidth, m_nImgHeight, res ? "Success" : "Fail");
		sprintf_s(name, "%s_10_RestoreMaskArea_%d-%d.raw", strPrefix, m_nImgWidth, m_nImgHeight);
		SaveIpp32f(name);
		// 2026-02-02. jg kim. include의 logger를 사용하도록 수정
		writelog(buf, LOG_FILE_NAME);
	}
	return res;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
unsigned char* CMceIppi::ConvertIpp32fTo8uRaw(void)
{
	Ipp16u* p16u_buf;
	int SB_p16u_buf;
	p16u_buf = ippiMalloc_16u_C1(m_nImgWidth, m_nImgHeight, &SB_p16u_buf);
	ippiConvert_32f16u_C1R(m_p32f_Buf, m_SB_p32f_Buf, p16u_buf, SB_p16u_buf, org_roiImg, ippRndNear);

	unsigned short* pResultRawBuf = new unsigned short[m_nImgWidth * m_nImgHeight];

	for (int itery = 0; itery < m_nImgHeight; itery++)
		memcpy(pResultRawBuf + itery * m_nImgWidth, (char*)p16u_buf + itery * SB_p16u_buf, m_nImgWidth * sizeof(p16u_buf[0]));

	int nW1 = (int)m_fSrcMin;
	int nW2 = (int)m_fSrcMax;

	double PixelValue;
	double dMaxValue = 255;
	double dCoeff = ((double)dMaxValue / (double)(nW2 - nW1));

	unsigned char* pImage = new unsigned char[m_nImgWidth * m_nImgHeight];
	memset(pImage, 0, m_nImgWidth * m_nImgHeight * sizeof(unsigned char));

	for (int itery = 0; itery < m_nImgHeight; itery++)
	{
		for (int iterx = 0; iterx < m_nImgWidth; iterx++)
		{
			PixelValue = *(pResultRawBuf + itery * m_nImgWidth + iterx);
			PixelValue = (double)(PixelValue - nW1) * dCoeff;

			if (PixelValue < 0)
				PixelValue = 1;
			else if (PixelValue > dMaxValue)
				PixelValue = dMaxValue;

			*(pImage + itery * m_nImgWidth + iterx) = (unsigned char)PixelValue;
		}
	}

	SAFE_DELETE_ARRAY(pResultRawBuf);

	ippiFree(p16u_buf);		// 더이상 사용하지 않으므로 해제.
	ippiFree(m_p32f_Buf);	// 더이상 사용하지 않으므로 해제.
	m_p32f_Buf = NULL;

	return pImage;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
float* CMceIppi::CorrectLine(unsigned short* Input_img)
{
	float* pfTempImg = new float[m_nImgWidth * m_nImgHeight];
	for (int i = 0; i < m_nImgWidth * m_nImgHeight; i++)
		pfTempImg[i] = Input_img[i];

	int nPreTerm = 9;
	int nPostTerm = 50;

	Correct_line_cpu(pfTempImg, pfTempImg, m_nImgWidth, m_nImgHeight, nPreTerm, nPostTerm);

	return pfTempImg;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::Correct_line_cpu(float* Output_img, float* Input_img, int nWidth, int nHeight, int nPreTerm, int nPostTerm, const IppiSize* pROISize)
{
	IppiSize org_roiImg = { nWidth, nHeight };
	if (pROISize)
		org_roiImg = *pROISize;

	Ipp32f* p32f_Input_img, * p32f_Input_img_tmp;
	int SB_p32f_Input_img, SB_p32f_Input_img_tmp;

	IppStatus	status;
	p32f_Input_img = ippiMalloc_32f_C1(nWidth, nHeight, &SB_p32f_Input_img);
	p32f_Input_img_tmp = ippiMalloc_32f_C1(nWidth, nHeight, &SB_p32f_Input_img_tmp);

	for (int itery = 0; itery < nHeight; itery++)
		memcpy(p32f_Input_img + itery * (SB_p32f_Input_img / 4), Input_img + itery * nWidth, nWidth * sizeof(float));

	//{ 가로줄 보정
	ippiCopy_32f_C1R(p32f_Input_img, SB_p32f_Input_img, p32f_Input_img_tmp, SB_p32f_Input_img_tmp, org_roiImg);
	IppiSize roimed_vrt = { nWidth, nHeight - (nPreTerm - 1) };
	IppiSize mask_med_vrt = { 1, nPreTerm };
	IppiPoint anchor_med_vrt = { 0, nPreTerm / 2 };
	status = ippiFilterMedian_32f_C1R(p32f_Input_img + anchor_med_vrt.y * (SB_p32f_Input_img / 4),
		SB_p32f_Input_img,
		p32f_Input_img_tmp + anchor_med_vrt.y * (SB_p32f_Input_img_tmp / 4),
		SB_p32f_Input_img_tmp,
		roimed_vrt, mask_med_vrt, anchor_med_vrt);
	status = ippiSub_32f_C1R(p32f_Input_img_tmp, SB_p32f_Input_img_tmp, p32f_Input_img, SB_p32f_Input_img, p32f_Input_img_tmp, SB_p32f_Input_img_tmp, org_roiImg);	// ippiSub_32f_C1R(A, B, C) => C = B - A

	IppiSize roimean_hor = { nWidth - (nPostTerm - 1), nHeight };
	IppiSize mask_mean_hor = { nPostTerm, 1 };
	IppiPoint anchor_mean_hor = { nPostTerm / 2, 0 };
	ippiThreshold_LTValGTVal_32f_C1IR(p32f_Input_img_tmp, SB_p32f_Input_img_tmp, org_roiImg, -NOISE_VALUE, -NOISE_VALUE, NOISE_VALUE, NOISE_VALUE);
	status = ippiFilterBox_32f_C1IR(p32f_Input_img_tmp + anchor_mean_hor.x, SB_p32f_Input_img_tmp, roimean_hor, mask_mean_hor, anchor_mean_hor);
	status = ippiSub_32f_C1IR(p32f_Input_img_tmp, SB_p32f_Input_img_tmp, p32f_Input_img, SB_p32f_Input_img, org_roiImg); // ippiSub_32f_C1IR(A, B) => B = B - A
	//} 가로줄 보정

	//{ 세로줄 보정
	ippiCopy_32f_C1R(p32f_Input_img, SB_p32f_Input_img, p32f_Input_img_tmp, SB_p32f_Input_img_tmp, org_roiImg);
	IppiSize roimed_hor = { nWidth - (nPreTerm - 1), nHeight };
	IppiSize mask_med_hor = { nPreTerm, 1 };
	IppiPoint anchor_med_hor = { nPreTerm / 2, 0 };
	status = ippiFilterMedian_32f_C1R(p32f_Input_img + anchor_med_hor.x,
		SB_p32f_Input_img,
		p32f_Input_img_tmp + anchor_med_hor.x,
		SB_p32f_Input_img_tmp,
		roimed_hor, mask_med_hor, anchor_med_hor);
	status = ippiSub_32f_C1R(p32f_Input_img_tmp, SB_p32f_Input_img_tmp, p32f_Input_img, SB_p32f_Input_img, p32f_Input_img_tmp, SB_p32f_Input_img_tmp, org_roiImg);	// ippiSub_32f_C1R(A, B, C) => C = B - A

	IppiSize roimean_vrt = { nWidth, nHeight - (nPostTerm - 1) };
	IppiSize mask_mean_vrt = { 1, nPostTerm };
	IppiPoint anchor_mean_vrt = { 0, nPostTerm / 2 };
	ippiThreshold_LTValGTVal_32f_C1IR(p32f_Input_img_tmp, SB_p32f_Input_img_tmp, org_roiImg, -NOISE_VALUE, -NOISE_VALUE, NOISE_VALUE, NOISE_VALUE);
	status = ippiFilterBox_32f_C1IR(p32f_Input_img_tmp + anchor_mean_vrt.y * (SB_p32f_Input_img_tmp / 4), SB_p32f_Input_img_tmp, roimean_vrt, mask_mean_vrt, anchor_mean_vrt);
	status = ippiSub_32f_C1IR(p32f_Input_img_tmp, SB_p32f_Input_img_tmp, p32f_Input_img, SB_p32f_Input_img, org_roiImg);
	//} 세로줄 보정

	for (int itery = 0; itery < nHeight; itery++)
		memcpy(Output_img + itery * nWidth, (char*)p32f_Input_img + itery * SB_p32f_Input_img, nWidth * sizeof(p32f_Input_img[0]));

	ippiFree(p32f_Input_img);
	ippiFree(p32f_Input_img_tmp);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::_CLAHE(int nMaxIntensity, int nCLAHEBlockSize, int nHistogramBins, float fCLAHEClipLimit, ExpandMode nExpandMode)
{
	if (fCLAHEClipLimit == 1.0)
		return;

	if (!m_p32f_Buf)
	{
		;// QString strTrace = "CMceIppi::CLAHE() -> m_p32f_Buf is 'NULL'.\n";
	}

	unsigned short* pTemp_usImage = new unsigned short[m_nImgWidth * m_nImgHeight];
	for (int itery = 0; itery < m_nImgHeight; itery++)
	{
		for (int iterx = 0; iterx < m_nImgWidth; iterx++)
		{
			if (*(m_p32f_Buf + itery * m_SB_p32f_Buf / 4 + iterx) < 0)
				*(pTemp_usImage + itery * m_nImgWidth + iterx) = 0;
			else
				*(pTemp_usImage + itery * m_nImgWidth + iterx) = (unsigned short)(*(m_p32f_Buf + itery * m_SB_p32f_Buf / 4 + iterx));
		}
	}

	CLAHE_Contrast(pTemp_usImage, m_nImgWidth, m_nImgHeight, nMaxIntensity, nCLAHEBlockSize, nHistogramBins, fCLAHEClipLimit, nExpandMode);

	for (int itery = 0; itery < m_nImgHeight; itery++)
	{
		for (int iterx = 0; iterx < m_nImgWidth; iterx++)
		{
			*(m_p32f_Buf + itery * m_SB_p32f_Buf / 4 + iterx) = *(pTemp_usImage + itery * m_nImgWidth + iterx);
		}
	}

	SAFE_DELETE_ARRAY(pTemp_usImage);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
int CMceIppi::CLAHE(u_short* pImage, unsigned int nWidth, unsigned int nHeight, u_short nMin, u_short nMax, unsigned int nNumOfBlockX, unsigned int nNumOfBlockY, unsigned int uiNrBins, double fCliplimit, int nMaxIntensity)
{
	unsigned int uiX, uiY; /* counters */
	unsigned int uiXSize, uiYSize, uiSubX, uiSubY; /* size of context. reg. and subimages */
	unsigned int uiXL, uiXR, uiYU, uiYB;  /* auxiliary variables interpolation routine */
	unsigned long ulClipLimit, ulNrPixels;/* clip limit and region pixel count */
	u_short* pImPointer; /* pointer to image */
	u_short* aLUT; /* lookup table used for scaling of input image */
	unsigned long* pulHist, * pulMapArray; /* pointer to histogram and mappings*/
	unsigned long* pulLU, * pulLB, * pulRU, * pulRB; /* auxiliary pointers interpolation */

	//if (nNumOfBlockX > UIMAX_REG_X) return -1;     /* # of regions x-direction too large */   
	//if (nNumOfBlockY > UIMAX_REG_Y) return -2;     /* # of regions y-direction too large */   

	if (nWidth % nNumOfBlockX) return -3;      /* x-resolution no multiple of nNumOfBlockX */
	if (nHeight % nNumOfBlockY) return -4;      /* y-resolution no multiple of nNumOfBlockY */

	if (nMax >= UINR_OF_GREY) return -5;     /* maximum too large */
	if (nMin >= nMax) return -6;         /* minimum equal or larger than maximum */
	if (nNumOfBlockX < 2 || nNumOfBlockY < 2) return -7;/* at least 4 contextual regions required */
	if (fCliplimit == 1.0) return 0;	  /* is OK, immediately returns original image. */
	if (uiNrBins == 0) uiNrBins = 128;	  /* default value when not specified */

	pulMapArray = (unsigned long*)malloc(sizeof(unsigned long) * nNumOfBlockX * nNumOfBlockY * uiNrBins);
	if (pulMapArray == 0) return -8;	  /* Not enough memory! (try reducing uiNrBins) */

	aLUT = new u_short[nMaxIntensity + 1];

	uiXSize = nWidth / nNumOfBlockX; uiYSize = nHeight / nNumOfBlockY;  /* Actual size of contextual regions */
	ulNrPixels = (unsigned long)uiXSize * (unsigned long)uiYSize;

	if (fCliplimit > 0.0) {		  /* Calculate actual cliplimit	 */
		ulClipLimit = (unsigned long)(fCliplimit * (uiXSize * uiYSize) / uiNrBins);
		ulClipLimit = (ulClipLimit < 1UL) ? 1UL : ulClipLimit;
	}
	else
		ulClipLimit = 1UL << 14;		  /* Large value, do not clip (AHE) */

	MakeLut(aLUT, nMin, nMax, uiNrBins);    /* Make lookup table for mapping of greyvalues */

	/* Calculate greylevel mappings for each contextual region */
	for (uiY = 0, pImPointer = pImage; uiY < nNumOfBlockY; uiY++)
	{
		for (uiX = 0; uiX < nNumOfBlockX; uiX++, pImPointer += uiXSize)
		{
			pulHist = &pulMapArray[uiNrBins * (uiY * nNumOfBlockX + uiX)];
			MakeHistogram(pImPointer, nWidth, uiXSize, uiYSize, pulHist, uiNrBins, aLUT);
			ClipHistogram(pulHist, uiNrBins, ulClipLimit);
			MapHistogram(pulHist, nMin, nMax, uiNrBins, ulNrPixels);
		}
		pImPointer += (uiYSize - 1) * nWidth;         /* skip lines, set pointer */
	}

	/* Interpolate greylevel mappings to get CLAHE image */
	for (pImPointer = pImage, uiY = 0; uiY <= nNumOfBlockY; uiY++)
	{
		if (uiY == 0)
		{					/* special case: top row */
			uiSubY = uiYSize >> 1;
			uiYU = 0;
			uiYB = 0;
		}
		else
		{
			if (uiY == nNumOfBlockY)
			{               /* special case: bottom row */
				uiSubY = uiYSize >> 1;
				uiYU = nNumOfBlockY - 1;
				uiYB = uiYU;
			}
			else
			{				/* default values */
				uiSubY = uiYSize;
				uiYU = uiY - 1;
				uiYB = uiYU + 1;
			}
		}

		for (uiX = 0; uiX <= nNumOfBlockX; uiX++)
		{
			if (uiX == 0)
			{				/* special case: left column */
				uiSubX = uiXSize >> 1;
				uiXL = 0;
				uiXR = 0;
			}
			else
			{
				if (uiX == nNumOfBlockX)
				{			/* special case: right column */
					uiSubX = uiXSize >> 1;
					uiXL = nNumOfBlockX - 1;
					uiXR = uiXL;
				}
				else
				{			/* default values */
					uiSubX = uiXSize;
					uiXL = uiX - 1;
					uiXR = uiXL + 1;
				}
			}

			pulLU = &pulMapArray[uiNrBins * (uiYU * nNumOfBlockX + uiXL)];
			pulRU = &pulMapArray[uiNrBins * (uiYU * nNumOfBlockX + uiXR)];
			pulLB = &pulMapArray[uiNrBins * (uiYB * nNumOfBlockX + uiXL)];
			pulRB = &pulMapArray[uiNrBins * (uiYB * nNumOfBlockX + uiXR)];
			Interpolate(pImPointer, nWidth, pulLU, pulRU, pulLB, pulRB, uiSubX, uiSubY, aLUT);
			pImPointer += uiSubX;			  /* set pointer on next matrix */
		}
		pImPointer += (uiSubY - 1) * nWidth;
	}

	free(pulMapArray);					  /* free space for histograms */
	SAFE_DELETE_ARRAY(aLUT);

	return 0;						  /* return status OK */
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::ClipHistogram(unsigned long* pulHistogram, unsigned int uiNrGreylevels, unsigned long ulClipLimit)
{
	unsigned long* pulBinPointer, * pulEndPointer, * pulHisto;
	unsigned long ulNrExcess, ulOldNrExcess, ulUpper, ulBinIncr, ulStepSize, i;
	long lBinExcess;

	ulNrExcess = 0;  pulBinPointer = pulHistogram;
	for (i = 0; i < uiNrGreylevels; i++)
	{ /* calculate total number of excess pixels */
		lBinExcess = (long)pulBinPointer[i] - (long)ulClipLimit;
		if (lBinExcess > 0)
			ulNrExcess += lBinExcess;	  /* excess in current bin */
	};

	/* Second part: clip histogram and redistribute excess pixels in each bin */
	ulBinIncr = ulNrExcess / uiNrGreylevels;		  /* average binincrement */
	ulUpper = ulClipLimit - ulBinIncr;	 /* Bins larger than ulUpper set to cliplimit */

	for (i = 0; i < uiNrGreylevels; i++)
	{
		if (pulHistogram[i] > ulClipLimit)
			pulHistogram[i] = ulClipLimit; /* clip bin */
		else
		{
			if (pulHistogram[i] > ulUpper)
			{		/* high bin count */
				ulNrExcess -= (ulClipLimit - pulHistogram[i]);		// LCY-???
				//ulNrExcess -= pulHistogram[i] - ulUpper;
				pulHistogram[i] = ulClipLimit;
			}
			else
			{					/* low bin count */
				ulNrExcess -= ulBinIncr;
				pulHistogram[i] += ulBinIncr;
			}
		}
	}

	/* ####

	IAC Modification:
	In the original version of the loop below it was possible for an infinite loop to get
	created.  If there was more pixels to be redistributed than available space then the
	while loop would never end.  This problem has been fixed by stopping the loop when all
	pixels have been redistributed OR when no pixels where redistributed in the previous iteration.
	This change allows very low clipping levels to be used.

	#### */

	do {   /* Redistribute remaining excess  */
		pulEndPointer = &pulHistogram[uiNrGreylevels]; pulHisto = pulHistogram;

		ulOldNrExcess = ulNrExcess;     /* Store number of excess pixels for test later. */

		while (ulNrExcess && pulHisto < pulEndPointer)
		{
			ulStepSize = uiNrGreylevels / ulNrExcess;
			if (ulStepSize < 1)
				ulStepSize = 1;		  /* stepsize at least 1 */
			for (pulBinPointer = pulHisto; pulBinPointer < pulEndPointer && ulNrExcess; pulBinPointer += ulStepSize)
			{
				if (*pulBinPointer < ulClipLimit)
				{
					(*pulBinPointer)++;	 ulNrExcess--;	  /* reduce excess */
				}
			}
			pulHisto++;		  /* restart redistributing on other bin location */
		}
	} while ((ulNrExcess) && (ulNrExcess < ulOldNrExcess));
	/* Finish loop when we have no more pixels or we can't redistribute any more pixels */
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::MakeHistogram(u_short* pImage, unsigned int nWidth, unsigned int uiSizeX, unsigned int uiSizeY,
	unsigned long* pulHistogram, unsigned int uiNrGreylevels, u_short* pLookupTable)
{
	u_short* pImagePointer;
	unsigned int i;

	for (i = 0; i < uiNrGreylevels; i++)
		pulHistogram[i] = 0L; /* clear histogram */

	for (i = 0; i < uiSizeY; i++)
	{
		pImagePointer = &pImage[uiSizeX];
		while (pImage < pImagePointer)
			pulHistogram[pLookupTable[*pImage++]]++;
		pImagePointer += nWidth;
		pImage = pImagePointer - uiSizeX;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::MapHistogram(unsigned long* pulHistogram, u_short nMin, u_short nMax, unsigned int uiNrGreylevels, unsigned long ulNrOfPixels)
{
	unsigned int i;  unsigned long ulSum = 0;
	const float fScale = ((float)(nMax - nMin)) / ulNrOfPixels;
	const unsigned long ulMin = (unsigned long)nMin;

	for (i = 0; i < uiNrGreylevels; i++)
	{
		ulSum += pulHistogram[i];
		pulHistogram[i] = (unsigned long)(ulMin + ulSum * fScale);
		if (pulHistogram[i] > nMax)
			pulHistogram[i] = nMax;
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::MakeLut(u_short* pLUT, u_short nMin, u_short nMax, unsigned int uiNrBins)
{
	int i;
	const u_short BinSize = (u_short)(1 + (nMax - nMin) / uiNrBins);

	for (i = nMin; i <= nMax; i++)
		pLUT[i] = static_cast<u_short>((i - nMin) / BinSize);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::Interpolate(u_short* pImage, int nWidth, unsigned long* pulMapLU, unsigned long* pulMapRU, unsigned long* pulMapLB, unsigned long* pulMapRB, unsigned int uiXSize, unsigned int uiYSize, u_short* pLUT)
{
	const unsigned int uiIncr = nWidth - uiXSize; /* Pointer increment after processing row */
	u_short GreyValue; unsigned int uiNum = uiXSize * uiYSize; /* Normalization factor */

	unsigned int uiXCoef, uiYCoef, uiXInvCoef, uiYInvCoef, uiShift = 0;

	if (uiNum & (uiNum - 1)) {  /* If uiNum is not a power of two, use division */
		for (uiYCoef = 0, uiYInvCoef = uiYSize; uiYCoef < uiYSize; uiYCoef++, uiYInvCoef--, pImage += uiIncr)
		{
			for (uiXCoef = 0, uiXInvCoef = uiXSize; uiXCoef < uiXSize; uiXCoef++, uiXInvCoef--)
			{
				GreyValue = pLUT[*pImage];			/* get histogram bin value */
				*pImage++ = (u_short)(((long long)uiYInvCoef * (uiXInvCoef * pulMapLU[GreyValue] + uiXCoef * pulMapRU[GreyValue])
					+ (long long)uiYCoef * (uiXInvCoef * pulMapLB[GreyValue] + uiXCoef * pulMapRB[GreyValue])) / uiNum);
			}
		}
	}
	else
	{				/* avoid the division and use a right shift instead */
		while (uiNum >>= 1) uiShift++;			/* Calculate 2log of uiNum */
		for (uiYCoef = 0, uiYInvCoef = uiYSize; uiYCoef < uiYSize; uiYCoef++, uiYInvCoef--, pImage += uiIncr)
		{
			for (uiXCoef = 0, uiXInvCoef = uiXSize; uiXCoef < uiXSize; uiXCoef++, uiXInvCoef--)
			{
				GreyValue = pLUT[*pImage];		/* get histogram bin value */
				*pImage++ = (u_short)(((long long)uiYInvCoef * (uiXInvCoef * pulMapLU[GreyValue]
					+ uiXCoef * pulMapRU[GreyValue])
					+ (long long)uiYCoef * (uiXInvCoef * pulMapLB[GreyValue]
						+ uiXCoef * pulMapRB[GreyValue])) >> uiShift);
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL CMceIppi::CLAHE_Contrast(u_short* pImage, int nWidth, int nHeight, int nMaxIntensity, int nBlockSize, int nHistogramBins, double dMaximumSlope, ExpandMode nExpandMode)
{
	int nSizeOfImage = nWidth * nHeight;

	int nMinValue = nMaxIntensity;
	int nMaxValue = 0;
	for (int i = 0; i < nSizeOfImage; i++)
	{
		if (nMaxValue < pImage[i]) nMaxValue = pImage[i];
		if (nMinValue > pImage[i]) nMinValue = pImage[i];
	}
	//int nIntensityRange = nMaxValue-nMinValue;
	int nNumOfBlockX = nWidth / nBlockSize;
	int nExpandX = nBlockSize - (nWidth - nNumOfBlockX * nBlockSize);
	if (nExpandX > 0) nNumOfBlockX++;
	int nNumOfBlockY = nHeight / nBlockSize;
	int nExpandY = nBlockSize - (nHeight - nNumOfBlockY * nBlockSize);
	if (nExpandY > 0) nNumOfBlockY++;

	int nNewWidth = nWidth + nExpandX;
	int nNewHeight = nHeight + nExpandY;
	u_short* pExpandImage = ExpandImage(pImage, nWidth, nHeight, nExpandX, nExpandY, nExpandMode);

	CLAHE(pExpandImage, nNewWidth, nNewHeight, static_cast<u_short>(nMinValue), static_cast<u_short>(nMaxValue),
		nNumOfBlockX, nNumOfBlockY, nHistogramBins, (double)dMaximumSlope, nMaxIntensity);

	ReduceImage(pExpandImage, pImage, nNewWidth, nNewHeight, nExpandX, nExpandY, nExpandMode);

	SAFE_DELETE_ARRAY(pExpandImage);

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void* CMceIppi::AllocateMemoryBuffer(int nSizeX, int nSizeY, int nBytePerPixel)
{
	int nSizeOfImage = nSizeX * nSizeY * nBytePerPixel;
	BYTE* pBuffer = new BYTE[nSizeOfImage];
	memset(pBuffer, 0x00, nSizeOfImage);
	return pBuffer;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
u_short* CMceIppi::ExpandImage(u_short* pImage, int nWidth, int nHeight, int nAddWidth, int nAddHeight, int nMode)
{
	if (pImage == NULL) return NULL;
	if (nAddWidth < 0 && nAddHeight < 0) return NULL;

	int nNewWidth = nWidth + nAddWidth;
	int nNewHeight = nHeight + nAddHeight;
	u_short* pExpandImage = (u_short*)AllocateMemoryBuffer(nNewWidth, nNewHeight, sizeof(u_short));

	if (nAddWidth == 0 && nAddHeight == 0)
	{
		memcpy(pExpandImage, pImage, nWidth * nHeight * sizeof(u_short));
		return pExpandImage;
	}

	int nOffsetX = nAddWidth / 2;
	int nOffsetY = nAddHeight / 2;

	int nInitPos = 0;

	switch (nMode) {
	case EXPAND_LEFTTOP:
		for (int i = 0; i < nHeight; i++)
		{
			memcpy(pExpandImage + (i * nNewWidth), pImage + (i * nWidth), nWidth * sizeof(u_short));
		}
		// outline (blank region)
		//Bottom
		for (int i = nHeight; i < nNewHeight; i++)
		{
			memcpy(pExpandImage + (i * nNewWidth), pImage + ((nHeight - 1) * nWidth), nWidth * sizeof(u_short));
		}
		//Right
		for (int i = 0; i < nNewHeight; i++)
		{
			int nIdx = i * nNewWidth + nWidth;
			int nOriIndex = i * nNewWidth + nWidth - 1;
			u_short uFillValue = pExpandImage[nOriIndex];
			for (int j = 0; j < nAddWidth; j++, nIdx++) {
				pExpandImage[nIdx] = uFillValue;
			}
		}
		break;
	case EXPAND_CENTER:
		for (int i = 0; i < nHeight; i++)
		{
			memcpy(pExpandImage + ((i + nOffsetY) * nNewWidth) + nOffsetX, pImage + (i * nWidth), nWidth * sizeof(u_short));
		}
		//Fill Left
		for (int i = 0; i < nHeight; i++)
		{
			int nIdx = (i + nOffsetY) * nNewWidth;
			int nOriIndex = i * nWidth;
			u_short uFillValue = pImage[nOriIndex];
			for (int j = 0; j < nOffsetX; j++, nIdx++)
			{
				pExpandImage[nIdx] = uFillValue;
			}
		}
		//Fill Right
		for (int i = 0; i < nHeight; i++)
		{
			int nOffset = nOffsetX + nWidth;
			int nIdx = (i + nOffsetY) * nNewWidth + nOffset;
			int nOriIndex = (i + 1) * nWidth - 1;
			u_short uFillValue = pImage[nOriIndex];
			for (int j = nOffset; j < nNewWidth; j++, nIdx++)
			{
				pExpandImage[nIdx] = uFillValue;
			}
		}

		//Fill Top
		for (int i = 0; i < nOffsetY; i++)
		{
			memcpy(pExpandImage + (i * nNewWidth), pExpandImage + (nOffsetY * nNewWidth), nNewWidth * sizeof(u_short));
		}
		//Fill Bottom
		nInitPos = nOffsetY + nHeight;
		for (int i = nInitPos; i < nNewHeight; i++)
		{
			memcpy(pExpandImage + (i * nNewWidth), pExpandImage + ((nInitPos - 1) * nNewWidth), nNewWidth * sizeof(u_short));
		}
		break;
	case EXPAND_RIGHTBOTTOM:
		break;
	default:
		break;
	}

	return pExpandImage;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CMceIppi::ReduceImage(u_short* pImage, u_short* pResultImage, int nWidth, int nHeight, int nSubjectWidth, int nSubjectHeight, int nMode)
{
	if (pImage == NULL) return;
	if (nSubjectWidth < 0 && nSubjectHeight < 0) return;
	if (nSubjectWidth == 0 && nSubjectHeight == 0)
	{
		memcpy(pResultImage, pImage, nWidth * nHeight * sizeof(u_short));
		return;
	}

	int nNewWidth = nWidth - nSubjectWidth;
	int nNewHeight = nHeight - nSubjectHeight;

	int nOffsetX = nSubjectWidth / 2;
	int nOffsetY = nSubjectHeight / 2;

	switch (nMode) {
	case EXPAND_LEFTTOP:
		for (int i = 0; i < nNewHeight; i++)
		{
			memcpy(pResultImage + i * nNewWidth, pImage + i * nWidth, nNewWidth * sizeof(u_short));
		}
		break;
	case EXPAND_CENTER:
		for (int i = 0; i < nNewHeight; i++)
		{
			memcpy(pResultImage + (i * nNewWidth), pImage + ((i + nOffsetY) * nWidth) + nOffsetX, nNewWidth * sizeof(u_short));
		}
		break;
	case EXPAND_RIGHTBOTTOM:
		break;
	default:
		break;
	}
}

void CMceIppi::SetSaveImageProcessingStep(bool bSaveStep)
{
	m_bSaveImageProcessingStep = bSaveStep;
}
