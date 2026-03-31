// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "common.h"
#include "postprocess.h"
#include "tiff/tiffio.h"
#include "tiff_image.h"
#include "global_data.h"
#include "common_type.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define  STB_IMAGE_IMPLEMENTATION // needs to be defined within a .c or.cpp file, not within a header

wchar_t loaded_image_name[MAX_PATH];
bool is_received_image = false;

int is_origin_raw = 0;
extern stcalibration default_calibrationdata;
extern stcalibration cur_calibrationdata;

extern int is_valid_stcalibration(stcalibration* stcali);


//------------------------------+---------------------------------------------------------------

static size_t sizes[] =
{
	geo_x * size0_h, geo_x * size1_h ,geo_x * size2_h ,geo_x * size3_h,
	geo_x * size0_h / 2, geo_x * size1_h / 2 ,geo_x * size2_h / 2 ,geo_x * size3_h / 2
};

static size_t sizes2[] =
{
	geo_x * (size0_h + 200), geo_x * (size1_h + 200) ,geo_x * (size2_h + 200) ,geo_x * (size3_h + 200),
	geo_x * (size0_h + 200) / 2, geo_x * (size1_h + 200) / 2 ,geo_x * (size2_h + 200) / 2 ,geo_x * (size3_h + 200) / 2
};

static size_t sizes3[] =
{
	geo_x * (size3_h + 600),
	geo_x * (size3_h + 600) / 2

};

int loaded_tiff_width = 0;
int loaded_tiff_height = 0;
extern unsigned int mirror_values[10];


int get_ip_size_of_dump(size_t size)
{
	static size_t size_of_dumps[] =
	{
		880 * 1240 * sizeof(short), 960 * 1600 * sizeof(short), 1240 * 1640 * sizeof(short), 1080 * 2160 * sizeof(short),//SR
		880 * 1240 * sizeof(char), 960 * 1600 * sizeof(char), 1240 * 1640 * sizeof(char), 1080 * 2160 * sizeof(char) // HR
	};

	for (int i = 0; i < 8; i++)
	{
		if (size_of_dumps[i] == size)
			return i;
	}

	return -1;
}


int  is_raw_size(size_t size)
{
	for (int j = 1; j < 3; j++)
	{
		size /= j;
		for (int i = 0; i < sizeof(sizes) / sizeof(size_t); i++)
		{
			if (size == sizes[i])
				return 1;
		}

		for (int i = 0; i < sizeof(sizes2) / sizeof(size_t); i++)
		{
			if (size == sizes2[i])
				return 2;
		}

		for (int i = 0; i < sizeof(sizes3) / sizeof(size_t); i++)
		{
			if (size == sizes3[i])
				return 3;
		}
	}

	return 0;
}

//------------------------------+---------------------------------------------------------------

void clear_raw(void)
{
	raw_t* data = GetPdxData()->map_raw.data();
	memset(data, 0, GetPdxData()->map_raw.w() * GetPdxData()->map_raw.h() * sizeof(raw_t));
}


void shift_raw(int size)
{
	//xldosxl
	if (size == 0 || size == 2)
	{
		MYLOGOLDW(L"shift_raw : latest raw or fin 1\n");
		return;
	}

	if (size == 1 || is_raw_size(size) == 1)  /*recevie image*/
	{
		MYLOGOLDW(L"shift_raw : old height raw image\n");
		GetPdxData()->map_origin = GetPdxData()->map_raw;//use origin to buffer
		clear_raw();

		static size_t heights[] =
		{
			size0_h, size1_h, size2_h, size3_h,
			size0_h / 2,size1_h / 2,size2_h / 2,size3_h / 2
		};

		int h = size3_h;
		int offset = 100;

		for (int i = 0; i < sizeof(heights) / sizeof(size_t); i++)
		{
			if ((size / (sizeof(raw_t) * geo_x)) == heights[i])
			{
				h = (int)heights[i];
				if (i > 3)
				{
					offset = 50;
				}

				char temp[256];
				sprintf(temp, "shift_rarw target h: %d  offset %d\n", h, offset);
				MYLOGOLDA(temp);
				break;
			}
		}

		raw_t* src = GetPdxData()->map_origin.data();
		raw_t* dst = GetPdxData()->map_raw.data();
		dst += (offset * geo_x);
		memcpy(dst, src, sizeof(raw_t) * geo_x * h);
	}
	else
	{
		MYLOGOLDW(L"shift_raw : latest raw or fin 2\n");
	}
}

bool get_image_of_tiff(co wchar_t* wstr, IN OUT int& w, IN OUT int& h)
{
	if (is_tiff_file(wstr) < 1)
	{
		return false;
	}

	TIFF* oTIFF = TIFFOpenW(wstr, "r");
	if (!oTIFF)
	{
		return false;
	}

	TIFFGetField(oTIFF, TIFFTAG_IMAGELENGTH, &h);
	TIFFGetField(oTIFF, TIFFTAG_IMAGEWIDTH, &w);
	auto is_raw = is_raw_size(sizeof(raw_t) * w * h);//loadtif

	memset(GetPdxData()->map_img16buffer.data(), 0, sizeof(raw_t) * GetPdxData()->map_img16buffer.w() * GetPdxData()->map_img16buffer.h());
	auto* line = GetPdxData()->map_img16buffer.data();

	for (auto iter = GetPdxData()->map_img16buffer.begin(); iter != GetPdxData()->map_img16buffer.end(); iter++)
	{
		*iter = 0;
	}

	for (int y = 0; y < h; y++)
	{
		TIFFReadScanline(oTIFF, line, y);
		line += w;	// 띄우지말고 붙여서 쌓아야 함
	}

	TIFFClose(oTIFF);
	return is_raw;
}

bool load_tiff(TEX& img, co wchar_t* wstr)
{
	UNREFERENCED_PARAMETER(img);

	if (is_tiff_file(wstr) < 1)
	{
		return false;
	}

	TIFF* oTIFF = TIFFOpenW(wstr, "r");
	if (!oTIFF)
	{
		return false;
	}

	int w = 0, h = 0;

	TIFFGetField(oTIFF, TIFFTAG_IMAGELENGTH, &h);
	TIFFGetField(oTIFF, TIFFTAG_IMAGEWIDTH, &w);

	auto is_raw = is_raw_size(sizeof(raw_t) * w * h);//loadtif
	is_origin_raw = is_raw;
	if (is_raw)
	{
		MYLOGOLDW(L"raw size tiff\n");
	}
	else
	{
		MYLOGOLDW(L"fin size tiff\n");
		loaded_tiff_width = w;
		loaded_tiff_height = h;
	}

	wchar_t temp[256];
	wsprintf(temp, L"w %d  h %d\n", w, h);
	MYLOGOLDW(temp);

	if (is_raw)
	{
		memset(GetPdxData()->map_raw.data(), 0, sizeof(raw_t) * GetPdxData()->map_raw.w() * GetPdxData()->map_raw.h());
	}
	else
	{
		memset(GetPdxData()->map_fin.data(), 0, sizeof(raw_t) * GetPdxData()->map_fin.w() * GetPdxData()->map_fin.h());
	}

	auto* line = is_raw ? GetPdxData()->map_raw.data() : GetPdxData()->map_fin.data();
	for (auto iter = GetPdxData()->map_fin.begin(); iter != GetPdxData()->map_fin.end(); iter++)
	{
		*iter = 0;
	}

	for (int y = 0; y < h; y++)
	{
		TIFFReadScanline(oTIFF, line, y);
		line += GetPdxData()->map_fin.w();
	}

	TIFFClose(oTIFF);
	shift_raw(is_raw);
	GetPdxData()->map_origin = is_raw ? GetPdxData()->map_raw : GetPdxData()->map_fin;//xldosxl

	if (is_raw)
	{
		for (auto iterf = GetPdxData()->map_fin.begin(), iterr = GetPdxData()->map_raw.begin(); iterf != GetPdxData()->map_fin.end(); iterf++, iterr++)
		{
			*iterf = *iterr;
		}
	}

	GetPdxData()->map_geo = GetPdxData()->map_fin;
	GetPdxData()->img_tex.size = GetPdxData()->map_fin.d();
	GetPdxData()->img_tex.channels = 1;
	GetPdxData()->g_is_image_loaded = 4;
	GetPdxData()->command_refresh_image();//load_tiff

	return is_raw;
}
