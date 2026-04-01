// Refactored by Makeit, on 2024/02/16.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "ipp.h"
#include <winsock.h>
#include "ImageProcessingInterface.h"



typedef enum
{
	Processing_MetalMask,
	Processing_Median,
	Processing_Bilateral,
	Processing_Log,
	Processing_Clahe,
	Processing_Invert,
	Processing_Musica,
	Processing_UnsharpMask,
	Processing_Window,
	Processing_Gamma,
	Processing_Linecorrect,
	Processing_Max,
	Processing_Histogram,
	Processing_GaussianBlur
}E_Process;

/******************** Clahe **************************/
#define UIMAX_REG_X		50
#define UIMAX_REG_Y		50
#define UINR_OF_GREY	65536 

typedef enum
{
	EXPAND_LEFTTOP = 0,
	EXPAND_CENTER,
	EXPAND_RIGHTBOTTOM
}ExpandMode;
/******************** Clahe **************************/

//{ LCY-101118-1: MceIppi 관련 parameter
//}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// image processing class of CMceIppi
class CMceIppi
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public constructor and destructor
public:
	// constructor
	CMceIppi();

	// destructor
	~CMceIppi();

	// 복사 금지.
	CMceIppi(CMceIppi&) = delete;
	CMceIppi& operator= (const CMceIppi&) = delete;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public methods
public:
	BOOL MetalThreshold(float fThreshold, int nErodeCount);
	BOOL ApplyMedian(int nKernelSize);
	BOOL ApplyBilateral(float fSigmaD1, float fSigmaR1);
	BOOL ApplyLog(void);
	BOOL ApplyInvert(void);
	BOOL ApplyMusica(float a, float M, float p, float c0, float fe, int ne, float fl, int nl);
	BOOL ApplyUnsharpMask(int nAmount = 100, double dRadius = 1.5, int nThreshold = 0);
	BOOL ApplyGaussianBlur(Ipp32f* img, int nAmount = 100, double dRadius = 1.5, int nThreshold = 0);
	BOOL ApplyGaussianBlur(void);
	BOOL SetWindow(int nPyramidLayer, float fPyramidW2Offset);
	BOOL ApplyGamma(float fGamma);
	BOOL ApplyLinecorrect(void);
	static void ApplyLinecorrect(float* Output_img, float* Input_img, int nWidth, int nHeight, int nPreTerm, int nPostTerm, bool bVertical);

	BOOL SaveIpp32f(char* file);
	void CopyOrgIpp32buftoIpp32buf(void);

	void SetHistogramOffset(int left, int top, int right, int bottom);
	void SetImage(unsigned short* image, int Width, int Height);
	void getProcessedImage(unsigned short* image, int &Width, int &Height);
	BOOL ProcessMceIppi(IMG_PROCESS_PARAM ipp);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public members
public:
	Ipp32f* m_p32f_Org_Buf;
	int m_SB_p32f_Org_Buf; // 버퍼있는지 확인하려고 public


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private methods
private:
	void BufferDelete(void);

	BOOL ScaleTo14bit(void);
	BOOL ScaleTo1xbit(int n=4);
	BOOL Resize(Ipp32f * scr, int nSrcWidth, int nSrcHeight, Ipp32f * dst, int nDesWidth, int nDesHeight);

	BOOL FillMaskAreaGeX_CR(unsigned short* pRawBuf);
	BOOL RestoreMaskArea(int nMaskValue);

	BOOL Convert16sRawToIpp32f(unsigned short* pRawBuf);
	unsigned char* ConvertIpp32fTo8uRaw(void);

	BOOL MakeMetalMask(Ipp32f * p32f_Buf, int SB_p32f_Buf, float fRefValue, int nErodeCount, Ipp8u * p8u_MaskImage, int SB_p8u_MaskImage);

	/// median
	BOOL BlurProcess(int nKernelSize, Ipp32f * p32f_SrcBuf, int SB_p32f_SrcBuf, Ipp32f * p32f_DesBuf, int SB_p32f_DesBuf);
	BOOL ApplySelectiveMedianFilter(int nKernelSize, Ipp32f * p32f_SrcBuf, int SB_p32f_SrcBuf, Ipp32f * p32f_DesBuf, int SB_p32f_DesBuf);
	BOOL SelectiveNoiseCount(int nWidth, int nHeight, int nKernelSize, Ipp32f * p32f_BufSrc, int SB_p32f_BufSrc, Ipp8u * p8u_BufMin, int SB_p8u_BufMin, Ipp8u * p8u_BufMax, int SB_p8u_BufMax);

	BOOL CreateNoiseMap(int nWidth, int nHeight, Ipp8u * p8u_BufMin, int SB_p8u_BufMin, Ipp8u * p8u_BufMax, int SB_p8u_BufMax, Ipp8u * p8u_BufNoise, int SB_p8u_BufNoise, int nThresholdNoiseCount);

	BOOL TestMedian(int nKernelSize, Ipp32f* p32f_SrcBuf, int SB_p32f_SrcBuf, Ipp32f* p32f_DesBuf, int SB_p32f_DesBuf, Ipp8u* p8u_Mask, int SB_p8u_Mask);
	/////

	/// bilateral
	BOOL ApplyBilateralFilter(float fSigmaD, float fSigmaR, Ipp32f * p32f_SrcBuf, int SB_p32f_SrcBuf, Ipp32f * p32f_DesBuf, int SB_p32f_DesBuf);

	/// musica
	BOOL GeneratePyramid(void);
	void CalcPower(Ipp32f * p32f_Buf, int SB_p32f_Buf, float fPower, IppiSize roiSize);
	BOOL ApplyPyramidCoefficient(float a, float M, float p, float c0, float fe, int ne, float fl, int nl);
	BOOL ReconstructPyrImage(void);
	///

	BOOL GetMinMaxUsingDownsampledImage(Ipp32f * p32f_Image, int SB_p32f_Image, Ipp8u * p8u_MaskImage, int SB_p8u_MaskImage, int nLayerW2, int nLayerW1, float * pMin, float * pMax);

	BOOL ApplyGammaCurve(Ipp32f* p32f_Buf, int SB_p32f_Buf, float fGamma, float fInW1, float fInW2, float fOutW1 = 0, float fOutW2 = 0);

	float* CorrectLine(unsigned short* Input_img);
	static void Correct_line_cpu(float* Output_img, float* Input_img, int nWidth, int nHeight, int nPreTerm, int nPostTerm, const IppiSize* pROISize = nullptr);

	/******************** Clahe **************************/
	void _CLAHE(int nMaxIntensity, int nCLAHEBlockSize, int nHistogramBins, float fCLAHEClipLimit, ExpandMode nExpandMode);
	int	CLAHE(u_short* pImage, unsigned int nWidth, unsigned int nHeight,
		u_short nMin, u_short nMax, unsigned int nNumOfBlockX, unsigned int nNumOfBlockY, unsigned int uiNrBins, double fCliplimit, int nMaxIntensity);
	void ClipHistogram(unsigned long* pulHistogram, unsigned int uiNrGreylevels, unsigned long ulClipLimit);
	void MakeHistogram(u_short* pImage, unsigned int nWidth, unsigned int uiSizeX, unsigned int uiSizeY,
		unsigned long* pulHistogram, unsigned int uiNrGreylevels, u_short* pLookupTable);
	void MapHistogram(unsigned long* pulHistogram, u_short nMin, u_short nMax,
		unsigned int uiNrGreylevels, unsigned long ulNrOfPixels);
	void MakeLut(u_short * pLUT, u_short nMin, u_short nMax, unsigned int uiNrBins);
	void Interpolate(u_short * pImage, int nWidth, unsigned long * pulMapLU,
		unsigned long * pulMapRU, unsigned long * pulMapLB, unsigned long * pulMapRB,
		unsigned int uiXSize, unsigned int uiYSize, u_short * pLUT);
	BOOL CLAHE_Contrast(u_short *pImage, int nWidth, int nHeight, int nMaxIntensity, int nBlockSize, int nHistogramBins, double dMaximumSlope, ExpandMode nExpandMode);
	void* AllocateMemoryBuffer(int nSizeX, int nSizeY, int nBytePerPixel);
	u_short* ExpandImage(u_short* pImage, int nWidth, int nHeight, int nAddWidth, int nAddHeight, int nMode);
	void ReduceImage(u_short* pImage, u_short* pResultImage, int nWidth, int nHeight, int nSubjectWidth, int nSubjectHeight, int nMode);
	int guessIpIdx(int left, int top, int width, int height);
	void getBoundingRect(int& left, int& top, int& width, int& height);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private members
private:
	int m_nImgWidth;
	int m_nImgHeight;

	int m_nHistogramLeftoffset;
	int m_nHistogramRightoffset;
	int m_nHistogramTopoffset;
	int m_nHistogramBottomoffset;

	char* m_pMaskImage;

	Ipp32f* m_p32f_Buf;
	int m_SB_p32f_Buf;	// 버퍼있는지 확인하려고 public

	Ipp8u* m_p8u_MaskImage;
	int m_SB_p8u_MaskImage;

	int m_nTotalPyrLevel;
	int* m_pPyrWidthArray;
	int* m_pPyrHeightArray;

	Ipp32f** m_p32f_LaplacianPyrImageArray;
	int* m_pSB_m_p32f_LaplacianPyrImageArray;

	float m_fBaseLevelValue;

	float m_fSrcMin;
	float m_fSrcMax;

	IppiSize org_roiImg;	// img 전체 영역
	IppiSize org_roiImgHisto;

	/******************** Clahe **************************/
	//int m_nPyramidLayer;
	//int m_fPyramidW2Offset;
	//int m_nCurrentProcessingIndex;

	struct InfoIP
	{
		int width;
		int height;
		float rad;
	};

	InfoIP ip[4] =
	{
		{880, 1240, 280.f}, //0
		{960, 1600, 280.f}, //1
		{1240, 1640, 283.5f}, //2
		{1080, 2160, 280.0f} //3
	};
	bool m_bSaveImageProcessingStep;
	bool m_bLowResolution;
public:
	void SetSaveImageProcessingStep(bool bSaveStep);
};

