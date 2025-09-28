// SharedMem.h: interface for the CSharedMemQueue class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SHAREDMEM_H__067280DE_BF70_478A_A48F_99A5A0388BBD__INCLUDED_)
#define AFX_SHAREDMEM_H__067280DE_BF70_478A_A48F_99A5A0388BBD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "define.h"

struct _SMQ_HEADER
{
	uintptr_t	Front;
	uintptr_t	Rear;
	int16_t		nCount;

	uint8_t		FrontMode;
	uint8_t		RearMode;

	uint32_t	WritePid;
	uint32_t	ReadPid;

	uint32_t	CreatePid;
};

class CSharedMemQueue
{
private:
	HANDLE			m_hMMFile;
	char*			m_lpMMFile;

	bool			m_bMMFCreate;
	int16_t			m_nMaxCount;
	uint32_t		m_wOffset;
	uintptr_t		m_lReference;
	_SMQ_HEADER*	m_pHeader;

public:
	inline uintptr_t GetFrontPointer() const {
		return m_pHeader->Front;
	}

	inline uintptr_t GetRearPointer() const {
		return m_pHeader->Rear;
	}

	inline uint32_t GetProcessId() const {
		return m_pHeader->CreatePid;
	}

	inline uint8_t GetFrontMode() const {
		return m_pHeader->FrontMode;
	}

	inline uint8_t GetRearMode() const {
		return m_pHeader->RearMode;
	}

	inline int GetCount() const {
		return m_pHeader->nCount;
	}

	int GetData(char* pBuf);
	int PutData(char* pBuf, int size);
	bool InitializeMMF(uint32_t dwOffsetsize, int maxcount, LPCTSTR lpname, bool bCreate = true);
	CSharedMemQueue();
	virtual ~CSharedMemQueue();
};

#endif // !defined(AFX_SHAREDMEM_H__067280DE_BF70_478A_A48F_99A5A0388BBD__INCLUDED_)
