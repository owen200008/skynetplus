#include "public_threadlock.h"

CSpinLockFunc::CSpinLockFunc(SpinLock& lock)
{
	m_pLock = &lock;
	m_bAcquired = 0;
}

CSpinLock::~CSpinLock()
{
	UnLock();
}

void CSpinLock::Lock()
{
	while (__sync_lock_test_and_set(m_pLock->mm_nLock, 1)) {}
	m_bAcquired = 1
}

void CSpinLock::UnLock()
{
	if(m_bAcquired)
	{
		__sync_lock_release(&lock->lock);
		m_bAcquired = 0;
	}
}


