#include "ctx_handle.h"
#include "log/ctx_log.h"
#include "coroutinedata/coroutinestackdata.h"
#include "net/ctx_netclientsevercommu.h"
#include "ctx_threadpool.h"

static CCoroutineCtxHandle m_gHandle;
CCoroutineCtxHandle* CCoroutineCtxHandle::GetInstance()
{
    return &m_gHandle;
}

CCoroutineCtxHandle::CCoroutineCtxHandle()
{
    m_pClient = nullptr;
    m_handle_index = HANDLE_ID_BEGIN;
    m_releaseHandleID.reserve(0xffff);
}

CCoroutineCtxHandle::~CCoroutineCtxHandle()
{
    //m_pClient应该析构，不过也不知道可以在什么时候析构
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
                    CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) exist globalctx %d", __FUNCTION__, __FILE__, __LINE__, pCtx->m_bFixCtxID);
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
            const int nSetParam = 3;
            void* pSetParam[nSetParam] = { (void*)pCtxName, &uHandleID, m_pClient };
            CreateResumeCoroutineCtx([](CCorutinePlus* pCorutine)->void{
                //外部变量一定要谨慎，线程安全问题
                CCoroutineCtx* pCurrentCtx = nullptr;
                CCorutinePlusThreadData* pCurrentData = nullptr;
                ctx_message* pCurrentMsg = nullptr;
                CCorutineStackDataDefault stackData;
                {
                    int nGetParamCount = 0;
                    void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
                    IsErrorHapper(nGetParamCount == 3, ASSERT(0); CCFrameSCBasicLogEventErrorV(pCurrentData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nGetParamCount); return);
                    const char* pCtxName = (const char*)pGetParam[0];
                    uint32_t uCtxID = *(uint32_t*)pGetParam[1];
                    CCoroutineCtx_NetClientServerCommu* pClient = (CCoroutineCtx_NetClientServerCommu*)pGetParam[2];

                    const int nSetParam = 2;
                    void* pSetParam[nSetParam] = { pGetParam[0], &uCtxID };
                    int nRetValue = 0;
                    if (!WaitExeCoroutineToCtxBussiness(pCorutine, 0, pClient->GetCtxID(), DEFINEDISPATCH_CORUTINE_NetClient_CreateMap, nSetParam, pSetParam, nullptr, nRetValue)){
                        ASSERT(0);
                        CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) WaitExeCoroutineToCtxBussiness FAIL ret(%d)", __FUNCTION__, __FILE__, __LINE__, nRetValue);
                    }
                }
            }, CCtx_ThreadPool::GetOrCreateSelfThreadData(), 0, nSetParam, pSetParam);
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
                const int nSetParam = 2;
                void* pSetParam[nSetParam] = { (void*)pCtxName, m_pClient };
                CreateResumeCoroutineCtx([](CCorutinePlus* pCorutine)->void{
                    //外部变量一定要谨慎，线程安全问题
                    CCoroutineCtx* pCurrentCtx = nullptr;
                    CCorutinePlusThreadData* pCurrentData = nullptr;
                    ctx_message* pCurrentMsg = nullptr;
                    CCorutineStackDataDefault stackData;
                    {
                        int nGetParamCount = 0;
                        void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
                        IsErrorHapper(nGetParamCount == 2, ASSERT(0); CCFrameSCBasicLogEventErrorV(pCurrentData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nGetParamCount); return);
                        const char* pCtxName = (const char*)pGetParam[0];
                        CCoroutineCtx_NetClientServerCommu* pClient = (CCoroutineCtx_NetClientServerCommu*)pGetParam[2];

                        const int nSetParam = 2;
                        void* pSetParam[nSetParam] = { pGetParam[1], &stackData };
                        int nRetValue = 0;
                        if (!WaitExeCoroutineToCtxBussiness(pCorutine, 0, pClient->GetCtxID(), DEFINEDISPATCH_CORUTINE_NetClient_DeleteMap, nSetParam, pSetParam, nullptr, nRetValue)){
                            ASSERT(0);
                            CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) WaitExeCoroutineToCtxBussiness FAIL ret(%d)", __FUNCTION__, __FILE__, __LINE__, nRetValue);
                        }
                    }
                }, CCtx_ThreadPool::GetOrCreateSelfThreadData(), 0, nSetParam, pSetParam);
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
    if (m_pClient){
        const int nSetParam = 1;
        void* pSetParam[nSetParam] = { (void*)pName };
        int nRetValue = 0;
        SkynetPlusGetCtxIDByNameResponse response;
        if (!WaitExeCoroutineToCtxBussiness(pCorutine, nResCtxID, m_pClient->GetCtxID(), DEFINEDISPATCH_CORUTINE_NetClient_GetMap, nSetParam, pSetParam, &response, nRetValue)){
            ASSERT(0);
            CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) WaitExeCoroutineToCtxBussiness FAIL ret(%d)", __FUNCTION__, __FILE__, __LINE__, nRetValue);
        }
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
        CCFrameSCBasicLogEventErrorV(nullptr, "HandleID Allocate warning %x, need restart!!", uRetHandleID);
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

