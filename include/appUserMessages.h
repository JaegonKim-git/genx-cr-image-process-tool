// Added by Makeit, on 2023/07/21.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once




// GenX-CR handler class 에서 영상 획득이 완료되었음을 알리는 이벤트. 받는 곳에서 획득(로딩) 처리 해야 한다.
// * 주의! 이벤트만 발생시킨다. 저장 파일명은 미리 정의되고 서로 약속되어 있어야 함.
// WPARAM : (사용 안함)
// LPARAM : (사용 안함).
#define WM_USER_CMD_LOADSCANIMAGE			( WM_USER + 3825 )

// GenX-CR handler class 에서 장비 상태가 변경되었음을 알리는 이벤트. 받는 곳에서 상태표시를 변경 해야 한다.
// WPARAM : new status value
// LPARAM : (사용 안함).
#define WM_USER_NOTIFY_DEVICESTATE			( WM_USER + 3826 )

// GenX-CR handler class 에서 progress 값이 변경되었음을 알리는 이벤트. 받는 곳에서 상태표시를 변경 해야 한다.
// WPARAM : 0~100 position (progress) value
// LPARAM : (사용 안함).
#define WM_USER_NOTIFY_PROGRESS_CHANGED		( WM_USER + 3827 )

// Thread-progress interface 전용 메시지. 다른 용도로 사용을 금지함.
#define WM_USER_PROGRESS_COMMAND			( WM_USER + 3828 )
// only for Thread-progress interface
enum E_PROGRESS_WPARAM
{
	EPW_STEPUP = 1,
	EPW_SETTITLE,
	EPW_SETDESC,
	EPW_RESET,
	EPW_ENABLEBTN,
	EPW_DONE
};

// CPopupMenuWnd 에서 focus를 잃어버렸을 경우 혹은 기타 주어진 조건(설정)으로 창을 닫아야 하는 경우 parent로 전송되는 메시지.
#define WM_USER_HIDE_POPUPMENUWINDOW		( WM_USER + 3829 )

// GenX-CR 영상 수신 표시를 위한 thread progress dialog를 띄우는 명령.
#define WM_USER_NOTIFY_LAUNCHTRANSFER		( WM_USER + 3930 )

// GenX-CR 영상 수신 GUI 취소(창닫기)
#define WM_USER_NOTIFY_STOPTRANSFER			( WM_USER + 3931 )

// GenX-CR 상태 알림 이벤트(에러 표시) 명령
#define WM_USER_NOTIFY_DEVICEEVENT			( WM_USER + 3932 )

// Handy 상태 변화 이벤트(에러 표시) 명령	// 2025-02-11. jg kim. PortView 코드 적용
#define WM_HANDY_NOTIFY_DEVICEEVENT			( WM_USER + 3933 )
