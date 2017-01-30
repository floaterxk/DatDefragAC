

#include "StdAfx.h"
#include "DefragDAT.h"

HINSTANCE g_hInstance;
BOOL g_bQuit = FALSE;

int APIENTRY WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR lpCmdLine,
	int nCmdShow)
{

	INITCOMMONCONTROLSEX iccex;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	iccex.dwICC = ICC_INTERNET_CLASSES;
	InitCommonControlsEx(&iccex);

	g_hInstance = hInstance;
	g_hWndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainProc);

	if (!g_hWndMain)
	{
		MessageBox(NULL, "There was an error creating the main dialog. Exiting..", "Fatal Error", MB_ICONHAND);
		return 0;
	}

	ShowWindow(g_hWndMain, nCmdShow);

	MSG msg;
	msg.message = WM_NULL;

	while (!g_bQuit)
	{
		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			if (IsDialogMessage(g_hWndMain, &msg))
				continue;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			DefragCheckup();
			Sleep(1);
		}

		if (msg.message == WM_QUIT)
			g_bQuit = TRUE;
	}

	DefragCleanup();

	return 0;
}





