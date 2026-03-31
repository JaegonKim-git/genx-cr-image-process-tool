// Added by Makeit, on 2023/06/14.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <memory>
#include <string>



// delete instance macro
#ifdef SAFE_DELETE
#error SAFE_DELETE macro already defined. Use only one define. Check your definitions.
#endif
#define SAFE_DELETE(x) if(x){ delete x; x = nullptr; }
#ifdef SAFE_DELETE_ARRAY
#error SAFE_DELETE_ARRAY macro already defined. Use only one define. Check your definitions.
#endif
#define SAFE_DELETE_ARRAY(x) if(x){ delete [] x; x = nullptr; }


// to fix compile errors
#ifdef UNICODE
#define __FUNCTION_NAME__ __FUNCTIONW__
#else
#define __FUNCTION_NAME__ __FUNCTION__
#endif


#ifndef _MFC_VER
#include <cassert>
#define ASSERT assert
#ifdef UNICODE
#define __T(x)		L ## x
#define CString std::wstring
#else
#define __T(x)      x
#define CString std::string
#endif
#define _T(x)		__T(x)
#define CStringA std::string
#define CStringW std::wstring
#endif


// 구분 가능한 product name 을 리턴한다.
static CString GetProductName(bool bIncludeVersion = false)
{
	// TODO: Makeit - 프로그램(product) 이름과 버전을 조합하여 구분 가능한 제품 이름을 리턴한다.
	// Global static 변수로 이름을 저장해두도록 application base 에서 처리한 후에
	// 해당 이름과 버전을 가져다가 조합해서 리턴하도록 개발하자. (Mini-dump 파일명 등으로 사용할 예정)
	CString strProduct = L"PortView";
	CString strVersion = L"2.2.5.19";	// _T(PORTVIEW_PRODUCTVERSION_VALUE);
	if (bIncludeVersion)
		strProduct += L"_" + strVersion;	// L"PortView_2.2.5.16";

	return strProduct;
}

// 함수가 호출된 현재 프로젝트의 이름(ProjectName)을 리턴한다.
static CString GetThisProjectName(void)
{
#ifdef _MFC_VER
	const char* pszProject = _THIS_PROJECT_;	
	CString strProjectName((LPCWSTR)CA2W(pszProject, CP_UTF8));	// Project 속성에서 정의해둘것.
	ASSERT(!strProjectName.IsEmpty());
	return strProjectName;
#else
	const TCHAR* pszProject = _T(_THIS_PROJECT_);		// Project 속성에서 정의해둘것.
	CString strProjectName = pszProject;
	ASSERT(!strProjectName.empty());
	return strProjectName;
#endif
}

// 함수가 호출된 현재 프로젝트의 DLL 파일명(ProjectName.DLL)을 리턴한다.
static CString GetThisProjectDllName(void)
{
	// TargetFileName을 사용하지 않는 이유는 프로젝트 이름만 필요한 경우도 있는데 매크로 2개 만들기 싫어서임.
	CString strProjectName = GetThisProjectName() + L".dll";
	return strProjectName;
}

// 함수가 호출된 현재 프로젝트의 EXE 파일명(ProjectName.EXE)을 리턴한다.
static CString GetThisProjectExeName(void)
{
	// TargetFileName을 사용하지 않는 이유는 프로젝트 이름만 필요한 경우도 있는데 매크로 2개 만들기 싫어서임.
	CString strProjectName = GetThisProjectName() + L".exe";
	return strProjectName;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get current module path (includes folder separator)
#ifdef _MFC_VER
static CString GetCurrentModPath(bool bIncludeFolderSepatator = true)
{
	CString strPath;

	TCHAR tszPath[MAX_PATH] = { 0, };
	GetModuleFileName(NULL, tszPath, MAX_PATH);

	CString strFolder(tszPath);
	auto nFound = strFolder.ReverseFind(_T('\\'));
	if (nFound > 0)
	{
		const int nLength = nFound + ( bIncludeFolderSepatator ? 1 : 0 );
		strPath = strFolder.Left( nLength );
	}
	else
	{
		ASSERT(0);
	}

	return strPath;
}
#endif


namespace gnInlines
{
	template <class T>
	inline T lbound(const T value, const T lb)
	{
		return (((value) < (lb)) ? (lb) : (value));
	}

	template <class T>
	inline T rbound(const T value, const T rb)
	{
		return (((value) > (rb)) ? (rb) : (value));
	}

	template <class T>
	inline T bound(const T value, const T lb, const T rb)
	{
		return lbound(rbound((value), (rb)), (lb));
	}
}

#define BOUND		::gnInlines::bound
#define LBOUND		::gnInlines::lbound
#define RBOUND		::gnInlines::rbound


// COLORREF related
#define GetRedByte(x)		(*((BYTE*)&x + 2))
#define GetGreenByte(x)		(*((BYTE*)&x + 1))
#define GetBlueByte(x)		(*((BYTE*)&x + 0))



// invalid characters for patient ID, name. (Because of using as filename)
#define INVALID_ID_CHARACTERS		_T("\\\"!@#$%%&*()[]{}<>_-+=|/:;?^'")
#define INVALID_NAME_CHARACTERS		_T("\\\"!@#$%%&*()[]{}<>_-+=|/:;?")
#define INVALID_FILENAME_CHARS		_T("<>:\"/\\|?*")
#define INVALID_FOLDERPATH_CHARS	_T("<>\"/|?*")




#ifdef _MFC_VER
// for CRUXELL

// KLOG3 매크로에 문제가 있어서 함수로 변경함.
#define	_FNW_		(wcsrchr(__FILEW__, L'\\') + 1)
#define	_FUW_		(__FUNCTIONW__)
#define	_LIW_		(__LINE__)
//#define KLOG3W(fmt, ...)	{ wchar_t debgmsg[MAX_PATH]; wsprintfW(debgmsg, L"[%s | %s | %d] "fmt" \n", _FNW_, _FUW_, _LIW_, ##__VA_ARGS__); OutputDebugStringW(debgmsg); _tprintf(debgmsg); }

#define	_FN_		(strrchr(__FILE__, '\\') + 1)
#define	_FU_		(__FUNCTION__)
#define	_LI_		(__LINE__)
//#define KLOG3A(fmt, ...)	{ char debgmsg[MAX_PATH]; sprintf(debgmsg, "[%s | %s | %d] "fmt" \n", _FN_, _FU_, _LI_, ##__VA_ARGS__); OutputDebugStringA(debgmsg); printf(debgmsg); }


static CStringW void2strW(LPCWSTR lpsz = nullptr)
{
	CStringW str(lpsz);
	return str;
}
template<typename... Types>
static CStringW void2strW(Types... args)
{
	CStringW str;
	str.Format(args...);
	return str;
}
static CStringA void2strA(LPCSTR lpsz = nullptr)
{
	CStringA str(lpsz);
	return str;
}
template<typename... Types>
static CStringA void2strA(Types... args)
{
	CStringA str;
	str.Format(args...);
	return str;
}

#define KLOG3W(...)\
{\
CStringW strHead; strHead.Format(L"[%s | %s | %d] ", _FNW_, _FUW_, _LIW_);\
CStringW params = void2strW(__VA_ARGS__);\
OutputDebugStringW(strHead + params + L" \n");\
wprintf(strHead + params + L" \n");\
}
#define KLOG3A(...)\
{\
CStringA strHead; strHead.Format("[%s | %s | %d] ", _FN_, _FU_, _LI_);\
CStringA params = void2strA(__VA_ARGS__);\
OutputDebugStringA(strHead + params + " \n");\
printf(strHead + params + " \n");\
}

#ifdef _UNICODE
#define KLOG3		KLOG3W
#else
#define KLOG3		KLOG3A
#endif


// Safe printf (to replace _stprintf)
template<typename... Types>
void SafePrintf(TCHAR* arr, int nSize, Types... args)
{
	CString str;
	str.Format(args...);
	_tcscpy_s(arr, nSize, (LPCTSTR)str);
}

template <size_t _Size, typename... Types>
void __CRTDECL SafePrintf(TCHAR(&_Destination)[_Size], Types... args) _CRT_SECURE_CPP_NOTHROW
{
	return SafePrintf(_Destination, _Size, args);
}

template<typename... Types>
static CString FormatString(Types... args)
{
	CString str;
	str.Format(args...);
	return str;
}

template<typename... Types>
static void VOutputDebugString(Types... args)
{
	CString str;
	str.Format(args...);
#ifdef _UNICODE
	OutputDebugStringW(str);
#else
	OutputDebugStringA(str);
#endif
}
#ifdef OutputDebugString
#undef OutputDebugString
#endif
#define OutputDebugString	VOutputDebugString

// FormatString 사용을 위해 특수문자 %를 이스케이프 처리하는 함수. FormatString 인자에 적용할것.
static CString EscapeFormatString(CString input)
{
	CString escapedString;
	escapedString.Preallocate(input.GetLength() * 2); // 예상 최대 길이로 메모리를 미리 할당

	for (int i = 0; i < input.GetLength(); ++i)
	{
		TCHAR ch = input[i];
		switch (ch)
		{
		case '%':
			escapedString += _T("%%"); // 퍼센트를 두 개로 이스케이프
			break;
		default:
			escapedString += ch; // 다른 문자들은 그대로 추가
			break;
		}
	}

	return escapedString;
}


// 변환 문자열의 life-cycle을 관리하기 위해 추가하는 클래스. 특수 목적으로 특정 상황에서만 사용하고 있다.
// 다른 용도로 사용 금지.
class _charsBuf
{
	char* pBuf;
public:
	_charsBuf(char* pChar)
		:pBuf(pChar)
	{
		if (pBuf == nullptr)
		{
			pBuf = new char[1];
			pBuf[0] = 0;
		}
	};
	~_charsBuf()
	{
		if (pBuf)
			delete[] pBuf;
		pBuf = nullptr;
	}
	operator const char* (void) const { return pBuf; };
};

// UTF-16 to UTF-8
static char* _w2u(LPCWSTR lpsz)
{
	char* pBuf = nullptr;
	if (lpsz && wcslen(lpsz) > 0)
	{
		auto nLen = WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, NULL, 0, NULL, NULL);
		if (nLen > 0)
		{
			pBuf = new char[nLen + 1];
			ASSERT(pBuf);

			WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, pBuf, nLen, NULL, NULL);
			pBuf[nLen] = 0;
		}
	}

	return pBuf;
}

// MBCS chars to UTF-8
static char* _c2u(const char* psz)
{
	char* pBuf = nullptr;
	if (psz)
	{
		// did you #include <string>?
		std::string src(psz);
		if (src.size() > 0)
		{
			// OS locale에 맞춰서 UNICODE UTF16으로 먼저 변환한다.
			int nWide = MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, nullptr, 0);
			if (nWide > 0)
			{
				auto lpsz = new wchar_t[nWide + 1];
				::ZeroMemory(lpsz, sizeof(wchar_t) * (nWide + 1));

				MultiByteToWideChar(CP_ACP, 0, src.c_str(), -1, lpsz, nWide);

				// UTF-16을 UTF-8로 다시 변환한다.
				auto nLen = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)lpsz, -1, NULL, 0, NULL, NULL);
				if (nLen > 0)
				{
					pBuf = new char[nLen + 1];
					ASSERT(pBuf);

					WideCharToMultiByte(CP_UTF8, 0, lpsz, -1, pBuf, nLen, NULL, NULL);
					pBuf[nLen] = 0;
				}

				delete[] lpsz;
			}
		}
	}

	return pBuf;
}

// TCHAR를 받기 위한 overloading 함수들.
static char* _t2u(LPCSTR psz)
{
	return _c2u(psz);
}
static char* _t2u(LPCWSTR lpsz)
{
	return _w2u(lpsz);
}
#define _T2U(x)		_charsBuf(_t2u(x))

#endif
