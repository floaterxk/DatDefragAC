
#pragma once

class BlockLoader;

struct BTEntry_t
{
	BTEntry_t()
	{
		m_FileID = 0;
		m_BlockHead = 0;
		m_FileLength = 0;
	}

	DWORD m_Unknown00;
	DWORD m_FileID;
	DWORD m_BlockHead;
	DWORD m_FileLength;
	DWORD m_TimeStamp;
	DWORD m_Unknown14;
};

class BTree
{
public:
	BTree(BlockLoader *);
	~BTree();

	BOOL Load(DWORD TreePos, float ProgressBase = 0, float ProgressDelta = 100.0f);

	DWORD GetTotalEntries();
	DWORD GetTotalBranches();

	BOOL CopyFiles(float ProgressBase = 0, float ProgressDelta = 100.0f);
	BOOL CopyTreeData(float ProgressBase = 0, float ProgressDelta = 100.0f);

	BYTE* NodeDataPtr();
	DWORD NodeDataSize();
	DWORD BlocksNeededForNode();

	BOOL AssignNodePositions(DWORD *pdwOffset);

private:

	BOOL ReadTree();

	HANDLE m_hFile;
	DWORD m_TreePos;

	BlockLoader *m_pBlockLoader;

	struct TreeData {
		DWORD SPACER;
		DWORD m_Branches[0x3E];
		DWORD m_EntryCount;
		BTEntry_t m_Entries[0x3D];
	} m_TreeData;

	BTree* m_pBranches[0x3E];
	BOOL m_bLeaf;

	// After defragmenting, the new info..

	DWORD m_TargetTreePos;
};

