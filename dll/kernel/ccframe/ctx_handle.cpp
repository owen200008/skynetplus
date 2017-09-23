#include "ctx_handle.h"
#include "log/ctx_log.h"
#include "coroutinedata/coroutinestackdata.h"
#include "net/ctx_netclientsevercommu.h"
#include "ctx_threadpool.h"

CCoroutineCtxHandle* m_pHandle = nullptr;
CCoroutineCtxHandle* CCoroutineCtxHandle::GetInstance()
{
    return m_pHandle;
}

CCoroutineCtxHandle::CCoroutineCtxHandle()
{
	m_pHandle = this;
    m_pClient = nullptr;
    m_handle_index = HANDLE_ID_BEGIN;
    m_releaseHandleID.reserve(0xffff);
}

CCoroutineCtxHandle::~CCoroutineCtxHandle(){
    //在自己析构前必须清空
	m_mapNameToCtx.clear();
	m_mapIDToCtx.clear();
}

//! 初始化函数
void CCoroutineCtxHandle::InitHandle(){
    CCtx_ThreadPool* pPool = CCtx_ThreadPool::GetThreadPool();
    uint32_t nIndex = pPool->CreateTemplateObjectCtx("CCoroutineCtx_NetClientServerCommu", nullptr, pPool->m_defaultFuncGetConfig);
    if (nIndex != 0){
        GrabContext(nIndex, [&](CCoroutineCtx* pCtx)->void{
            m_pClient = (CCoroutineCtx_NetClientServerCommu*)pCtx;
        });
    }
}

uint32_t CCoroutineCtxHandle::Register(CCoroutineCtx* pCtx){
	const char* pCtxName = pCtx->GetCtxName();
    bool bRegisterGlobal = false;
    uint32_t uHandleID = 0;
    {
        basiclib::CRWLockFunc lock(&m_lock, true, false);
        if (pCtxName){
            if (m_mapNameToCtx.find(pCtxName) != m_mapNameToCtx.end()){
                return 0;
            }
            m_mapNameToCtx[pCtxName] = pCtx;
            if (pCtx->m_bFixCtxID != 0){
                uHandleID = pCtx->m_bFixCtxID;
                if (m_mapIDToCtx.find(uHandleID) != m_mapIDToCtx.end()){
                    CCFrameSCBasicLogEventErrorV("%s(%s:%d) exist globalctx %d", __FUNCTION__, __FILE__, __LINE__, pCtx->m_bFixCtxID);
                }
                if (pCtx->m_bUseForServer == false){
                    bRegisterGlobal = true;
                }
            }
            else{
                uHandleID = GetNewHandleID();
            }
        }
        else{
            uHandleID = GetNewHandleID();
        }
        m_mapIDToCtx[uHandleID] = pCtx;
        pCtx->m_ctxID = uHandleID;
    }
    if (bRegisterGlobal){
        if (m_pClient){
			Ctx_CreateCoroutine(0, [](CCorutinePlus* pCorutine){
				const char* pCtxName = pCorutine->GetParamPoint<const char>(0);
                uint32_t uCtxID = pCorutine->GetParam<uint32_t>(1);
				CCoroutineCtx_NetClientServerCommu* pClient = pCorutine->GetParamPoint<CCoroutineCtx_NetClientServerCommu>(2);
				MACRO_ExeToCtxParam2(pCorutine, 0, pClient->GetCtxID(), DEFINEDISPATCH_CORUTINE_NetClient_CreateMap, (void*)pCtxName, &uCtxID, nullptr,
									 return);
            }, (void*)pCtxName, &uHandleID, m_pClient);
        }
    }
	return uHandleID;
}

int CCoroutineCtxHandle::UnRegister(CCoroutineCtx* pCtx)
{
	uint32_t uHandleID = pCtx->GetCtxID();
	return UnRegister(uHandleID);
}

int CCoroutineCtxHandle::UnRegister(uint32_t uHandleID)
{
    bool bDeleteGlobal = false;
	int ret = 0;
	basiclib::CRWLockFunc lock(&m_lock, true, false);
	MapHandleIDToCtx::iterator iter = m_mapIDToCtx.find(uHandleID);
	if (iter != m_mapIDToCtx.end())
	{
		CRefCoroutineCtx ctx = iter->second;
		const char* pCtxName = ctx->GetCtxName();
        if (pCtxName){
            m_mapNameToCtx.erase(pCtxName);
            if (ctx->m_bFixCtxID != 0 && ctx->m_bUseForServer == false){
                bDeleteGlobal = true;
            }
        }
		m_mapIDToCtx.erase(iter);
		
		m_releaseHandleID.push_back(uHandleID);
		lock.UnLockWrite();
        if (bDeleteGlobal){
            if (m_pClient){
				Ctx_CreateCoroutine(0, [](CCorutinePlus* pCorutine){
					const char* pCtxName = pCorutine->GetParamPoint<const char>(0);
					CCoroutineCtx_NetClientServerCommu* pClient = pCorutine->GetParamPoint<CCoroutineCtx_NetClientServerCommu>(1);
					MACRO_ExeToCtxParam1(pCorutine, 0, pClient->GetCtxID(), DEFINEDISPATCH_CORUTINE_NetClient_DeleteMap, (void*)pCtxName, nullptr, 
										 ASSERT(0));
                }, (void*)pCtxName, m_pClient);
            }
        }
		ret = 1;
	}
	return ret;
}

//! 根据名字查找ctxid
uint32_t CCoroutineCtxHandle::GetCtxIDByName(const char* pName, CCorutinePlus* pCorutine, uint32_t nResCtxID)
{
    {
        basiclib::CRWLockFunc lock(&m_lock, false, true);
        MapNameToCtx::iterator iter = m_mapNameToCtx.find(pName);
        if (iter != m_mapNameToCtx.end())
            return iter->second->GetCtxID();
    }

    //去全局查找
    if (m_pClient && pCorutine){
		SkynetPlusGetCtxIDByNameResponse response;
		MACRO_ExeToCtxParam1(pCorutine, nResCtxID, m_pClient->GetCtxID(), DEFINEDISPATCH_CORUTINE_NetClient_GetMap, (void*)pName, &response,
							 return 0);
        if (response.m_nError == 0)
            return response.m_nCtxID;
    }
    return 0;
}

bool CCoroutineCtxHandle::GrabContext(uint32_t handle, const std::function<void(CCoroutineCtx*)>& func)
{
	CRefCoroutineCtx pRet = GetContextByHandleID(handle);
	if (pRet != nullptr)
	{
		func(pRet.GetResFunc());
	}
	return pRet != nullptr;
}
CRefCoroutineCtx CCoroutineCtxHandle::GetContextByHandleID(uint32_t handle)
{
	basiclib::CRWLockFunc lock(&m_lock, false, true);
	MapHandleIDToCtx::iterator iter = m_mapIDToCtx.find(handle);
	if (iter != m_mapIDToCtx.end())
	{
		return iter->second;
	}
	return nullptr;
}

uint32_t CCoroutineCtxHandle::GetNewHandleID()
{
	int nSize = m_releaseHandleID.size();
	if (nSize > 0)
	{
		uint32_t uRetHandleID = m_releaseHandleID.back();
		m_releaseHandleID.pop_back();
		return uRetHandleID;
	}
	uint32_t uRetHandleID = ++m_handle_index;
	if (uRetHandleID > HANDLE_ID_ALLOCATE_LOG)
	{
        //100 log
        CCFrameSCBasicLogEventErrorV("HandleID Allocate warning %x, need restart!!", uRetHandleID);
	}
	return uRetHandleID;
}
//! 判断是否是remote server
bool CCoroutineCtxHandle::IsRemoteServerCtxID(uint32_t nCtxID){
    if (m_pClient){
        Net_UChar cIndex = (nCtxID >> 24);
        if (cIndex == 0)
            return false;
        return cIndex != m_pClient->GetSelfServerID();
    }
    return false;
}

uint32_t CCoroutineCtxHandle::GetServerCommuCtxID(){
    if (m_pClient)
        return m_pClient->GetCtxID();
    return 0;
}
//! 注册序列化反序列化
void CCoroutineCtxHandle::RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func, bool bSelf){
    if (m_pClient){
        m_pClient->RegisterSerializeAndUnSerialize(nDstCtxID, nType, func, bSelf);
    }
}

