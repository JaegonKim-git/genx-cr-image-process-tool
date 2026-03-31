#pragma once

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>
#include <cassert>



#ifdef DLL_EXPORT
#define DLL_DECLSPEC		__declspec(dllexport)
#else
#define DLL_DECLSPEC		__declspec(dllimport)
#endif



#pragma warning(push)
#pragma warning(disable:4505)
#include "../include/inlineFunctions.h"
#pragma warning(pop)

