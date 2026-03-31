// Added by Makeit, on 2024/02/16.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#define _inline_imgProc_h_





typedef enum
{
	noneBlur,
	Median3x3,
	Median5x5,
	Gauss3x3,
	Gauss5x5,
	Wiener3x3,
	Wiener5x5
}BlurFilter;

typedef enum
{
	Panorama,
	Sinus_LAT,
	Sinus_AP,
	TMJ_LAT,
	TMJ_AP,
	Cephalo_LAT,
	Cephalo_AP,
	Carpus,
	Watersview,
	E2V,
	Bitewing,
	GIX,
	FireCR,
	IRAY,
	GenXCR,
}ImgType;

typedef enum
{
	noneMirror,
	AxsHorizontal,
	AxsVertical,
	AxsBoth
}MirrorAxis;

typedef enum
{
	noneRotate,
	CCW,
	CW
}RotateDirection;

struct Mce_PARAM
{
	// Contrast Equilization param
	float	fScaling;			// a
	float	fExponential;		// p
	float	fMaxDifference;		// M
	float	fc0;				// c0

	// Additional param
	float	fEdgeEnhancement;	//fe
	int		nEdgeLayerNumb;		//ne
	float	fLatitudeReduction;	//fl
	int		nLatitudeLayerNumb;	//nl

	Mce_PARAM()
	{
		memset(this, 0, sizeof(Mce_PARAM));
	}
};


//{ LCY-101118-1: MceIppi 관련 parameter
struct IMG_PROCESS_PARAM
{
	char ImgPrcTitle[64];
	char ImgPrcDescription[256];

	ImgType	nImgType;

	// Pre & Post Bluring process param
	BlurFilter nPreBlur;
	BlurFilter nPostBlur;

	// Image information
	int nImgWidth;
	int nImgHeight;

	//{ LCY-120927-3: crop roi 관련 변수 이름 변경
	// Crop ROI info
	int nCropLeftoffset;
	int nCropRightoffset;
	int nCropTopoffset;
	int nCropBottomoffset;
	//} 

	//{ LCY-120927-2: metal segmentation 기능 추가
	// Metal ROI info
	int nMetalLeftoffset;
	int nMetalRightoffset;
	int nMetalTopoffset;
	int nMetalBottomoffset;

	// Metal masking
	int nMetalMaskingThreshold;
	int nMetalMaskingDilationCount;
	//} LCY-120927-2: metal segmentation 기능 추가

	//{ LCY-120927-1: 밝기 조절 변수 추가
	//Contrast & Brightness control to display
	int nContrastBrightCtrlToDisp_w1; //percent
	int nContrastBrightCtrlToDisp_w2;
	//} LCY-120927-1: 밝기 조절 변수 추가

	// Gamma param
	float fGamma;

	float fPyramidW2Offset;
	int nPyramidLayer;

	// Xray-Off 구간 검출용 threshold
	float fDarkThresholdLevel;

	// Find WindowLevel using histogram parameter
	int nHistoCountLevel;									// Histogram bin level 수 (ex: 만약 256이라면 count 단계가 256개가 된다.)
	int nThresholdW1;			int nThresholdW2;			// W1을 찾는데 사용되는 Threshold count 값			

	//{ LCY-120927-3: histogram roi 관련 변수 이름 변경
	int nHistogramLeftoffset;
	int nHistogramRightoffset;
	int nHistogramTopoffset;
	int nHistogramBottomoffset;
	//} LCY-120927-3: histogram roi 관련 변수 이름 변경

	// Bilateral filter marameters
	BOOL bUseBLFilter1;
	float fSigmaD1;
	float fSigmaR1;

	BOOL bUseBLFilter2;
	float fSigmaD2;
	float fSigmaR2;

	float fHistoOutOffsetW1;		float fHistoOutOffsetW2;	// 찾은 WL로 부터 offset 주기 (+/-)(%)

	BOOL bInvert;
	MirrorAxis nMirrorAxis;
	RotateDirection nRotateDirection;

	// MceIppi Parameter
	Mce_PARAM mp;

	// HCW-20150708-ADD : image quality개선(filter2적용)
	typedef struct tagImageFilterInfo
	{
		BOOL bApply;
		INT nAmount;
		double dRadius;
		INT nThreshold;
	}ImageFilterInfo;

	ImageFilterInfo cImageFilterInfo;
	// END

	// LCY-20151217: CLAHE 적용
	BOOL bApplyCLAHE; // 2024-08-18. jg kim. 적용 여부를 변수화
	int nCLAHEBlockSize;
	float fCLAHEClipLimit;


	IMG_PROCESS_PARAM()
	{
		memset(this, 0, sizeof(IMG_PROCESS_PARAM));
		nImgWidth = 1000;
		nImgHeight = 1000;
		fGamma = 1;

		nCropLeftoffset = 0;
		nCropRightoffset = 0;
		nCropTopoffset = 0;
		nCropBottomoffset = 0;

		mp.fScaling = 1;
		mp.fExponential = 0.8f;
		mp.fMaxDifference = 80;
		mp.fc0 = 5;

		cImageFilterInfo.bApply = TRUE;
		cImageFilterInfo.nAmount = 50;
		cImageFilterInfo.dRadius = 0.5;
		cImageFilterInfo.nThreshold = 0;

		nCLAHEBlockSize = 512;
		fCLAHEClipLimit = 1.2f;
	}
};
