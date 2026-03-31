# GenX-CR Image Process Tool

PortView OP 솔루션에서 정리/개선한 코드를 기반으로 GenX-CR 공정 영상을 제작/관리하기 위한 프로젝트입니다.

## 개요

GenX-CR Image Process Tool은 X-Ray 이미지 처리 및 분석을 위한 Windows 기반 C++ 애플리케이션입니다. 이 도구는 의료 영상 처리, 칼리브레이션, 이미지 분석 기능을 제공합니다.

## 주요 기능

- **이미지 처리**: OpenCV 기반 고급 이미지 처리 알고리즘
- **캘리브레이션**: 하드웨어 캘리브레이션 및 파라미터 관리
- **스캐너 관리**: 다중 스캐너 장치 지원 및 관리
- **실시간 분석**: 실시간 이미지 분석 및 품질 검증
- **GUI 인터페이스**: 사용자 친화적인 Windows 기반 인터페이스
- **비동기 영상처리**: 영상처리 작업을 별도 스레드에서 실행하여 GUI 응답성 향상

## 최근 업데이트

### 2026-02-05: 비동기 영상처리 기능 개선
- **OnBnClickedButtonManualProcess()** 및 **OnBnClickedButtonBasicCorrection()** 함수를 비동기 방식으로 변경
- 영상처리 작업이 별도 워커 스레드에서 실행되어 GUI가 얼어지지 않음
- 콜백 기반 완료 처리로 안정적인 UI 업데이트
- 작업 진행 중 버튼 비활성화 및 진행 상태 표시
- 스레드 안전성을 위한 PostMessage 기반 UI 업데이트

## 프로젝트 구조

```
GenX-CR-Image-Process-Tool/
├── cruxcan_tray/          # 메인 애플리케이션 (System Tray)
├── gnCalibration/        # 캘리브레이션 DLL
├── gnHardwareInfo/       # 하드웨어 정보 DLL
├── gnPreProcessing/      # 전처리 DLL
├── gnPostProcessing/     # 후처리 DLL
├── predix_sdk/           # Predix SDK 라이브러리
├── ParameterHandler/     # 파라미터 관리 모듈
├── cruxcan_sdk/          # CRUXCAN SDK
├── imagecontrol/         # 이미지 컨트롤 UI 컴포넌트
├── IpDllTest/           # DLL 테스트 프로젝트
├── IpImageAnalysis/     # 이미지 분석 모듈
├── ExtLibs/             # 외부 라이브러리 (OpenCV, TIFF 등)
└── include/             # 공통 헤더 파일
```

## 시스템 요구사항

- **운영체제**: Windows 10 이상
- **개발환경**: Visual Studio 2017 이상
- **플랫폼**: x86, x64 지원
- **의존성**: 
  - OpenCV
  - TIFF 라이브러리
  - GDI+ 라이브러리

## 빌드 방법

1. Visual Studio에서 `GenX_CR_Image_Process_Tool.sln` 파일을 엽니다.
2. 솔루션 구성을 선택합니다 (Debug/Release, x86/x64).
3. 전체 솔루션을 빌드합니다.

### 빌드 구성

- **Debug|x86**: 디버그 빌드 (32비트)
- **Release|x86**: 릴리스 빌드 (32비트)  
- **Debug|x64**: 디버그 빌드 (64비트)
- **Release|x64**: 릴리스 빌드 (64비트)

## 주요 모듈

### 1. cruxcan_tray (predix_sw)
- 메인 애플리케이션
- 시스템 트레이 인터페이스
- 스캐너 관리 및 설정

### 2. gnCalibration
- 캘리브레이션 기능 DLL
- 이미지 칼리브레이션 알고리즘

### 3. gnHardwareInfo
- 하드웨어 정보 관리 DLL
- 장치 정보 조회 및 관리

### 4. gnPreProcessing
- 이미지 전처리 DLL
- OpenCV 기반 전처리 알고리즘

### 5. gnPostProcessing
- 이미지 후처리 DLL
- OpenCV 기반 후처리 알고리즘

### 6. predix_sdk
- Predix 플랫폼 SDK
- 하드웨어 통신 인터페이스

### 7. ParameterHandler
- 시스템 파라미터 관리
- 설정값 저장 및 로드

## 사용법

1. 애플리케이션을 실행합니다.
2. 시스템 트레이에서 아이콘을 확인합니다.
3. 스캐너 장치를 연결하고 설정합니다.
4. 이미지 처리 작업을 시작합니다.

## API 참조


```

## 개발 환경 설정

### 필수 라이브러리
프로젝트는 다음 외부 라이브러리들을 사용합니다:

- **OpenCV**: 이미지 처리 및 컴퓨터 비전
- **TIFF Library**: TIFF 이미지 포맷 지원
- **GDI+**: Windows 그래픽 인터페이스

### 프로젝트 설정
1. `ExtLib.props` 파일에서 라이브러리 경로를 확인합니다.
2. 필요한 경우 외부 라이브러리 경로를 수정합니다.
3. 프로젝트 속성에서 올바른 플랫폼 타겟을 설정합니다.

## 테스트

### DLL 테스트
`IpDllTest` 프로젝트를 사용하여 이미지 처리 DLL을 테스트할 수 있습니다:

```bash
# Debug 모드로 테스트 실행
IpDllTest.exe [테스트 이미지 경로]
```

### 단위 테스트
각 모듈별로 독립적인 테스트가 가능합니다.

## 문제 해결

### 일반적인 문제들

1. **빌드 오류**: 
   - Visual Studio 버전 호환성 확인
   - 외부 라이브러리 경로 설정 확인

2. **런타임 오류**:
   - DLL 의존성 확인
   - 하드웨어 연결 상태 확인

3. **이미지 처리 오류**:
   - 입력 이미지 포맷 확인
   - 메모리 사용량 모니터링

## 라이선스

이 프로젝트는 내부 개발용으로 제작되었습니다.

## 기여

내부 개발팀만 접근 가능합니다.

## 지원

기술 지원이나 문의사항이 있으시면 개발팀에 연락해주세요.

## 버전 히스토리

- **v1.0**: 초기 릴리스
- 지속적인 업데이트 및 개선 진행 중

---

*마지막 업데이트: 2025년 10월 28일*
