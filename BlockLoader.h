
#pragma once

struct DATHeader;

class BlockLoader
{
public:
	BlockLoader();
	~BlockLoader();

	// Init Stuff
	void Init(HANDLE hSourceFile, HANDLE hTempFile);
	BOOL LoadHeader(DATHeader *pHeader);
	BOOL AllocBlockForCopying();
	void FreeBlockForCopying();

	DWORD GetBlockSize();
	DWORD GetFileSize();

	// Block Chain
	BOOL CopyBlockChain(DWORD dwSourceChain);

	// Read/Write
	BOOL ReadSourceData(LPVOID pBuffer, DWORD dwLength, DWORD dwOffset);
	BOOL WriteTempData(LPVOID pBuffer, DWORD dwLength, DWORD dwOffset);

	// Read blocks
	BOOL Load_Data(LPVOID pBuffer, DWORD Length, DWORD dwFirstBlock);

	// How many blocks are needed to fit this allocation size?
	DWORD BlocksNeededForSize(DWORD dwAllocSize);

	// Write data into next blocks.
	BOOL CopyDataAsBlocks(BYTE* pbData, DWORD dwSize);

	// Write empty blocks
	BOOL WriteEmptyBlocks(DWORD Head, DWORD *pTail, DWORD *pCount, DWORD FileSize);

	// Where the new data should be written
	DWORD GetTargetBlockOffset();
	void SetTargetBlockOffset(DWORD dwOffset);

	BOOL CalcAllocBlocks(DWORD BlockHead, DWORD *pCount);
	BOOL IsValidBlock(DWORD dwOffset);

private:

	HANDLE m_hSourceFile;
	HANDLE m_hTempFile;

	DATHeader *m_pHeader;

	BYTE *m_pCopyBlock;
	DWORD m_dwTargetBlock;
};

