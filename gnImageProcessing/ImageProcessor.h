// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "ImageProcessingInterface.h"
#include "defines.h"
#include "../predix_sdk/constants.h" // 2025-02-11. jg kim. constants.h로 이동된 macro들을 사용하기 위함
#include "opencv2/opencv.hpp"
#include <queue>
#include <numeric>

struct CRX_RECT2
{
	struct {
		int left;
		int right;
		int top;
		int bottom;
	} box;
	struct {
		float angle; // 2025-02-11. jg kim. 오타 수정
		int cx;
		int cy;
		int width;
		int height;
	} rot;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class ImageProcessor
class ImageProcessor
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public constructor and destructor
public:
	// constructor
	ImageProcessor(unsigned short* image, int width, int height);

	// destructor
	~ImageProcessor();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public methods
public:
	// 2024-12-09. jg kim. 공정툴용 영상처리와 통합
	std::vector<cv::Mat> Run();
	CRX_RECT2 _FindIPArea();// 2024-06-25. jg kim. Cruxell 종속성 제거를 위해 구현


public:
	struct InfoIP
	{
		int width;
		int height;
		float rad;
	};

	InfoIP ip[IP_INDICES] = {
		{880, 1240, 280.f}, //0
		{960, 1600, 280.f}, //1
		{1240, 1640, 283.5f}, //2
		//{1080, 2160, 260.0f} //3. jg kim.크럭셀에서 값을 잘못 입력했었음.
		{1080, 2160, 280.0f}
	};

	enum usedCorner
	{
		e_LeftTop = 0,
		e_RightTop,
		e_LeftBottom,
		e_RightBottom,
		e_LeftEdge,//4
		e_RightEdge,
		e_TopEdge,
		e_BottomEdge,
		e_AllCorner,
		e_NoCorner
	};

	struct stInfoCorner
	{
		float scoreCorner[CORNERS] = { 0, };
		int countOverCornerThreshold = 0;
		float meanScoreCornerOverCornerThreshold = 0.f;
		float errRatio[2] = { 0.f, };
		usedCorner e_usedCorner;
		int corners_L = 0;
		int corners_R = 0;
		int corners_T = 0;
		int corners_B = 0;
		int n_guessedIndex = -1;
		int n_maxIndex = -1;
	};
	
	struct ArcInfo
	{
		cv::RotatedRect rr;
		cv::Point2d arc_angle;
		int nPixels;
	};

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private methods
private:
	std::vector<cv::Mat> getCorrectionCoefficients();
	// 2024-12-09. jg kim. 기존의 영상처리와 공정툴용 영상처리가 동일한 입출력 형식을 가지도록 refactoring 함.
	std::vector<cv::Mat> Preprocess(std::vector<cv::Mat> coeffs);
	cv::Mat BlendImage(cv::Mat gainOrg, cv::Mat imgF, cv::Mat gainCorrected, cv::Mat NedPositive, cv::Mat NedNegative, cv::Mat bin);
	std::vector<cv::Mat> Preprocess_PE(std::vector<cv::Mat> coeffs);

	int find_global_minimum(float * fpt_data, int L);

	cv::Point2d calculateVector(cv::Point2d point1, cv::Point2d point2);

	cv::Point2d getArcAngle(cv::Rect rt, cv::Point2d center, eCornerType type);

	void fitEllipseToContour(const std::vector<cv::Point>& points, cv::Point2f & center, cv::Size2f & axes, float & angle);

	cv::RotatedRect estimateCircle2(const std::vector<cv::Point2f> points);

	cv::RotatedRect estimateCircle(const std::vector<cv::Point> points);

	void getStartEndIndex(cv::Mat data, int & Start, int & End, float threshold);

	ImageProcessor::ArcInfo getArcInfoFromBin(cv::Mat bin, eCornerType type);
	
	cv::Mat getArcImage(cv::Mat img_uchar);

	cv::Mat getCleanBinaryMask(cv::Mat bin, int nBlockSize, float fEdgeThresh);
	void GetRotatedImage(cv::Mat image, cv::Mat &rotImg, cv::RotatedRect minRect);
	cv::Mat GetRotatedImage(cv::Mat image, cv::RotatedRect minRect);
	
	// 2024-04-09. jg kim. 품혁 이슈 대응을 위해 추가
	int PixelCounts(cv::Mat image, int threshold);
	template <typename T>
	cv::Mat GetThresholdedImage(cv::Mat image, T threshold);
	cv::Mat RemoveTag(cv::Mat& input);
	template <typename T> // 2024-10-29. jg kim. 공정툴에서 사용할 수 있도록 구현
	void RemoveTag(T *input, T *output, int width, int height);
	cv::Mat CreateConvexHull(cv::Mat& input);

	cv::Mat SeededRegiongrowing(cv::Mat input, std::queue<cv::Vec2i> q);

	cv::Mat CreateInitialBackground(cv::Mat& input);
	cv::Mat CreateInitialBackground_New(cv::Mat& input);
	double AverageLine8(unsigned short* input, int width, int height, const ushort* mask, int x_start, int y_start, int y_end, int stride, int widthKernel = 64);

	//using 20211113
	void kernel_set_avr_line8(unsigned short* input, int width, int height, const ushort* mask, float* output, int x_start, int y_start, int y_end, double offset, int stride, double background, int widthKernel = 64);
	// 2026-01-06. jg kim. Wobble 보정 함수 관련 코드 일괄 추가
	void getIpHeight(cv::Mat img, int xs, int & ys, int & ye);

	cv::Mat CorrectWobble(cv::Mat img);
	void getMeanStdOfMat(cv::Mat img, double &mean, double &stdev);
	bool getBandRegion(cv::Mat img, int &xs, int &xe);
	cv::Rect getBandRegion(cv::Mat img);
	// 유틸: 홀수 커널 보장
	static inline int makeOdd(int v) { return (v % 2 == 0) ? (v + 1) : v; }
	// -----------------------------
	// Hessian |Largest Eigenvalue| 영상 계산
	// -----------------------------
	cv::Mat computeHessianEigenImage(const cv::Mat src16u, double sigma);
	template<typename T> int findMinLoc(T * img, int nWidth, int y, int nMaxLoc, int nSearchRange, bool bLeft);
	template<typename T> int findMinLoc_thres(T * img, int nWidth, int y, int nMaxLoc, int nSearchRange, double ratio_max, bool bLeft);
	double getBestMatchPositionNormalizedFit(double * fInputImg, int nWidth, int nHeight, double * fReferance, int nLengthReference, int y, int nMaxLoc, int nDataPoinits, double trustRangePx=2);
	// 유틸: 라인 단위 bilinear 보간 (double 소스 → double/ushort 타겟)
	template<typename SrcT, typename DstT>
	static inline void resampleRowShifted(const SrcT* srcRow, DstT* dstRow, int w, double shift, int mode_interpolation = 0);
	static std::pair<double, double> mean_std(const std::vector<double>& v);
	static std::vector<double> mean_std_min_max(const std::vector<double>& v);
	static void printShiftStats(const std::vector<double>& shift, const char* label);
	static std::vector<double> makeNormalizedPatch(const std::vector<double>& src, int start, int len);
	static int findMaxX(int y, int xs, int xe, int width, const std::vector<double>& mat);
	static int findLocalMaxAroundPrev(const std::vector<double>& mat, int y, int prevX, int searchRange, int width);
	std::pair<std::vector<double>, std::vector<double>> computeShiftVectors(const uint16_t* nInputImg, int nWidth, int nHeight, int xs, int xe, int nSearchRange, double sigma);
	std::vector<std::vector<double>> computeShiftVectors(const uint16_t* nInputImg, int nWidth, int nHeight, int xs, int xe, int ys, int ye, int nSearchRange, double sigma);
	void calcMeanStd(const std::vector<double>& v, double & mean, double & stddev);
	std::vector<std::vector<double>> separateGroupsMeanStd(const std::vector<double>& data, double k, double min_std_scale, bool use_fixed_center, double fixed_mean, double fixed_std);
	std::vector<double> refineShiftVector(std::vector<double> shifts, int ys, int ye);
	cv::Mat CorrectWobble(cv::Mat InputImg, int xs, int xe, int ys, int ye, bool bLowResolution, int nSearchRange);
	// 2024-07-08. jg kim. 영상 보정을 단계적으로 테스트하기 위해 외부 접근 함수 추가
public:
	cv::Mat CorrectMirrorUniformity(cv::Mat & input, cv::Mat & bin, int nWidthKernel = 64);
	cv::Mat RemoveWormPattern(cv::Mat input, int radius, int medRadius, int margin, double cutoff, bool want_old_mode, int nradius, double fstripe_gain, double fvary, bool bPE);
	cv::Mat LineCorrect(cv::Mat input, int preTerm, int postTerm, bool vertical);
	int BinarizeIPArea(cv::Mat& output, cv::Mat input);
	cv::Mat ExtractIP(cv::Mat image, cv::Rect rt, ImageProcessor::stInfoCorner info); // 2024-06-10. jg kim. 함수 내부에서 background의 max값을 구하도록 변경
	template <typename T>
	cv::Rect getBoundingRect(T* image, int width, int height);  // 2024-10-29. jg kim. public로 변경
	cv::Rect getBoundingRect(cv::Mat bin, int nMargin = 10);
	cv::Rect getBoundingRect2(cv::Mat bin); // cv::boundingRect를 사용하는 버전. findNonZero 실행 등의 절차가 있어 별도의 함수로 구현.

private:
	template <typename T>
	void drawStripe(T* image, int width, int height, int coordinate, int nTick, int value, bool bVer = false);
	cv::Mat CorrectPixelGains(cv::Mat img, std::vector<cv::Mat> coeffs, cv::Mat &gainMap, cv::Mat &NedMap); // 2024-09-24. jg kim. NED map을 위해 rtBounding을 사용하지 않음
	cv::Mat ProcessNedArea(cv::Mat img, cv::Mat res, cv::Mat NedMap);
	cv::Mat CorrectPixelGains(cv::Mat img, std::vector<cv::Mat> coeffs);
    void GetNedMap(cv::Mat img, cv::Mat &gainMap, cv::Mat &NedMap);
	cv::Mat getCompactRegion(cv::Mat bin, float fThresRatio);
	void getMeanStddev(cv::Mat img, float &mean, float &stddev);
	void getMeanStddev(std::vector<float> vtData, float &mean, float &stddev);
	std::vector<float> getAspectRatios(std::vector<cv::Mat> imgs);
	 // 2024-08-18. jg kim. CropImage에 mapNED를 전달하기 위해 argument 추가
	cv::Rect GetCropInfo(ImageProcessor::stInfoCorner info, cv::Rect rect); // 2024-04-09. jg kim. 품혁 이슈 대응을 위해 추가
	cv::Mat polyfit(const cv::Mat & src_x, const cv::Mat & src_y, int order);
	double getGain(cv::Mat p1, cv::Mat p2, double level); // 2024-07-22. jg kim. NED영역에서 세로줄 발생 디버깅
	double get_fitted_value(cv::Mat p, double valueInput); // 2024-07-22. jg kim. NED영역에서 세로줄 발생 디버깅
	// 2024-04-09. jg kim. 품혁 이슈 대응을 위해 추가
	// 2024-10-14. jg kim. corner 정보를 CheckBinarizedIPArea_New에서 사용하기 위해 따로 분리
	float getCornerScore(cv::Mat subMask, int nCornerIndex);
	std::vector<cv::Mat> getSubMasks(cv::Mat Mask, std::vector<cv::Rect> rois);
	// 2024-10-14. jg kim. corner 정보를 받을 수 있도록 변경
	cv::Point getCornerCenter(cv::Mat subMask, int nCornerIndex); // 2024-04-09. jg kim. 품혁 이슈 대응을 위해 추가
	cv::Mat CheckRemoveTagedImage(cv::Mat image, int nROI);
	cv::Mat GetBinaryImage(cv::Mat image);  // 2024-07-01. jg kim. 새로운 binarize 함수. IP의 BG 보다 살짝 어두운 영역도 검출하도록 구현
	cv::Mat VerticalExpand(cv::Mat image);
	cv::Mat VerticalShrink(cv::Mat image);
	cv::Mat ZeropadImage(cv::Mat image, int nTimes);
	cv::Mat DepadImage(cv::Mat image, int nTimes);
	cv::Mat DrawIP(int ipIndex);
	cv::Mat DrawIP(int ip_width, int ip_height, int ip_rad);
	 // 2024-08-18. jg kim. mapNED를 활용하도록 변경
	cv::Mat getHorizonGainCorrectedImage(cv::Mat img, cv::Mat p, cv::Rect rt);
	cv::Mat getCorrectionCoefficientsForSingleImage(cv::Mat img, cv::Rect rt, float fUseRatioWidth);
	cv::Mat getMaskedImage(cv::Mat img, cv::Mat mask);
	cv::Rect checkRoiBound(cv::Rect rtImg, cv::Rect roi);
	ImageProcessor::stInfoCorner getInfoCorner();
	void setInfoCorner(ImageProcessor::stInfoCorner info);
	void SuppressAmbientLight(unsigned short* pRawImage, int nWidth, int nHeight, int nLowRes);
public:
	void guessAndAnalyseIP(ImageProcessor::stInfoCorner &info, cv::Mat bin, cv::Mat Rot_bin, float thresholdCorner, bool bRotate);
	void guessAndAnalyseIP(ImageProcessor::stInfoCorner &info, cv::Mat bin);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private members
private:
	// 2024-10-14. jg kim. IP 추정을 개선하기 위하여 argument 조정
	int m_nLowResolution;
	cv::Mat _input;
	bool m_bSaveCorrectionStep;
	bool m_bImageProcess; // 2024-12-09. jg kim. 영상처리 옵션 추가
	int m_nIpIndex;
	int m_nImageProcessMode; // 2024-12-09. jg kim. 영상처리 모드 추가
	int m_nFWxs;
	int m_nFWxe;
	std::vector<int> m_tiffParams; // TIFF 저장 시 해상도 설정용 파라미터
	bool m_bDoorOpen; // 2026-02-09. jg kim. 공정툴에서 문 열림 상태 변수; HardwareInfo class에서 이동
public:
	void SetSaveCorrectionStep(bool bSaveStep);
	void SetImageProcess(bool bImageProcess);
	void SetTiffDPI(int nResolutionMode); // 0: HR (400, 400), 1: LR (800, 800), 2: LR_intermediate (400, 800)
	void SetTiffDPI(int xdpi, int ydpi); // TIFF 저장 시 해상도(DPI) 설정 함수
	//2024-03-26. jg kim. 장비에서 보내 준 IP index를 이용하기 위하여 추가한 함수들.
	void GetIpSize(int &width, int &height, int nIndex);
	void SetIpIndex(int nIpIndex);
	int GetIpIndex();
	void SetImageProcessMode(int nImageProcessMode); // 2024-12-09. jg kim. 영상처리 모드 추가
	void SetDoorOpen(bool bOpen); // 2026-02-09. jg kim. 공정툴에서 문 열림 상태 설정 함수; HardwareInfo class에서 이동
	cv::Mat mergeMask(cv::Mat mask1, cv::Mat mask2, cv::RotatedRect &rr);
	cv::RotatedRect getMinRect(cv::Mat bin);
	cv::RotatedRect getMinRect2(cv::Mat bin);
	cv::RotatedRect getMinRect_test(cv::Mat bin);

	ImageProcessor::stInfoCorner getCornerInfo(cv::Mat bin, int nIpIndex, float error, float cornerThres); // 2024-07-01. jg kim. guessIpIdx_New에 있던 코드를 함수로 추출함
	void getCornerInfos(cv::Mat bin, float error, std::vector<ImageProcessor::stInfoCorner> &infos, float cornerThres);
	std::vector<cv::Rect> getSubMaskRois(cv::Rect rtBound, int radi);
	stInfoCorner m_stInfoCorner;
	cv::Rect m_roiBackground;
	double m_fBackgroundmin;
	double m_fBackgroundmax;
	cv::Mat m_checkedBin;
};

