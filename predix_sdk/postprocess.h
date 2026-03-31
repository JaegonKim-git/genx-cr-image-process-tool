// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once



enum class PostProcessMode;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Unused classes removed: proc_param, enhance_param


#ifndef PP_TYPE_DEFINED
#define PP_TYPE_DEFINED
#define PP_KAR_TYPE3		4

#define PP_DEFAULT			PP_KAR_TYPE3

//>>POSTPROCESSF
// 20210317 enhance8을 위해 PPF_PP_TYPE_DEFAULT 를 3으로 바꿈 
// 20210603 사장님 지시로 PPF_PP_TYPE_DEFAULT를 2로 회귀  다시 3으로 원복
#define PPF_PP_TYPE_DEFAULT	      			3
#define PPF_BLUR_RADIUS_DEFAULT				2
#define PPF_EDGE_ENHANCE_DEFAULT			1
#define PPF_BRIGHTNESS_OFFSET_DEFAULT		7000
//20210319 7000 -> 0
//20210614  0 -> 7000으로 다시 번경
//<<POSTPROCESSF
#endif



// 2023.02.13 불필요한 param 제거
// 2024-12-09. jg kim. 공정툴용 영상처리와 통합. 불필요한 함수 제거
AFX_EXT_API void pre_postprocess_by_dll(int nMode = 0);
AFX_EXT_API void last_image_to_fin_by_dll(void);

// 2026-03-04. jg kim. 영상 파일 이름을 전역 변수로 관리
extern AFX_EXT_DATA CString g_strCurrentImageFileName;
