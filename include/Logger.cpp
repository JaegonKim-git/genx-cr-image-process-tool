// Added by 
// 2026-01-30. 표준 로깅 시스템 구현
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <fstream>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <cstring>
#include "Logger.h"

namespace {
	std::mutex g_logMutex;
}

// DLL 내부용 로깅 함수 - 파일명을 인자로 받음
void writelog_impl(char* contents, const char* sourceFile, int line, const char* function, const char* logFileName)
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
