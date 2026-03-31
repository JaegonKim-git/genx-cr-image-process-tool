#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // 거의 사용되지 않는 내용은 Windows 헤더에서 제외합니다.
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 일부 CString 생성자는 명시적으로 선언됩니다.

#include <afxwin.h>         // MFC 핵심 및 표준 구성 요소입니다.
#include <afxext.h>         // MFC 확장입니다.

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE 클래스입니다.
#include <afxodlgs.h>       // MFC OLE 대화 상자 클래스입니다.
#include <afxdisp.h>        // MFC 자동화 클래스입니다.
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC 데이터베이스 클래스입니다.
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO 데이터베이스 클래스입니다.
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // Internet Explorer 4 공용 컨트롤에 대한 MFC 지원입니다.
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // Windows 공용 컨트롤에 대한 MFC 지원입니다.
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>            // MFC 소켓 확장


//----------------------------------------------------------------
// 
//----------------------------------------------------------------

#include <cstdarg>
#include <vector>
#include <string>
#include <cstdint>


#include "../include/inlineFunctions.h"		// to use KLOG3, KLOGW, KLOGA
#include "crx_processing.hpp"


// kiwa72(2022.11.24 18h) - CRC-32/MPEG-2 적용 유(1)/무(0)
#define DEF_CRC32_MPEG2			(1)

// kiwa72(2022.12.06 00h) - 'Backward Offset Length' 추가
#define DEF_BACKWARD_OFFSET_LENGTH		(1)



// 일부 malloc 을 사용하는 코드들의 메모리 누수를 잡기 위해서 사용.
#ifdef _DEBUG
#define DEBUG_MALLOC(size)	_malloc_dbg(size, _NORMAL_BLOCK, __FILE__, __LINE__)
#define malloc				DEBUG_MALLOC
#define DEBUG_FREE(size)	_free_dbg(size, _NORMAL_BLOCK)
#define free				DEBUG_FREE
#endif

