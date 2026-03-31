// Added by 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "PspCalibration.h"
#include <math.h>

using namespace cv;



// macro constants
#define DEGREE_COUNT 4
#define MAX_CALI_IMAGE_COUNT 10



// 다른 파일에서 정의하고 있음. 중복 정의이므로 주석처리함.
//extern "C" void __cdecl calculateCoefficients(float *coefficients, unsigned short *images, int width, int height, int L, int T, int W, int H, int count)
//{
//	CPspCalibration cal(coefficients, images, width, height, L, T, W, H, count);
//}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
CPspCalibration::CPspCalibration(float* coefficients, unsigned short* images, int width, int height, int L, int T, int W, int H, int count)
{
	calculateCoefficients(coefficients, images, width, height, L, T, W, H, count);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
CPspCalibration::~CPspCalibration()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void CPspCalibration::calculateCoefficients(float* coefficients, unsigned short* images, int width, int height, int L, int T, int W, int H, int count)
{
	// L, T, W, H 는 calibration 계수 계산에 사용할 ROI
	int N = count + 1;
	Mat Xs = Mat::zeros(W, 1, CV_32F);
	Mat Ys = Mat::zeros(W, N, CV_32F);
	Mat Blurred = Mat::zeros(height, width, CV_16U);
	int nKernelSize = 49;
	for (int n = 0; n < N; n++)
	{
		if (n)
			cv::GaussianBlur(get_raw_mat(width, height, &images[(n - 1) * width * height]), Blurred, Size(nKernelSize, nKernelSize), 0, 0, BORDER_REFLECT_101);

		for (int w = 0; w < W; w++)
		{
			if (n < 1)
				Xs.at<float>(w) = float(L + w);
			Ys.at<float>(w, n) = (float)mean(Blurred(Rect(L + w, T, 1, H))).val[0]; // Rect로 정의한 sub image의 평균값
		}

		coefficients[n] += float(mean(Ys.col(n)).val[0]);
		memcpy(coefficients + N + (DEGREE_COUNT + 1) * n, (float*)(polyfit(Xs, Ys.col(n), DEGREE_COUNT)).data, sizeof(float) * (DEGREE_COUNT + 1));
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat CPspCalibration::get_raw_mat(int width, int height, unsigned short* cs_img)
{
	cv::Mat mat = cv::Mat(height, width, CV_16UC1, cs_img);
	cv::Mat img = mat.clone();// clone 안해주면 아마 원본 포인터 그대로 당겨서 쓰는게 아닌가싶다
	return img;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
cv::Mat CPspCalibration::polyfit(const cv::Mat& src_x, const cv::Mat& src_y, int order)
{
	CV_Assert((src_x.rows > 0) && (src_y.rows > 0) && (src_x.cols == 1) && (src_y.cols == 1) && (order >= 1));
	Mat X = Mat::zeros(src_x.rows, order + 1, CV_32FC1);
	Mat copy;
	for (int i = 0; i <= order; i++)
	{
		copy = src_x.clone();
		pow(copy, i, copy);
		Mat M1 = X.col(i);
		copy.col(0).copyTo(M1);
	}
	Mat X_t, X_inv;
	transpose(X, X_t);
	invert((X_t * X), X_inv);
	return (X_inv * X_t) * src_y;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
float CPspCalibration::getGain(cv::Mat p1, cv::Mat p2, float level)
{
	return get_fitted_value(p2, level) / get_fitted_value(p1, get_fitted_value(p2, level));
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
float CPspCalibration::get_fitted_value(cv::Mat p, float valueInput)
{
	float* pP = (float*)p.data;
	auto vReturn = pP[4] * pow(valueInput, 4) + pP[3] * pow(valueInput, 3) + pP[2] * pow(valueInput, 2) + pP[1] * valueInput + pP[0];
	return static_cast<float>(vReturn);
}
