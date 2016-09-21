#include "skynetplus_monitor.h"

CMonitor::CMonitor()
{
	m_nThreadCount = 0;
}

CMonitor::~CMonitor()
{
	if(m_pMonitor)
	{
		basiclib::BasicDeallocate(m_pMonitor);
		m_pMonitor = nullptr;
	}
}

void CMonitor::InitMonitor(int nThreadCount)
{
	m_nThreadCount = nThreadCount;
	m_pMonitor = (skynet_monitor*)basiclib::BasicAllocate(nThreadCount * sizeof(skynet_monitor));
	memset(m_pMonitor, 0, nThreadCount * sizeof(skynet_monitor));
}

void CMonitor::Check()
{
	for(int i = 0;i < m_nThreadCount;i++)
	{
		skynet_monitor* pCheck = &(m_pMonitor[i]);
		if(pCheck->m_nVersion == pCheck->m_nCheckVersion)
		{
			if(pCheck->m_nDestination)
			{
				basiclib::BasicLogEventErrorV("A message from [ :%08x ] to [ :%08x ] maybein an endless loop (version = %d)", pCheck->m_nSource, pCheck->m_nDestination, pCheck->m_nVersion);
			}
		}
		else
		{
			pCheck->m_nCheckVersion = pCheck->m_nVersion;
		}
	}
}
