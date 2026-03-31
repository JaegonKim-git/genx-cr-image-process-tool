#pragma once
#define _USE_MATH_DEFINES
#include <math.h>

#define NumberHorizontalGrid 6
#define NumberVerticalGrid 8
#define RawImageWidth 1536
#define RawImageHeight 2360

#include <iostream>
#include "opencv2/opencv.hpp"
class CParameteExtractor
{
public:
	CParameteExtractor();
	~CParameteExtractor();
	
	/* Raw Image를 처리해서 ScanSpeed, BLDC shift Index를 계산하기 위한 함수들*/
public:
	// 2024-11-12. jg kim. 실행 결과를 반환하도록 수정
	bool getScannerTunningParameters(float & fAspectRatio, int & nBldcShiftPixels, cv::Rect &rt, cv::Mat img, int nKernelSize, bool bUseCorrelation);
	
private:
	// 2024-11-12. jg kim. template와의 correlation결과가 높은 두 점을 이용하여 종횡비 계산
	enum eCornerType
	{
		LeftTop = 0,
		LeftCenter,
		LeftBottom,
		CenterTop,
		CenterCenter,
		CenterBottom,
		RightTop,
		RightCenter,
		RightBottom
	};

	cv::Mat getTemplate(cv::Mat img, eCornerType type);
	std::vector<cv::Point2f> growCenterPointsSet(std::vector<cv::Point2f> vtInside, std::vector<cv::Point2f> vtOutside, float errorDistance, int nDistance);

	void getMeanStddev(std::vector<cv::Point2f> vtPts, float & mean, float & stddev, bool bXcoordinate);

	void getMeanStddev(std::vector<float> vtPts, float & mean, float & stddev);

	void getMeanStddev(cv::Mat img, float & mean, float & stddev);

	std::vector<cv::Point2f> sortPointVector(std::vector<cv::Point2f> vtPts, bool bAscending, bool bXcoordinate);

	bool checkSortCondition(cv::Point2f pt1, cv::Point2f pt2, bool bAscending, bool bXcoordinate);

	int IsSamePlane(cv::Point2f pt1, cv::Point2f pt2, float dist, float error);

	std::vector<cv::Point2f> popPoint(std::vector<cv::Point2f> pts, int index);

	bool IsInsideCircle(cv::Point2f ptCenter, cv::Point2f pt, float radius);

	cv::Mat normalizeImage(cv::Mat img, double scale, double fNoiseVariation=10);

	float getDistance(cv::Point2f pt1, cv::Point2f pt2);

	std::vector<std::vector<cv::Point2f>> getPointsLayers(std::vector<cv::Point2f> vtSorted, bool bXcoordinate, float stdTh, bool bAscending, int nDistanceLayer);
	
	cv::Rect getBoundingRect(cv::Mat bin);

	float getDisplacement(cv::Point2f pt1, cv::Point2f pt2, bool bHorizontal);

	float getValueFromPoint(cv::Point2f pt, bool bXcoordinate);

	std::vector<cv::Point2f> getMergeDownLayers(std::vector<std::vector<cv::Point2f>> vtLayers);

	std::vector<cv::Point2f> getBoundOfPoints(std::vector<std::vector<cv::Point2f>> vtLayers);

	bool IsInsideRange(cv::Point2f ptRef, cv::Point2f pt, cv::Point2f ferror);

	bool IsExceedBound(cv::Point pt, cv::Point LT, cv::Point RB);
	
	std::vector<cv::Point2f>  getCrossPoints(cv::Mat imgNorm, cv::Mat bin, int nBlockSize, int nKSize, double k);

	void checkNeighborPoints(cv::Point2f **ptGrid, int nGridWidth, int nGridHeight);

	float getAspectRatio(cv::Point2f **ptGrid, int nGridWidth, int nGridHeight);
	// 2024-11-12. jg kim. template와의 correlation결과가 높은 두 점을 이용하여 종횡비 계산
	bool getAspectRatio(cv::Mat orgImg, cv::RotatedRect minRect, cv::Mat bin, float & fAspectRatio, cv::Rect &rt);
	
	void outputGridSearchingResults(cv::Mat imgNorm, cv::Point2f **ptGrid, int nGridWidth, int nGridHeight);

	void GetRotatedImage(cv::Mat image, cv::Mat & rotImg, cv::RotatedRect minRect);

	template<typename T>
	cv::Rect getBoundingRect(T * image, int width, int height);
};

