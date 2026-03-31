#include "ParameterExtractorInterface.h"
#include "ParameterExtractor.h"
//#include "../gnPreProcessing/PreProcessingInterface.h"
#include "../gnScanInformation/HardwareInfo.h" // 2024-08-06. jg kim. Added to handle images with ambient light during production
// 2026-02-02. jg kim. include�� logger�� ����ϵ��� ����
#include "../include/Logger.h"

const char *LOG_FILE_NAME = "Image Processing Tool.log";
// Suppress unreferenced parameter warnings
#ifndef UNREFERENCED_PARAMETER
#define UNREFERENCED_PARAMETER(P) (void)(P)
#endif

#define SAFE_DELETE_ARRAY(x) if(x){ delete [] x; x = nullptr; }
// 2024-11-12. jg kim. Modified to return execution result
extern "C" bool __cdecl getScannerTunningParameters(float &fAspectRatio, int &nBldcShiftPixels, int &x, int &y, int &nBoundingWidth, int &nBoundingHeight, ushort *imgRaw, int nImgWidth, int nImgHeight, int nKernelSize, bool bUseCorrelation)
{
	cv::Mat img(cv::Size(nImgWidth, nImgHeight), CV_16U, imgRaw);
	CParameteExtractor cp;
	cv::Rect rt;
	bool bResult = cp.getScannerTunningParameters(fAspectRatio, nBldcShiftPixels, rt, img, nKernelSize, bUseCorrelation);
	x = rt.x;
	y = rt.y;
	nBoundingWidth = rt.width;
	nBoundingHeight = rt.height;
	return bResult;
}

char *file_name;
extern "C" void __cdecl setLogFileNamePE(char *name)
{
	file_name = name;
}

extern "C" void __cdecl  writelogPE(char *filename, char *contents)
{
	FILE* fh;
	if (fopen_s(&fh, filename, "at") == 0)
	{
		fprintf(fh, contents);
		fclose(fh);
	}
}

CParameteExtractor::CParameteExtractor()
{
}

CParameteExtractor::~CParameteExtractor()
{
}

	// 2024-11-12. jg kim. Calculate aspect ratio using two points with high correlation with template
cv::Mat CParameteExtractor::getTemplate(cv::Mat img, eCornerType type)
{
	double min, max;
	cv::minMaxLoc(img, &min, &max);
	float mean = float(cv::mean(img).val[0]);
	cv::Mat matTemplate = cv::Mat::ones(cv::Size(41, 41), img.type())*mean;
	cv::Mat horStripe = cv::Mat::ones(cv::Size(41, 9), img.type())*min;
	cv::Mat verStripe = cv::Mat::ones(cv::Size(9, 41), img.type())*min;
	cv::Point ptHorStripe;
	cv::Point ptVerStripe;
	switch (type)
	{
	case LeftTop:
		ptHorStripe = cv::Point(0, 0);
		ptVerStripe = cv::Point(0, 0);
		break;
	case LeftCenter:
		ptHorStripe = cv::Point(0, 16);
		ptVerStripe = cv::Point(0, 0);
		break;
	case LeftBottom:
		ptHorStripe = cv::Point(0, 32);
		ptVerStripe = cv::Point(0, 0);
		break;
	case CenterTop:
		ptHorStripe = cv::Point(0, 0);
		ptVerStripe = cv::Point(16, 0);
		break;
	case CenterCenter:
		ptHorStripe = cv::Point(0, 16);
		ptVerStripe = cv::Point(16, 0);
		break;
	case CenterBottom:
		ptHorStripe = cv::Point(0, 32);
		ptVerStripe = cv::Point(16, 0);
		break;
	case RightTop:
		ptHorStripe = cv::Point(0, 0);
		ptVerStripe = cv::Point(32, 0);
		break;
	case RightCenter:
		ptHorStripe = cv::Point(0, 16);
		ptVerStripe = cv::Point(32, 0);
		break;
	case RightBottom:
		ptHorStripe = cv::Point(0, 32);
		ptVerStripe = cv::Point(32, 0);
		break;
	}

	horStripe.copyTo(matTemplate(cv::Rect(ptHorStripe.x, ptHorStripe.y, horStripe.cols, horStripe.rows)));
	verStripe.copyTo(matTemplate(cv::Rect(ptVerStripe.x, ptVerStripe.y, verStripe.cols, verStripe.rows)));
	return matTemplate;
}

	// 2024-11-12. jg kim. Calculate aspect ratio using two points with high correlation with template
bool CParameteExtractor::getAspectRatio(cv::Mat orgImg, cv::RotatedRect minRect, cv::Mat bin, float & fAspectRatio, cv::Rect &rt)
{
	cv::Mat gray_0;
	orgImg.convertTo(gray_0, CV_8U);

	cv::Mat imgF; gray_0.convertTo(imgF, CV_32F);
	cv::Mat imgRot(imgF.size(), imgF.type());
	
	GetRotatedImage(imgF, imgRot, minRect);
	cv::imwrite("imgRot.tif", imgRot);
	imgRot.convertTo(gray_0, CV_8U);

	cv::Mat output(orgImg.size(), CV_8U);
	// adaptive thresholding

	double maxValue = 255;
	int blockSize = 101;
	double C = 5;
	cv::adaptiveThreshold(gray_0, output, maxValue, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY, blockSize, C);

	for (int y = 0; y < orgImg.rows; y++) // Remove blobs around IP
	{
		bool stop = false;
		int x = 0;
		while (!stop && x < 1000) // left to right
		{
			if (output.at<uchar>(y, x) > 100)
				output.at<uchar>(y, x) = 0;
			else
				stop = true;
			x++;
		}
		x = 0;
		stop = false;
		while (!stop && x < 1000) // left to right
		{
			if (output.at<uchar>(y, output.cols - 1 - x) > 100)
				output.at<uchar>(y, output.cols - 1 - x) = 0;
			else
				stop = true;
			x++;
		}
	}
	gray_0 = output.clone();
	cv::imwrite("adaptiveThreshold.png", output);

	rt=getBoundingRect(bin); // Bounding rect of IP

	cv::Mat mTemplate = getTemplate(output(rt), CenterCenter); // 41x41

	cv::Mat matCorr = cv::Mat::zeros(output.size(), CV_32F);

	cv::Mat corr;

	int rCorner = 283;
	int fullWidth = mTemplate.cols;
	int halfWidth = int(mTemplate.cols / 2);

	for (int r = rt.y; r < rt.y + rCorner; r++)
	{
		for (int c = rt.x; c < rt.x + rCorner; c++)
		{
			cv::Rect roi(c - halfWidth, r - halfWidth, fullWidth, fullWidth);
			cv::matchTemplate(mTemplate, output(roi), corr, cv::TM_CCORR_NORMED);
			matCorr.at<float>(r, c) = corr.at<float>(0, 0);
		}
	}

	for (int r = rt.y + rt.height - rCorner; r < rt.y + rt.height; r++)
	{
		for (int c = rt.x + rt.width - rCorner; c < rt.x + rt.width; c++)
		{
			cv::Rect roi(c - halfWidth, r - halfWidth, fullWidth, fullWidth);
			cv::matchTemplate(mTemplate, output(roi), corr, cv::TM_CCORR_NORMED);
			matCorr.at<float>(r, c) = corr.at<float>(0, 0);
		}
	}

	for (int r = 0; r < output.rows; r++)
	{
		for (int c = 0; c < output.cols; c++)
		{
			float val = matCorr.at<float>(r, c);
			matCorr.at<float>(r, c) = val > 0.82f ? val : 0;
		}
	}

	unsigned short *rowVector = new unsigned short[matCorr.rows];
	unsigned short *colVector = new unsigned short[matCorr.cols];

	memset(rowVector, 0, sizeof(unsigned short)*matCorr.rows);
	memset(colVector, 0, sizeof(unsigned short)*matCorr.cols);

	for (int r = 0; r < output.rows; r++)
	{
		for (int c = 0; c < output.cols; c++)
		{
			if (matCorr.at<float>(r, c))
				rowVector[r] += 1;
		}
	}

	for (int c = 0; c < output.cols; c++)
	{
		for (int r = 0; r < output.rows; r++)
		{
			if (matCorr.at<float>(r, c))
				colVector[c] += 1;
		}
	}

	cv::imwrite("correlation.tif", matCorr);
	rt = getBoundingRect<float>((float*)matCorr.ptr(), matCorr.cols, matCorr.rows);

	int L = 0;
	int T = 0;
	int R = 0;
	int B = 0;
	int cx = rt.x + int(rt.width / 2);
	int cy = rt.y + int(rt.height / 2);

	int max = 0;
	for (int c = 0; c < cx; c++)
	{
		if (colVector[c] > max)
		{
			max = colVector[c];
			L = c;
		}
	}

	max = 0;
	for (int c = cx; c < matCorr.cols; c++)
	{
		if (colVector[c] > max)
		{
			max = colVector[c];
			R = c;
		}
	}

	max = 0;
	for (int r = 0; r < cy; r++)
	{
		if (rowVector[r] > max)
		{
			max = rowVector[r];
			T = r;
		}
	}

	max = 0;
	for (int r = cy; r < matCorr.rows; r++)
	{
		if (rowVector[r] > max)
		{
			max = rowVector[r];
			B = r;
		}
	}

	rt.x = L;
	rt.y = T;
	rt.width = R - L + 1;
	rt.height = B - T + 1;

	float nHorGrid = roundf(rt.width / 200.f);
	float nVerGrid = roundf(rt.height / 200.f);

	fAspectRatio = float(rt.height) / float(rt.width) * nHorGrid / nVerGrid;
	return true;
}

bool CParameteExtractor::getScannerTunningParameters(float & fAspectRatio, int & nBldcShiftPixels, cv::Rect &rt, cv::Mat img, int nKernelSize, bool bUseCorrelation)
{
	UNREFERENCED_PARAMETER(nKernelSize);
	float mean, stddev;
	getMeanStddev(img, mean, stddev);
	if (mean == 0 && stddev == 0)
	{
		fAspectRatio = -1;
		nBldcShiftPixels = 65535;
		return false;
	}
	else
	{
		char buf[1024];
		int nBlockSize = 25; int nKSize = 25; double k = 0.15;
		//img = removeTag(img); // 2024-10-28. jg kim. Moved to PreprocessForParameterExtraction
		 // 2024-08-06. jg kim. Added to handle images with ambient light during production
		// 2024-12-09. jg kim. Integrated with production tool image processing
		CHardwareInfo hi; 
		bool bDoorOpen = hi._CheckDoorClosed((unsigned short*)img.ptr(), img.cols, img.rows, 0);
		// if (!bDoorOpen) // false means door is open
		// {
		// 	hi.SuppressAmbientLight((unsigned short*)img.ptr(), img.cols, img.rows, 0);
		// }

		//cv::GaussianBlur(img, img, cv::Size(7, 7), 1);

		cv::Mat bin = cv::Mat(img.size(), img.type());
		// 2024-10-31. jg kim. Added option to use RemoveWormPattern function when image processing is performed in production tool
		// 2024-12-09. jg kim. Integrated with production tool image processing
		int result_width;
		int result_height;
		unsigned short * result = nullptr;
		bool bResult = false;
		bool bSaveCorrectionStep = false;
		int nImageProcessMode = 1;//image correction_PE (Do not perform extract IP)
		ProcessAPI(&result, result_width, result_height, bResult, (unsigned short*)bin.ptr(), (unsigned short*)img.ptr(), img.cols, img.rows, -1, bSaveCorrectionStep, nImageProcessMode,bDoorOpen);
		cv::Mat gain(img.size(), img.type(), result);
		
		float *fscores = nullptr;
		int nGuessed = -1;
		nBldcShiftPixels = GetBldcShiftIndex(&fscores,nGuessed, (unsigned short*)bin.ptr(), bin.cols, bin.rows);
		// Need to implement BLDC shift pixel calculation function using these results.
		
		rt = getBoundingRect(bin);
		cv::Mat imgNorm = normalizeImage(gain, 255);
		SAFE_DELETE_ARRAY(result);
		// 2024-10-25. jg kim. For easier judgment of image abnormalities during production work
		cv::Mat img8u; imgNorm.convertTo(img8u, CV_8U);
		cv::imwrite("_imgNorm.png", img8u);
		cv::imwrite("_bin.tif", bin);
	//printf_s("growCenterPointsSet\n");

		// Sort points in Inside by y-axis coordinate (ascending order. small value -> large value)

		std::vector<cv::Point2f> vtSorted = getCrossPoints(imgNorm, bin, nBlockSize, nKSize, k);
		//printf_s("sortPointVector. %d\n", vtSorted.size());

		if (vtSorted.size() > 5)
		{
			if (file_name)
			{
				sprintf_s(buf, "Bounding Rect=(%d, %d, %d, %d)\n", rt.x, rt.y, rt.width, rt.height);
				writelog(buf,LOG_FILE_NAME);
			}
			bool bAscending = true;
			float stdTh = 10;
			int nDistanceLayer = 90; // pixel unit

			std::vector<std::vector<cv::Point2f>> vtLayersY;
			// 1. Generate a layer along the y-axis from vtSorted.
			// 2-1. Compute the mean and standard deviation of y-coordinates in the layer.
			// 2-2. If the standard deviation is greater than stdTh, remove the point with the largest deviation.
			// 2-3. Update the mean and standard deviation after removal.
			// 2-4. Repeat steps 2-2 through 2-4 until the standard deviation falls below stdTh.

			bool bXcoordinate = false;
			vtLayersY = getPointsLayers(vtSorted, bXcoordinate, stdTh, bAscending, nDistanceLayer);
			//printf_s("getPointsLayers\n");

			// Combine all points and sort them by x-axis for classification.
			bXcoordinate = true;
			vtSorted = sortPointVector(getMergeDownLayers(vtLayersY), bAscending, bXcoordinate);

			bXcoordinate = true;
			std::vector<std::vector<cv::Point2f>> vtLayersX = getPointsLayers(vtSorted, bXcoordinate, stdTh, bAscending, nDistanceLayer);

			std::vector<cv::Point2f> merged = getMergeDownLayers(vtLayersX);
			std::vector<cv::Point> pixels;
			for (unsigned int i = 0; i < merged.size(); i++)
				pixels.push_back(cv::Point((int)merged.at(i).x, (int)merged.at(i).y));

			cv::RotatedRect minRect = minAreaRect(pixels);
			float angle = minRect.angle;
			if (minRect.size.width >= minRect.size.height)
			{
				angle += 90;
				minRect.angle = angle;
				cv::Size2f sz = minRect.size;
				minRect.size = cv::Size2f(sz.height, sz.width);
			}
			if(bUseCorrelation)
				getAspectRatio(imgNorm, minRect, bin, fAspectRatio, rt);
			// 2024-11-15. jg kim. Restored existing code for result comparison
			std::vector<cv::Point2f> ptBound = getBoundOfPoints(vtLayersX);

			float fWidthBound = ptBound.at(1).x - ptBound.at(0).x;
			float fHeightBound = ptBound.at(1).y - ptBound.at(0).y;

			float fHorSpacing = fWidthBound / float(NumberHorizontalGrid - 1);
			float fVerSpacing = fHeightBound / float(NumberVerticalGrid - 1);

			cv::Point2f **ptGrid = new cv::Point2f*[NumberHorizontalGrid];
			for (int n = 0; n < NumberHorizontalGrid; n++)
				ptGrid[n] = new cv::Point2f[NumberVerticalGrid];

			// Initialize grid
			for (int v = 0; v < NumberVerticalGrid; v++)
				for (int h = 0; h < NumberHorizontalGrid; h++)
					ptGrid[h][v] = cv::Point2f(-1, -1);

			float ferror = 0.2f;
			cv::Point2f error(fHorSpacing*ferror, fVerSpacing*ferror);

			for (int n = 0; n < (int)merged.size(); n++)
			{
				cv::Point2f pt = merged.at(n);
				for (int v = 0; v < NumberVerticalGrid; v++)
					for (int h = 0; h < NumberHorizontalGrid; h++)
					{
						cv::Point2f ptRef = cv::Point2f(ptBound.at(0).x + fHorSpacing * h, ptBound.at(0).y + fVerSpacing * v);
						if (IsInsideRange(ptRef, pt, error))
						{
							float dist = getDistance(pt, ptRef);
							if (dist < error.x && dist < error.y)
								ptGrid[h][v] = pt;
							else
								ptGrid[h][v] = cv::Point2f(-dist, -dist);
						}
					}
			}

			checkNeighborPoints(ptGrid, NumberHorizontalGrid, NumberVerticalGrid);

			float fAspectRatioOrg = getAspectRatio(ptGrid, NumberHorizontalGrid, NumberVerticalGrid);
			if (!bUseCorrelation)
				fAspectRatio = fAspectRatioOrg;

			// Output results
			if (file_name)
			{
				sprintf_s(buf, "Bounding Rect_current method= (%.0f, %.0f, %.0f, %.0f). Aspect ratio = %f\n", ptBound.at(0).x, ptBound.at(0).y, fWidthBound, fHeightBound, fAspectRatioOrg);
				//writelogPE(file_name, buf);
				writelog(buf,LOG_FILE_NAME);
			}

			outputGridSearchingResults(imgNorm, ptGrid, NumberHorizontalGrid, NumberVerticalGrid);
			return true;
		}
		else
		{
			if (file_name)
			{
				sprintf_s(buf, "else_Bounding Rect=(%d, %d, %d, %d)\n", rt.x, rt.y, rt.width, rt.height);
				//writelogPE(file_name, buf);
				writelog(buf,LOG_FILE_NAME);
			}
			cv::RotatedRect minRect;
			minRect.angle = 0;
			minRect.size = cv::Size2f(float(imgNorm.rows) , float(imgNorm.cols));
			minRect.center = cv::Point2f(float(imgNorm.rows)/2, float(imgNorm.cols)/2);
			getAspectRatio(imgNorm, minRect, bin, fAspectRatio, rt);
			return false;
		}
	}	
}

std::vector<cv::Point2f> CParameteExtractor::growCenterPointsSet(std::vector<cv::Point2f> vtInside, std::vector<cv::Point2f> vtOutside, float errorDistance, int nDistance)
{
	// Repeat searching for points adjacent to Inside and including them in Inside (similar to region growing)
	unsigned int notChangedCount = 0;
	while (vtOutside.size() > 0 && notChangedCount < vtInside.size())
	{
		unsigned int j = 0;
		while (j < vtInside.size())
		{
			unsigned int i = 0;
			bool bNotFound = true;
			while (i < vtOutside.size() && bNotFound)
			{
				int ret = IsSamePlane(vtOutside.at(i), vtInside.at(j), (float)nDistance, errorDistance);
				if (ret != -1)
				{
					vtInside.push_back(vtOutside.at(i));
					vtOutside = popPoint(vtOutside, i);
					bNotFound = false;
					notChangedCount = 0;
				}
				i++;
			}
			j++;
		}
		notChangedCount++;
	}
	std::vector<cv::Point2f> vtOutput = vtInside;
	return vtOutput;
}

void CParameteExtractor::getMeanStddev(std::vector<cv::Point2f> vtPts, float & mean, float & stddev, bool bXcoordinate)
{
	std::vector<float> vt;
	for (int n = 0; n < (int)vtPts.size(); n++)
	{
		if (bXcoordinate)
			vt.push_back(vtPts.at(n).x);
		else
			vt.push_back(vtPts.at(n).y);
	}
	getMeanStddev(vt, mean, stddev);
}

void CParameteExtractor::getMeanStddev(std::vector<float> vtPts, float & mean, float & stddev)
{
	mean = 0;
	stddev = 0; // std = sum( (x-m)^2 )
	if (vtPts.size() > 0)
	{
		for (int r = 0; r < (int)vtPts.size(); r++)
		{
			float value = 0;
			value = vtPts.at(r);
			mean += value;
		}
		mean /= float(vtPts.size());

		for (int r = 0; r < (int)vtPts.size(); r++)
		{
			float value = 0;
			value = vtPts.at(r);
			float e = mean - value;
			stddev += e * e;
		}
		stddev = sqrt(stddev / float(vtPts.size()));
	}
}

void CParameteExtractor::getMeanStddev(cv::Mat img, float & mean, float & stddev)
{
	cv::Mat temp; img.convertTo(temp, CV_32F);
	mean = 0;
	float *pt = (float*)temp.ptr();
	for (int n = 0; n < temp.cols * temp.rows; n++)
	{	
		mean += pt[n];
	}
	mean /= float(temp.rows*temp.cols);

	stddev = 0; // std = sum( (x-m)^2 )
	for (int n = 0; n < temp.cols * temp.rows; n++)
	{
		mean += pt[n];
	}
	mean /= float(temp.rows*temp.cols);

	for (int n = 0; n < temp.cols * temp.rows; n++)
	{
		float e = mean - pt[n];
		stddev += e * e;
	}
	stddev = sqrt(stddev / float(temp.rows*temp.cols));
}

std::vector<cv::Point2f> CParameteExtractor::sortPointVector(std::vector<cv::Point2f> vtPts, bool bAscending, bool bXcoordinate)
{
	std::vector<cv::Point2f> vtSorted;
	int *nVisited = new int[vtPts.size()];
	memset(nVisited, 0, sizeof(int)*vtPts.size());

	int count = 0;
	while (count < (int)vtPts.size())
	{
		cv::Point2f pt;
		if (bXcoordinate) // x coordinate
		{	// Ascending:: min first max last. Descending:: max first min last
			if (bAscending) pt = cv::Point2f(65535, 0);
			else			pt = cv::Point2f(0, 0);

		}
		else // y coordinate
		{
			if (bAscending) pt = cv::Point2f(0, 65535);
			else			pt = cv::Point2f(0, 0);
		}
		int i = 0;
		int index = 0;
		while (i < (int)vtPts.size())
		{
			if (checkSortCondition(vtPts.at(i), pt, bAscending, bXcoordinate) && nVisited[i] == 0)
			{
				pt = vtPts.at(i);
				index = i;
			}
			i++;
		}
		vtSorted.push_back(pt);
		nVisited[index] = 1;
		count++;
	}
	SAFE_DELETE_ARRAY(nVisited);
	return vtSorted;
}

bool CParameteExtractor::checkSortCondition(cv::Point2f pt1, cv::Point2f pt2, bool bAscending, bool bXcoordinate)
{
	bool bCondition;
	if (bXcoordinate)	// x coordinate
	{	// Ascending:: min first max last. Descending:: max first min last
		if (bAscending)		bCondition = pt1.x <= pt2.x;
		else				bCondition = pt1.x > pt2.x;

	}
	else				// y coordinate
	{
		if (bAscending)		bCondition = pt1.y <= pt2.y;
		else				bCondition = pt1.y > pt2.y;
	}
	return bCondition;
}

////////////////////////////////////////////////////////////////////////////////
// IsSamePlane
// return value
// 0 : same plane in horizontal, 1: same plane in vertical, -1 : not same plane
int CParameteExtractor::IsSamePlane(cv::Point2f pt1, cv::Point2f pt2, float dist, float error)
{
	int ret = -1;
	float distance = getDistance(pt1, pt2);
	if (distance < dist * (1 + error) && distance > dist * (1 - error))
	{
		if (getDisplacement(pt1, pt2, true) < 50) // vertical plane
			ret = 1;
		else if (getDisplacement(pt1, pt2, false) < 50) // horizontal plane
			ret = 0;
	}
	return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Function to remove Point at desired index from Point2 vector
//
std::vector<cv::Point2f> CParameteExtractor::popPoint(std::vector<cv::Point2f> pts, int index)
{
	std::vector<cv::Point2f> out;
	int N = pts.size();
	for (int i = 0; i < N; i++)
	{
		if (i == index)
			;
		else
			out.push_back(pts.at(i));
	}
	return out;
}

bool CParameteExtractor::IsInsideCircle(cv::Point2f ptCenter, cv::Point2f pt, float radius)
{
	bool ret = false;
	ret = getDistance(ptCenter, pt) < radius ? true : false;
	return ret;
}

cv::Mat CParameteExtractor::normalizeImage(cv::Mat img, double scale, double fNoiseVariation)
{
	if (img.type() != CV_32F)
		img.convertTo(img, CV_32F);
	cv::Mat imgNorm = img.clone();

	double min, max;
	cv::minMaxLoc(img, &min, &max);
	min += fNoiseVariation;
	for (int r = 0; r < img.rows; r++)
	{
		for (int c = 0; c < img.cols; c++)
		{
			float value = imgNorm.at<float>(r, c);
			imgNorm.at<float>(r, c) = float((value - min) / (max - min) * scale);
		}
	}
	return imgNorm;
}


float CParameteExtractor::getDistance(cv::Point2f pt1, cv::Point2f pt2)
{
	if (pt1.x == -1 || pt1.y == -1 || pt2.x == -1 || pt2.y == -1)
		return -1;
	else
		return sqrt(pow(pt1.x - pt2.x, 2) + pow(pt1.y - pt2.y, 2));
}

////////////////////////////////////////////////////////////////////////////////
// 
// 1. Generate a layer along the x-axis (or y-axis) from vtSorted.
// 2-1. Compute the mean and standard deviation of the orthogonal axis coordinates in the layer.
// 2-2. If the standard deviation is greater than stdTh, remove the point with the largest deviation.
// 2-3. Update the mean and standard deviation after removal.
// 2-4. Repeat steps 2-2 through 2-4 until the standard deviation falls below stdTh.
std::vector<std::vector<cv::Point2f>> CParameteExtractor::getPointsLayers(std::vector<cv::Point2f> vtSorted, bool bXcoordinate, float stdTh, bool bAscending, int nDistanceLayer)
{
	unsigned int nCountChecked = 0;
	bool bSortXcoordinate = !bXcoordinate;
	std::vector<std::vector<cv::Point2f>> vtLayers;
	while (nCountChecked < vtSorted.size() - 1)
	{
		// 1. Generate a layer along the x-axis (or y-axis) from vtSorted.
		std::vector<cv::Point2f> subLayer;
		while (getDisplacement(vtSorted.at(nCountChecked + 1), vtSorted.at(nCountChecked), bXcoordinate) < nDistanceLayer && nCountChecked < vtSorted.size() - 2)
		{
			subLayer.push_back(vtSorted.at(nCountChecked));
			nCountChecked++;
		}
		subLayer.push_back(vtSorted.at(nCountChecked));
		if (nCountChecked + 1 == vtSorted.size() - 1)
			subLayer.push_back(vtSorted.at(nCountChecked + 1));

		std::vector<cv::Point2f> subLayerSorted = sortPointVector(subLayer, bAscending, bSortXcoordinate);
		std::vector<cv::Point2f> subLayerRemovePoints = subLayerSorted;
		float mean, stddev;
		getMeanStddev(subLayerSorted, mean, stddev, bXcoordinate);

		// 2-1. Compute the mean and standard deviation of the orthogonal axis coordinates in the layer.
		// 2-2. If the standard deviation is greater than stdTh, remove the point with the largest deviation.
		// 2-3. Update the mean and standard deviation after removal.
		// 2-4. Repeat steps 2-2 through 2-4 until the standard deviation falls below stdTh.
		if (stddev > stdTh)
		{
			while (stddev > stdTh)
			{
				float diffs = 0;
				int index=0;
				for (int i = 0; i < (int)subLayerSorted.size(); i++)
				{
					getValueFromPoint(subLayerSorted.at(i), bXcoordinate);
					float value = abs(mean - getValueFromPoint(subLayerSorted.at(i), bXcoordinate));
					if (value > diffs)
					{
						diffs = value;
						index = i;
					};
				}
				subLayerRemovePoints = popPoint(subLayerSorted, index);
				getMeanStddev(subLayerRemovePoints, mean, stddev, bXcoordinate);
				subLayerSorted = subLayerRemovePoints;
			}
		}

		vtLayers.push_back(subLayerRemovePoints);
		nCountChecked++;
	}
	return vtLayers;
}

cv::Rect CParameteExtractor::getBoundingRect(cv::Mat bin)
{
	cv::Mat input;
	if (bin.type() == CV_32F)
		input = bin.clone();
	else
		bin.convertTo(input, CV_32F);
	const float* mask = input.ptr<float>(0);

	// mirror uniformity
	int width = bin.cols;
	int height = bin.rows;

	int l = -1;// left
	for (int x = 0; x < width; x++) {
		for (int y = 0; y < height; y++) {
			int n = y * width + x;
			if (mask[n] != 0) {
				l = x;	break;
			}
		}
		if (l > 0)			break;
	}
	int r = -1;// right
	for (int x = width - 1; x > l; x--) {
		for (int y = 0; y < height; y++) {
			int n = y * width + x;
			if (mask[n] != 0) {
				r = x + 1;	break;
			}
		}
		if (r > 0)			break;
	}
	int t = -1;// top
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int n = y * width + x;
			if (mask[n] != 0) {
				t = y;	break;
			}
		}
		if (t > 0)	break;
	}
	int b = -1;// bottom
	for (int y = height - 1; y > t; y--) {
		for (int x = 0; x < width; x++) {
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
}

////////////////////////////////////////////////////////////////////////////////
// Return the absolute value of displacement.
float CParameteExtractor::getDisplacement(cv::Point2f pt1, cv::Point2f pt2, bool bHorizontal)
{
	float fdisplacement = 0;
	if (bHorizontal)
		fdisplacement = pt1.x - pt2.x;
	else // vertical
		fdisplacement = pt1.y - pt2.y;

	return fdisplacement < 0 ? fdisplacement * -1 : fdisplacement;
}

////////////////////////////////////////////////////////////////////////////////
// Selects and returns the x- or y-axis coordinate of a point.
// Used in getPointsLayers(), etc.
float CParameteExtractor::getValueFromPoint(cv::Point2f pt, bool bXcoordinate)
{
	if (bXcoordinate)
		return pt.x;
	else
		return pt.y;
}

////////////////////////////////////////////////////////////////////////////////
// Convert 2D vector-defined pointer to 1D vector 
std::vector<cv::Point2f> CParameteExtractor::getMergeDownLayers(std::vector<std::vector<cv::Point2f>> vtLayers)
{
	std::vector<cv::Point2f> mergedLayer;
	for (int i = 0; i < (int)vtLayers.size(); i++)
	{
		std::vector<cv::Point2f> subLayer = vtLayers.at(i);
		for (int j = 0; j < (int)subLayer.size(); j++)
		{
			mergedLayer.push_back(subLayer.at(j));
		}
	}

	return mergedLayer;
}

////////////////////////////////////////////////////////////////////////////////
// Similar function to getBoundingRect.
std::vector<cv::Point2f> CParameteExtractor::getBoundOfPoints(std::vector<std::vector<cv::Point2f>> vtLayers)
{
	float minx = 65535.f;
	float maxx = 0.f;
	float miny = 65535.f;
	float maxy = 0.f;

	for (int j = 0; j < (int)vtLayers.size(); j++)
	{
		std::vector<cv::Point2f> temp = vtLayers.at(j);
		for (int i = 0; i < (int)temp.size(); i++)
		{
			cv::Point2f pt = temp.at(i);
			minx = pt.x < minx ? pt.x : minx;
			miny = pt.y < miny ? pt.y : miny;
			maxx = pt.x > maxx ? pt.x : maxx;
			maxy = pt.y > maxy ? pt.y : maxy;
		}
	}
	std::vector<cv::Point2f> ret;
	ret.push_back(cv::Point2f(minx, miny));
	ret.push_back(cv::Point2f(maxx, maxy));
	return ret;
}

////////////////////////////////////////////////////////////////////////////////
// Return true if pt is within range, false otherwise
// range is defined as ptRef +/- ferror
bool CParameteExtractor::IsInsideRange(cv::Point2f ptRef, cv::Point2f pt, cv::Point2f ferror)
{
	if ((pt.x > ptRef.x - ferror.x) && (pt.x < ptRef.x + ferror.x) && (pt.y > ptRef.y - ferror.y) && (pt.y < ptRef.y + ferror.y))
		return true;
	else
		return false;
}

bool CParameteExtractor::IsExceedBound(cv::Point pt, cv::Point LT, cv::Point RB)
{
	if (pt.x < LT.x || pt.y < LT.y || pt.x == RB.x || pt.y == RB.y)
		return true;
	else
		return false;
}

std::vector<cv::Point2f> CParameteExtractor::getCrossPoints(cv::Mat imgNorm, cv::Mat bin, int nBlockSize, int nKSize, double k)
{
	cv::Rect rt = getBoundingRect(bin);
	cv::Mat harris;
	cv::cornerHarris(imgNorm, harris, nBlockSize, nKSize, k);
	cv::Mat harris_thres_8U;
	{
		double fmean = 0;
		int count = 0;
		for (int r = 0; r < harris.rows; r++)
		{
			for (int c = 0; c < harris.cols; c++)
			{
				unsigned short value = bin.at<unsigned short>(r, c);
				float valueF = harris.at<float>(r, c);
				if (value == 0)
					harris.at<float>(r, c) = 0;
				if (value > 0 && valueF > 0)
				{
					fmean += valueF;
					count++;
				}
			}
		}

		fmean /= double(count);

		for (int r = 0; r < harris.rows; r++)
		{
			for (int c = 0; c < harris.cols; c++)
			{
				float valueF = harris.at<float>(r, c);
				if (valueF < fmean * 2)
					harris.at<float>(r, c) = 0;
				else
					harris.at<float>(r, c) = 255;
			}
		}
		harris.convertTo(harris_thres_8U, CV_8U);
	}
	cv::Mat harris_morph;
	auto se = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(int(nBlockSize*0.5), int(nBlockSize*0.5)));
	cv::erode(harris_thres_8U, harris_morph, se);
	cv::dilate(harris_morph, harris_morph, se);
	//sprintf_s(buf,"_harris_thres_morph.tif");			cv::imwrite(buf, harris_morph);//29

	cv::Mat canny_output;
	cv::Mat gray = harris_morph.clone();
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;

	// detect edges using canny
	cv::Canny(gray, canny_output, 50, 150, 3);

	// find contours
	findContours(canny_output, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE, cv::Point(0, 0));

	// get the moments
	std::vector<cv::Moments> mu(contours.size());
	for (unsigned int i = 0; i < contours.size(); i++)
		mu[i] = moments(contours[i], false);

	float radius = float(100 * sqrt(2));
	std::vector<cv::Point2f> mc(contours.size());
	cv::Point2f ptPrev(0, 0);
	cv::Point2f ptCenter(float(rt.x + rt.width / 2), float(rt.y + rt.height / 2));
	//printf_s("moments\n");

	std::vector<cv::Point2f> vtInside;
	std::vector<cv::Point2f> vtOutside;

	// Classify points within a certain radius from the center of bounding rect
	for (unsigned int i = 0; i < contours.size(); i++)
	{
		int idx = contours.size() - 1 - i;
		// get the centroid of figures.
		float x = float(mu[idx].m10 / mu[idx].m00);
		float y = float(mu[idx].m01 / mu[idx].m00);
		mc[i] = cv::Point2f(x, y);
		cv::Point2f ptNew = cv::Point2f((mc[i].x), (mc[i].y));

		if (ptNew.x >= rt.x && ptNew.x < rt.x + rt.width && ptNew.y >= rt.y && ptNew.y < rt.y + rt.height)
		{
			if (getDistance(ptNew, ptPrev) > 5)
			{
				if (IsInsideCircle(ptCenter, ptNew, radius))
					vtInside.push_back(ptNew);
				else
					vtOutside.push_back(ptNew);
			}
			ptPrev = ptNew;
		}
	}

	//printf_s("Classification Inside Outside. Contour size : %d\n", contours.size());
	float errorDistance = 0.07f;
	int nDistance = 200;
	if (vtInside.size() > 1)
	{
		std::vector<cv::Point2f> vtOrg = vtInside;
		std::vector<cv::Point2f> vtTemp = vtInside;
		vtInside.clear();
		unsigned int i = 0;
		while (i < (unsigned int)vtTemp.size() - 1)
		{
			int ret = IsSamePlane(vtTemp.at(i), vtTemp.at(i + 1), (float)nDistance, errorDistance);
			if (ret != -1)
			{
				vtInside.push_back(vtTemp.at(i));
				vtInside.push_back(vtTemp.at(i + 1));
			}
			i++;
		}
		if (vtInside.size() == 0)
			vtInside = vtOrg;
	}
	// Expand Inside by adding adjacent points (similar to region growing).
	vtInside = growCenterPointsSet(vtInside, vtOutside, errorDistance, nDistance);

	//printf_s("growCenterPointsSet\n");

	// Sort the points in Inside by y-axis coordinate (ascending: smaller to larger).

	bool bAscending = true;
	bool bSortXcoordinate = false;
	return sortPointVector(vtInside, bAscending, bSortXcoordinate);
}

void CParameteExtractor::checkNeighborPoints(cv::Point2f ** ptGrid, int nGridWidth, int nGridHeight)
{
	for (int v = 0; v < nGridHeight; v++)
		for (int h = 0; h < nGridWidth; h++)
		{
			int nExceedCount = 0;
			int nNeighborCount = 0;

			int Ex = h + 1;
			int Ey = v;
			if (IsExceedBound(cv::Point(Ex, Ey), cv::Point(0, 0), cv::Point(nGridWidth, nGridHeight)))
				nExceedCount++;
			else if (ptGrid[Ex][Ey].x < 0 && ptGrid[Ex][Ey].y < 0)
				nExceedCount++;
			else
				nNeighborCount++;

			int Wx = h - 1;
			int Wy = v;

			if (IsExceedBound(cv::Point(Wx, Wy), cv::Point(0, 0), cv::Point(nGridWidth, nGridHeight)))
				nExceedCount++;
			else if (ptGrid[Wx][Wy].x < 0 && ptGrid[Wx][Wy].y < 0)
				nExceedCount++;
			else
				nNeighborCount++;


			int Nx = h;
			int Ny = v - 1;

			if (IsExceedBound(cv::Point(Nx, Ny), cv::Point(0, 0), cv::Point(nGridWidth, nGridHeight)))
				nExceedCount++;
			else if (ptGrid[Nx][Ny].x < 0 && ptGrid[Nx][Ny].y < 0)
				nExceedCount++;
			else
				nNeighborCount++;

			int Sx = h;
			int Sy = v + 1;

			if (IsExceedBound(cv::Point(Sx, Sy), cv::Point(0, 0), cv::Point(nGridWidth, nGridHeight)))
				nExceedCount++;
			else if (ptGrid[Sx][Sy].x < 0 && ptGrid[Sx][Sy].y < 0)
				nExceedCount++;
			else
				nNeighborCount++;

			if (nNeighborCount < 2 || nExceedCount > 3)
				ptGrid[h][v] = cv::Point2f(-1, -1);
		}
}

float CParameteExtractor::getAspectRatio(cv::Point2f ** ptGrid, int nGridWidth, int nGridHeight)
{
	char buf[1024];
	std::vector<cv::Point2f> vt;
	for (int v = 0; v < nGridHeight; v++)
	{
		for (int h = 0; h < nGridWidth; h++)
		{
			vt.push_back(ptGrid[h][v]);
		}
	}

	std::vector<float> vtDistHor;
	std::vector<float> vtDistVer;
	for (int v = 0; v < nGridHeight; v++)
		for (int h = 0; h < nGridWidth - 1; h++)
		{
			if (ptGrid[h][v].x > 0 && ptGrid[h][v].y > 0 && ptGrid[h + 1][v].x > 0 && ptGrid[h + 1][v].y > 0)
			{
				float dist = getDistance(ptGrid[h][v], ptGrid[h + 1][v]);
				if (dist >= 0)
				{
					vtDistHor.push_back(dist);
				}
			}
		}

	for (int v = 0; v < nGridHeight - 1; v++)
		for (int h = 0; h < nGridWidth; h++)
		{
			if (ptGrid[h][v].x > 0 && ptGrid[h][v].y > 0 && ptGrid[h][v + 1].x > 0 && ptGrid[h][v + 1].y > 0)
			{
				float dist = getDistance(ptGrid[h][v], ptGrid[h][v + 1]);
				if (dist >= 0)
				{
					vtDistVer.push_back(dist);
				}
			}
		}

	float averageHorDist, stdHorDist, averageVerDist, stdVerDist;
	getMeanStddev(vtDistHor, averageHorDist, stdHorDist);
	getMeanStddev(vtDistVer, averageVerDist, stdVerDist);

	float fAspectRatio = -1;
	if (averageHorDist > 100 && averageVerDist > 100)
		fAspectRatio = averageVerDist / averageHorDist;
	if (file_name)
	{
		sprintf_s(buf, "Before) average Hor/Ver :: %.1f, %.1f. std Hor/Ver :: %.1f, %.1f. Ver/Hor ratio :: %.3f\n", averageHorDist, averageVerDist, stdHorDist, stdVerDist, fAspectRatio);
		//writelogPE(file_name, buf);
		writelog(buf,LOG_FILE_NAME);
	}

	std::vector<float> vtDistHorCompact;
	std::vector<float> vtDistVerCompact;

	float thStd = 1.1f;

	for (unsigned int n = 0; n < vtDistHor.size(); n++)
	{
		float value = vtDistHor.at(n);
		float difference = abs(value - averageHorDist);
		if (difference <= /*stdHorDist*/thStd)
			vtDistHorCompact.push_back(value);
	}

	for (unsigned int n = 0; n < vtDistVer.size(); n++)
	{
		float value = vtDistVer.at(n);
		float difference = abs(value - averageVerDist);
		if (difference <= /*stdVerDist*/thStd)
			vtDistVerCompact.push_back(value);
	}
	
	getMeanStddev(vtDistHorCompact, averageHorDist, stdHorDist);
	getMeanStddev(vtDistVerCompact, averageVerDist, stdVerDist);

	/*float*/ fAspectRatio = -1;
	if (averageHorDist > 100 && averageVerDist > 100 && stdHorDist >= 0 && stdVerDist >= 0)// 2024-08-05. jg kim. Improve ScanSpeed calculation
		fAspectRatio = averageVerDist / averageHorDist;
	else
		fAspectRatio = -1;

	if (file_name)
	{
		sprintf_s(buf, "After) average Hor/Ver :: %.1f, %.1f. std Hor/Ver :: %.1f, %.1f. Ver/Hor ratio :: %.3f\n", averageHorDist, averageVerDist, stdHorDist, stdVerDist, fAspectRatio);
		writelogPE(file_name, buf);
	}

	return fAspectRatio;
}

void CParameteExtractor::outputGridSearchingResults(cv::Mat imgNorm, cv::Point2f ** ptGrid, int nGridWidth, int nGridHeight)
{
	cv::Mat drawing;
	cvtColor(imgNorm, drawing, cv::COLOR_GRAY2BGR);
	char buf[1024];
	//sprintf_s(buf, "_HorGainCorrected.png");			cv::imwrite(buf, drawing);//29

	for (int i = 0; i < nGridHeight; i++)
	{
		cv::Scalar color;
		int nBright = 255;
		nBright = nBright > 255 ? 255 : nBright;
		if (i % 3 == 0)
			color = cv::Scalar(0, 0, nBright); // B G R values
		else if (i % 3 == 1)
			color = cv::Scalar(0, nBright, 0); // B G R values
		else if (i % 3 == 2)
			color = cv::Scalar(nBright, 0, 0); // B G R values

		for (int j = 0; j < nGridWidth; j++)
		{
			cv::Point2f pt = ptGrid[j][i];
			if (pt.x > 0 && pt.y > 0)
				circle(drawing, pt, 5, color, -1, 8, 0);
		}
	}
	sprintf_s(buf, "_circle_checked_X.png");			cv::imwrite(buf, drawing);//29
}

template<typename T>
cv::Rect CParameteExtractor::getBoundingRect(T * image, int width, int height)
{
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
	// 2024-02-23. jg kim. Fixed issue where existing code didn't properly return width and height.
}

void CParameteExtractor::GetRotatedImage(cv::Mat image, cv::Mat &rotImg, cv::RotatedRect minRect)
{
	float deg = minRect.angle;
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
}