// Added by CRUXELL, on 202x/0x/0x.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include "crx_pipe_class.h"
#include <windows.h>
#include <iostream>
#include <queue>



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



// macro constants
#define PIPE_BUF_SIZE	(512)
#define PIPE_TIMEOUT	(50)

#ifndef BLOCK_SIZE
#define BLOCK_SIZE		(512)
#endif


typedef struct  _pipe_queue_data_
{
	unsigned char bytes[PIPE_BUF_SIZE];

	_pipe_queue_data_& operator = (const _pipe_queue_data_& r)
	{
		memmove(&(this->bytes[0]), &(r.bytes[0]), 512);
		return *this;
	}
} pipe_queue_data;


std::queue<pipe_queue_data>recv_pipe_queue;
std::queue<pipe_queue_data>send_pipe_queue;


#if defined(EMUL_PIPE_TRAY) &&  EMUL_PIPE_TRAY
#define CRUXCAN_TRAY
#endif


#ifndef __MYPLOGM__
#define __MYPLOGM__
#include "common_type.h"
//#define MYPLOGMA( ...)	//{ char debgmsg[MAX_PATH]; sprintf(debgmsg,"[CRX/TRY]"__VA_ARGS__);OutputDebugStringA(debgmsg); }
//#define MYPLOGMW( ...)	//{ wchar_t debgmsg[MAX_PATH]; wsprintf(debgmsg,L"[CRX/TRY]"__VA_ARGS__);OutputDebugString(debgmsg); }
#endif


#define PIPE_NAME_SDK_2_TRAY	(_T("\\\\.\\pipe\\crx_sdk_2_tray_pipe"))
#define PIPE_NAME_TRAY_2_SDK	(_T("\\\\.\\pipe\\crx_tray_2_sdk_pipe"))


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructor
crx_pipe_class::crx_pipe_class()
	: ol({ 0,0,0,0,NULL })
	, alived_thread_count(0)
	, hThread_pipe(nullptr)
	, hThread_read_queue(nullptr)
	, hThread_write_queue(nullptr)
	, hWritePipe(nullptr)
	, hReadPipe(nullptr)
	, isEnd(-1)
	, is_recv_queue_using(false)
	, is_send_queue_using(false)
	, m_isConnected(false)
	, fOnReceiveFromPipe(nullptr)
	, max_send_queue_size(0)
	, max_recv_queue_size(0)
{
	AfxSocketInit(NULL);	// 소켓생성때문에;;;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// destructor
crx_pipe_class::~crx_pipe_class()
{
	if (isEnd < 1)
	{
		end_read_pipe();
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL crx_pipe_class::wait_write_pipe(int waittime)
{
	return WaitNamedPipe(PIPE_NAME_TRAY_2_SDK, waittime);	// default 3000 = 3sec
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL crx_pipe_class::write_pipe(unsigned char* data, DWORD size, bool forced_send)
{
	DWORD sent_size = 0;
	static bool is_sending = 0;

	while (is_sending)
		Sleep(10);

	if (!forced_send && isEnd > 0)
		return 1;

	is_sending = true;

	hWritePipe = CreateFile(
		PIPE_NAME_TRAY_2_SDK,
		GENERIC_WRITE,
		0,	// no sharing
		NULL,//default securtity
		OPEN_EXISTING,
		0,	// deffault attributes
		NULL);

	BOOL result = 0;
	DWORD lasterror = GetLastError();

	if (hWritePipe != INVALID_HANDLE_VALUE && hWritePipe != NULL)
	{
		//MYPLOGMA("could not open pipe1 [%d]\n", lasterror); is_sending = false;
		//result = -1;
		result = 0;
	}

	if (lasterror != ERROR_SUCCESS)
	{
		if (lasterror != ERROR_PIPE_BUSY)
		{
			MYPLOGMA("[pipe]could not open pipe other [%d]\n", lasterror);
			result = -2;
		}
		else
		{
			result = -3;
		}
	}

	if (result == 0)
	{
		result = size > 0 ? WriteFile(hWritePipe, data, size, &sent_size, NULL) : TRUE;//블러킹이므로 최소한 다 보내고 빠져나오니 덜 보낸건 신경쓰지 말자
	}

	if (hWritePipe)
		CloseHandle(hWritePipe);

	hWritePipe = NULL;
	is_sending = false;

	return  result;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL crx_pipe_class::push_write_pipe(unsigned char* data, DWORD size)
{
	// thread로 여러군데서 보내다 보면 busy될수있지 않을까?
	//

	while (is_send_queue_using)
		Sleep(10);

	if (isEnd > 0)
		return TRUE;

	is_send_queue_using = true;
	pipe_queue_data send_data;
	memcpy(send_data.bytes, data, size);
	send_pipe_queue.push(send_data);
	is_send_queue_using = false;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
UINT crx_pipe_class::send_pipe_queue_ThreadFunction(LPVOID pVoid)
{
	MYPLOGDG("[pipe]send_pipe_queue_ThreadFunction\n");

	crx_pipe_class* pThis = (crx_pipe_class*)pVoid;

	pThis->alived_thread_count += 100;
	long lresult = 0;

	while (pThis->isEnd < 1)
	{
		if (!send_pipe_queue.empty())
		{
			while (pThis->is_send_queue_using && pThis->isEnd < 1)
				Sleep(10);

			if (pThis->isEnd > 0)
				break;

			pThis->is_send_queue_using = true;
			pipe_queue_data data = send_pipe_queue.front();

			if (pThis->isEnd < 1)
			{
				lresult = pThis->write_pipe(data.bytes, sizeof(data));

				if (lresult != -3)
				{
					send_pipe_queue.pop();
				}

				if (pThis->max_send_queue_size < (ULONG)send_pipe_queue.size())
				{
					pThis->max_send_queue_size = (ULONG)send_pipe_queue.size();
				}

				if (send_pipe_queue.size() > 10)
				{
					auto nSize = send_pipe_queue.size();
					MYPLOGMA("send_pipe_queue size  %d / %d\n", nSize, pThis->max_send_queue_size);
				}
			}

			pThis->is_send_queue_using = false;
		}

		Sleep(10);
	}

	pThis->alived_thread_count -= 100;

	MYPLOGDG("[pipe]end send_pipe_queue_ThreadFunction - 100\n");

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
UINT crx_pipe_class::receive_pipe_queue_ThreadFunction(LPVOID pVoid)
{
	AfxSocketInit(NULL);

	MYPLOGDG("[pipe]receive_pipe_queue_ThreadFunction + 2000\n");

	crx_pipe_class* pThis = (crx_pipe_class*)pVoid;
	pThis->alived_thread_count += 2000;

	while (pThis->isEnd < 1)
	{
		if (!recv_pipe_queue.empty())
		{
			while (pThis->is_send_queue_using && pThis->isEnd < 1)
				Sleep(10);

			if (pThis->isEnd > 0)
				break;

			pThis->is_send_queue_using = true;
			pipe_queue_data data = recv_pipe_queue.front();
			recv_pipe_queue.pop();

			if (pThis->max_recv_queue_size < (ULONG)recv_pipe_queue.size())
			{
				pThis->max_recv_queue_size = (ULONG)recv_pipe_queue.size();
			}

			if (recv_pipe_queue.size() > 1)
			{
				auto nSize = recv_pipe_queue.size();
				MYPLOGMA("pipe_queue size is %d / %d\n", nSize, pThis->max_recv_queue_size);
			}

			pThis->is_send_queue_using = false;

			if (pThis->isEnd < 1 && pThis->fOnReceiveFromPipe)
			{
				pThis->fOnReceiveFromPipe(data.bytes, sizeof(data));
			}
		}

		Sleep(10);
	}

	MYPLOGDG("[pipe]end receive_pipe_queue_ThreadFunction - 2000\n");

	pThis->alived_thread_count -= 2000;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
UINT crx_pipe_class::read_pipe_ThreadFunction(LPVOID pVoid)
{
	MYPLOGDG("[pipe]read_pipe_ThreadFunction +30000\n");

	crx_pipe_class* pThis = (crx_pipe_class*)pVoid;

	pThis->alived_thread_count += 30000;

	unsigned char pipe_read_buffer[PIPE_BUF_SIZE];
	DWORD dwRead;

	DWORD dwRemained = BLOCK_SIZE;
	DWORD copy_size = 0;
	pipe_queue_data  pipe_data;
	unsigned char* dst = pipe_data.bytes;
	unsigned char* src = &pipe_read_buffer[0];

	DWORD lasterror = 0;
	DWORD dwErrorCode = 0;

	HANDLE hreadyMutex = CreateMutex(NULL, TRUE, _T("CRUXCAN_CRX-1000_TRAY_PIPE_READY_MUTEX"));
	dwErrorCode = GetLastError();
	if (dwErrorCode == 0x0005 || dwErrorCode == 0x00B7)
	{
		MYPLOGDG("[CRXE0201] CRUXCAN_CRX-1000_TRAY_PIPE_READY_MUTEX is already exist");

	}
	else
	{
		MYPLOGMA("CRUXCAN_CRX-1000_TRAY_PIPE_READY_MUTEX is ready\n");
	}

	while (pThis->isEnd < 1)
	{
		pThis->m_isConnected = ConnectNamedPipe(pThis->hReadPipe, NULL) ? TRUE : (lasterror = GetLastError()) == ERROR_PIPE_CONNECTED;//connect pipe to listen
		if (pThis->m_isConnected)
		{
			if (pThis->isEnd > 0)
				break;

			if (ReadFile(pThis->hReadPipe, pipe_read_buffer, sizeof(pipe_read_buffer), &dwRead, NULL) != FALSE)
			{
				src = &pipe_read_buffer[0];

				while ((pThis->isEnd < 1) && dwRead > 0)
				{
					copy_size = (dwRead > dwRemained) ? dwRemained : dwRead;
					memcpy(dst, src, copy_size);
					dst += copy_size;
					src += copy_size;
					dwRead -= copy_size;
					dwRemained -= copy_size;
					if (dwRemained < 1)
					{
						while (pThis->is_recv_queue_using)
							Sleep(10);

						if (pThis->isEnd > 0)
							break;

						pThis->is_recv_queue_using = true;//queue에 넣고 빼는걸로 락 걸리지는 말도록
						recv_pipe_queue.push(pipe_data);
						pThis->is_recv_queue_using = false;
						dst = pipe_data.bytes;
						dwRemained = BLOCK_SIZE;

						Sleep(10);
					}

					Sleep(1);
				}
			}
			else
			{
				lasterror = GetLastError();
				if (lasterror == ERROR_BROKEN_PIPE)
				{
					MYPLOGMA("ReadFile is broken[%d]\n", lasterror);
				}
				else
				{
					MYPLOGMA("ReadFile is errored[%d]\n", lasterror);
				}
			}

			DisconnectNamedPipe(pThis->hReadPipe);
		}
		else
		{
			switch (lasterror)
			{
			case ERROR_IO_PENDING://997
				CancelIo(pThis->hReadPipe);
				break;
			case ERROR_NO_DATA://232 ??? 
				break;
			case ERROR_PIPE_BUSY://231 ???
				MYPLOGMA("ERROR_PIPE_BUSY\n");
				break;
			case ERROR_PIPE_NOT_CONNECTED://233???
				break;
			default:
				MYPLOGMA("unknown error from connectnamedpipe[%d] \n", lasterror);
				break;
			}
		}

		Sleep(10);
	}

	if (hreadyMutex)
		ReleaseMutex(hreadyMutex);

	pThis->m_isConnected = FALSE;
	pThis->isEnd = 2;

	MYPLOGDG("[pipe]end read_pipe_ThreadFunction -30000(isEnd = 2)\n");

	pThis->alived_thread_count -= 30000;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL crx_pipe_class::start_read_pipe(void)
{
	ASSERT(hThread_pipe == nullptr);
	ASSERT(hThread_read_queue == nullptr);
	ASSERT(hThread_write_queue == nullptr);

	isEnd = 0;// -1  to 0

	max_send_queue_size = 0;
	max_recv_queue_size = 0;
	alived_thread_count = 0;

	MYPLOGMA("[pipe] start blocking pipe mode\n");

	hReadPipe = CreateNamedPipe(
		PIPE_NAME_SDK_2_TRAY,
		PIPE_ACCESS_INBOUND,
		PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
		1,
		PIPE_BUF_SIZE,	// output
		PIPE_BUF_SIZE,	// input
		PIPE_TIMEOUT,
		//0,	// 0 == 50msec
		NULL	// not security option
	);

	if (hReadPipe == INVALID_HANDLE_VALUE || hReadPipe == NULL)
	{
		MYPLOGDG("[CRXE0202]failed to create read_namedpipe[%d]\n", GetLastError());
		return -1;
	}

	hThread_read_queue = AfxBeginThread(crx_pipe_class::receive_pipe_queue_ThreadFunction, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (hThread_read_queue == NULL)
	{
		MYPLOGDG("[CRXE0203]failed to create pop_queue thread[%d]\n", GetLastError());
		return -2;
	}

	hThread_read_queue->m_bAutoDelete = FALSE;
	hThread_read_queue->ResumeThread();

	hThread_write_queue = AfxBeginThread(crx_pipe_class::send_pipe_queue_ThreadFunction, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (hThread_write_queue == NULL)
	{
		MYPLOGDG("[CRXE0204]failed to create push_queue thread[%d]\n", GetLastError());
		return -2;
	}

	hThread_write_queue->m_bAutoDelete = FALSE;
	hThread_write_queue->ResumeThread();

	hThread_pipe = AfxBeginThread(crx_pipe_class::read_pipe_ThreadFunction, (LPVOID)this, THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);
	if (hThread_pipe == NULL)
	{
		MYPLOGDG("[CRXE0205]failed to create read_pipe thread[%d]\n", GetLastError());
		return -3;
	}

	hThread_pipe->m_bAutoDelete = FALSE;
	hThread_pipe->ResumeThread();

	return 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
void crx_pipe_class::clear_max_queue_record(void)
{
	max_send_queue_size = 0;
	max_recv_queue_size = 0;
	MYPLOGMA("clear max queue record\n");
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
char* crx_pipe_class::check_queue_size(void)
{
	static char szqueuesize[128];
	sprintf_s(szqueuesize, 128, "send queue max %d ,  recv queue max %d ", max_send_queue_size, max_recv_queue_size);
	return szqueuesize;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL crx_pipe_class::end_read_pipe(void)
{
	fOnReceiveFromPipe = NULL;

	if (isEnd == -1)
		return TRUE;

	if (isEnd > -1)
	{
		// thread is working
		//
		MYPLOGDN("record %s\n", check_queue_size());

		isEnd = 1;

		try
		{
			if (hThread_read_queue)
			{
				ASSERT((UINT_PTR)hThread_read_queue != 0xDDDDDDDD);
				WaitForSingleObject(hThread_read_queue->m_hThread, 3000);
			}

			//CloseHandle(hThread_read_queue);
			delete hThread_read_queue;
			hThread_read_queue = NULL;
		}
		catch (...)
		{
		}

		hThread_read_queue = NULL;

		try
		{
			if (hThread_write_queue)
			{
				ASSERT((UINT_PTR)hThread_write_queue != 0xDDDDDDDD);
				WaitForSingleObject(hThread_write_queue->m_hThread, 3000);
			}

			delete hThread_write_queue;
			hThread_write_queue = NULL;

			//CloseHandle(hThread_write_queue);
		}
		catch (...)
		{
		}

		hThread_write_queue = NULL;

		MYPLOGDN("CancelIO \n");

		if (hReadPipe)
		{
			CancelIo(hReadPipe);
		}

		DeleteFile(PIPE_NAME_SDK_2_TRAY);

		// 프로세스가 2개 이상 떠있을 경우 문제가 발생한다. 우선순위가 낮은 프로세스에서 무한 wait 발생.
		// 최대 2초까지만 대기하도록 변경함.
		int nTimeout = 0;
		while (isEnd == 1 && nTimeout <= 20)
		{
			Sleep(100);
			nTimeout++;
		}

		MYPLOGDN("isEnd is 2");

		if (hThread_pipe)
		{
			ASSERT((UINT_PTR)hThread_pipe != 0xDDDDDDDD);

			WaitForSingleObject(hThread_pipe->m_hThread, 3000);
			try
			{
				//CloseHandle(hThread_pipe);
				delete hThread_pipe;
			}
			catch (...)
			{
			}

			hThread_pipe = NULL;
		}
	}

	if (hReadPipe)
	{
		//WaitForSingleObject(hReadPipe, 3000);

		try
		{
			CloseHandle(hReadPipe);
		}
		catch (...)
		{
		}
	}

	hReadPipe = NULL;
	MYPLOGDN("[pipe] wait until alived_thread_count is 0  [%d][%d]\n", alived_thread_count, isEnd);

	// Memo: Makeit - 프로그램이 종료되지 않고 멈추는 현상이 있어서 임의로 일정 시간 후에 그냥 리턴하도록 변경합니다.
	volatile int nTimes = 0;
	while (alived_thread_count > 0)
	{
		// 2.5초간 기다려도 응답이 없으면 그냥 종료.
		if ( nTimes >= 25 )
		{
			break;
		}

		Sleep(100);
		nTimes++;
	}

	MYPLOGDN("[pipe] done end_read_pipe of all \n");
	isEnd = -1;

	return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
BOOL crx_pipe_class::end_write_pipe(void)
{
	// no action
	MYPLOGMA("done end_write_pipe\n");
	return TRUE;
}
