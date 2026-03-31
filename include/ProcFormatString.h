// Added by Makeit, on 2024/05/23.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once



// TODO: Makeit - 구현중. FormatString이 %문자를 처리하지 못하는 문제를 해결하기 위해 만든 클래스인데 잘 안되고 있음. --;
// 일단 파일로 저장하고, 나중에 시간이 되면 그때 다시 살펴보자.
class CProcFormatString
{
public:
	// 특수문자를 이스케이프 처리하는 함수
	static CString EscapeString(CString input)
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

	// 다양한 타입을 CString으로 변환하는 템플릿 함수
	static CString ToCString(LPCTSTR value)
	{
		CString str;
		str.Format(_T("%s"), value);
		return str;
	}

	// 특수화된 템플릿 멤버 함수: int 타입에 대한 특수화
	static CString ToCString(int value)
	{
		CString str;
		str.Format(_T("%d"), value);
		return str;
	}

	// 특수화된 템플릿 멤버 함수: double 타입에 대한 특수화
	static CString ToCString(double value)
	{
		CString str;
		str.Format(_T("%f"), value);
		return str;
	}

	// 특수화된 템플릿 멤버 함수: float 타입에 대한 특수화
	static CString ToCString(float value)
	{
		CString str;
		str.Format(_T("%f"), value);
		return str;
	}

	// CStringArray에 인수를 추가하는 함수
	template<typename T>
	static void AppendArgs(CStringArray& stringArgs, T arg)
	{
		CString strArg = ToCString(arg);
		CString escapedArg = EscapeString(strArg);
		stringArgs.Add(escapedArg);
	}

	// 가변 인수를 처리하여 CStringArray에 추가하는 함수
	template<typename T, typename... Types>
	static void AppendArgs(CStringArray& stringArgs, T arg, Types... args)
	{
		AppendArgs(stringArgs, arg);
		AppendArgs(stringArgs, args...);
	}

	// 안전한 문자열 포맷팅을 수행하는 함수
	static CString FormatSafeString(const CString& format, CStringArray& args)
	{
		CString str = format;
		for (int i = 0; i < args.GetCount(); ++i)
		{
			CString placeholder;
			placeholder.Format(_T("%%%d"), i + 1);
			str.Replace(placeholder, args[i]);
		}
		return str;
	}

	// 문자열 포맷팅 함수
	template<typename... Types>
	static CString FormatString(CString format, Types... args)
	{
		CStringArray stringArgs;
		AppendArgs(stringArgs, args...);

		CString safeFormat = format;

		CString str = FormatSafeString(safeFormat, stringArgs);
		return str;
	}
};
