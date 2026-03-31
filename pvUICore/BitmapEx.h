// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../include/myGdiPlus.h"
#include <gdiplusheaders.h>

using namespace Gdiplus;



class AFX_EXT_CLASS BitmapEx : public Bitmap
{
public:
	//BitmapEx();
	BitmapEx(INT nWidth, INT nHeight, PixelFormat format = (10 | (32 << 8) | 262144 | 131072 | 2097152))
		: Bitmap(nWidth, nHeight, format) {};
	virtual ~BitmapEx();


public:
	Bitmap* Resize(INT nWidth, INT nHeight);
	inline Bitmap* Crop(Rect r);
	BOOL AverageFilter(int l = 0);
	BOOL MedianFilter(int l = 0);
	BOOL Fill(int v);
	BOOL Combine(Bitmap* pBitmap, int nWidth, int nHeight, int nSrcX, int nSrcY, int nTgtX, int nTgtY, int nOpMode = 0); //OP_ADD)
	Bitmap* Size(INT nNewWidth, INT nNewHeight)
	{
		Bitmap* pNewBitmap = (Bitmap*)this->GetThumbnailImage(nNewWidth, nNewHeight);
		return pNewBitmap;
	}
};


/*
public static Bitmap Resize(Bitmap b, int nWidth, int nHeight, bool bBilinear)
{
	Bitmap bTemp = (Bitmap)b.Clone();
	b = new Bitmap(nWidth, nHeight, bTemp.PixelFormat);

	double nXFactor = (double)bTemp.Width/(double)nWidth;
	double nYFactor = (double)bTemp.Height/(double)nHeight;

	if (bBilinear)
	{
		// Not yet 80)
	}
	else
	{
		for (int x = 0; x < b.Width; ++x)
			for (int y = 0; y < b.Height; ++y)
				b.SetPixel(x, y, bTemp.GetPixel((int)(Math.Floor(x * nXFactor)),
						  (int)(Math.Floor(y * nYFactor))));
	}

	return b;
}
*/


inline Bitmap* BitmapEx::Crop(Rect r)
{
	Bitmap* pBitmap = this->Clone(r, this->GetPixelFormat());
	return pBitmap;
}
