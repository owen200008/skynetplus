#include "ctx_threadstate.h"
#include "../log/ctx_log.h"
#include "../ctx_threadpool.h"

CreateTemplateSrc(CCtx_ThreadState)

CCtx_ThreadState::CCtx_ThreadState(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName)
{
}

CCtx_ThreadState::~CCtx_ThreadState()
{

}

int CCtx_ThreadState::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func)
{
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
    if (nRet != 0)
        return nRet;
    //60s
    CoroutineCtxAddOnTimer(m_ctxID, 6000, OnTimerShowThreadState);
    return 0;
}

//! 不需要自己delete，只要调用release
void CCtx_ThreadState::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimerShowThreadState);
    CCoroutineCtx::ReleaseCtx();
}

//业务类, 全部使用静态函数
void CCtx_ThreadState::OnTimerShowThreadState(CCoroutineCtx* pCtx, CCtx_CorutinePlusThreadData* pData)
{
    CCFrameSCBasicLogEventV(pData, "ThreadID(%08d): CorutinePool(%d/%d Stack(%d/%d))",
        pData->GetThreadID(),
        pData->GetCorutinePlusPool()->GetVTCorutineSize(), pData->GetCorutinePlusPool()->GetCreateCorutineTimes(),
        pData->GetCorutinePlusPool()->GetVTShareStackCount(), pData->GetCorutinePlusPool()->GetCreateTimesShareStackCount());
}
