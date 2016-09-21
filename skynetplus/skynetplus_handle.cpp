#include "skynetplus_handle.h"
#include "skynetplus_context.h"

CSkynetContextHandle::CSkynetContextHandle()
{
	rwlock_init(&m_lock);
	m_handle_index = 0;
}

CSkynetContextHandle::~CSkynetContextHandle()
{
}

uint32_t CSkynetContextHandle::GetNewHandleID()
{
	int nSize = m_releaseHandleID.size();
	if(nSize > 0)
	{
		uint32_t uRetHandleID = m_releaseHandleID.back();
		m_releaseHandleID.pop_back();
		return uRetHandleID;
	}
	uint32_t uRetHandleID = ++m_handle_index;
	if(uRetHandleID > HANDLE_ID_ALLOCATE_LOG)
	{
		if(uRetHandleID % 10000 == 0)
		{
			//100 log
			basiclib::BasicLogEventErrorV("HandleID Allocate warning %d, need restart!!", uRetHandleID);	
		}
	}

	return uRetHandleID;
}

void CSkynetContextHandle::Init()
{
	m_handle_index = HANDLE_ID_BEGIN;
	m_releaseHandleID.reserve(0xff);
}

uint32_t CSkynetContextHandle::Register(CSkynetContext* ctx)
{
	CRWLockFunc lock(&m_lock, true, false);

	uint32_t uHandleID = GetNewHandleID();
	m_mapNameToCtx[ctx->GetContextName()] = ctx;
	m_mapIDToCtx[uHandleID] = ctx;

	ctx->SetContextHandleID(uHandleID);
	return uHandleID;
}

int CSkynetContextHandle::UnRegister(CSkynetContext* ctx)
{
	uint32_t uHandleID = ctx->GetContextHandleID();
	return UnRegister(uHandleID);
}

int CSkynetContextHandle::UnRegister(uint32_t uHandleID)
{
	int ret = 0;
	CRWLockFunc lock(&m_lock, true, false);
	MapHandleIDToCtx::iterator iter = m_mapIDToCtx.find(uHandleID);
	if(iter != m_mapIDToCtx.end())
	{
		CSkynetContext* ctx = iter->second;
		basiclib::CBasicString& strName = ctx->GetContextName();

		m_mapIDToCtx.erase(iter);
		m_mapNameToCtx.erase(strName);

		m_releaseHandleID.push_back(uHandleID);
		lock.UnLockWrite();
		ret = 1;
	}
	return ret;
}

bool CSkynetContextHandle::GrabContext(uint32_t handle, const std::function<void(CSkynetContext*)>& func)
{
	CSkynetContext* pRet = nullptr;
	{
		CRWLockFunc lock(&m_lock, false, true);
		MapHandleIDToCtx::iterator iter = m_mapIDToCtx.find(handle);
		if(iter != m_mapIDToCtx.end())
		{
			pRet = iter->second;
			pRet->AddRef();
		}
	}
	if(pRet != nullptr)
	{
		func(pRet);
		pRet->DelRef();
	}
	return pRet != nullptr;
}

uint32_t CSkynetContextHandle::GetHandleIDByName(const char* pName)
{
	uint32_t uRet = 0;
	GrabContextByName(pName, [&](CSkynetContext* pRet)->void{
		if(pRet)
			uRet = pRet->GetContextHandleID();
	});
	return uRet;
}
bool CSkynetContextHandle::GrabContextByName(const char* pName, const std::function<void(CSkynetContext*)>& func)
{
	CSkynetContext* pRet = nullptr;
	{
		CRWLockFunc lock(&m_lock, false, true);
		MapNameToCtx::iterator iter = m_mapNameToCtx.find(pName);
		if(iter != m_mapNameToCtx.end())
		{
			pRet = iter->second;
			pRet->AddRef();
		}
	}
	if(pRet != nullptr)
	{
		func(pRet);
		pRet->DelRef();
	}
	return pRet != nullptr;
}




