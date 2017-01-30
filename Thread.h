
#pragma once

class CThread
{
public:
	CThread();
	~CThread();

	static DWORD WINAPI ThreadProc(CThread* This);

	// Checks the thread-signal state.
	BOOL Signaled();

	// Check the quit-signal state.
	BOOL ShouldQuit();

	// Shutdown the thread (call if application shutdown)
	void Terminate();

	// Resumes the thread (call this after construction)
	void Run();

	// The virtual work cycle.
	virtual BOOL Work();

protected:

	HANDLE m_hThread;
	DWORD m_dwThreadId;

	HANDLE m_hQuit;
};