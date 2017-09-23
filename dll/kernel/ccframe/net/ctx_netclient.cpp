#include "ctx_netclient.h"
#include "../log/ctx_log.h"
#include "../ctx_threadpool.h"
#include "../coroutinedata/coroutinestackdata.h"
#include "ccframedefaultfilter.h"

using namespace basiclib;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CCoroutineCtx_NetClient)
CCoroutineCtx_NetClient::CCoroutineCtx_NetClient(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName){
    m_pAddress = nullptr;
    m_pClient = nullptr;
}

CCoroutineCtx_NetClient::~CCoroutineCtx_NetClient(){
    if (m_pClient){
        m_pClient->SafeDelete();
    }
}

void CCoroutineCtx_NetClient::CheckConnect(){
    m_pClient->DoConnect();
}
void CCoroutineCtx_NetClient::ErrorClose(){
    m_pClient->Close(0);
}
//! 判断是否已经认证成功
bool CCoroutineCtx_NetClient::IsTransmit(){
    return m_pClient->IsTransmit();
}
//! 发送数据
void CCoroutineCtx_NetClient::SendData(const char* pData, int nLength){
    m_pClient->Send((void*)pData, nLength);
}

int CCoroutineCtx_NetClient::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
    IsErrorHapper(nRet == 0, return nRet);
    m_pAddress = func(InitGetParamType_Config, "address", nullptr);
    if (m_pAddress == nullptr || strlen(m_pAddress) == 0){
        return 2;
    }

    m_pClient = CreateCommonClientSession(func);
    CCCFrameDefaultFilter filter;
    m_pClient->RegistePreSend(&filter);
    m_pClient->bind_connect(MakeFastFunction(this, &CCoroutineCtx_NetClient::OnConnect));
    m_pClient->bind_idle(MakeFastFunction(this, &CCoroutineCtx_NetClient::OnIdle));
    m_pClient->bind_rece(MakeFastFunction(this, &CCoroutineCtx_NetClient::OnReceive));
    m_pClient->bind_disconnect(MakeFastFunction(this, &CCoroutineCtx_NetClient::OnDisconnect));
    m_pClient->Connect(m_pAddress);
    //1s检查一次
    CoroutineCtxAddOnTimer(m_ctxID, 100, OnTimer);
    return 0;
}
CCommonClientSession* CCoroutineCtx_NetClient::CreateCommonClientSession(const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    return CCommonClientSession::CreateNetClient();
}

//! 不需要自己delete，只要调用release
void CCoroutineCtx_NetClient::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimer);
    CCoroutineCtx::ReleaseCtx();
}

int32_t CCoroutineCtx_NetClient::OnConnect(CBasicSessionNetNotify* pClient, uint32_t nCode){
    return BASIC_NET_OK;
}

int32_t CCoroutineCtx_NetClient::OnIdle(basiclib::CBasicSessionNetNotify* pSession, uint32_t nIdle){
    return BASIC_NET_OK;
}

//! 创建协程
void CCoroutineCtx_NetClient::Corutine_OnReceiveData(CCorutinePlus* pCorutine){
    {
        do{
            CCorutineStackDataDefault stackData;
			CCoroutineCtx_NetClient* pCtx = pCorutine->GetParamPoint<CCoroutineCtx_NetClient>(0);
            stackData.CopyData(pCorutine->GetParamPoint<const char>(1), pCorutine->GetParam<Net_Int>(2));//拷贝到堆数据
			MACRO_ExeToCtxParam1(pCorutine, 0, pCtx->GetCtxID(), DEFINEDISPATCH_CORUTINE_NetClient_Receive, &stackData, nullptr,
								 return);
        } while (false);
    }
}

int32_t CCoroutineCtx_NetClient::OnReceive(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    if (cbData > 0){
		Ctx_CreateCoroutine(0, CCoroutineCtx_NetClient::Corutine_OnReceiveData, this, (void*)pszData, &cbData);
    }
    return BASIC_NET_OK;
}

int32_t CCoroutineCtx_NetClient::OnDisconnect(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode){
    return BASIC_NET_OK;
}

void CCoroutineCtx_NetClient::OnTimer(CCoroutineCtx* pCtx){
    CCoroutineCtx_NetClient* pRedisCtx = (CCoroutineCtx_NetClient*)pCtx;
    pRedisCtx->OnTimerNetClient();
}
void CCoroutineCtx_NetClient::OnTimerNetClient(){
    CheckConnect();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! 协程里面调用Bussiness消息
int CCoroutineCtx_NetClient::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket){
    switch (nType){
    case DEFINEDISPATCH_CORUTINE_NetClient_Receive:
        return DispathBussinessMsg_Receive(pCorutine, nParam, pParam, pRetPacket);
    default:
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
        break;
    }
    return -99;
}



