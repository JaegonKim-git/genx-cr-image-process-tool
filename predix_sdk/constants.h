#pragma once
//<Genoray calibration 관련 정의. 시작>//2023.11.22 jg kim.
// macro constants
// <X-ray calibration 관련>
#define MAX_CALI_IMAGE_COUNT	10
#define MAX_CALI_DATA_COUNT		50
#define DEGREE_COUNT			4
#define DEFAULT_VALID_WIDTH		90 // 2024-02-28. jg kim. calibration에 사용하는 ROI 조정.
#define DEFAULT_VALID_HEIGHT	30
#define CALI_IMAGE_COUNT	5
// </X-ray calibration 관련>

// <Hardware 관련>
#define MPPC_COUNT 3
#define MAIN_CLOCK_FREQUENCY 100 // GenX-CR mainboard main clock frequency. MHz unit
#define SCAN_SPEED_DEFAULT	5113 // step motor pulse period
#define	BACKWARD_OFFSET_DEFAULT 4.0 // mm
#define	POSITIONING_OFFSET_DEFAULT 4.0 // mm  // 2024-06-18. jg kim. Positioning 지그 사용을 위해 추가
#define	EJECT_TIME_DEFAULT 1.3 // sec
#define	EJECT_SPEED_DEFAULT 8000	// 2025-07-03. jg kim. 기본값 추가
#define ENTRY_OFFSET_DEFAULT2	100 // pixel	// 2025-07-03. jg kim. 기본값 변경
#define ENTRY_OFFSET_DEFAULT3	100 // pixel
#define BLDC_MAX_RPM 2022 // spec상 최대 RPM
#define BLDC_RPM_DEFAULT 1800
#define BLDC_SHIFT_INDEX_DEFAULT 8000 // spec상 최대 RPM
#define TRAY_MAX_SPEED 12.7831732323803 // GenX-CR 스탭모터 구동속도 한계로 인한 트레이 최대 이동 속도
#define TRAY_EJECT_TIME 2.5 // sec
#define MPPC_VOLTAGE_LOWER_BOUND 50000 // 전압에 1000을 곱합 값
#define MPPC_VOLTAGE_UPPER_BOUND 59000 // 전압에 1000을 곱합 값
#define MPPC_DIGITAL_VALUE_LOWER_BOUND 31000 // 실제 장비에 사용하는 값의 하한
#define MPPC_DIGITAL_VALUE_DEFAULT 35500 // 실제 장비에 사용하는 값의 하한
#define MPPC_DIGITAL_VALUE_UPPER_BOUND 40000 // 실제 장비에 사용하는 값의 상한
#define MPPC_CONSTANT_VOLTAGE_CONVERSION 90000 // 전압에 1000을 곱합 값
#define LASER_ON_POSITION	150  // 2024-08-06. jg kim. 상수의 의미를 명확히 하기 위하여 매크로로 변경
#define LASER_ON_WIDTH		1280
#define SCAN_WIDTH			1240
#define SCAN_START_POSITION 60
// </Hardware 관련>

// <Raw image 및 IP 관련>
#define	RAW_WIDTH			1536
#define	RAW_HEIGHT			2360
#define TAG_POSITION		6
#define CORNERS				4
#define IP_INDICES			4
#define IP_LENGTH_ERROR		0.05 // 5%
#define IP_CORNER_THRESHOLD 0.9  // 이상적인 IP 각 귀퉁이와의 유사도 기준
#define IP_CORNER_THRESHOLD_LOOSE 0.8  // 이상적인 IP 각 귀퉁이와의 유사도 기준(완화된 기준)
#define IP_CORNER_RADIUS	283 // Size 2 귀퉁이 반지름
#define PixelSizeError		0.002 // 0.2% 에러를 기준
#define PixelSize			25 // um 단위. HR 모드 기준.
// </Raw image 및 IP 관련>

#define pow2(x) (x)*(x)
#define pow3(x) (x)*(x)*(x)
#define constant_rad2deg (180 / CV_PI)
#define rad2deg(x) (x*180/CV_PI)
#define deg2rad(x) (x*CV_PI/180)

enum eCornerType
{
	LeftTop = 0,
	LeftCenter,
	LeftBottom,
	CenterTop,
	CenterCenter,
	CenterBottom,
	RightTop,
	RightCenter,
	RightBottom
};

