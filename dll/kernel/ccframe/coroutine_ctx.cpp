#include "coroutine_ctx.h"
#include "ctx_handle.h"
#include "ctx_threadpool.h"
#include "net/ctx_netclientsevercommu.h"
extern "C" {
#include "lua.h"
}

//////////////////////////////////////////////////////////////////////////////
extern CCoroutineCtxHandle* m_pHandle;
uint32_t CCoroutineCtx::m_nTotalCtx = 0;
WakeUpCorutineType g_wakeUpTypeSuccess = WakeUpCorutineType_Success;
uint32_t& CCoroutineCtx::GetTotalCreateCtx()
{ 
	return m_nTotalCtx; 
}

//////////////////////////////////////////////////////////////////////////////
CCoroutineCtx::CCoroutineCtx(const char* pName, const char* pClassName)
{
	m_bRelease = false;
	m_bFixCtxID = 0;
    m_bUseForServer = false;
	m_ctxID = 0;
	m_sessionID_Index = 0;
    if (pName)
        m_pCtxName = pName;
    else
        m_pCtxName = nullptr;
    m_pClassName = pClassName;
	m_ctxPacketDealType = 0;
	basiclib::BasicInterlockedIncrement((LONG*)&m_nTotalCtx);
}

CCoroutineCtx::~CCoroutineCtx()
{
	ASSERT(m_bRelease == true || CCtx_ThreadPool::GetThreadPool()->GetSetClose());
	basiclib::BasicInterlockedDecrement((LONG*)&m_nTotalCtx);
}

//初始化
int CCoroutineCtx::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func)
{
	if (m_ctxID != 0)
		return 1;
    //默认回调一次初始化在调用加入handle之前
    func(InitGetParamType_Init, (const char*)this, nullptr);
    if (0 == m_pHandle->Register(this)){
		return 1;
	}
    m_ctxMsgQueue.InitCtxMsgQueue(m_ctxID, pMQMgr);
    m_Ctoken.InitQueue(m_ctxMsgQueue);
	return 0;
}

void CCoroutineCtx::ReleaseCtx()
{
	m_bRelease = true;
	if (m_ctxID != 0){
		m_ctxMsgQueue.PushToMQMgr();
		return;
	}
}

//分配一个新的sessionid
int32_t CCoroutineCtx::GetNewSessionID()
{
	return basiclib::BasicInterlockedIncrement((LONG*)&m_sessionID_Index);
}

bool CCoroutineCtx::PushMessageByID(uint32_t nDstCtxID, ctx_message& msg)
{
    CRefCoroutineCtx pCtx = m_pHandle->GetContextByHandleID(nDstCtxID);
	if (pCtx == nullptr)
		return false;
	pCtx->PushMessage(msg);
    return true;
}
//! 删除某个ctx
void CCoroutineCtx::ReleaseCtxByID(uint32_t nDstCtxID){
    CRefCoroutineCtx pCtx = m_pHandle->GetContextByHandleID(nDstCtxID);
    if (pCtx == nullptr)
        return;
    pCtx->ReleaseCtx();
}
void CCoroutineCtx::PushMessage(ctx_message& msg)
{
	m_ctxMsgQueue.MQPush(msg);
}

//! 分配任务
void CCoroutineCtx::DispatchMsg(ctx_message& msg)
{
	switch (msg.m_nType)
	{
		case CTXMESSAGE_TYPE_RUNFUNC:
		{
			pRunFuncCtxMessageCallback pFunc = (pRunFuncCtxMessageCallback)msg.m_pFunc;
			pFunc(this, &msg);
		}
		break;
		case CTXMESSAGE_TYPE_RUNCOROUTINE:
		{
            WaitResumeCoroutineCtx((CCorutinePlus*)msg.m_pFunc, this, &msg);
		}
		break;
		case CTXMESSAGE_TYPE_ONTIMER:
		{
			pCallbackOnTimerFunc pFunc = (pCallbackOnTimerFunc)msg.m_pFunc;
			(*pFunc)(this);
		}
        break;
	}
}

//! 协程里面调用Bussiness消息
int CCoroutineCtx::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    return 0;
}

//获取状态
void CCoroutineCtx::GetState(basiclib::CBasicSmartBuffer& smBuf){
    char szBuf[256];
    sprintf(szBuf, "[id]%08X Name:%s KeyName:%s SessionIndex:%08d\r\n", m_ctxID, m_pClassName ? m_pClassName : "", m_pCtxName ? m_pCtxName : "", m_sessionID_Index);
    smBuf.AppendString(szBuf);
}

//! 测试接口
bool CCoroutineCtx::DebugInterface(const char* pDebugInterface, uint32_t nType, lua_State* L) {
	//默认发送给自己
	int argc = lua_gettop(L);
	if (argc == 3) {
		const int nSetParam = 2;
		void* pSetParam[nSetParam] = { this, &nType };
		CreateResumeCoroutineCtx([](CCorutinePlus* pCorutine)->void {
			int nGetParamCount = 0;
			void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
			CCoroutineCtx* pCtx = (CCoroutineCtx*)pGetParam[0];
			uint32_t nType = *(uint32_t*)pGetParam[1];
			int nRetValue = 0;
			IsErrorHapper(WaitExeCoroutineToCtxBussiness(pCorutine, pCtx->GetCtxID(), pCtx->GetCtxID(), nType, 0, nullptr, nullptr, nRetValue),
				CCFrameSCBasicLogEventErrorV("%s(%s:%d) WaitExeCoroutineToCtxBussiness error", __FUNCTION__, __FILE__, __LINE__));
		}, 0, 0, nullptr);
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<class... _Types>
bool TemplateResumeCoroutineCtx(CCorutinePlus* pCorutine, CCorutinePlusThreadData* pCurrentData, uint32_t nSourceCtxID, _Types&&... _Args){
    if (pCorutine->Resume(pCurrentData->GetCorutinePlusPool(), g_wakeUpTypeSuccess, std::forward<_Types>(_Args)...) == CoroutineState_Suspend){
        uint32_t nNextCtxID = pCorutine->GetYieldParam<uint32_t>(0);
        if (nNextCtxID != 0){
            ctx_message sendmsg(nSourceCtxID, pCorutine);
            if (CCoroutineCtx::PushMessageByID(nNextCtxID, sendmsg)){
                return true;
            }
            CCFrameSCBasicLogEventErrorV("%s(%s:%d) SendNext NoFind SourceCtx(%x) NextCtx(%x)", __FUNCTION__, __FILE__, __LINE__, nSourceCtxID, nNextCtxID);
            //唤醒返回执行失败
			WaitResumeCoroutineCtxFail(pCorutine);
			return false;
        }
        else{
            return true;
        }
        return false;
    }
    return true;
}

bool CreateResumeCoroutineCtx(coroutine_func func, uint32_t nSourceCtxID, int nParam, void* pParam){
	CCorutinePlusThreadData* pCurrentData = CCtx_ThreadPool::GetOrCreateSelfThreadData();
    CCorutinePlus* pCorutine = pCurrentData->GetCorutinePlusPool()->GetCorutine();
    pCorutine->ReInit(func);
    return TemplateResumeCoroutineCtx(pCorutine, pCurrentData, nSourceCtxID, nParam, pParam);
}

bool WaitResumeCoroutineCtx(CCorutinePlus* pCorutine, CCoroutineCtx* pCtx, ctx_message* pMsg){
    return TemplateResumeCoroutineCtx(pCorutine, CCtx_ThreadPool::GetOrCreateSelfThreadData(), pCtx->GetCtxID(), pCtx, pMsg);
}

//! 唤醒协程执行失败
void WaitResumeCoroutineCtxFail(CCorutinePlus* pCorutine){
	CCorutinePlusThreadData* pCurrentData = CCtx_ThreadPool::GetOrCreateSelfThreadData();
    WakeUpCorutineType wakeUpType = WakeUpCorutineType_Fail;
	pCorutine->Resume(pCurrentData->GetCorutinePlusPool(), wakeUpType);
}

void RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func, bool bSelfCtx){
	m_pHandle->RegisterSerializeAndUnSerialize(nDstCtxID, nType, func, bSelfCtx);
}


bool WaitExeCoroutineToCtxBussiness(CCorutinePlus* pCorutine, uint32_t nSourceCtxID, uint32_t nDstCtxID, uint32_t nType, int nParam, void** pParam, void* pRetPacket, int& nRetValue){
    CCoroutineCtx* pCurrentCtx = nullptr;
    ctx_message* pCurrentMsg = nullptr;
    //判断是否是远程ctxid
    if (m_pHandle->IsRemoteServerCtxID(nDstCtxID)){
        if (!YieldCorutineToCtx(pCorutine, m_pHandle->GetServerCommuCtxID(), pCurrentCtx, pCurrentMsg))
            return false;
        const int nSetParam = 4;
        void* pSetParam[nSetParam] = { &nDstCtxID, &nType, &nParam, pParam };
        nRetValue = pCurrentCtx->DispathBussinessMsg(pCorutine, DEFINEDISPATCH_CORUTINE_NetClient_Request, nSetParam, pSetParam, pRetPacket, pCurrentMsg);
    }
    else{
		if (!YieldCorutineToCtx(pCorutine, nDstCtxID, pCurrentCtx, pCurrentMsg)) {
			return false;
		}
        nRetValue = pCurrentCtx->DispathBussinessMsg(pCorutine, nType, nParam, pParam, pRetPacket, pCurrentMsg);
    }
    if (nSourceCtxID != 0){
        if (!YieldCorutineToCtx(pCorutine, nSourceCtxID, pCurrentCtx, pCurrentMsg))
            return false;
    }
    return true;
}

bool CCoroutineCtx::YieldAndResumeSelf(CCorutinePlus* pCorutine) {
#ifdef _DEBUG
	if (pCorutine->GetRunPool() == nullptr) {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) pCorutine->GetRunPool() != nullptr", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
	CCorutinePlusPool* pCurrentThreadPool = CCtx_ThreadPool::GetOrCreateSelfThreadData()->GetCorutinePlusPool();

	if (pCorutine->GetRunPool() != pCurrentThreadPool) {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) pCorutine->GetRunPool() != pCurrentThreadPool", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
#endif
	uint32_t nNextCtxID = 0;
	pCorutine->YieldCorutine(nNextCtxID);
	WakeUpCorutineType wakeUpType = pCorutine->GetResumeParam<WakeUpCorutineType>(0);
	if (wakeUpType != WakeUpCorutineType_Success) {
		return false;
	}
	return true;
}

//yield协程
bool YieldCorutineToCtx(CCorutinePlus* pCorutine, uint32_t nNextCtxID, CCoroutineCtx*& pCtx, ctx_message*& pMsg){
#ifdef _DEBUG
	CCorutinePlusThreadData* pCheckCurrentData = CCtx_ThreadPool::GetOrCreateSelfThreadData();
	if (pCorutine->GetRunPool() != pCheckCurrentData->GetCorutinePlusPool()) {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) pCorutine->GetRunPool() != pCheckCurrentData->GetCorutinePlusPool()", __FUNCTION__, __FILE__, __LINE__);
		return false;
	}
#endif
    pCorutine->YieldCorutine(nNextCtxID);
    WakeUpCorutineType wakeUpType = pCorutine->GetResumeParam<WakeUpCorutineType>(0);
    if (wakeUpType != WakeUpCorutineType_Success){
        return false;
    }
    pCtx = pCorutine->GetResumeParam<CCoroutineCtx*>(1);
    pMsg = pCorutine->GetResumeParam<ctx_message*>(2);
    return true;
}

//! 获取参数个数和参数,只能在resume之后调用
void** GetCoroutineCtxParamInfo(CCorutinePlus* pCorutine, int& nCount){
    nCount = pCorutine->GetResumeParam<int>(1);
    return pCorutine->GetResumeParamPoint<void**>(2)[0];
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! 加入timer
void CCoroutineCtxOnTimer(Net_PtrInt nKey, Net_PtrInt pParam1){
    uint32_t nSourceCtxID = pParam1;
    ctx_message msg(nSourceCtxID, nKey);
    if (!CCoroutineCtx::PushMessageByID(nSourceCtxID, msg)){
        //失败删除定时器
        CCtx_ThreadPool::GetThreadPool()->GetOnTimerModule().DelTimer(nKey);
    }
}

bool CoroutineCtxAddOnTimer(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback){
    return CCtx_ThreadPool::GetThreadPool()->GetOnTimerModule().AddOnTimer((Net_PtrInt)pCallback, CCoroutineCtxOnTimer, nTimes, nCtxID);
}
bool CoroutineCtxAddOnTimeOut(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback){
    return CCtx_ThreadPool::GetThreadPool()->GetOnTimerModule().AddTimeOut((Net_PtrInt)pCallback, CCoroutineCtxOnTimer, nTimes, nCtxID);
}
void CoroutineCtxDelTimer(pCallbackOnTimerFunc pCallback){
    return CCtx_ThreadPool::GetThreadPool()->GetOnTimerModule().DelTimer((Net_PtrInt)pCallback);
}

//! 加入timer
void CCoroutineCtxOnTimerWakeup(Net_PtrInt nKey, Net_PtrInt pParam1){
    uint32_t nSourceCtxID = pParam1;
    CCorutinePlus* pCorutine = (CCorutinePlus*)nKey;
    ctx_message msg(nSourceCtxID, pCorutine);
    if (!CCoroutineCtx::PushMessageByID(nSourceCtxID, msg)){
        ASSERT(0);
        return;
    }
}
//! 定时唤醒协程
bool OnTimerToWakeUpCorutine(uint32_t nCtxID, int nTimes, CCorutinePlus* pCorutine){
    bool bRet = CCtx_ThreadPool::GetThreadPool()->GetOnTimerModule().AddTimeOut((Net_PtrInt)pCorutine, CCoroutineCtxOnTimerWakeup, nTimes, nCtxID);
    if (bRet){
        CCoroutineCtx* pCurrentCtx = nullptr;
        ctx_message* pCurrentMsg = nullptr;
        if (!YieldCorutineToCtx(pCorutine, 0, pCurrentCtx, pCurrentMsg)){
            ASSERT(0);
            bRet = false;
        }
    }
	else{
		ASSERT(0);
		bRet = false;
	}
    return bRet;
}
