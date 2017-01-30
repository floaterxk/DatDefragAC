
#pragma once

// Dialog Variables
extern HWND g_hWndMain;
extern string g_strDATFile;
extern string g_strDefaultDATFolder;

// Dialog Functions
extern int CALLBACK MainProc(HWND hWnd, UINT MsgID, WPARAM wParam, LPARAM lParam);
extern void ShowDATFileDialog(void);
extern void SetDATFilePath(const char* FilePath);
extern const char* GetDATFilePath(void);
extern void SetDefaultDATFolder(const char* FolderPath);
extern const char* GetDefaultDATFolder(void);
extern void ClearOutput();
extern void OutputLastError();
extern void Output(const char* Format, ...);
extern void InitProgressBar(long lMin, long lMax, COLORREF crColor);
extern void UpdateProgressBar(long lPos);
extern void UpdateProgressBar(float fPos);