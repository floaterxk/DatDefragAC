
#pragma once

#include "Thread.h"
#include "BlockLoader.h"

class BTree;

#pragma pack(push, 1)
struct DATHeader
{
	DWORD FileType;
	DWORD BlockSize;
	DWORD FileSize;
	DWORD DataSet; // 1 = portal
	DWORD DataSubset;
	DWORD FreeHead;
	DWORD FreeTail;
	DWORD FreeCount;
	DWORD BTree;
	DWORD NewLRU; // 0
	DWORD OldLRU; // 0
	bool bUseLRU; // False
	DWORD MasterMapID;
	DWORD EnginePackVersion;
	DWORD GamePackVersion; // 0
	BYTE VersionMajor[16];
	DWORD VersionMinor;
};
#pragma pack(pop)

#define DEFRAG_BASIC ( 0x00000001UL )
#define DEFRAG_ALL ( 0xFFFFFFFFUL )

void DefragDAT(const char *DATFile, DWORD dwFlags);
void DefragCheckup();
void DefragCleanup();

#define FREE_BLOCK ( 0x80000000UL )

class DATDefragger : public CThread
{
public:
	DATDefragger(const char *DATFile);
	~DATDefragger();

	static void Init(const char *DATFile);
	static void Release();
	static DATDefragger *m_pDefragger;

	virtual BOOL Work();

protected:

	void CloseFileHandles();

	void OpenSourceFile(const char *DATFile);
	void CreateTempFile();
	void DeleteTempFileIfExists();

	BOOL LoadHeader();
	BOOL LoadBTree();

	BOOL SortBTreeData(DWORD *pdwFilesOffset);
	BOOL WriteBlankTempFile();
	BOOL WriteFileData(DWORD dwFilesOffset);
	BOOL WriteBTreeData();
	BOOL WriteEmptyBlocks();
	BOOL WriteHeaderData();
	BOOL FinishDefrag();

	// Variables
	HANDLE m_hSourceFile;
	HANDLE m_hTempFile;
	char *m_SourceFilePath;
	char *m_TempFilePath;

	BlockLoader m_BlockLoader;
	DATHeader m_Header;

	BTree *m_pBTree;
	DWORD m_dwBTreeBlocks;
	DWORD m_dwBTreeBytes;

	DWORD m_dwNewFileSize;
	DWORD m_dwNewFreeCount;
	DWORD m_dwNewFreeHead;
	DWORD m_dwNewFreeTail;
};



