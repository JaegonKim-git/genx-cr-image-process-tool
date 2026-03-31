// Added by Makeit, on 20203/07/13.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

// Referenced from http://yoway.asuscomm.com/wordpress/2006/04/19/gdi-%EC%9D%98-%EB%A9%94%EB%AA%A8%EB%A6%AC-%EB%A6%AD-%EB%94%94%ED%85%8D%ED%8A%B8-%EB%B0%A9%EB%B2%95/
#ifndef _GDIPLUSH_H_INCLUDED_
#define _GDIPLUSH_H_INCLUDED_


/// @file               AntGdiplusHelper.h
/// @brief              GDI+ 메모리 연산자 재정의 헤더파일. 메모리 누수시 감지를 위함. 
/// @warning    Gdiplus.h 대신 해당 파일을 인클루드 해야 함.
/// @warning    Debug 에서는 GDIPLUS_USE_GDIPLUS_MEM 정의 하지 않음 -> Gdiplus 메모리 연산자를 사용하지 않고 릭 감지 가능
/// @warning    Release 에서는 GDIPLUS_USE_GDIPLUS_MEM를 정의함 -> Gdiplus 메모리 연산자를 사용하지 않고 릭 감지 못함

//
// GDI+ helper file v1.0
// Written by Zoltan Csizmadia (zoltan_csizmadia@yahoo.com)
//

// GDIPLUS_USE_GDIPLUS_MEM:
// 위의 메크로가 정의 되어있을때 GDI+ 에서 제공하는 메모리 할당 함수를 사용한다.
// 이 경우 _CrtXXXXX에 해당하는 메모리 디버깅용 함수가 일반적으로 작동하지 않는다.
// (원래 GDI+는 _CrtXXXX 가 작동하지 못했다. 즉 해당 메크로를 설정하는게 기본설정.)
//#define GDIPLUS_USE_GDIPLUS_MEM


#ifdef _GDIPLUS_H
#error Gdiplus.h is already included. You have to include this file instead.
#endif

// Memo: Makeit - native multi-targeting (visual studio 버전을 의미) 개발 지원을 위해서 어쩔 수 없음.
#pragma warning(push)
#pragma warning(disable:4458)

#define _GDIPLUSBASE_H

namespace Gdiplus
{
    namespace DllExports
    {
#include "GdiplusMem.h"
    };

    class GdiplusBase
    {
    public:
#ifdef _DEBUG
        static void* __cdecl GdiplusAlloc(size_t nSize, LPCSTR szFileName, int nLine)
        {
#ifdef GDIPLUS_USE_GDIPLUS_MEM
            UNREFERENCED_PARAMETER(szFileName);
            UNREFERENCED_PARAMETER(nLine);
            return DllExports::GdipAlloc(nSize);
#else
            return ::operator new(nSize, szFileName, nLine);
#endif  // GDIPLUS_USE_GDIPLUS_MEM
        }

        static void GdiplusFree(void* pVoid, LPCSTR szFileName, int nLine)
        {
#ifdef GDIPLUS_USE_GDIPLUS_MEM
            UNREFERENCED_PARAMETER(szFileName);
            UNREFERENCED_PARAMETER(nLine);
            DllExports::GdipFree(pVoid);
#else
            ::operator delete(pVoid, szFileName, nLine);
#endif  // GDIPLUS_USE_GDIPLUS_MEM
        }

        void* (operator new)(size_t nSize)
        {
            return GdiplusAlloc(nSize, __FILE__, __LINE__);
        }
        void* (operator new[])(size_t nSize)
        {
            return GdiplusAlloc(nSize, __FILE__, __LINE__);
        }
        void* (operator new)(size_t nSize, LPCSTR lpszFileName, int nLine)
        {
            return GdiplusAlloc(nSize, lpszFileName, nLine);
        }
        void(operator delete)(void* pVoid)
        {
            GdiplusFree(pVoid, __FILE__, __LINE__);
        }
        void(operator delete[])(void* pVoid)
        {
            GdiplusFree(pVoid, __FILE__, __LINE__);
        }
        void operator delete(void* pVoid, LPCSTR lpszFileName, int nLine)
        {
            GdiplusFree(pVoid, lpszFileName, nLine);
        }
#else // _DEBUG

        static void* __cdecl GdiplusAlloc(size_t nSize)
        {
#ifdef GDIPLUS_USE_GDIPLUS_MEM
            return DllExports::GdipAlloc(nSize);
#else
            return ::operator new(nSize);
#endif  // GDIPLUS_USE_GDIPLUS_MEM
        }

        static void GdiplusFree(void* pVoid)
        {
#ifdef GDIPLUS_USE_GDIPLUS_MEM
            DllExports::GdipFree(pVoid);
#else
            ::operator delete(pVoid);
#endif  // GDIPLUS_USE_GDIPLUS_MEM
        }

        void* (operator new)(size_t nSize)
        {
            return GdiplusAlloc(nSize);
        }
        void* (operator new[])(size_t nSize)
        {
            return GdiplusAlloc(nSize);
        }
        void(operator delete)(void* pVoid)
        {
            GdiplusFree(pVoid);
        }
        void(operator delete[])(void* pVoid)
        {
            GdiplusFree(pVoid);
        }
#endif  // _DEBUG
    };
};

#include <Gdiplus.h>

#pragma warning(pop)

#endif
