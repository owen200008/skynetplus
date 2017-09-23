#include "interationcenter.h"
#include "../skynetplusprotocal/skynetplusprotocal_function.h"
#include "../skynetplusprotocal/skynetplusprotocal_error.h"
#include "../kernel/ccframe/log/ctx_log.h"
#include "interationcenterserversessionctx.h"
#include "../kernel/ccframe/net/ccframedefaultfilter.h"

CreateTemplateSrc(CInterationCenterCtx)
static CInterationCenterCtx* g_pCenterCtx = nullptr;
CInterationCenterCtx* CInterationCenterCtx::GetInterationCenterCtx(){
    return g_pCenterCtx;
}
CInterationCenterCtx::CInterationCenterCtx(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName){
}

CInterationCenterCtx::~CInterationCenterCtx(){
}


int CInterationCenterCtx::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    g_pCenterCtx = this;
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
	IsErrorHapper(nRet == 0, return nRet);

    if (!m_server.InitServer("server", func)){
		CCFrameSCBasicLogEventError("CInterationCenterCtx 启动服务失败");
		return 2;
	}
	CCFrameServer* pServer = m_server.GetServer();
    CCCFrameDefaultFilter filter;
    m_server.StartServer(&filter);
	return 0;
}

//! 启动的时候注册
void CInterationCenterCtx::RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func){
    m_mapRegister[(nDstCtxID | 0xFF000000)][nType] = func;
}

bool CCFrameGetInterationCenterCtxStoreInfoByCIndex(uint32_t nResCtxID, CCorutinePlus* pCorutine, Net_UChar& cIndex, InterationCenterCtxStoreInfo& replyPacket){
	MACRO_ExeToCtxParam1(pCorutine, nResCtxID, g_pCenterCtx->GetServer().GetGateCtxID(), CCoroutineCtx_GateInteration_GetByID, &cIndex, &replyPacket,
        ASSERT(0);return false;)
    return true;
}


