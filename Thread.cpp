
#include "StdAfx.h"
#include "Thread.h"

CThread::CThread()
{
	m_hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)&ThreadProc,
		(LPVOID)this,
		CREATE_SUSPENDED,
		&m_dwThreadId);

	m_hQuit = CreateEvent(NULL, TRUE, FALSE, NULL);
}

CThread::~CThread()
{
	Terminate();
}

void CThread::Terminate()
{
	if (m_hThread)
	{
		if (m_hQuit)
		{
			SetEvent(m_hQuit);

			if (WAIT_TIMEOUT == WaitForSingleObject(m_hThread, 3000))
				TerminateThread(m_hThread, 0);

			CloseHandle(m_hQuit);
			m_hQuit = 0;
		}
		else
			TerminateThread(m_hThread, 0);

		CloseHandle(m_hThread);
		m_hThread = 0;
	}
	else if (m_hQuit)
	{
		CloseHandle(m_hQuit);
		m_hQuit = 0;
	}
}

void CThread::Run()
{
	ResumeThread(m_hThread);
}

BOOL CThread::Signaled()
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hThread, 0))
		return TRUE;
	else
		return FALSE;
}

BOOL CThread::ShouldQuit()
{
	if (WAIT_OBJECT_0 == WaitForSingleObject(m_hQuit, 0))
		return TRUE;
	else
		return FALSE;
}

DWORD WINAPI CThread::ThreadProc(CThread* This)
{
	BOOL Result = This->Work();

#if _DEBUG
	if (!Result)
		OutputLastError();
#endif

	return (DWORD)Result;
}

BOOL CThread::Work()
{
	return TRUE;
}

