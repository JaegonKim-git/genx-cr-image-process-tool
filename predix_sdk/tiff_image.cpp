// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "tiff/tiffio.h"
#include "tiff_image.h"
#include "common_type.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



void save_tiff_image(wchar_t* file_name, unsigned short int* data, int w, int h, int w2, float xresolution, float yresolution)
{
    TIFF* oTIFF = nullptr;
    try
    {
        if (w2 < 1)
            w2 = w;
        int Compression = COMPRESSION_NONE;
        int Quality = 100;

        struct tm* local_time = new struct tm;

        // Get time as 64-bit integer.
        __time64_t long_time;
        _time64(&long_time);
        localtime_s(local_time, &long_time);

        char time[100];
        sprintf_s(time, 100, "%04d.%02d.%02d %02d:%02d:%02d", local_time->tm_year + 1900, local_time->tm_mon + 1, local_time->tm_mday,
            local_time->tm_hour, local_time->tm_min, local_time->tm_sec);

        delete local_time;

        char file_name_ch[1024];
        int len = ::WideCharToMultiByte(CP_ACP, 0, file_name, -1, file_name_ch, 0, NULL, NULL);
        WideCharToMultiByte(CP_ACP, 0, file_name, len, file_name_ch, len, NULL, NULL);

        oTIFF = TIFFOpen(file_name_ch, "w");
        if (!oTIFF)
        {
            Sleep(100);
            MYPLOGMA("retry open new file  %s\n", file_name_ch);
            oTIFF = TIFFOpen(file_name_ch, "w");
        }
        if (oTIFF)
        {
            TIFFSetField(oTIFF, TIFFTAG_MODEL, "PREDIX");
            TIFFSetField(oTIFF, TIFFTAG_MAKE, "genoray");
            TIFFSetField(oTIFF, TIFFTAG_SOFTWARE, "PREDIX 1.0");
            TIFFSetField(oTIFF, TIFFTAG_DATETIME, time);
            TIFFSetField(oTIFF, TIFFTAG_SUBFILETYPE, 0);
            TIFFSetField(oTIFF, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
            TIFFSetField(oTIFF, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
            TIFFSetField(oTIFF, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
            TIFFSetField(oTIFF, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER);
            TIFFSetField(oTIFF, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
            TIFFSetField(oTIFF, TIFFTAG_IMAGEWIDTH, w);
            TIFFSetField(oTIFF, TIFFTAG_IMAGELENGTH, h);
            TIFFSetField(oTIFF, TIFFTAG_BITSPERSAMPLE, 16);
            TIFFSetField(oTIFF, TIFFTAG_SAMPLESPERPIXEL, 1);
            TIFFSetField(oTIFF, TIFFTAG_ROWSPERSTRIP, 32);
            TIFFSetField(oTIFF, TIFFTAG_XRESOLUTION, xresolution);
            TIFFSetField(oTIFF, TIFFTAG_YRESOLUTION, yresolution);
            TIFFSetField(oTIFF, TIFFTAG_XPOSITION, 0.f);
            TIFFSetField(oTIFF, TIFFTAG_YPOSITION, 0.f);
            TIFFSetField(oTIFF, TIFFTAG_COMPRESSION, Compression);
            TIFFSetField(oTIFF, TIFFTAG_IMAGEDESCRIPTION, "PREDIX image");
            if (Compression == COMPRESSION_JPEG)
                TIFFSetField(oTIFF, TIFFTAG_JPEGQUALITY, Quality);
            uint16* ptr = data;
            for (int y = 0; y < h; y++)
            {
                TIFFWriteScanline(oTIFF, ptr, y, 0);
                //ptr += w;
                ptr += w2;
            }
            TIFFWriteDirectory(oTIFF);
        }
        else
        {
            MYPLOGMA("Can't write file1 %s\n", file_name_ch);
        }
    }
    catch (...)
    {
        //Application->MessageBox(L"Can't write file", L"File Error", IDOK);
    }
    
    // 예외 발생 여부와 관계없이 핸들 닫기
    if (oTIFF)
    {
        TIFFClose(oTIFF);
    }
}
