#include "ctx_ccframeserversession.h"

CreateTemplateSrc(CCoroutineCtx_CCFrameServerSession)
CCoroutineCtx_CCFrameServerSession::CCoroutineCtx_CCFrameServerSession(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName){
    m_pSession = nullptr;
}

CCoroutineCtx_CCFrameServerSession::~CCoroutineCtx_CCFrameServerSession(){

}

int CCoroutineCtx_CCFrameServerSession::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
    IsErrorHapper(nRet == 0, return nRet);
    const char* pRetObject = func(InitGetParamType_CreateUser, "object", nullptr);
    if (pRetObject == nullptr){
        return 2;
    }
    m_pSession = (CCFrameServerSession*)pRetObject;
    return 0;
}
