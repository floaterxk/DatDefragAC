
#include "StdAfx.h"
#include "DefragDAT.h"

// Main Window Handle
HWND g_hWndMain = 0;

// Current File.
string g_strDATFile;

// Default DAT Folder
string g_strDefaultDATFolder;

int CALLBACK MainProc(HWND hWnd, UINT MsgID, WPARAM wParam, LPARAM lParam)
{
	switch (MsgID)
	{
		case WM_INITDIALOG:
		{
			g_hWndMain = hWnd;

			HICON hIcon = LoadIcon(g_hInstance, MAKEINTRESOURCE(IDI_MAINICON));
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
			DeleteObject(hIcon);

			HKEY hKey;
			char szACPath[MAX_PATH + 32];
			memset(szACPath, 0, sizeof(szACPath));
			DWORD dwLength = MAX_PATH;
			BOOL bFolderSet = FALSE;

			if (!RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Decal\\Agent", 0, KEY_READ, &hKey))
			{
				if (!RegQueryValueEx(hKey, "PortalPath", NULL, NULL, (BYTE*)szACPath, &dwLength))
				{
					// Append backslash to path.
					if (szACPath[strlen(szACPath) - 1] != '\\')
					{
						char *end = &szACPath[strlen(szACPath)];
						end[0] = '\\';
						end[1] = '\0';
					}

					SetDefaultDATFolder(szACPath);

					strcat(szACPath, "client_cell_1.dat");
					SetDATFilePath(szACPath);
					bFolderSet = TRUE;
				}

				RegCloseKey(hKey);
			}

			if (!bFolderSet)
			{
				strcpy(szACPath, "C:\\Program Files\\Turbine\\Asheron's Call - Throne of Destiny\\");
				SetDATFilePath(szACPath);
				strcat(szACPath, "client_cell_1.dat");
				SetDATFilePath(szACPath);
			}

			return TRUE;
		}
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			DestroyWindow(hWnd);

			return FALSE;
		}
		case WM_COMMAND:
		{
			WORD wEvent = HIWORD(wParam);
			WORD wID = LOWORD(wParam);

			switch (wID)
			{
				case IDC_OPENDAT:
				case ID_FILE_OPENDAT:
				{
					ShowDATFileDialog();
					break;
				}
				case ID_FILE_EXIT:
				{
					PostQuitMessage(0);
					DestroyWindow(hWnd);
					break;
				}
				case ID_HELP_ABOUT:
				{
					MessageBox(hWnd, "DAT Defragmenter for Asheron's Call: Throne of Destiny\n\nWritten by Sean Hunczak (\"Pea\")\nCompiled on " __TIMESTAMP__ "\n\nContact: sean@hunczak.com", "About DAT Defragmenter", MB_OK);
					break;
				}
				case IDC_DEFRAG:
				{
					const char* DATFile = GetDATFilePath();

					if (DATFile)
					{
						DefragDAT(DATFile, DEFRAG_ALL);
					}

					break;
				}
			}

			return FALSE;
		}
	}

	return FALSE;
}

void SetDATFilePath(const char* FilePath)
{
	g_strDATFile = FilePath;

	// Update the dialog.
	if (g_hWndMain)
	{
		HWND hWndFilePath = GetDlgItem(g_hWndMain, IDC_FILE);

		if (hWndFilePath)
			SetWindowText(hWndFilePath, FilePath);
	}
}

const char* GetDATFilePath()
{
	if (g_strDATFile.length() > 0)
		return g_strDATFile.c_str();
	else
		return NULL;
}

void SetDefaultDATFolder(const char* FolderPath)
{
	g_strDefaultDATFolder = FolderPath;
}

const char* GetDefaultDATFolder()
{
	if (g_strDefaultDATFolder.length() > 0)
		return g_strDefaultDATFolder.c_str();
	else
		return NULL;
}

void ShowDATFileDialog(void)
{
	char szFile[MAX_PATH];
	*szFile = '\0';

	char szFileTitle[MAX_PATH];
	*szFileTitle = '\0';

	OPENFILENAME OpenFileName;

	OpenFileName.lStructSize = sizeof(OPENFILENAME);
	OpenFileName.hwndOwner = g_hWndMain;
	OpenFileName.hInstance = NULL;
	OpenFileName.lpstrFilter = ".DAT File\0*.dat\0All Files\0*\0\0";
	OpenFileName.lpstrCustomFilter = NULL;
	OpenFileName.nMaxCustFilter = 0;
	OpenFileName.nFilterIndex = 0;
	OpenFileName.lpstrFile = szFile;
	OpenFileName.nMaxFile = sizeof(szFile);
	OpenFileName.lpstrFileTitle = szFileTitle;
	OpenFileName.nMaxFileTitle = sizeof(szFileTitle);
	OpenFileName.lpstrInitialDir = GetDefaultDATFolder();
	OpenFileName.lpstrTitle = "Select .DAT File";
	OpenFileName.nFileOffset = 0;
	OpenFileName.nFileExtension = 0;
	OpenFileName.lpstrDefExt = NULL;
	OpenFileName.lCustData = (long)NULL;
	OpenFileName.lpfnHook = NULL;
	OpenFileName.lpTemplateName = NULL;
	OpenFileName.Flags = OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&OpenFileName))
	{
		SetDATFilePath(szFile);
	}
}

void ClearOutput()
{
	if (g_hWndMain)
	{
		HWND hWndOutDlg = GetDlgItem(g_hWndMain, IDC_OUTPUT);

		if (hWndOutDlg)
			SetWindowText(hWndOutDlg, "");
	}
}

void Output(const char* Format, ...)
{
	va_list ArgPtr;
	va_start(ArgPtr, Format);

	// How many characters are needed?
	int CharsNeeded = _vscprintf(Format, ArgPtr) + 1;

	// Allocate temporary buffer.
	char *TempBuffer = new char[CharsNeeded];

	// Write the formatted string.
	int CharsWritten = _vsnprintf(TempBuffer, CharsNeeded, Format, ArgPtr);

	// Output to Dialog.
	if ((g_hWndMain) && (CharsWritten > 0))
	{
		HWND hWndOutDlg = GetDlgItem(g_hWndMain, IDC_OUTPUT);

		if (hWndOutDlg)
		{
			DWORD OldSelStart, OldSelEnd;
			int TextLength;

			TextLength = (int)SendMessage(hWndOutDlg, WM_GETTEXTLENGTH, 0, 0);
			SendMessage(hWndOutDlg, EM_GETSEL, (WPARAM)&OldSelStart, (LPARAM)&OldSelEnd);
			SendMessage(hWndOutDlg, EM_SETSEL, TextLength, TextLength);
			SendMessage(hWndOutDlg, EM_REPLACESEL, FALSE, (LPARAM)TempBuffer);
			SendMessage(hWndOutDlg, EM_SETSEL, OldSelStart, OldSelEnd);
		}
	}

	// Free temporary buffer.
	delete[] TempBuffer;

	va_end(ArgPtr);
}

void OutputLastError()
{
	LPTSTR lpBuffer;
	DWORD LastError = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpBuffer,
		0,
		NULL);

	Output("Error #%X returned:\r\n", LastError);
	Output("%s", lpBuffer);

	LocalFree(lpBuffer);
}

long g_lPBRMin = 0;
long g_lPBRMax = 0;
long g_lPBRPos = 0;

void InitProgressBar(long lMin, long lMax, COLORREF crColor)
{
	if (g_hWndMain)
	{
		HWND hWndPB = GetDlgItem(g_hWndMain, IDC_PROGRESS);

		if (hWndPB)
		{
			g_lPBRMin = lMin;
			g_lPBRMax = lMax;
			g_lPBRPos = 0;

			SendMessage(hWndPB, PBM_SETRANGE32, lMin, lMax);
			SendMessage(hWndPB, PBM_SETPOS, 0, 0);
			SendMessage(hWndPB, PBM_SETBARCOLOR, 0, crColor);
		}
	}
}

void UpdateProgressBar(long lPos)
{
	if (lPos == g_lPBRPos)
		return;

	g_lPBRPos = lPos;

	if (g_hWndMain)
	{
		HWND hWndPB = GetDlgItem(g_hWndMain, IDC_PROGRESS);

		if (hWndPB)
			PostMessage(hWndPB, PBM_SETPOS, lPos, 0);
	}
}

void UpdateProgressBar(float fPercent)
{
	long Position;
	Position = (long)(g_lPBRMin + ((g_lPBRMax - g_lPBRMin) * (fPercent / 100.0f)));

	if (Position > g_lPBRMax)
		Position = g_lPBRMax;

	UpdateProgressBar(Position);
}



