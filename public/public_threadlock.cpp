#include "public_threadlock.h"

void BasicSleep(unsigned int dwMilliseconds)
{
	timespec tmDelay;
	tmDelay.tv_sec = dwMilliseconds/1000;	//秒	
	tmDelay.tv_nsec = (dwMilliseconds%1000)*1000000;
	nanosleep(&tmDelay, NULL);
}

CSpinLockFunc::CSpinLockFunc(SpinLock& lock)
{
	m_pLock = &lock;
	m_bAcquired = 0;
}

CSpinLockFunc::~CSpinLockFunc()
{
	UnLock();
}

void CSpinLockFunc::Lock()
{
	while (__sync_lock_test_and_set(&(m_pLock->m_nLock), 1)) {}
	m_bAcquired = 1;
}

void CSpinLockFunc::LockAndSleep(unsigned short usSleep)
{
	while (__sync_lock_test_and_set(&(m_pLock->m_nLock), 1)) 
	{
		BasicSleep(usSleep);
	}

	m_bAcquired = 1;
}

void CSpinLockFunc::UnLock()
{
	if(m_bAcquired)
	{
		__sync_lock_release(&(m_pLock->m_nLock));
		m_bAcquired = 0;
	}
}


