#include "ccframeserver_frame.h"
#include "../log/ctx_log.h"
#include "../ctx_handle.h"
#include "ctx_gate.h"

CCFrameServer_Frame::CCFrameServer_Frame(){
    m_pListenAddress = nullptr;
    m_pIPTrust = nullptr;
	m_nCtxIDGate = 0;
}

CCFrameServer_Frame::~CCFrameServer_Frame(){
	if (m_nCtxIDGate != 0) {
		CCtx_ThreadPool::GetThreadPool()->ReleaseObjectCtxByCtxID(m_nCtxIDGate);
	}
    if (m_pServer){
        m_pServer->SafeDelete();
    }
}

bool CCFrameServer_Frame::InitServer(const char* pKey, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    m_pServer = CreateInitServer(pKey, func);
    if (m_pServer == nullptr)
        return false;
    return true;
}

CCFrameServer* CCFrameServer_Frame::CreateInitServer(const char* pKey, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
	char szBuf[64] = { 0 };
	//先启动网关
	sprintf(szBuf, "%s_gate", pKey);
	const char* pGate = func(InitGetParamType_Config, szBuf, nullptr);
	if (nullptr == pGate)
		return nullptr;
	//创建ctx
	m_nCtxIDGate = CCtx_ThreadPool::GetThreadPool()->CreateTemplateObjectCtx(pGate, szBuf, [&](InitGetParamType type, const char* pKey, const char* pDefault)->const char* {
		return func(type, pKey, pDefault);
	});
	if (m_nCtxIDGate == 0) {
		ASSERT(0);
		return nullptr;
	}

    sprintf(szBuf, "%s_Address", pKey);
    m_pListenAddress = func(InitGetParamType_Config, szBuf, "");
    if (strlen(m_pListenAddress) == 0){
        return nullptr;
    }
    sprintf(szBuf, "%s_serverlistentimeout", pKey);
    int nTimeout = atol(func(InitGetParamType_Config, szBuf, "45"));
	sprintf(szBuf, "%s_serverlisteniptrust", pKey);
    m_pIPTrust = func(InitGetParamType_Config, szBuf, "*");

	CCFrameServer* pRet = CreateServer();
    pRet->SetClientRecTimeout((uint16_t)nTimeout);
    pRet->SetIpTrust(m_pIPTrust);

	CCoroutineCtxHandle::GetInstance()->GrabContext(m_nCtxIDGate, [&](CCoroutineCtx* pCtx) {
		CCoroutineCtx_CCFrameServerGate* pGate = (CCoroutineCtx_CCFrameServerGate*)pCtx;
		//获取网关，绑定收到数据处理
		pRet->bind_rece(MakeFastFunction(pGate, &CCoroutineCtx_CCFrameServerGate::OnReceive));
		pRet->bind_disconnect(MakeFastFunction(pGate, &CCoroutineCtx_CCFrameServerGate::OnDisconnect));
	});
    return pRet;
}
CCFrameServer* CCFrameServer_Frame::CreateServer() {
	return CCFrameServer::CreateNetServer();
}

//! 开始服务
void CCFrameServer_Frame::StartServer(basiclib::CBasicPreSend* pPreSend){
    int32_t nRetListen = m_pServer->StartServer(m_pListenAddress, pPreSend);
    CCFrameSCBasicLogEventV("ServerListen %s(%d)", m_pListenAddress, nRetListen);
}

