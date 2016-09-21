#ifndef SKYNETPLUA_MONITOR_H
#define SKYNETPLUA_MONITOR_H

#include <basic.h>

struct skynet_monitor
{
	int m_nVersion;
	int m_nCheckVersion;
	uint32_t m_nSource;
	uint32_t m_nDestination;
	void Trigger(uint32_t source, uint32_t destination)
	{
		m_nSource = source;
		m_nDestination = destination;
		basiclib::BasicInterlockedIncrement((LONG*)&m_nVersion);
	}
	
};

class CMonitor : public basiclib::CBasicObject
{
public:
	CMonitor();
	virtual ~CMonitor();

	void InitMonitor(int nThreadCount);
	void Check();
public:
	int				m_nThreadCount;
	skynet_monitor* m_pMonitor;
};

#endif
