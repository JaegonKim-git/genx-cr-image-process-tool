#include "pch.h"
#include "../predix_sdk/CiniFile.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CINIFile::CINIFile(CString filepath)
{
	FilePath = filepath;
}

CINIFile::~CINIFile()
{
}

void CINIFile::ClearSection(CString section)
{
	LastSection = section;
	WritePrivateProfileString((LPTSTR)(LPCTSTR)section, NULL, NULL, (LPTSTR)(LPCTSTR)FilePath);
}

void CINIFile::SetValue(CString section, CString key, CString value)
{
	LastSection = section;
	WritePrivateProfileString((LPTSTR)(LPCTSTR)section, (LPTSTR)(LPCTSTR)key, (LPTSTR)(LPCTSTR)value, (LPTSTR)(LPCTSTR)FilePath);
}

void CINIFile::SetValue(CString key, CString value)
{
	SetValue(LastSection, key, value);
}

int CINIFile::GetInt(CString section, CString key)
{
	LastSection = section;
	return GetPrivateProfileInt((LPTSTR)(LPCTSTR)section, (LPTSTR)(LPCTSTR)key, -1, (LPTSTR)(LPCTSTR)FilePath);
}

int CINIFile::GetInt(CString key)
{
	return GetInt(LastSection, key);
}

CString CINIFile::GetValue(CString section, CString key)
{
	LastSection = section;
	TCHAR value[256];
	GetPrivateProfileString((LPTSTR)(LPCTSTR)section, (LPTSTR)(LPCTSTR)key, _T("NONE"), value, sizeof(value) / sizeof(TCHAR), (LPTSTR)(LPCTSTR)FilePath);
	return value;
}

CString CINIFile::GetValueH(CString section, CString key)
{
	CString value = GetValue(section, key);
	return value.MakeUpper();
}

CString CINIFile::GetValueL(CString section, CString key)
{
	CString value = GetValue(section, key);
	return value.MakeLower();
}

// 2024-08-19. jg kim. key 값이 ini 파일에 없을 경우 default값을 반환하도록 GetValueL 함수 overloading
CString CINIFile::GetValueL(CString section, CString key, CString keyDefault)
{
	CString value = GetValue(section, key);
	if (value == (L"NONE"))
		value = keyDefault;
	return value.MakeLower();
}

CString CINIFile::GetValueL(CString key)
{
	return GetValueL(LastSection, key);
}

CString CINIFile::GetValueH(CString key)
{
	return GetValueH(LastSection, key);
}

CString CINIFile::GetValue(CString key)
{
	return GetValue(LastSection, key);
}
