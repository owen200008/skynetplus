#include "ctx_threadpool.h"
#include <basic.h>
#include "dllmodule.h"
#include "log/ctx_log.h"
#include "state/ctx_threadstate.h"
#include "log/ctx_log.h"
#include "net/ctx_redis.h"
#include "net/ctx_mysql.h"
#include "net/ctx_netclientsevercommu.h"
#include "net/ctx_http.h"
#include "net/ctx_ccframeserversession.h"
#include "paiming/ctx_tiaozhanpaiming.h"
#include "paiming/ctx_tiaozhanpaiming_record.h"

DWORD g_TiaoShiFlag = 0;

CCtxMessageQueue* DispathCtxMsg(CMQMgr* pMgrQueue, moodycamel::ConsumerToken& token, CCtxMessageQueue* pQ, uint32_t& usPacketNumber, CCorutinePlusThreadData* pCurrentData){
    if (pQ == NULL) {
        pQ = pMgrQueue->GlobalMQPop(token);
        if (pQ == NULL)
            return NULL;
    }
    uint32_t nCtxID = pQ->GetCtxID();
    CRefCoroutineCtx pCtx = CCoroutineCtxHandle::GetInstance()->GetContextByHandleID(nCtxID);
    if (pCtx == nullptr){
        //发生异常
        CCFrameSCBasicLogEventError("分发队列(DispathCtxMsg)找不到ctxid!!!");
        return pMgrQueue->GlobalMQPop(token);
    }
    if (pCtx->IsReleaseCtx()){
        //清空队列
        moodycamel::ConsumerToken tokenpQ(*pQ);
        ctx_message msg;
        while (pQ->MQPop(tokenpQ, msg)){
            //如果是协程需要归还
            if (msg.m_nType == CTXMESSAGE_TYPE_RUNCOROUTINE){
				pCurrentData->GetCorutinePlusPool()->ReleaseCorutine((CCorutinePlus*)msg.m_pFunc);
            }
            msg.FormatMsgQueueToString([&](const char* pData, int nLength)->void{
                CCFrameSCBasicLogEvent(pData);
            });
        }
        CCoroutineCtxHandle::GetInstance()->UnRegister(pCtx->GetCtxID());
        return pMgrQueue->GlobalMQPop(token);
    }
	bool bNoStateCtx = pCtx->IsNoStateCtx();
    for (uint32_t i = 0; bNoStateCtx || i < usPacketNumber; i++){
        ctx_message msg;
        if (!pQ->MQPop(msg)){
            return pMgrQueue->GlobalMQPop(token);
        }
        pCtx->DispatchMsg(msg, pCurrentData);
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
void ExecThreadOnWork(CCtx_ThreadPool* pThreadPool)
{
	CCorutinePlusThreadData* pCurrentData = CCtx_ThreadPool::GetOrCreateSelfThreadData();
	//线程内队列
	CMQMgr threadMgrQueue;
	//全局消息消费者token
	moodycamel::ConsumerToken globalCToken;
	globalCToken.InitQueue(pThreadPool->m_globalMQMgrModule);
	moodycamel::ConsumerToken threadCToken;
	threadCToken.InitQueue(threadMgrQueue);

    //创建状态上下文
    if (CCtx_ThreadPool::GetThreadPool()->CreateTemplateObjectCtx(pThreadPool->m_defaultFuncGetConfig(InitGetParamType_Config, "statetemplate", "CCtx_ThreadState"), nullptr, pThreadPool->m_defaultFuncGetConfig, &threadMgrQueue) == 0){
        CCFrameSCBasicLogEventErrorV("获取启动的StateTemplate失败%s", CCtx_ThreadPool::GetThreadPool()->m_defaultFuncGetConfig(InitGetParamType_Config, "statetemplate", "CCtx_ThreadState"));
        return;
    }

	CMQMgr* pMgrQueue = &pThreadPool->m_globalMQMgrModule;
	CMQMgr* pThreadMQ = &threadMgrQueue;
	uint32_t& usPacketNumber = pThreadPool->m_nDPacketNumPerTime;
	bool &bRunning = pThreadPool->m_bRunning;

	CCtxMessageQueue* pQ = nullptr;
	CCtxMessageQueue* pThreadQ = nullptr;

	CCFrameSCBasicLogEventV("启动ExecThreadOnWork %d ThreadID(%d)", basiclib::BasicGetCurrentThreadId(), pCurrentData->GetThreadID());
	while (bRunning){
        pQ = DispathCtxMsg(pMgrQueue, globalCToken, pQ, usPacketNumber, pCurrentData);
		if (pQ == nullptr) {
            pThreadQ = DispathCtxMsg(pThreadMQ, threadCToken, pThreadQ, usPacketNumber, pCurrentData);
			if (pThreadQ == nullptr)
				pMgrQueue->WaitForGlobalMQ();
		}
	}
	CCFrameSCBasicLogEventV("退出ExecThreadOnWork %d ThreadID(%d)", basiclib::BasicGetCurrentThreadId(), pCurrentData->GetThreadID());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
static CCtx_ThreadPool* g_pCtxThreadPool = nullptr;

CCtx_ThreadPool* CCtx_ThreadPool::GetThreadPoolOrCreate(){
	static basiclib::CMutex lockPool;
	if(g_pCtxThreadPool)
		return g_pCtxThreadPool;
	basiclib::CSingleLock lock(&lockPool, TRUE);
	if(g_pCtxThreadPool)
		return g_pCtxThreadPool;
	g_pCtxThreadPool = new CCtx_ThreadPool();
	atexit([](){
		if(g_pCtxThreadPool){
			g_pCtxThreadPool->SetClose();
			g_pCtxThreadPool->Wait();
			delete g_pCtxThreadPool;
		}
		g_pCtxThreadPool = nullptr;
	});
	return g_pCtxThreadPool;
}

CCtx_ThreadPool* CCtx_ThreadPool::GetThreadPool(){
	return g_pCtxThreadPool;
}
CCorutinePlusThreadData* CCtx_ThreadPool::GetOrCreateSelfThreadData(){
	void* pRet = GetThreadPool()->m_tls.GetValue();
	if(pRet){
		return (CCorutinePlusThreadData*)pRet;
	}
	CCorutinePlusThreadData* pRetData = new CCorutinePlusThreadData();
	g_pCtxThreadPool->m_tls.SetValue(pRetData);
	pRetData->Init();
	return pRetData;
}
//////////////////////////////////////////////////////////////////////////////////////////////
CCtx_ThreadPool::CCtx_ThreadPool(){
	m_nDefaultHttpCtxID = 0;
	m_nDPacketNumPerTime = 1;
	m_bRunning = false;
	m_pLog = nullptr;
	m_bSetClose = false;
	m_tls.CreateTLS();
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

bool CCtx_ThreadPool::Init(){
	//加载rsa
	const char* pRSAPath = m_defaultFuncGetConfig(InitGetParamType_Config, "RSALoadPath", nullptr);
	if (pRSAPath) {
		m_rsaMgr.InitFromPath((basiclib::BasicGetModulePath() + pRSAPath).c_str());
	}

	int nWorkThreadCount = atol(m_defaultFuncGetConfig(InitGetParamType_Config, "workthread", "4"));
	m_nDPacketNumPerTime = atol(m_defaultFuncGetConfig(InitGetParamType_Config, "threadworkpacketnumber", "1"));

    //timer先初始化，log用到
    m_ontimerModule.InitTimer();
    //初始化handle
    CCoroutineCtxHandle::GetInstance()->InitHandle();
	//default log
    if (CreateTemplateObjectCtx(m_defaultFuncGetConfig(InitGetParamType_Config, "logtemplate", "CCoroutineCtx_Log"), nullptr, m_defaultFuncGetConfig) == 0){
        return false;
    }
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//log创建即可
	m_bRunning = true;

	DWORD dwThreadID = 0;
	for (int i = 0; i < nWorkThreadCount; i++){
        m_vtHandle.push_back(basiclib::BasicCreateThread(ThreadOnWork, nullptr, &dwThreadID));
	}
	m_hMonitor = basiclib::BasicCreateThread(ThreadOnMonitor, this, &dwThreadID);
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	basiclib::SetNetInitializeGetParamFunc([](const char* pParam)->basiclib::CBasicString{
		return CCtx_ThreadPool::GetThreadPool()->m_defaultFuncGetConfig(InitGetParamType_Config, pParam, "4");
	});

	if (atol(m_defaultFuncGetConfig(InitGetParamType_Config, "StartDefaultHttp", "0"))) {
		m_nDefaultHttpCtxID = CCtx_ThreadPool::GetThreadPool()->CreateTemplateObjectCtx("CCoroutineCtx_Http", nullptr, m_defaultFuncGetConfig);
	}

	//main module
    if (CreateTemplateObjectCtx(m_defaultFuncGetConfig(InitGetParamType_Config, "maintemplate", ""), nullptr, m_defaultFuncGetConfig) == 0){
        CCFrameSCBasicLogEventErrorV("获取启动的MainTemplate失败%s", m_defaultFuncGetConfig(InitGetParamType_Config, "maintemplate", ""));
        return false;
    }
    return true;
}

//! 等待退出
void CCtx_ThreadPool::Wait(){
	basiclib::BasicWaitThread(m_hMonitor);
	for (auto& workHandle : m_vtHandle){
		basiclib::BasicWaitThread(workHandle);
	}
	m_ontimerModule.CloseTimer();
	m_ontimerModule.WaitThreadExit();
}

THREAD_RETURN ThreadOnWork(void* pArgv){
    //随机因子
    srand((unsigned int)(time(NULL) + basiclib::BasicGetTickTime() + basiclib::BasicGetCurrentThreadId()));
	ExecThreadOnWork(g_pCtxThreadPool);
	return 0;
}


THREAD_RETURN ThreadOnMonitor(void* pArgv){
	//随机因子
    srand((unsigned int)(time(NULL) + basiclib::BasicGetTickTime() + basiclib::BasicGetCurrentThreadId()));
	CCtx_ThreadPool* pPool = (CCtx_ThreadPool*)pArgv;
	pPool->ExecThreadOnMonitor();
	return 0;
}
void CCtx_ThreadPool::ExecThreadOnMonitor(){
	uint32_t& nTotalCtx = CCoroutineCtx::GetTotalCreateCtx();
    CCFrameSCBasicLogEventV("启动ExecThreadOnMonitor %d", basiclib::BasicGetCurrentThreadId());
	while (nTotalCtx != 0 && !m_bSetClose){
		basiclib::BasicSleep(1000);
	}
	//保证日志处理完成
	ctx_message msg(0, [](CCoroutineCtx* pCtx, ctx_message*)->void {
		CCoroutineCtx_Log* pLog = (CCoroutineCtx_Log*)pCtx;
		pLog->SetCanExit();
	});
	CCoroutineCtx::PushMessageByID(CCoroutineCtx_Log::GetLogCtxID(), msg);
	while (CCoroutineCtx_Log::IsCanExit() == false) {
		basiclib::BasicSleep(100);
	}
	m_bRunning = false;
    CCFrameSCBasicLogEventV("退出ExecThreadOnMonitor %d", basiclib::BasicGetCurrentThreadId());
}

//! 主动设置退出
void CCtx_ThreadPool::SetClose() {
	m_bSetClose = true;
}

