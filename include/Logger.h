// 2026-01-30. Standard logging system implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <cstring>

namespace {
	// 각 DLL이 자체 mutex를 가지도록 익명 namespace 사용
	std::mutex g_logMutex;
}

// DLL 내부용 로깅 함수 - 인라인으로 정의하여 각 DLL이 독립적으로 컴파일
inline void writelog_impl(char* contents, const char* sourceFile, int /*line*/, const char* function, const char* logFileName)
{
	// 인자 검증
	if (!contents || !logFileName) {
		return;
	}

	// 스레드 안전성 보장
	std::lock_guard<std::mutex> lock(g_logMutex);

	try {
		std::ofstream ofs(logFileName, std::ios::app);
		if (ofs.is_open()) {
			// 타임스탬프 추가
			auto now = std::chrono::system_clock::now();
			auto time_t = std::chrono::system_clock::to_time_t(now);
			auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
				now.time_since_epoch()) % 1000;

			std::tm tm_buf;
			localtime_s(&tm_buf, &time_t);

			// 소스 파일명에서 경로 제거 (파일명만 출력)
			const char* fileNameOnly = sourceFile;
			const char* lastSlash = strrchr(sourceFile, '\\');
			if (lastSlash) fileNameOnly = lastSlash + 1;
			else {
				lastSlash = strrchr(sourceFile, '/');
				if (lastSlash) fileNameOnly = lastSlash + 1;
			}

			// 로그 형식: [타임스탬프] [함수명] 내용
			ofs << "["
				<< std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S")
				<< "." << std::setfill('0') << std::setw(3) << ms.count()
				<< "] [" << function << "] "
				<< contents;

			ofs.flush();
		}
	}
	catch (...) {
		// 로깅 실패 시 무시 (로깅이 프로그램 중단시키면 안됨)
	}
}


#define writelog(contents, logFileName) \
	writelog_impl(contents, __FILE__, __LINE__, __FUNCTION__, logFileName)