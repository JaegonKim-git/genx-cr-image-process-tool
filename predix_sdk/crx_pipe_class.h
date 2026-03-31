// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

//if require common_type.h  remove  #include crx_pipe_class in common_type.h

#ifndef __ONREIEVEFROMPIPE__
#define __ONREIEVEFROMPIPE__
typedef BOOL(*ONREIEVEFROMPIPE)(unsigned char* data, DWORD size);
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// class crx_pipe_class
class AFX_EXT_CLASS crx_pipe_class
{
	// public constructor and destructor
public:
	// constructor
	crx_pipe_class();

	// destructor
	~crx_pipe_class();


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public members
public:
	ONREIEVEFROMPIPE fOnReceiveFromPipe;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// public methods
public:
	static UINT read_pipe_ThreadFunction(LPVOID pVoid);
	static UINT receive_pipe_queue_ThreadFunction(LPVOID pVoid);
	static UINT send_pipe_queue_ThreadFunction(LPVOID pVoid);

	BOOL start_read_pipe(void);
	BOOL end_read_pipe(void);
	BOOL wait_write_pipe(int waittime = 3000);
	BOOL push_write_pipe(unsigned char* data, DWORD size);


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// protected methods
protected:
	BOOL write_pipe(unsigned char* data, DWORD size, bool forced_send = false);
	BOOL end_write_pipe(void);
	void clear_max_queue_record(void);

	char* check_queue_size(void);	// for development


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// protected members
protected:
	volatile long isEnd;				// TODO: Makeit - 이 변수의 값, 용도, 사용 방법 등이 명확하지 않음. 리팩토링 필요.
	volatile bool is_recv_queue_using;
	volatile bool is_send_queue_using;
	BOOL m_isConnected;
	OVERLAPPED ol;
	unsigned long max_send_queue_size;
	unsigned long max_recv_queue_size;
	long alived_thread_count;


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	// private members
private:
	HANDLE hWritePipe;
	HANDLE hReadPipe;
	CWinThread* hThread_pipe;
	CWinThread* hThread_read_queue;
	CWinThread* hThread_write_queue;
};

