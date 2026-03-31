#pragma once
#include <vector>
#define MAX_CALI_IMAGE_COUNT 10
#define DEGREE_COUNT	4

struct crxSIZE
{
	int width;
	int height;
};

enum class	 RESOLUTION : unsigned char
{
	High = 0,
	Low = 1,
};

struct							InfoProcessing
{
	RESOLUTION					resolution;
	crxSIZE						size_raw;
	crxSIZE						size_pp;
	int							size_ip; // 0, 1, 2, 3
	int							result;
	std::vector<unsigned short> result_img;
};

struct							PspCalibrationParameters
{
	float						coefs[((DEGREE_COUNT + 1)* MAX_CALI_IMAGE_COUNT)];
	unsigned short				means[MAX_CALI_IMAGE_COUNT];
};