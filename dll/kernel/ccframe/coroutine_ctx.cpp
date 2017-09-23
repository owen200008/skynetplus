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
uint32_t& CCoroutineCtx::GetTotalCreateCtx()
{ 
	return m_nTotalCtx; 
}

//////////////////////////////////////////////////////////////////////////////
CCoroutineCtx::CCoroutineCtx(const char* pName, const char* pClassName){
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

CCoroutineCtx::~CCoroutineCtx(){
	ASSERT(m_bRelease == true || CCtx_ThreadPool::GetThreadPool()->GetSetClose());
	basiclib::BasicInterlockedDecrement((LONG*)&m_nTotalCtx);
}

//初始化
int CCoroutineCtx::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
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

void CCoroutineCtx::ReleaseCtx(){
	m_bRelease = true;
	if (m_ctxID != 0){
		m_ctxMsgQueue.PushToMQMgr();
		return;
	}
}
//! 强制push到全局队列
void CCoroutineCtx::PushGlobalQueue(){
	if(m_bRelease == false && m_ctxID != 0){
		m_ctxMsgQueue.PushToMQMgr();
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
bool CCoroutineCtx::DispatchMsg(ctx_message& msg, CCorutinePlusThreadData* pCurrentData){
	switch (msg.m_nType){
		case CTXMESSAGE_TYPE_RUNFUNC:
		{
			pRunFuncCtxMessageCallback pFunc = (pRunFuncCtxMessageCallback)msg.m_pFunc;
			pFunc(this, &msg);
		}
		break;
		case CTXMESSAGE_TYPE_RUNCOROUTINE:
		{
			Ctx_ResumeCoroutine((CCorutinePlus*)msg.m_pFunc, this, pCurrentData);

		}
		break;
		case CTXMESSAGE_TYPE_ONTIMER:
		{
			pCallbackOnTimerFunc pFunc = (pCallbackOnTimerFunc)msg.m_pFunc;
			(*pFunc)(this);
		}
        break;
	}
	return true;
}

//! 协程里面调用Bussiness消息
int CCoroutineCtx::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket){
    return DEFINECTX_RET_TYPE_SUCCESS;
}

//获取状态
void CCoroutineCtx::GetState(basiclib::CBasicSmartBuffer& smBuf){
    char szBuf[256];
    sprintf(szBuf, "[id]%08X Name:%s KeyName:%s SessionIndex:%08d\r\n", m_ctxID, m_pClassName ? m_pClassName : "", m_pCtxName ? m_pCtxName : "", m_sessionID_Index);
    smBuf.AppendString(szBuf);
}

void DebugInterfaceCorutine(CCorutinePlus* pCorutine){
	CCoroutineCtx* pCtx = pCorutine->GetParamPoint<CCoroutineCtx>(0);
	uint32_t nType = pCorutine->GetParam<uint32_t>(1);
	MACRO_ExeToCtx(pCorutine, 0, pCtx->GetCtxID(), nType, 0, nullptr, nullptr,
				   return);
}
//! 测试接口
bool CCoroutineCtx::DebugInterface(const char* pDebugInterface, uint32_t nType, lua_State* L) {
	//默认发送给自己
	int argc = lua_gettop(L);
	if (argc == 3) {
		Ctx_CreateCoroutine(0, DebugInterfaceCorutine, this, &nType);
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Ctx_DispathCoroutine(CCorutinePlus* pCorutine, uint32_t nSrcCtxID, uint32_t nDstCtxID){
	ctx_message sendmsg(nSrcCtxID, pCorutine);
	if(CCoroutineCtx::PushMessageByID(nDstCtxID, sendmsg)){
		return true;
	}
	CCFrameSCBasicLogEventErrorV("%s(%s:%d) SendNext NoFind SourceCtx(%x) NextCtx(%x)", __FUNCTION__, __FILE__, __LINE__, nSrcCtxID, nDstCtxID);
	//唤醒返回执行失败
	Ctx_ResumeCoroutine_Fail(pCorutine);
	return false;
}

bool Ctx_ResumeCoroutine(CCorutinePlus* pCorutine, CCoroutineCtx* pCtx, CCorutinePlusThreadData* pCurrentData){
	static WakeUpCorutineType wakeUpType = WakeUpCorutineType_Success;
	//占有当前资源
	pCorutine->SetCurrentCtx(pCtx);
	if(pCorutine->Resume(pCurrentData->GetCorutinePlusPool(), &wakeUpType) == CoroutineState_Suspend){
		//释放当前资源
		pCorutine->SetCurrentCtx(nullptr);
		uint32_t nNextCtxID = pCorutine->GetParam<uint32_t>(0);
		if(nNextCtxID != 0){
			return Ctx_DispathCoroutine(pCorutine, pCtx->GetCtxID(), nNextCtxID);
		}
	}
	return true;
}

bool Ctx_YieldCoroutine(CCorutinePlus* pCorutine, uint32_t nNextCtxID){
	if(pCorutine->IsSameCurrentCtx(nNextCtxID)){
		return true;
	}
	pCorutine->YieldCorutine(&nNextCtxID);
	WakeUpCorutineType wakeUpType = pCorutine->GetParam<WakeUpCorutineType>(0);
	if(wakeUpType != WakeUpCorutineType_Success){
		return false;
	}
	return true;
}

int Ctx_ExeCoroutine(CCorutinePlus* pCorutine, uint32_t nSourceCtxID, uint32_t nDstCtxID, uint32_t nType, int nParam, void** pParam, void* pRetPacket){
	//判断是否是远程ctxid
	if(m_pHandle->IsRemoteServerCtxID(nDstCtxID)){
		MACRO_YieldToCtx(pCorutine, m_pHandle->GetServerCommuCtxID(),
						 return DEFINECTX_RET_TYPE_COROUTINEERR;);
		const int nSetParam = 4;
		void* pSetParam[nSetParam] = { &nDstCtxID, &nType, &nParam, pParam };
		return pCorutine->GetCurrentCtx()->DispathBussinessMsg(pCorutine, DEFINEDISPATCH_CORUTINE_NetClient_Request, nSetParam, pSetParam, pRetPacket);
	}
	else{
		MACRO_YieldToCtx(pCorutine, nDstCtxID,
						 return DEFINECTX_RET_TYPE_COROUTINEERR;);
		return pCorutine->GetCurrentCtx()->DispathBussinessMsg(pCorutine, nType, nParam, pParam, pRetPacket);
	}
	if(nSourceCtxID != 0){
		MACRO_YieldToCtx(pCorutine, nSourceCtxID,
						 return DEFINECTX_RET_TYPE_COROUTINEERR;);
	}
	return DEFINECTX_RET_TYPE_SUCCESS;
}

//! 唤醒协程执行失败
void Ctx_ResumeCoroutine_Fail(CCorutinePlus* pCorutine){
	CCorutinePlusThreadData* pCurrentData = CCtx_ThreadPool::GetOrCreateSelfThreadData();
    WakeUpCorutineType wakeUpType = WakeUpCorutineType_Fail;
	pCorutine->Resume(pCurrentData->GetCorutinePlusPool(), &wakeUpType);
}

void RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func, bool bSelfCtx){
	m_pHandle->RegisterSerializeAndUnSerialize(nDstCtxID, nType, func, bSelfCtx);
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
	pCorutine->YieldCorutine(&nNextCtxID);
	WakeUpCorutineType wakeUpType = pCorutine->GetParam<WakeUpCorutineType>(0);
	if (wakeUpType != WakeUpCorutineType_Success) {
		return false;
	}
	return true;
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
		MACRO_YieldToCtx(pCorutine, 0,
            ASSERT(0);bRet = false;)
    }
	else{
		ASSERT(0);
		bRet = false;
	}
    return bRet;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCorutinePlus::CCorutinePlus(){
	m_pCurrentCtx = nullptr;
}

CCorutinePlus::~CCorutinePlus(){

}

//! 初始化
void CCorutinePlus::ReInit(coroutine_funcframe pFunc){
	CCorutinePlusBase::Create((coroutine_func)pFunc);
	m_pCurrentCtx = nullptr;
}
////////////////////////////////////////////////////////////////////////////////
CCorutinePlusPool::CCorutinePlusPool(){

}
CCorutinePlusPool::~CCorutinePlusPool(){

}
void CCorutinePlusPool::FinishFunc(CCorutinePlusBase* pCorutine){
	CCorutinePlus* pPlus = (CCorutinePlus*)pCorutine;
	if(pPlus->m_vtStoreCtx.size() > 0){
		for(auto& ctx : pPlus->m_vtStoreCtx){
			//需要释放到队列中
			ctx->PushGlobalQueue();
		}
		pPlus->m_vtStoreCtx.clear();
	}
}
////////////////////////////////////////////////////////////////////////////////
CCorutinePlusThreadData::CCorutinePlusThreadData(){

}

CCorutinePlusThreadData::~CCorutinePlusThreadData(){

}