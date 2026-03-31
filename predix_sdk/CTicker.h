// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "tray_network.h"


class AFX_EXT_CLASS CTicker
{
public:
	CTicker();
	~CTicker();


public:
	CWinThread* m_pThread_main;

	volatile bool m_bEnd;
	volatile bool m_bRunning;


public:
	static UINT UDP_ThreadFunction_main(LPVOID pVoid);

	void startTicker(void);
	void endTicker(void);

	void pauseTicker(void);
	void resumeTicker(bool is_first = false);
};

