
#include "StdAfx.h"
#include "DefragDAT.h"
#include "BTree.h"

DATDefragger* DATDefragger::m_pDefragger = NULL;

void DefragDAT(const char *DATFile, DWORD dwFlags)
{
	if (DATDefragger::m_pDefragger)
		return;

	ClearOutput();
	Output("DAT defragmentation initiated.\r\n\r\n");

	DATDefragger::Init(DATFile);
}

void DefragCheckup()
{
	if (DATDefragger::m_pDefragger)
	{
		if (DATDefragger::m_pDefragger->Signaled())
		{
			Output("DAT defragmentation completed.\r\n");
			DATDefragger::Release();
		}
	}
}

void DefragCleanup()
{
	if (DATDefragger::m_pDefragger)
	{
		DATDefragger::m_pDefragger->Terminate();
		DATDefragger::Release();
	}
}

void DATDefragger::Init(const char *DATFile)
{
	m_pDefragger = new DATDefragger(DATFile);
	m_pDefragger->Run();
}

void DATDefragger::Release()
{
	if (m_pDefragger)
	{
		delete m_pDefragger;
		m_pDefragger = NULL;
	}
}

DATDefragger::DATDefragger(const char* DATFile)
{
	OpenSourceFile(DATFile);
	CreateTempFile();

	ZeroMemory(&m_Header, sizeof(m_Header));
	m_pBTree = NULL;
}

DATDefragger::~DATDefragger()
{
	if (m_pBTree)
		delete m_pBTree;

	CloseFileHandles();
	DeleteTempFileIfExists();

	if (m_SourceFilePath)
		free(m_SourceFilePath);
	if (m_TempFilePath)
		free(m_TempFilePath);
}

void DATDefragger::CloseFileHandles()
{
	if (m_hSourceFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hSourceFile);
		m_hSourceFile = INVALID_HANDLE_VALUE;
	}

	if (m_hTempFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_hTempFile);
		m_hTempFile = INVALID_HANDLE_VALUE;
	}
}

void DATDefragger::OpenSourceFile(const char *DATFile)
{
	m_SourceFilePath = _strdup(DATFile);
	m_hSourceFile = CreateFile(m_SourceFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

	if (m_hSourceFile == INVALID_HANDLE_VALUE)
	{
		MessageBox(NULL, "Failed opening source file!", "Catastrophic error", MB_ICONHAND | MB_OK);
		Output("Failed opening source file!\r\n");
	}
}

void DATDefragger::CreateTempFile()
{
	char TempFilePath[MAX_PATH];
	const char *TempFolder = GetDefaultDATFolder();

	if (!PathFileExists(TempFolder))
		TempFolder = ".";

	UINT uid = GetTempFileName(TempFolder, "dfrag", 0, TempFilePath);

	if (!uid)
	{
		MessageBox(NULL, "Failed creating temporary file!", "Catastrophic error", MB_ICONHAND | MB_OK);
		Output("Failed creating temporary file!\r\n");
		m_hTempFile = INVALID_HANDLE_VALUE;
	}
	else
	{
		m_TempFilePath = _strdup(TempFilePath);
		m_hTempFile = CreateFile(TempFilePath, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	}
}

void DATDefragger::DeleteTempFileIfExists()
{
	if (PathFileExists(m_TempFilePath))
		DeleteFile(m_TempFilePath);
}

BOOL DATDefragger::Work()
{
	if (!m_hTempFile || !m_hSourceFile)
		return FALSE;

	m_BlockLoader.Init(m_hSourceFile, m_hTempFile);

	if (!LoadHeader())
		return FALSE;
	if (!LoadBTree())
		return FALSE;

	DWORD FilesOffset;
	if (!SortBTreeData(&FilesOffset))
		return FALSE;
	if (!WriteBlankTempFile())
		return FALSE;
	if (!WriteFileData(FilesOffset))
		return FALSE;
	if (!WriteBTreeData())
		return FALSE;
	if (!WriteEmptyBlocks())
		return FALSE;
	if (!WriteHeaderData())
		return FALSE;
	if (!FinishDefrag())
		return FALSE;

	return TRUE;
}

BOOL DATDefragger::FinishDefrag()
{

	Output("========================================\r\n");
	Output("Finish up defragmentation..\r\n");

	CloseFileHandles();

	char BackupFile[MAX_PATH + 20];
	strncpy(BackupFile, m_SourceFilePath, MAX_PATH);
	strcat(BackupFile, "-backup");

	if (MoveFileEx(m_SourceFilePath, BackupFile, 0) || (GetLastError() == 183))
	{
		if (MoveFileEx(m_TempFilePath, m_SourceFilePath, MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING))
		{
		}
		else
		{
			Output("Failed renaming of temporary file, please restore your backup..\r\n");
			goto error_exit;
		}
	}
	else
	{
		Output("Failed backing up original data, aborting operation..\r\n");
		goto error_exit;
	}

	Output("========================================\r\n\r\n");
	return TRUE;

error_exit:
	Output("========================================\r\n\r\n");
	return FALSE;
}

BOOL DATDefragger::LoadHeader()
{
	return m_BlockLoader.LoadHeader(&m_Header);
}

BOOL DATDefragger::LoadBTree()
{
	BOOL bResult;

	InitProgressBar(0, 200, (COLORREF)0x00AA0000);

	Output("========================================\r\n");
	Output("Loading BTree data.. please wait.\r\n");

	m_pBTree = new BTree(&m_BlockLoader);

	if (bResult = m_pBTree->Load(m_Header.BTree))
	{
		DWORD TotalBranches = m_pBTree->GetTotalBranches();
		DWORD TotalEntries = m_pBTree->GetTotalEntries();

		Output("Total Branches: %u\r\n", TotalBranches);
		Output("Total Entries: %u\r\n", TotalEntries);
	}
	else
		Output("Error loading BTree data!\r\n");

	return TRUE;
}

BOOL DATDefragger::WriteBlankTempFile()
{
	m_dwNewFileSize = m_Header.FileSize;

	if (INVALID_SET_FILE_POINTER == SetFilePointer(m_hTempFile, 0, 0, FILE_BEGIN))
		return FALSE;

	BYTE BlankHeader[1024];
	ZeroMemory(BlankHeader, 1024);

	DWORD dwBytesWritten;
	WriteFile(m_hTempFile, BlankHeader, 1024, &dwBytesWritten, FALSE);

	if (dwBytesWritten != 1024)
		return FALSE;

	return TRUE;
}

BOOL DATDefragger::SortBTreeData(DWORD *pdwFilesOffset)
{
	Output("\r\nSorting BTree data..\r\n");

	DWORD dwOffset = 1024;

	if (!m_pBTree->AssignNodePositions(&dwOffset))
		goto error_exit;

	*pdwFilesOffset = dwOffset;

	Output("BTree size: %u bytes\r\n", dwOffset - 1024);
	Output("========================================\r\n\r\n");
	return TRUE;
error_exit:
	Output("Error while sorting BTree data!\n");
	Output("========================================\r\n\r\n");
	return FALSE;
}

BOOL DATDefragger::WriteFileData(DWORD dwOffset)
{
	Output("========================================\r\n");
	Output("Defragmenting file entries..\r\n");

	m_BlockLoader.SetTargetBlockOffset(dwOffset);
	InitProgressBar(0, 200, (COLORREF)0x000000FF);

	if (!m_pBTree->CopyFiles())
		goto error_exit;

	m_dwNewFreeHead = m_BlockLoader.GetTargetBlockOffset();

#if _DEBUG
	Output("DEBUG: Free blocks will start at %u\r\n", m_dwNewFreeHead);
#endif

	Output("========================================\r\n\r\n");
	return TRUE;
error_exit:
	Output("Error while defragmenting!\r\n");
	Output("========================================\r\n\r\n");
	return FALSE;
}

BOOL DATDefragger::WriteBTreeData()
{
	Output("========================================\r\n");
	Output("Defragmenting BTree data..\r\n");

	m_BlockLoader.SetTargetBlockOffset(1024);
	InitProgressBar(0, 200, (COLORREF)0x008000FF);
	if (!m_pBTree->CopyTreeData())
		goto error_exit;

#if _DEBUG
	Output("DEBUG: End offset %u\r\n", m_BlockLoader.GetTargetBlockOffset());
#endif

	Output("========================================\r\n\r\n");
	return TRUE;
error_exit:
	Output("Error while defragmenting!\r\n");
	Output("========================================\r\n\r\n");
	return FALSE;
}

BOOL DATDefragger::WriteEmptyBlocks()
{
	Output("========================================\r\n");
	Output("Writing empty blocks..\r\n");

	if (!m_BlockLoader.WriteEmptyBlocks(m_dwNewFreeHead, &m_dwNewFreeTail, &m_dwNewFreeCount, m_dwNewFileSize))
		goto error_exit;

	Output("========================================\r\n\r\n");
	return TRUE;

error_exit:
	Output("Error writing empty blocks!\r\n");
	Output("========================================\r\n\r\n");
	return FALSE;
}

BOOL DATDefragger::WriteHeaderData()
{
	Output("========================================\r\n");
	Output("Writing header data..\r\n");

	DATHeader NewHeader = m_Header;
	NewHeader.FileSize = m_dwNewFileSize;
	NewHeader.FreeHead = m_dwNewFreeHead;
	NewHeader.FreeTail = m_dwNewFreeTail;
	NewHeader.FreeCount = m_dwNewFreeCount;
	NewHeader.BTree = 1024;

	if (!m_BlockLoader.WriteTempData(&NewHeader, sizeof(DATHeader), 0x140))
		goto error_exit;

	Output("========================================\r\n\r\n");
	return TRUE;

error_exit:
	Output("Error writing header!\r\n");
	Output("========================================\r\n\r\n");
	return FALSE;
}


