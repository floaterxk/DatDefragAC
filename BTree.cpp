
#include "StdAfx.h"
#include "BTree.h"
#include "BlockLoader.h"

BTree::BTree(BlockLoader *pBlockLoader)
{
	m_pBlockLoader = pBlockLoader;
	m_TreePos = -1;
	m_bLeaf = TRUE;

	ZeroMemory(&m_TreeData, sizeof(m_TreeData));
	ZeroMemory(&m_pBranches, sizeof(m_pBranches));
}

BTree::~BTree()
{
	for (DWORD i = 0; i < 0x3E; i++)
	{
		if (m_pBranches[i])
			delete m_pBranches[i];
	}
}

BOOL BTree::Load(DWORD TreePos, float ProgressBase, float ProgressDelta)
{
	m_TreePos = TreePos;

	if (!m_pBlockLoader->Load_Data(&m_TreeData, sizeof(m_TreeData) - sizeof(DWORD), TreePos))
	{
		Output("Failed loading BTree @ 0x%08X\r\n", TreePos);
		return FALSE;
	}

	if (!m_TreeData.m_Branches[0])
	{
		m_bLeaf = TRUE;
	}
	else
	{
		m_bLeaf = FALSE;

		// Load Children
		DWORD BranchCount = m_TreeData.m_EntryCount + 1;
		float DeltaStep = ProgressDelta / BranchCount;

		for (DWORD i = 0; i < BranchCount; i++)
		{
			m_pBranches[i] = new BTree(m_pBlockLoader);

			if (!m_pBranches[i]->Load(m_TreeData.m_Branches[i], ProgressBase, DeltaStep))
				return FALSE;
			if (g_bQuit)
				return FALSE;

			ProgressBase += DeltaStep;
			UpdateProgressBar(ProgressBase);
		}
	}

	return TRUE;
}

DWORD BTree::GetTotalEntries()
{
	DWORD TotalEntries = m_TreeData.m_EntryCount;

	if (!m_bLeaf)
	{
		DWORD Branches = m_TreeData.m_EntryCount + 1;

		for (UINT i = 0; i < Branches; i++)
		{
			if (m_pBranches[i])
				TotalEntries += m_pBranches[i]->GetTotalEntries();
		}
	}

	return TotalEntries;
}

DWORD BTree::GetTotalBranches()
{
	if (m_bLeaf)
		return 0;

	DWORD Branches = m_TreeData.m_EntryCount + 1;
	DWORD TotalBranches = Branches;

	for (UINT i = 0; i < Branches; i++)
	{
		if (m_pBranches[i])
			TotalBranches += m_pBranches[i]->GetTotalBranches();
	}

	return TotalBranches;
}

BYTE* BTree::NodeDataPtr()
{
	return((BYTE *)&m_TreeData.m_Branches);
}

DWORD BTree::NodeDataSize()
{
	return(sizeof(TreeData) - sizeof(DWORD));
}

DWORD BTree::BlocksNeededForNode()
{
	return(m_pBlockLoader->BlocksNeededForSize(NodeDataSize()));
}

BOOL BTree::AssignNodePositions(DWORD *pdwOffset)
{
	// Assign target file offsets for nodes.
	DWORD NodeBlocksSize = BlocksNeededForNode() * m_pBlockLoader->GetBlockSize();

	m_TargetTreePos = *pdwOffset;
	*pdwOffset = m_TargetTreePos + NodeBlocksSize;

	if (!m_bLeaf)
	{
		DWORD BranchCount = m_TreeData.m_EntryCount + 1;

		for (UINT i = 0; i < BranchCount; i++)
		{
			m_TreeData.m_Branches[i] = *pdwOffset;

			if (!m_pBranches[i]->AssignNodePositions(pdwOffset))
				return FALSE;
		}
	}

	return TRUE;
}

BOOL BTree::CopyFiles(float ProgressBase, float ProgressDelta)
{
	if (g_bQuit)
		return FALSE;

	DWORD EntryCount = m_TreeData.m_EntryCount;
	DWORD BranchCount = m_TreeData.m_EntryCount + 1;
	float DeltaStep = ProgressDelta / BranchCount;

	for (UINT i = 0; i < EntryCount; i++)
	{
		if (!m_bLeaf)
		{
			if (m_pBranches[i])
			{
				if (!m_pBranches[i]->CopyFiles(ProgressBase, DeltaStep))
					return FALSE;
			}
			else
				return FALSE;

			ProgressBase += DeltaStep;
			UpdateProgressBar(ProgressBase);
		}

		// Copy this file.
		DWORD dwTargetOffset = m_pBlockLoader->GetTargetBlockOffset();

		if (!m_pBlockLoader->CopyBlockChain(m_TreeData.m_Entries[i].m_BlockHead))
		{
			return FALSE;
		}

		m_TreeData.m_Entries[i].m_BlockHead = dwTargetOffset;
	}

	if (!m_bLeaf)
	{
		if (m_pBranches[EntryCount])
			m_pBranches[EntryCount]->CopyFiles(ProgressBase, DeltaStep);
		else
			return FALSE;
	}

	ProgressBase += DeltaStep;
	UpdateProgressBar(ProgressBase);

	return TRUE;
}

BOOL BTree::CopyTreeData(float ProgressBase, float ProgressDelta)
{
	if (g_bQuit)
		return FALSE;

	m_pBlockLoader->CopyDataAsBlocks(NodeDataPtr(), NodeDataSize());

	if (!m_bLeaf)
	{
		DWORD BranchCount = m_TreeData.m_EntryCount + 1;
		float DeltaStep = ProgressDelta / BranchCount;

		for (UINT i = 0; i < BranchCount; i++)
		{
			if (!m_pBranches[i]->CopyTreeData(ProgressBase, DeltaStep))
				return FALSE;

			ProgressBase += DeltaStep;
			UpdateProgressBar(ProgressBase);
		}
	}

	return TRUE;
}








