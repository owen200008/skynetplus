#include "coroutineplus.h"
#include <assert.h>
#include <string.h>

void* s_malloc(size_t size)
{
	return malloc(size);
}

void s_free(void* ptr)
{
	free(ptr);
}

template<class T>
T* NewObject()
{
	void* p = s_malloc(sizeof(T));
	::new (p)T;
	return (T*)p;
}

template<class T>
void DeleteObject(T* obj)
{
	obj->~T();
	s_free(obj);
}
///////////////////////////////////////////////////////////////////////////
CCorutinePlus::CCorutinePlus()
{
	m_state = CoroutineState_Ready;
	m_nCap = 0;
	m_nSize = 0;
	m_stack = nullptr;
	m_pRunPool = nullptr;
	memset(m_pResumeParam, 0, sizeof(void*) * RESUME_MAXPARAM);
}

CCorutinePlus::~CCorutinePlus()
{
	if(m_stack)
	{
		s_free(m_stack);
		m_nSize = 0;
		m_nCap = 0;
		m_stack = nullptr;
	}
}

void CCorutinePlus::CheckStackSize(int nDefaultStackSize)
{
	if(nDefaultStackSize > m_nCap)
	{
		if(m_stack)
		{
			s_free(m_stack);
		}
		m_nCap = nDefaultStackSize;
		m_stack = (char*)s_malloc(m_nCap);
	}
}

void CCorutinePlus::ReInit(const std::function<void(CCorutinePlus*)>& func)
{
	m_func = func;
	m_state = CoroutineState_Ready;
}

void CCorutinePlus::SaveStack(char* pTop)
{
	char dummy = 0;
	int nLength = pTop - &dummy;
	CheckStackSize(nLength);
	m_nSize = nLength;
	memcpy(m_stack, &dummy, m_nSize);
}

void CCorutinePlus::Yield()
{
	SaveStack(m_pRunPool->m_stack + STACK_SIZE);
	m_state = CoroutineState_Suspend;
	swapcontext(&m_ctx, &m_pRunPool->m_ctxMain);
}

static void MakeContextFunc(uint32_t low32, uint32_t hi32)
{
	uintptr_t ptr = (uintptr_t)low32 | ((uintptr_t)hi32 << 32);
	CCorutinePlus* pThis = (CCorutinePlus*)ptr;
	pThis->StartFunc();
}

void CCorutinePlus::StartFunc()
{
	m_func(this);
	m_state = CoroutineState_Dead;
}

void CCorutinePlus::Resume(CCorutinePlusPool* pPool)
{
	m_pRunPool = pPool;
	switch(m_state)
	{
	case CoroutineState_Ready:
		{
			getcontext(&m_ctx);
			m_ctx.uc_stack.ss_sp = pPool->m_stack;
			m_ctx.uc_stack.ss_size = STACK_SIZE;
			m_ctx.uc_link = &pPool->m_ctxMain;
			m_state = CoroutineState_Running;
			uintptr_t ptr = (uintptr_t)this;
			makecontext(&m_ctx, (void (*)(void))MakeContextFunc, 2, (uint32_t)ptr, (uint32_t)(ptr>>32));
			swapcontext(&pPool->m_ctxMain, &m_ctx);
			if(m_state == CoroutineState_Dead)
			{
				pPool->ReleaseCorutine(this);
			}
		}
		break;
	case CoroutineState_Suspend:
		{
			m_ctx.uc_stack.ss_sp = pPool->m_stack;
			m_ctx.uc_stack.ss_size = STACK_SIZE;
			m_ctx.uc_link = &pPool->m_ctxMain;

			m_state = CoroutineState_Running;
			swapcontext(&pPool->m_ctxMain, &m_ctx);
			if(m_state == CoroutineState_Dead)
			{
				 pPool->ReleaseCorutine(this);
			}
		}
		break;
	case CoroutineState_Dead:
		{
			//do nothing
		}
		break;
	default:
		assert(0);
		break;
	}
	m_pRunPool = nullptr;
}

///////////////////////////////////////////////////////////////////////////
CCorutinePlusPool::CCorutinePlusPool()
{
}

CCorutinePlusPool::~CCorutinePlusPool()
{
}

bool CCorutinePlusPool::InitCorutine(int nDefaultSize, int nDefaultStackSize)
{
	if(nDefaultSize <= 0 || nDefaultStackSize < 1024)
	{
		return false;
	}
	m_nDefaultStackSize = nDefaultStackSize;
	m_vtCorutinePlus.reserve(nDefaultSize * 2);
	for(int i = 0;i < nDefaultSize;i++)
	{
		CreateCorutine(true);
	}
	return true;
}

CCorutinePlus* CCorutinePlusPool::GetCorutine()
{
	int nSize = m_vtCorutinePlus.size();
	if(nSize == 0)
	{
		return CreateCorutine(false);
	}
	CCorutinePlus* pRet = m_vtCorutinePlus[nSize - 1];
	m_vtCorutinePlus.pop_back();
	return pRet;
}

void CCorutinePlusPool::ReleaseCorutine(CCorutinePlus* pPTR)
{
	m_vtCorutinePlus.push_back(pPTR);
}

CCorutinePlus* CCorutinePlusPool::CreateCorutine(bool bPush)
{
	CCorutinePlus* pRet = NewObject<CCorutinePlus>();	
	pRet->CheckStackSize(m_nDefaultStackSize);
	m_usCreateTimes++;
	if(bPush)
		m_vtCorutinePlus.push_back(pRet);
	return pRet;
}
