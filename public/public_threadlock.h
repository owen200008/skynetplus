#ifndef INC_THREADLOCK_H
#define INC_THREADLOCK_H

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
	void UnLock();
protected:
	SpinLock* 	m_pLock;
	uint8_t		m_bAcquired;
};

#endif
