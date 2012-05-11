#pragma once

class CKuCriticalSection
{
public:
	CKuCriticalSection()
	{ InitializeCriticalSection(&m_cs); }
	virtual ~CKuCriticalSection()
	{ DeleteCriticalSection(&m_cs); }

	void Enter()
	{ EnterCriticalSection(&m_cs); }
	BOOL TryEnter()
	{ return TryEnterCriticalSection(&m_cs); }
	void Leave()
	{ LeaveCriticalSection(&m_cs); }
private:
	CRITICAL_SECTION m_cs;
};

class CKuAutoCriticalSection
{
public:
	CKuAutoCriticalSection(CKuCriticalSection *pCs) : m_pCs(pCs)
	{ ASSERT(m_pCs); m_pCs->Enter(); }
	CKuAutoCriticalSection(CKuCriticalSection &cs) : m_pCs(&cs)
	{ ASSERT(m_pCs); m_pCs->Enter(); }
	~CKuAutoCriticalSection()
	{ m_pCs->Leave(); }
private:
	CKuCriticalSection *m_pCs;
};

/*
class CReadWriteLocker
{
public:
	CReadWriteLocker()
		: m_uReadLocks(0)
	{
		m_hEvent = CreateEvent(NULL, TRUE, TRUE, NULL);
		ASSERT(m_hEvent);
	}
	virtual ~CReadWriteLocker()
	{
		CloseHandle(m_hEvent);
	}
	bool WriteLock(DWORD dwMilliseconds = INFINITE)
	{
		CKuAutoCriticalSection theRequestLocker(m_RequestLock); //wait for other threads which requesting reading or writing rights.

		WaitForSingleObject(m_hEvent, INFINITE); //wait for reading threads

		return m_WriteLock.Lock(dwMilliseconds);
	}
	bool WriteUnlock()
	{
		return m_WriteLock.Unlock();
	}
	UINT ReadLock()
	{
		CKuAutoCriticalSection theRequestLocker(m_RequestLock); //wait for other threads which requesting reading or writing rights.
		CKuAutoCriticalSection theReadCountLocker(m_ReadCountLock); //avoid the readers' counter modified by other threads

		if (m_uReadLocks == 0) {
			CKuAutoCriticalSection theWritingLocker(m_WriteLock); //wait for writing threads
			ResetEvent(m_hEvent);
		}

		return ++m_uReadLocks;
	}
	UINT ReadUnlock()
	{
		CKuAutoCriticalSection theReadCountLocker(m_ReadCountLock); //avoid the readers' counter modified by other threads

		if (m_uReadLocks > 0) {
			m_uReadLocks--;
			if (m_uReadLocks == 0)
				SetEvent(m_hEvent);
		}

		return m_uReadLocks;
	}
private:
	CKuCriticalSection m_WriteLock;
	CKuCriticalSection m_ReadCountLock;
	CKuCriticalSection m_RequestLock;
	HANDLE m_hEvent;
	UINT m_uReadLocks;
};
*/
