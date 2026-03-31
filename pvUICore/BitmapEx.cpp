// 
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "BitmapEx.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



//BitmapEx::BitmapEx() : Bitmap(){}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
BitmapEx::~BitmapEx()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL BitmapEx::AverageFilter(int nLevel)
{
	if (nLevel == 0)
		return FALSE;

	Bitmap* pInput = this;

	PixelFormat fmt = pInput->GetPixelFormat();
	int nWidth = pInput->GetWidth();
	int nHeight = pInput->GetHeight();
	Rect rect(0, 0, nWidth, nHeight);

	Bitmap* pTemp = this->Clone(rect, fmt);
	int nScalarSize = 1;

	int nBits = 8;
	switch (fmt)
	{
	case PixelFormat1bppIndexed:
	case PixelFormat4bppIndexed:
	case PixelFormat8bppIndexed:
		nBits = 8;
		break;
	case PixelFormat16bppGrayScale:
	case PixelFormat16bppRGB555:
	case PixelFormat16bppRGB565:
	case PixelFormat16bppARGB1555:
		nBits = 16;
		nScalarSize = 2;
		break;
	case PixelFormat24bppRGB:
		nBits = 24;
		break;
	case PixelFormat32bppRGB:
	case PixelFormat32bppARGB:
	case PixelFormat32bppPARGB:
		nBits = 32;
		break;
	case PixelFormat48bppRGB:
		nBits = 48;
		break;
	case PixelFormat64bppARGB:
	case PixelFormat64bppPARGB:
		nBits = 64;
		break;
	}
	int nBytes = nBits / 8;

	BitmapData thisData;
	BitmapData tempData;
	pInput->LockBits(&rect, ImageLockModeRead | ImageLockModeWrite, fmt, &thisData);
	pTemp->LockBits(&rect, ImageLockModeRead, fmt, &tempData);

	//int nStride = thisData.Stride;
	//int nOffset = nStride - nWidth * nBytes; // bytes to skip at end of each row

	if (nScalarSize == 1)
	{
		BYTE* pSrcBuf = (BYTE*)thisData.Scan0;
		BYTE* pTmpBuf = (BYTE*)tempData.Scan0;
		if (pSrcBuf && pTmpBuf)
		{
			BYTE* p = pTmpBuf;
			double	xv = (1. / 9.);
			double	k[3][3] = { {xv,xv,xv},{xv,xv,xv},{xv,xv,xv} };
			int		w = nWidth;
			int		h = nHeight;
			double	v = 0;
			int c = nBytes;
			for (int y = nLevel; y < h - nLevel; y++)
			{
				for (int x = nLevel; x < w - nLevel; x++)
				{
					for (int i = 0; i < c; i++) {
						v = (double)p[(y - 1) * w * c + (x - 1) * c + i] * k[0][0] + (double)p[(y - 1) * w * c + (x + 0) * c + i] * k[0][1] + (double)p[(y - 1) * w * c + (x + 1) * c + i] * k[0][2]
							+ (double)p[(y + 0) * w * c + (x - 1) * c + i] * k[1][0] + (double)p[(y + 0) * w * c + (x + 0) * c + i] * k[1][1] + (double)p[(y + 0) * w * c + (x + 1) * c + i] * k[1][2]
							+ (double)p[(y + 1) * w * c + (x - 1) * c + i] * k[2][0] + (double)p[(y + 1) * w * c + (x + 0) * c + i] * k[2][1] + (double)p[(y + 1) * w * c + (x + 1) * c + i] * k[2][2];
						pSrcBuf[(y + 0) * w * c + (x + 0) * c + i] = static_cast<BYTE>(v);
					}
				}
			}
		}
	}
	else
	{
		unsigned short* pSrcBuf = (unsigned short*)thisData.Scan0;
		unsigned short* pTmpBuf = (unsigned short*)tempData.Scan0;
		if (pSrcBuf && pTmpBuf)
		{
			unsigned short* p = pTmpBuf;
			double	xv = (1. / 9.);
			double	k[3][3] = { {xv,xv,xv},{xv,xv,xv},{xv,xv,xv} };
			int		w = nWidth;
			int		h = nHeight;
			double	v = 0;
			int c = 1;
			for (int y = nLevel; y < h - nLevel; y++)
			{
				for (int x = nLevel; x < w - nLevel; x++)
				{
					for (int i = 0; i < c; i++) {
						v = (double)p[(y - 1) * w * c + (x - 1) * c + i] * k[0][0] + (double)p[(y - 1) * w * c + (x + 0) * c + i] * k[0][1] + (double)p[(y - 1) * w * c + (x + 1) * c + i] * k[0][2]
							+ (double)p[(y + 0) * w * c + (x - 1) * c + i] * k[1][0] + (double)p[(y + 0) * w * c + (x + 0) * c + i] * k[1][1] + (double)p[(y + 0) * w * c + (x + 1) * c + i] * k[1][2]
							+ (double)p[(y + 1) * w * c + (x - 1) * c + i] * k[2][0] + (double)p[(y + 1) * w * c + (x + 0) * c + i] * k[2][1] + (double)p[(y + 1) * w * c + (x + 1) * c + i] * k[2][2];
						pSrcBuf[(y + 0) * w * c + (x + 0) * c + i] = static_cast<USHORT>(v);
					}
				}
			}
		}
	}

	pInput->UnlockBits(&thisData);
	pTemp->UnlockBits(&tempData);
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL BitmapEx::MedianFilter(int nLevel)
{
	/*
	return AverageFilter(nLevel);
	/*/
	if (nLevel == 0) return FALSE;

	Bitmap* pInput = this;

	PixelFormat fmt = pInput->GetPixelFormat();
	int  nWidth = pInput->GetWidth();
	int  nHeight = pInput->GetHeight();
	Rect rect(0, 0, nWidth, nHeight);

	Bitmap* pTemp = this->Clone(rect, fmt);
	int nScalarSize = 1;

	int nBits = 8;
	switch (fmt)
	{
	case PixelFormat1bppIndexed:
	case PixelFormat4bppIndexed:
	case PixelFormat8bppIndexed:
		nBits = 8;
		break;
	case PixelFormat16bppGrayScale:
	case PixelFormat16bppRGB555:
	case PixelFormat16bppRGB565:
	case PixelFormat16bppARGB1555:
		nBits = 16;
		nScalarSize = 2;
		break;
	case PixelFormat24bppRGB:
		nBits = 24;
		break;
	case PixelFormat32bppRGB:
	case PixelFormat32bppARGB:
	case PixelFormat32bppPARGB:
		nBits = 32;
		break;
	case PixelFormat48bppRGB:
		nBits = 48;
		break;
	case PixelFormat64bppARGB:
	case PixelFormat64bppPARGB:
		nBits = 64;
		break;
	}
	int nBytes = nBits / 8;

	BitmapData thisData;
	BitmapData tempData;
	pInput->LockBits(&rect, ImageLockModeRead | ImageLockModeWrite, fmt, &thisData);
	pTemp->LockBits(&rect, ImageLockModeRead, fmt, &tempData);

	//int nStride = thisData.Stride;
	//int nOffset = nStride - nWidth * nBytes; // bytes to skip at end of each row

	double kernel[9] = { 0,0,0,0,0,0,0,0,0 };
	std::vector<double> vec(kernel, kernel + 9);

	if (nScalarSize == 1)
	{
		BYTE* pSrcBuf = (BYTE*)thisData.Scan0;
		BYTE* pTmpBuf = (BYTE*)tempData.Scan0;
		if (pSrcBuf && pTmpBuf)
		{
			BYTE* p = pTmpBuf;
			int		w = nWidth;
			int		h = nHeight;
			int		c = nBytes;
			for (int y = nLevel; y < h - nLevel; y++)
			{
				for (int x = nLevel; x < w - nLevel; x++)
				{
					for (int i = 0; i < c; i++) {
						kernel[0] = (double)p[(y - 1) * w * c + (x - 1) * c + i];
						kernel[1] = (double)p[(y - 1) * w * c + (x + 0) * c + i];
						kernel[0] = (double)p[(y - 1) * w * c + (x + 1) * c + i];

						kernel[0] = (double)p[(y + 0) * w * c + (x - 1) * c + i];
						kernel[0] = (double)p[(y + 0) * w * c + (x + 0) * c + i];
						kernel[0] = (double)p[(y + 0) * w * c + (x + 1) * c + i];

						kernel[0] = (double)p[(y + 1) * w * c + (x - 1) * c + i];
						kernel[0] = (double)p[(y + 1) * w * c + (x + 0) * c + i];
						kernel[0] = (double)p[(y + 1) * w * c + (x + 1) * c + i];

						std::sort(vec.begin(), vec.end(), std::greater<double>());
						pSrcBuf[(y + 0) * w * c + (x + 0) * c + i] = static_cast<BYTE>(vec[4]);
					}
				}
			}
		}
	}
	else
	{
		unsigned short* pSrcBuf = (unsigned short*)thisData.Scan0;
		unsigned short* pTmpBuf = (unsigned short*)tempData.Scan0;
		if (pSrcBuf && pTmpBuf)
		{
			unsigned short* p = pTmpBuf;
			int		w = nWidth;
			int		h = nHeight;
			for (int y = nLevel; y < h - nLevel; y++)
			{
				for (int x = nLevel; x < w - nLevel; x++)
				{
					kernel[0] = (double)p[(y - 1) * w + (x - 1)];
					kernel[1] = (double)p[(y - 1) * w + (x + 0)];
					kernel[2] = (double)p[(y - 1) * w + (x + 1)];

					kernel[3] = (double)p[(y + 0) * w + (x - 1)];
					kernel[4] = (double)p[(y + 0) * w + (x + 0)];
					kernel[5] = (double)p[(y + 0) * w + (x + 1)];

					kernel[6] = (double)p[(y + 1) * w + (x - 1)];
					kernel[7] = (double)p[(y + 1) * w + (x + 0)];
					kernel[8] = (double)p[(y + 1) * w + (x + 1)];

					std::sort(vec.begin(), vec.end(), std::greater<double>());
					pSrcBuf[(y + 0) * w + (x + 0)] = static_cast<USHORT>(vec[4]);
				}
			}
		}
	}

	pInput->UnlockBits(&thisData);
	pTemp->UnlockBits(&tempData);
	return TRUE;
	//*/
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL BitmapEx::Combine(Bitmap* pBitmap, int nWidth, int nHeight, int nSrcX, int nSrcY, int nTgtX, int nTgtY, int nOpMode)//OP_ADD)
{
	UNREFERENCED_PARAMETER(nSrcX);
	UNREFERENCED_PARAMETER(nSrcY);
	UNREFERENCED_PARAMETER(nTgtX);
	UNREFERENCED_PARAMETER(nTgtY);

	if (!pBitmap) return FALSE;

	Bitmap* pInput = this;

	//int nW = pBitmap->GetWidth();
	//int nH = pBitmap->GetHeight();

	PixelFormat fmt = pInput->GetPixelFormat();
	/*int*/  nWidth = pInput->GetWidth();
	/*int*/  nHeight = pInput->GetHeight();
	Rect rect(0, 0, nWidth, nHeight);

	int nScalarSize = 1;
	int nBits = 8;
	switch (fmt)
	{
	case PixelFormat1bppIndexed:
	case PixelFormat4bppIndexed:
	case PixelFormat8bppIndexed:
		nBits = 8;
		break;
	case PixelFormat16bppGrayScale:
	case PixelFormat16bppRGB555:
	case PixelFormat16bppRGB565:
	case PixelFormat16bppARGB1555:
		nBits = 16;
		nScalarSize = 2;
		break;
	case PixelFormat24bppRGB:
		nBits = 24;
		break;
	case PixelFormat32bppRGB:
	case PixelFormat32bppARGB:
	case PixelFormat32bppPARGB:
		nBits = 32;
		break;
	case PixelFormat48bppRGB:
		nBits = 48;
		break;
	case PixelFormat64bppARGB:
	case PixelFormat64bppPARGB:
		nBits = 64;
		break;
	}
	int nBytes = nBits / 8;

	BitmapData thisData;
	BitmapData tempData;
	pInput->LockBits(&rect, ImageLockModeWrite, fmt, &thisData);
	pBitmap->LockBits(&rect, ImageLockModeWrite, fmt, &tempData);

	//int nStride = thisData.Stride;
	//int nOffset = nStride - nWidth * nBytes; // bytes to skip at end of each row

	if (nScalarSize == 1)
	{
		BYTE* pSrcBuf = (BYTE*)thisData.Scan0;
		BYTE* pTmpBuf = (BYTE*)tempData.Scan0;

		if (pSrcBuf && pTmpBuf)
		{
			//BYTE* p = pSrcBuf;
			double	xv = (1. / 9.);
			double	k[3][3] = { {xv,xv,xv},{xv,xv,xv},{xv,xv,xv} };
			int		w = nWidth;
			int		h = nHeight;
			//double	v = 0;
			int c = nBytes;
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					for (int i = 0; i < c; i++) {
						switch (nOpMode) {
						case 0://OP_ADD:
							pSrcBuf[(y)*w * c + (x)*c + i] += pTmpBuf[(y)*w * c + (x)*c + i];
							break;
						case 1://OP_SUBSRC:
							pSrcBuf[y * w * c + x * c + i] = pTmpBuf[y * w * c + x * c + i] - pSrcBuf[y * w * c + x * c + i];
							break;
						}
					}
				}
			}
		}
	}
	else
	{
		unsigned short* pSrcBuf = (unsigned short*)thisData.Scan0;
		unsigned short* pTmpBuf = (unsigned short*)tempData.Scan0;

		if (pSrcBuf && pTmpBuf)
		{
			//unsigned short* p = pSrcBuf;
			int		w = nWidth;
			int		h = nHeight;
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					switch (nOpMode) {
					case 0: //OP_ADD:
						pSrcBuf[y * w + x] += pTmpBuf[y * w + x];
						break;
					case 1: //OP_SUBSRC:
						pSrcBuf[y * w + x] = pTmpBuf[y * w + x] - pSrcBuf[y * w + x];
						break;
					}
				}
			}
		}
	}
	pInput->UnlockBits(&thisData);
	pBitmap->UnlockBits(&tempData);
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL BitmapEx::Fill(int v)
{
	Bitmap* pInput = this;

	PixelFormat fmt = pInput->GetPixelFormat();
	int  nWidth = pInput->GetWidth();
	int  nHeight = pInput->GetHeight();
	Rect rect(0, 0, nWidth, nHeight);

	int nScalarSize = 1;
	int nBits = 8;
	switch (fmt)
	{
	case PixelFormat1bppIndexed:
	case PixelFormat4bppIndexed:
	case PixelFormat8bppIndexed:
		nBits = 8;
		break;
	case PixelFormat16bppGrayScale:
	case PixelFormat16bppRGB555:
	case PixelFormat16bppRGB565:
	case PixelFormat16bppARGB1555:
		nBits = 16;
		nScalarSize = 2;
		break;
	case PixelFormat24bppRGB:
		nBits = 24;
		break;
	case PixelFormat32bppRGB:
	case PixelFormat32bppARGB:
	case PixelFormat32bppPARGB:
		nBits = 32;
		break;
	case PixelFormat48bppRGB:
		nBits = 48;
		break;
	case PixelFormat64bppARGB:
	case PixelFormat64bppPARGB:
		nBits = 64;
		break;
	}
	int nBytes = nBits / 8;

	BitmapData thisData;
	pInput->LockBits(&rect, ImageLockModeRead | ImageLockModeWrite, fmt, &thisData);

	//int nStride = thisData.Stride;
	//int nOffset = nStride - nWidth * nBytes; // bytes to skip at end of each row

	if (nScalarSize == 1)
	{
		BYTE* pSrcBuf = (BYTE*)thisData.Scan0;

		if (pSrcBuf)
		{
			BYTE* p = pSrcBuf;
			int		w = nWidth;
			int		h = nHeight;
			int		c = nBytes;
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					for (int i = 0; i < c; i++) {
						p[y * w * c + x * c + i] = static_cast<BYTE>(v);
					}
				}
			}
		}
	}
	else
	{
		unsigned short* pSrcBuf = (unsigned short*)thisData.Scan0;

		if (pSrcBuf)
		{
			unsigned short* p = pSrcBuf;
			int		w = nWidth;
			int		h = nHeight;
			for (int y = 0; y < h; y++)
			{
				for (int x = 0; x < w; x++)
				{
					p[y * w + x] = static_cast<USHORT>(v);
				}
			}
		}
	}

	pInput->UnlockBits(&thisData);
	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
Bitmap* BitmapEx::Resize(INT nNewWidth, INT nNewHeight)
{
	int nWidth = this->GetWidth();
	int nHeight = this->GetHeight();

	PixelFormat fmt = this->GetPixelFormat();
	Bitmap* pBitmap = new Bitmap(nNewWidth, nNewHeight, fmt);

	/*TEST
		// Initialize the color matrix.
		// Notice the value 0.8 in row 4, column 4.

		ColorMatrix colorMatrix
			= { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 0.8f, 0.0f,
				0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

		// Create an ImageAttributes object and set its color matrix.
		ImageAttributes imageAtt;
		imageAtt.SetColorMatrix(&colorMatrix, ColorMatrixFlagsDefault,
											ColorAdjustTypeBitmap);

		//During rendering, the alpha values in the bitmap are converted to 80 percent of what they were.
		//This results in an image that is blended with the background. As the following illustration shows, the bitmap image looks transparent; you can see the solid black line through it.
	//*/
	REAL scaleX = (REAL)nNewWidth / (REAL)nWidth;
	REAL scaleY = (REAL)nNewHeight / (REAL)nHeight;

	Matrix* matrix = new Matrix();
	matrix->Scale(scaleX, scaleY, MatrixOrderAppend);

	Graphics graphics(pBitmap);
	graphics.SetSmoothingMode(SmoothingModeAntiAlias);
	graphics.SetInterpolationMode(InterpolationModeBilinear);
	graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);

	graphics.SetTransform(matrix);

	graphics.DrawImage(this, 0, 0, nNewWidth, nNewHeight
		//, UnitPixel, &imageAtt
	);

	delete matrix;
	return pBitmap;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
template <class T>
BOOL CMce<T>::AverageFilter(T* pPixels, IMAGE_INFO* pii, int nLevel)
{
	if(!pPixels || nLevel==0)
		return FALSE;
	T		*p= pPixels;
	double	xv= (1./9.);
	double	k[3][3]={{xv,xv,xv},{xv,xv,xv},{xv,xv,xv}};
	int		w = pii->nWidth;
	int		h = pii->nHeight;
	double	v = 0;
	int c=1;
	for(int y=nLevel; y<h-nLevel; y++)
	{
		for(int x=nLevel; x<w-nLevel; x++)
		{
			for(int i=0; i<c; i++){
				v = (double)p[(y-1)*w*c+(x-1)*c+i]*k[0][0] + (double)p[(y-1)*w*c+(x+0)*c+i]*k[0][1] + (double)p[(y-1)*w*c+(x+1)*c+i]*k[0][2]
				  + (double)p[(y+0)*w*c+(x-1)*c+i]*k[1][0] + (double)p[(y+0)*w*c+(x+0)*c+i]*k[1][1] + (double)p[(y+0)*w*c+(x+1)*c+i]*k[1][2]
				  + (double)p[(y+1)*w*c+(x-1)*c+i]*k[2][0] + (double)p[(y+1)*w*c+(x+0)*c+i]*k[2][1] + (double)p[(y+1)*w*c+(x+1)*c+i]*k[2][2];
				p[(y+0)*w*c+(x+0)*c+i] = v;
			}
		}
	}
	return TRUE;
}

template <class T>
BOOL CMce<T>::MedianFilter(T* pPixels, IMAGE_INFO* pii, int nLevel)
{
	return AverageFilter(pPixels, pii, nLevel);
}
*/

