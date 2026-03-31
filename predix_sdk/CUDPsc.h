#pragma once

#include "tray_network.h"
#include "CUDPReceiveSocket.h"


class AFX_EXT_CLASS CUDPsc
{
public:
	CUDPsc();
	virtual ~CUDPsc();

public:
	

	//! UDP 서버 소켓
	CUDPReceiveSocket* m_pUDPs;

	//! kiwa72(2023.05.01 11h) - UDP NtoN
	stUDP m_Broadcast_Packet;	// 브로드캐스팅 패킷
	int m_NtoN_SelectDevice;	// 디바이스 선택(-1:미선택, >=0:장비선택)

	//! kiwa72(2023.08.30 11h) - 장비 지정(이름, IP주소) 브로드캐스팅 관련
	//-------------------------------------------
	// 브로드캐스팅 구분 : =255(Default) 모든 장비, =1 지정 장비
	//-------------------------------------------
	int m_Broadcasting_State;

	int Get_Broadcasting_State()
	{
		return m_Broadcasting_State;
	}

	void Set_Broadcasting_State(int state)
	{
		m_Broadcasting_State = state;
	}

	//--------------------------------------------------------
	// 장비 지정(이름, IP주소) 브로드캐스팅 카운트
	//--------------------------------------------------------
	int m_Direct_Broadcasting_Count;

	void Reset_Direct_Broadcasting_Count()
	{
		m_Direct_Broadcasting_Count = 0;
	}

	int Get_Direct_Broadcasting_Count()
	{
		return m_Direct_Broadcasting_Count;
	}
	//!...

public:
	BOOL Ready_to_UDPSockets(ON_RECEIVE_DATA_RAW fnOnReceiveData);
	void BroadcastData(unsigned char * data, unsigned long size, LPCTSTR sendip, int port);
	void SendData(unsigned char * data, unsigned long size, CString ip, unsigned long port);

	void BroadcastTrayUDP(bool isFirst = false, NETWORK_COMMAND enc = nc_tray);

	void Start_Tray_Leader(void);
	void End_Tray_Leader(void);

	void set_time2udp(stUDP& udp, bool isfirst = false);
	void set_checksum_udp(stUDP& udp);

	void Force_Disconnect_DeviceSocket(void);
};
