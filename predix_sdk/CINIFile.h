#pragma once


class AFX_EXT_CLASS CINIFile
{
private:
	CString FilePath;
	CString LastSection;

public:
	CINIFile(CString filepath);
	~CINIFile();

	void ClearSection(CString section);

	void SetValue(CString section, CString key, CString value);
	void SetValue(CString key, CString value);

	int GetInt(CString section, CString key);
	int GetInt(CString key);

	CString GetValue(CString section, CString key);
	CString GetValueL(CString section, CString key);
	CString GetValueL(CString section, CString key, CString keyDefault); // 2024-08-19. jg kim. key 값이 ini 파일에 없을 경우 default값을 반환하도록 GetValueL 함수 overloading
	CString GetValueH(CString section, CString key);
	CString GetValue(CString key);
	CString GetValueL(CString key);
	CString GetValueH(CString key);

	CString GetPath()
	{
		return FilePath;
	}
};
