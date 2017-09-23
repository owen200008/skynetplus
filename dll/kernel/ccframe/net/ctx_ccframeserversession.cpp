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


//! 协程里面调用Bussiness消息
int CCoroutineCtx_CCFrameServerSession::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket) {
	switch (nType) {
	case CCoroutineCtx_CCFrameServerSession_Init:
		return DispathBussinessMsg_0_Init(pCorutine, nParam, pParam, pRetPacket);
	case CCoroutineCtx_CCFrameServerSession_SessionRelease:
		return DispathBussinessMsg_1_SessionRelease(pCorutine, nParam, pParam, pRetPacket);
	case CCoroutineCtx_CCFrameServerSession_ReceiveData:
		return DispathBussinessMsg_2_ReceiveData(pCorutine, nParam, pParam, pRetPacket);
	default:
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
		break;
	}
	return -99;
}

long CCoroutineCtx_CCFrameServerSession::DispathBussinessMsg_0_Init(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	return 0;
}
long CCoroutineCtx_CCFrameServerSession::DispathBussinessMsg_1_SessionRelease(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	return 0;
}
long CCoroutineCtx_CCFrameServerSession::DispathBussinessMsg_2_ReceiveData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	return 0;
}