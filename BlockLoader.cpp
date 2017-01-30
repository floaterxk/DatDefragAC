
#include "StdAfx.h"
#include "BlockLoader.h"
#include "DefragDAT.h"

BlockLoader::BlockLoader()
{
	m_hSourceFile = INVALID_HANDLE_VALUE;
	m_hTempFile = INVALID_HANDLE_VALUE;
	m_pHeader = NULL;

	m_pCopyBlock = NULL;
	m_dwTargetBlock = 1024;
}

BlockLoader::~BlockLoader()
{
	FreeBlockForCopying();
}

void BlockLoader::Init(HANDLE hSourceFile, HANDLE hTempFile)
{
	m_hSourceFile = hSourceFile;
	m_hTempFile = hTempFile;
}

DWORD BlockLoader::GetBlockSize()
{
	return m_pHeader->BlockSize;
}

DWORD BlockLoader::GetFileSize()
{
	return m_pHeader->FileSize;
}

BOOL BlockLoader::LoadHeader(DATHeader *pHeader)
{
	if (!ReadSourceData(pHeader, sizeof(DATHeader), 0x140))
	{
		Output("** Failed reading header!\r\n");
		return FALSE;
	}
	m_pHeader = pHeader;

	AllocBlockForCopying();

	return TRUE;
}

BOOL BlockLoader::ReadSourceData(LPVOID pBuffer, DWORD dwLength, DWORD dwOffset)
{
	if (INVALID_SET_FILE_POINTER == SetFilePointer(m_hSourceFile, dwOffset, 0, FILE_BEGIN))
	{
#if _DEBUG
		__asm int 3;
#endif
		return FALSE;
	}

	DWORD dwBytesRead;

	if (!ReadFile(m_hSourceFile, pBuffer, dwLength, &dwBytesRead, FALSE))
		return FALSE;

	if (dwBytesRead != dwLength)
		return FALSE;

	return TRUE;
}

BOOL BlockLoader::WriteTempData(LPVOID pBuffer, DWORD dwLength, DWORD dwOffset)
{
	if (INVALID_SET_FILE_POINTER == SetFilePointer(m_hTempFile, dwOffset, 0, FILE_BEGIN))
	{
#if _DEBUG
		__asm int 3;
#endif
		return FALSE;
	}

	DWORD dwBytesWritten;

	if (!WriteFile(m_hTempFile, pBuffer, dwLength, &dwBytesWritten, FALSE))
	{
#if _DEBUG
		__asm int 3;
#endif
		return FALSE;
	}

	if (dwBytesWritten != dwLength)
		return FALSE;

	return TRUE;
}

BOOL BlockLoader::AllocBlockForCopying()
{
	m_pCopyBlock = new BYTE[GetBlockSize()];

	return(m_pCopyBlock ? TRUE : FALSE);
}

void BlockLoader::FreeBlockForCopying()
{
	if (m_pCopyBlock)
	{
		delete[] m_pCopyBlock;
		m_pCopyBlock = NULL;
	}
}

DWORD BlockLoader::GetTargetBlockOffset()
{
	return m_dwTargetBlock;
}

void BlockLoader::SetTargetBlockOffset(DWORD dwOffset)
{
	m_dwTargetBlock = dwOffset;
}

BOOL BlockLoader::CopyBlockChain(DWORD dwSourceChain)
{
	DWORD dwBlockSize = GetBlockSize();

	while (dwSourceChain)
	{
		if (dwSourceChain & FREE_BLOCK)
			return FALSE;
		if (!IsValidBlock(dwSourceChain))
			return FALSE;

		if (!ReadSourceData(m_pCopyBlock, dwBlockSize, dwSourceChain))
			return FALSE;

		dwSourceChain = ((DWORD *)m_pCopyBlock)[0];

		DWORD ThisBlock = m_dwTargetBlock;
		DWORD NextBlock = m_dwTargetBlock + dwBlockSize;

		if (dwSourceChain)
			((DWORD *)m_pCopyBlock)[0] = NextBlock;
		else
			((DWORD *)m_pCopyBlock)[0] = 0;

		if (!WriteTempData(m_pCopyBlock, dwBlockSize, ThisBlock))
			return FALSE;

		m_dwTargetBlock = NextBlock;
	}

	return TRUE;
}

BOOL BlockLoader::CopyDataAsBlocks(BYTE* pbData, DWORD dwSize)
{
	DWORD dwBlockSize = GetBlockSize();
	DWORD dwDataPerBlock = dwBlockSize - sizeof(DWORD);

	while (dwSize > 0)
	{
		if (dwSize > dwDataPerBlock)
		{
			((DWORD *)m_pCopyBlock)[0] = m_dwTargetBlock + dwBlockSize;
			memcpy(((DWORD *)m_pCopyBlock) + 1, pbData, dwDataPerBlock);
			dwSize -= dwDataPerBlock;
			pbData += dwDataPerBlock;
		}
		else
		{
			ZeroMemory(m_pCopyBlock, dwBlockSize);
			memcpy(((DWORD *)m_pCopyBlock) + 1, pbData, dwSize);
			dwSize = 0;
		}

		if (!WriteTempData(m_pCopyBlock, dwBlockSize, m_dwTargetBlock))
			return FALSE;

		m_dwTargetBlock += dwBlockSize;
	}

	return TRUE;
}

BOOL BlockLoader::WriteEmptyBlocks(DWORD Head, DWORD *pTail, DWORD *pCount, DWORD FileSize)
{
	DWORD BlockSize = GetBlockSize();

	if ((Head + BlockSize) > FileSize)
		return FALSE;

	ZeroMemory(m_pCopyBlock, BlockSize);

	DWORD dwTailBlock = Head;
	DWORD dwNextBlock = Head;
	DWORD dwCount = 0;

	while (dwNextBlock)
	{
		dwCount++;
		dwTailBlock = dwNextBlock;
		dwNextBlock = dwTailBlock + BlockSize;

		if ((dwNextBlock + BlockSize) > FileSize)
			dwNextBlock = 0;

		((DWORD *)m_pCopyBlock)[0] = dwNextBlock | FREE_BLOCK;

		if (!WriteTempData(m_pCopyBlock, BlockSize, dwTailBlock))
			return FALSE;
	}

	*pTail = dwTailBlock;
	*pCount = dwCount;

	return TRUE;
}

// For alloc chains, this returns the number of blocks in the chain.
BOOL BlockLoader::CalcAllocBlocks(DWORD BlockHead, DWORD *pCount)
{
	DWORD Block = BlockHead;
	DWORD NumAllocs = 0;

	if ((Block & FREE_BLOCK))
		return FALSE;

	while (Block)
	{
		if (!IsValidBlock(Block))
			return FALSE;

		NumAllocs++;

		// What's the next free block?
		if (!ReadSourceData(&Block, sizeof(DWORD), Block))
			return FALSE;

		// If this isn't a free block, something's wrong..
		if ((Block & FREE_BLOCK))
			return FALSE;
	}

	*pCount = NumAllocs;
	return TRUE;
}

BOOL BlockLoader::Load_Data(LPVOID pBuffer, DWORD Length, DWORD dwFirstBlock)
{
	DWORD dwLength = Length;
	BYTE* pbBuffer = (BYTE *)pBuffer;

	DWORD dwBlockSize = m_pHeader->BlockSize;
	DWORD dwDataSize = dwBlockSize - sizeof(DWORD);

	BOOL bOK = TRUE;

	if (!dwLength)
		return TRUE;

	DWORD dwBlock = dwFirstBlock;

	while (dwBlock)
	{
		if (!bOK)
			return FALSE;

		DWORD dwOldValue = *((DWORD *)pbBuffer);

		if (dwDataSize > dwLength)
		{
			dwBlockSize = dwLength + sizeof(DWORD);
			dwDataSize = dwLength;
		}

		if (ReadSourceData(pbBuffer, dwBlockSize, dwBlock))
		{
			dwBlock = *((DWORD *)pbBuffer);
			*((DWORD *)pbBuffer) = dwOldValue;
			dwLength -= dwDataSize;
			pbBuffer += dwDataSize;

			if (dwBlock & 0x80000000)
			{
				dwBlock &= ~0x80000000;
				bOK = FALSE;
			}
		}
		else
			bOK = FALSE;

		if (!dwLength)
			break;
	}

	if (dwLength)
		bOK = FALSE;

	return bOK;
}

BOOL BlockLoader::IsValidBlock(DWORD dwOffset)
{
	dwOffset &= ~0x80000000;

	if (dwOffset < 1024)
		return FALSE;

	if (dwOffset > (m_pHeader->FileSize - m_pHeader->BlockSize))
		return FALSE;

	if (dwOffset % m_pHeader->BlockSize)
		return FALSE;

	return TRUE;
}

DWORD BlockLoader::BlocksNeededForSize(DWORD dwAllocSize)
{
	DWORD dwDataPerBlock = m_pHeader->BlockSize - sizeof(DWORD);

	if (dwAllocSize % dwDataPerBlock)
		return((dwAllocSize / dwDataPerBlock) + 1);
	else
		return((dwAllocSize / dwDataPerBlock));
}





