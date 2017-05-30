#include "ctx_threadpool.h"
#include <basic.h>
#include "dllmodule.h"
#include "log/ctx_log.h"
#include "state/ctx_threadstate.h"
#include "log/ctx_log.h"
#include "net/ctx_redis.h"
#include "net/ctx_mysql.h"
#include "net/ctx_netclientsevercommu.h"

CCtxMessageQueue* DispathCtxMsg(CMQMgr* pMgrQueue, moodycamel::ConsumerToken& token, CCtxMessageQueue* pQ, uint32_t& usPacketNumber, CCtx_CorutinePlusThreadData* pThreadData)
{
    if (pQ == NULL) {
        pQ = pMgrQueue->GlobalMQPop(token);
        if (pQ == NULL)
            return NULL;
    }
    uint32_t nCtxID = pQ->GetCtxID();
    CRefCoroutineCtx pCtx = CCoroutineCtxHandle::GetInstance()->GetContextByHandleID(nCtxID);
    if (pCtx == nullptr){
        //发生异常
        CCFrameSCBasicLogEventError(pThreadData, "分发队列(DispathCtxMsg)找不到ctxid!!!");
        return pMgrQueue->GlobalMQPop(token);
    }
    if (pCtx->IsReleaseCtx()){
        //清空队列
        moodycamel::ConsumerToken token(*pQ);
        ctx_message msg;
        while (pQ->MQPop(token, msg)){
            //如果是协程需要归还
            if (msg.m_nType == CTXMESSAGE_TYPE_RUNCOROUTINE){
                pThreadData->GetCorutinePlusPool()->ReleaseCorutine((CCorutinePlus*)msg.m_pFunc);
            }
            msg.FormatMsgQueueToString([&](const char* pData, int nLength)->void{
                CCFrameSCBasicLogEvent(pThreadData, pData);
            });
        }
        CCoroutineCtxHandle::GetInstance()->UnRegister(pCtx->GetCtxID());
        return pMgrQueue->GlobalMQPop(token);
    }

    for (uint32_t i = 0; i < usPacketNumber; i++){
        ctx_message msg;
        if (!pQ->MQPop(msg)){
            return pMgrQueue->GlobalMQPop(token);
        }
        pCtx->DispatchMsg(msg, pThreadData);
        msg.ReleaseData();
    }
    CCtxMessageQueue* pNextQ = pMgrQueue->GlobalMQPop(token);
    if (pNextQ) {
        pMgrQueue->GlobalMQPush(pQ);
        pQ = pNextQ;
    }
    return pQ;
}
////////////////////////////////////////////////////////////////////////////////////////////
CCtx_CorutinePlusThreadData::CCtx_CorutinePlusThreadData(CCtx_ThreadPool* pThreadPool) : CCorutinePlusThreadData(&pThreadPool->m_threadIndexTLS, [&](CCorutinePlusThreadData* pData)->void*{
    pData->GetCorutinePlusPool()->InitCorutine(atol(pThreadPool->GetCtxInitString(InitGetParamType_Config, "workthreadcorutinecount", "1024")));
    return pThreadPool->m_pCreateFunc(pData);
}, [&](void* pParam)->void{
    pThreadPool->m_pReleaseFunc(pParam);
})
{
	m_pThreadPool = pThreadPool;
	m_globalCToken.InitQueue(pThreadPool->m_globalMQMgrModule);
}

CCtx_CorutinePlusThreadData::~CCtx_CorutinePlusThreadData()
{
}

void CCtx_CorutinePlusThreadData::ExecThreadOnWork()
{
    //创建状态上下文
    if (CCtx_ThreadPool::GetThreadPool()->CreateTemplateObjectCtx(CCtx_ThreadPool::GetThreadPool()->GetCtxInitString(InitGetParamType_Config, "statetemplate", "CCtx_ThreadState"), nullptr, m_pThreadPool->m_defaultFuncGetConfig, &m_threadMgrQueue) == 0){
        CCFrameSCBasicLogEventErrorV(nullptr, "获取启动的StateTemplate失败%s", CCtx_ThreadPool::GetThreadPool()->GetCtxInitString(InitGetParamType_Config, "statetemplate", "CCtx_ThreadState"));
        return;
    }

	CMQMgr* pMgrQueue = &m_pThreadPool->m_globalMQMgrModule;
	CMQMgr* pThreadMQ = &m_threadMgrQueue;
	uint32_t& usPacketNumber = m_pThreadPool->m_nDPacketNumPerTime;
	bool &bRunning = m_pThreadPool->m_bRunning;

	CCtxMessageQueue* pQ = nullptr;
	CCtxMessageQueue* pThreadQ = nullptr;

	CCFrameSCBasicLogEventV(this, "启动ExecThreadOnWork %d ThreadID(%d)", basiclib::BasicGetCurrentThreadId(), m_dwThreadID);
	while (bRunning){
        pQ = DispathCtxMsg(pMgrQueue, m_globalCToken, pQ, usPacketNumber, this);
		if (pQ == nullptr) {
            pThreadQ = DispathCtxMsg(pThreadMQ, m_threadCToken, pThreadQ, usPacketNumber, this);
			if (pThreadQ == nullptr)
				pMgrQueue->WaitForGlobalMQ(100);
		}
	}
	CCFrameSCBasicLogEventV(this, "退出ExecThreadOnWork %d ThreadID(%d)", basiclib::BasicGetCurrentThreadId(), m_dwThreadID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CCtx_ThreadPool* m_pCtxThreadPool;
CCtx_ThreadPool* CCtx_ThreadPool::GetThreadPool(){
	return m_pCtxThreadPool;
}
void CCtx_ThreadPool::CreateThreadPool(CCtx_ThreadPool* pPool){
	m_pCtxThreadPool = pPool;
}
CCorutinePlusThreadData* CCtx_ThreadPool::GetOrCreateSelfThreadData(){
    CCorutinePlusThreadData* pRet = (CCorutinePlusThreadData*)m_pCtxThreadPool->m_threadIndexTLS.GetValue();
    if (pRet)
        return pRet;
    return new CCorutinePlusThreadData(&m_pCtxThreadPool->m_threadIndexTLS, [&](CCorutinePlusThreadData* pData)->void*{
        return m_pCtxThreadPool->m_pCreateFunc(pData);
    }, [&](void* pParam)->void{
    });
}
//////////////////////////////////////////////////////////////////////////////////////////////
CCtx_ThreadPool::CCtx_ThreadPool()
{
	m_nDPacketNumPerTime = 1;
	m_bRunning = false;
	m_pLog = nullptr;
    m_defaultFuncGetConfig = [&](InitGetParamType type, const char* pKey, const char* pDefault)->const char*{
        return GetCtxInitString(type, pKey, pDefault);
    };
    m_mgtDllCtx.Register(CCoroutineCtx_Log::CreateTemplate, ReleaseTemplate);
    m_mgtDllCtx.Register(CCtx_ThreadState::CreateTemplate, ReleaseTemplate);
    m_mgtDllCtx.Register(CCoroutineCtx_CRedis::CreateTemplate, ReleaseTemplate);
    m_mgtDllCtx.Register(CCoroutineCtx_CRedisWatch::CreateTemplate, ReleaseTemplate);
    m_mgtDllCtx.Register(CCoroutineCtx_Mysql::CreateTemplate, ReleaseTemplate);
    m_mgtDllCtx.Register(CCoroutineCtx_NetClientServerCommu::CreateTemplate, ReleaseTemplate);
}

CCtx_ThreadPool::~CCtx_ThreadPool()
{
}

uint32_t CCtx_ThreadPool::CreateTemplateObjectCtx(const char* pName, const char* pKeyName, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func, CMQMgr* pMQMgr)
{
    CCoroutineCtxTemplate* pMainTemplate = m_mgtDllCtx.GetCtxTemplate(pName);
    if (pMainTemplate == nullptr){
        return 0;
    }

    CRefCoroutineCtx pCtx = pMainTemplate->GetCreate()(pKeyName);
    if (pCtx->InitCtx(pMQMgr ? pMQMgr : &m_globalMQMgrModule, func) != 0){
        pCtx->ReleaseCtx();
        return 0;
    }
    return pCtx->GetCtxID();
}
void CCtx_ThreadPool::ReleaseObjectCtxByCtxID(uint32_t nCtxID)
{
    CRefCoroutineCtx pCtx = CCoroutineCtxHandle::GetInstance()->GetContextByHandleID(nCtxID);
    if (pCtx != nullptr){
        pCtx->ReleaseCtx();
    }
}

bool CCtx_ThreadPool::Init(const std::function<void*(CCorutinePlusThreadData*)>& pCreateFunc,
	const std::function<void(void*)>& pReleaseParamFunc)
{
	int nWorkThreadCount = atol(GetCtxInitString(InitGetParamType_Config, "workthread", "4"));
	m_nDPacketNumPerTime = atol(GetCtxInitString(InitGetParamType_Config, "threadworkpacketnumber", "1"));

    //timer先初始化，log用到
    m_ontimerModule.InitTimer();
    //初始化handle
    CCoroutineCtxHandle::GetInstance()->InitHandle();
	//default log
    if (CreateTemplateObjectCtx(GetCtxInitString(InitGetParamType_Config, "logtemplate", "CCoroutineCtx_Log"), nullptr, m_defaultFuncGetConfig) == 0){
        return false;
    }
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//log创建即可
	m_bRunning = true;

	m_pCreateFunc = pCreateFunc;
	m_pReleaseFunc = pReleaseParamFunc;
	//创建indextls
	m_threadIndexTLS.CreateTLS();

	DWORD dwThreadID = 0;
	for (int i = 0; i < nWorkThreadCount; i++){
        m_vtHandle.push_back(basiclib::BasicCreateThread(ThreadOnWork, nullptr, &dwThreadID));
	}
	m_hMonitor = basiclib::BasicCreateThread(ThreadOnMonitor, this, &dwThreadID);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	basiclib::SetNetInitializeGetParamFunc([](const char* pParam)->basiclib::CBasicString{
		return CCtx_ThreadPool::GetThreadPool()->GetCtxInitString(InitGetParamType_Config, pParam, "4");
	});

	//main module
    if (CreateTemplateObjectCtx(GetCtxInitString(InitGetParamType_Config, "maintemplate", ""), nullptr, m_defaultFuncGetConfig) == 0){
        CCFrameSCBasicLogEventErrorV(nullptr, "获取启动的MainTemplate失败%s", GetCtxInitString(InitGetParamType_Config, "maintemplate", ""));
        return false;
    }
    return true;
}

//! 等待退出
void CCtx_ThreadPool::Wait()
{
	basiclib::BasicWaitThread(m_hMonitor);
	for (auto& workHandle : m_vtHandle){
		basiclib::BasicWaitThread(workHandle);
	}
	m_ontimerModule.CloseTimer();
	m_ontimerModule.WaitThreadExit();
}

THREAD_RETURN ThreadOnWork(void* pArgv)
{
    //随机因子
    srand((unsigned int)(time(NULL) + basiclib::BasicGetTickTime() + basiclib::BasicGetCurrentThreadId()));

    CCtx_CorutinePlusThreadData* pThreadData = new CCtx_CorutinePlusThreadData(m_pCtxThreadPool);
	pThreadData->ExecThreadOnWork();
	return 0;
}


THREAD_RETURN ThreadOnMonitor(void* pArgv)
{
	//随机因子
    srand((unsigned int)(time(NULL) + basiclib::BasicGetTickTime() + basiclib::BasicGetCurrentThreadId()));
	CCtx_ThreadPool* pPool = (CCtx_ThreadPool*)pArgv;
	pPool->ExecThreadOnMonitor();
	return 0;
}
void CCtx_ThreadPool::ExecThreadOnMonitor()
{
	uint32_t& nTotalCtx = CCoroutineCtx::GetTotalCreateCtx();
    CCFrameSCBasicLogEventV(nullptr, "启动ExecThreadOnMonitor %d", basiclib::BasicGetCurrentThreadId());
	while (nTotalCtx != 0){
		basiclib::BasicSleep(100);
	}
	m_bRunning = false;
    CCFrameSCBasicLogEventV(nullptr, "退出ExecThreadOnMonitor %d", basiclib::BasicGetCurrentThreadId());
}

//! 注册测试用接口
void CCtx_ThreadPool::RegisterServerForDebug(const char* pName, CCFrameServer_Frame* pRegister){
    basiclib::CSpinLockFunc lock(&m_Debuglock, TRUE);
    m_mapServerFrame[pName] = pRegister;
}
CCFrameServer_Frame* CCtx_ThreadPool::GetRegisterServer(const char* pName){
    basiclib::CSpinLockFunc lock(&m_Debuglock, TRUE);
    auto& iter = m_mapServerFrame.find(pName);
    if (iter != m_mapServerFrame.end())
        return iter->second;
    return nullptr;
}

