#ifndef INC_PUBLIC_THREADLOCK_H
#define INC_PUBLIC_THREADLOCK_H

#include <stdlib.h>
#include <stdint.h>

struct SpinLock
{
	int m_nLock;
	SpinLock()
	{
		m_nLock = 0;
	}
};

class CSpinLockFunc
{
public:
	CSpinLockFunc(SpinLock& lock);
	virtual ~CSpinLockFunc();

	void Lock();
	void LockAndSleep(unsigned short usSleep = 100);
	void UnLock();
protected:
	SpinLock* 	m_pLock;
	uint8_t		m_bAcquired;
};

#endif
