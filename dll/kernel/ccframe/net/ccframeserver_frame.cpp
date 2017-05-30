#include "ccframeserver_frame.h"
#include "../log/ctx_log.h"

CCFrameServer_Frame::CCFrameServer_Frame(){
    m_pServerSessionCtx = nullptr;
    m_pListenAddress = nullptr;
    m_pIPTrust = nullptr;
}

CCFrameServer_Frame::~CCFrameServer_Frame(){
    if (m_pServer){
        m_pServer->Release();
    }
}

bool CCFrameServer_Frame::InitServer(const char* pKey, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    char szBuf[64] = { 0 };
    sprintf(szBuf, "%s_serversessionctx", pKey);
    m_pServerSessionCtx = func(InitGetParamType_Config, szBuf, "CCoroutineCtx_CCFrameServerSession");
    IsErrorHapper(strlen(m_pServerSessionCtx) != 0, CCFrameSCBasicLogEventErrorV(nullptr, "ServerListen createserversessionctx %s 0", szBuf); return false);
    m_pServer = CreateServer(pKey, func);
    if (m_pServer == nullptr)
        return false;
    return true;
}

CCFrameServer* CCFrameServer_Frame::CreateServer(const char* pKey, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    char szBuf[64] = { 0 };
    sprintf(szBuf, "%s_Address", pKey);
    m_pListenAddress = func(InitGetParamType_Config, szBuf, "");
    if (strlen(m_pListenAddress) == 0){
        return nullptr;
    }
    sprintf(szBuf, "%s_serverlistentimeout", pKey);
    int nTimeout = atol(func(InitGetParamType_Config, szBuf, "45"));
	sprintf(szBuf, "%s_serverlisteniptrust", pKey);
    m_pIPTrust = func(InitGetParamType_Config, szBuf, "*");

    CCFrameServer* pRet = CCFrameServer::CreateCCFrameServer();
    pRet->SetClientRecTimeout(nTimeout);
    pRet->SetIpTrust(m_pIPTrust);
    return pRet;
}

//! 开始服务
void CCFrameServer_Frame::StartServer(basiclib::CBasicPreSend* pPreSend){
    int32_t nRetListen = m_pServer->StartServer(m_pListenAddress, pPreSend);
    CCFrameSCBasicLogEventV(nullptr, "ServerListen %s(%d)", m_pListenAddress, nRetListen);
}

//! 认证通过开启serversession
bool CCFrameServer_Frame::VerifySuccessCreateServerSession(basiclib::CBasicSessionNetClient* pClient){
    CCFrameServerSession* pSession = (CCFrameServerSession*)pClient;
    if (pSession->CreateCtx(m_pServerSessionCtx, [&](InitGetParamType, const char* pKey, const char* pDefault)->const char*{
        return nullptr;
    })){
        return true;
    }
    CCFrameSCBasicLogEvent(nullptr, "VerifySuccessCallback but createctx fail");
    return false;
}
