#include "ctx_netclientsevercommu.h"
#include "../log/ctx_log.h"
#include "../ctx_threadpool.h"
#include "../coroutinedata/coroutinestackdata.h"
#include "ccframedefaultfilter.h"
#include "../../../skynetplusprotocal/skynetplusprotocal_error.h"

using namespace basiclib;

void CCoroutineCtx_NetClientServerCommu::RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func, bool bSelf){
    if (bSelf){
        ASSERT((nDstCtxID >> 24) == 0);
        nDstCtxID = nDstCtxID | (m_request.m_cIndex << 24);
    }
    else{
        ASSERT((nDstCtxID >> 24) != 0);
    }
    m_mapRegister[nDstCtxID][nType] = func;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CCoroutineCtx_NetClientServerCommu)
CCoroutineCtx_NetClientServerCommu::CCoroutineCtx_NetClientServerCommu(const char* pKeyName, const char* pClassName) : CCoroutineCtx_NetClient(pKeyName, pClassName){
    m_nDefaultTimeoutRequest = 5;
    m_bVerifySuccess = false;
}

CCoroutineCtx_NetClientServerCommu::~CCoroutineCtx_NetClientServerCommu(){
}

int CCoroutineCtx_NetClientServerCommu::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    const char* pServerID = func(InitGetParamType_Config, "serverid", nullptr);
    const char* pServerKey = func(InitGetParamType_Config, "servername", nullptr);
    if (pServerID == nullptr || pServerKey == nullptr)
        return -1;
    m_request.m_cIndex = (Net_UChar)atol(pServerID);
    m_request.m_strKeyName = pServerKey;
    m_nDefaultTimeoutRequest = atol(func(InitGetParamType_Config, "servercommutimeout", "10"));
    
    int nRet = CCoroutineCtx_NetClient::InitCtx(pMQMgr, [&](InitGetParamType nType, const char* pKey, const char* pDefault)->const char*{
        if (nType == InitGetParamType_Config){
            if (strcmp(pKey, "address") == 0)
                return func(nType, "serveraddress", pDefault);
        }
        return func(nType, pKey, pDefault);
    });
    if (nRet == 0){
        //默认timeout时间检查两次
        CoroutineCtxAddOnTimer(m_ctxID, m_nDefaultTimeoutRequest * 100 / 2, OnTimerCheckTimeout);
    }
    return nRet;
}


//! 不需要自己delete，只要调用release
void CCoroutineCtx_NetClientServerCommu::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimerCheckTimeout);
    CCoroutineCtx_NetClient::ReleaseCtx();
}

int32_t CCoroutineCtx_NetClientServerCommu::OnConnect(basiclib::CBasicSessionNetClient* pClient, uint32_t nCode){
    //发送注册
    SendWithSprotoStruct(pClient, m_request, m_ins);
    return BASIC_NET_OK;
}

int32_t CCoroutineCtx_NetClientServerCommu::OnIdle(basiclib::CBasicSessionNetClient* pSession, uint32_t nIdle){
    if (nIdle % 15 == 14){
        SendWithSprotoStruct(pSession, m_ping, m_insServer);
    }
    return BASIC_NET_OK;
}

int32_t CCoroutineCtx_NetClientServerCommu::OnDisconnect(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode){
    ctx_message ctxMsg(0, CCoroutineCtx_NetClientServerCommu::Func_ReceiveCommuDisconnect);
    PushMessage(ctxMsg);
    return BASIC_NET_OK;
}

void CCoroutineCtx_NetClientServerCommu::Func_ReceiveCommuDisconnect(CCoroutineCtx* pCtx, ctx_message* pMsg){
    CCoroutineCtx_NetClientServerCommu* pCommu = (CCoroutineCtx_NetClientServerCommu*)pCtx;
	pCommu->ReceiveDisconnect(pMsg);
}
void CCoroutineCtx_NetClientServerCommu::ReceiveDisconnect(ctx_message* pMsg){
    //断开,不能保存指针不然指向同一个(最后一个)
	basiclib::basic_vector<KernelRequestStoreData> vtDeleteRequest;
    m_vtRequest.ForeachData([&](KernelRequestStoreData* pRequest)->void{
		vtDeleteRequest.push_back(*pRequest);
    });
	for (auto& deldata : vtDeleteRequest) {
		WaitResumeCoroutineCtxFail(deldata.m_pCorutine);
	}
    if (m_vtRequest.GetMQLength() != 0){
        ASSERT(0);
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) vtrequest no empty", __FUNCTION__, __FILE__, __LINE__);
        m_vtRequest.Drop_Queue([&](KernelRequestStoreData* pRequest)->void{});
    }
	basiclib::basic_vector<KernelRequestStoreData> vtDeleteRequestStoreData;
    for (auto& data : m_mapRequest){
		vtDeleteRequestStoreData.push_back(data.second);
    }
	for (auto& deldata : vtDeleteRequestStoreData) {
		WaitResumeCoroutineCtxFail(deldata.m_pCorutine);
	}
}

void CCoroutineCtx_NetClientServerCommu::OnTimerCheckTimeout(CCoroutineCtx* pCtx){
    CCoroutineCtx_NetClientServerCommu* pRealCtx = (CCoroutineCtx_NetClientServerCommu*)pCtx;
    time_t tmNow = time(NULL);

    KernelRequestStoreData* pRequestStore = pRealCtx->m_vtRequest.FrontData();
    IsErrorHapper(pRequestStore == nullptr || !pRequestStore->IsTimeOut(tmNow, pRealCtx->m_nDefaultTimeoutRequest), CCFrameSCBasicLogEventErrorV("%s(%s:%d) vt Timeout, Close", __FUNCTION__, __FILE__, __LINE__); pRealCtx->ErrorClose(); return);
    for (auto& data : pRealCtx->m_mapRequest){
#ifndef _DEBUG
        IsErrorHapper(!data.second.IsTimeOut(tmNow, pRealCtx->m_nDefaultTimeoutRequest), 
			CCFrameSCBasicLogEventErrorV("%s(%s:%d) map Timeout, Close", __FUNCTION__, __FILE__, __LINE__); pRealCtx->ErrorClose(); return);
#endif
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! 协程里面调用Bussiness消息
int CCoroutineCtx_NetClientServerCommu::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    switch (nType){
    case DEFINEDISPATCH_CORUTINE_NetClient_CreateMap:
        return DispathBussinessMsg_CreateMap(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_NetClient_DeleteMap:
        return DispathBussinessMsg_DelMap(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_NetClient_GetMap:
        return DispathBussinessMsg_GetMap(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_NetClient_Request:
        return DispathBussinessMsg_Request(pCorutine, nParam, pParam, pRetPacket);
    default:
        return CCoroutineCtx_NetClient::DispathBussinessMsg(pCorutine, nType, nParam, pParam, pRetPacket, pCurrentMsg);
    }
    return -99;
}

//! 创建协程
void CCoroutineCtx_NetClientServerCommu::Corutine_ReceiveRequest(CCorutinePlus* pCorutine){

}

long CCoroutineCtx_NetClientServerCommu::DispathBussinessMsg_Receive(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCorutineStackDataDefault* pDefaultStack = (CCorutineStackDataDefault*)pParam[0];

    Net_UInt nMethod = GetDefaultStackMethod(pDefaultStack);
    bool bResponse = ((nMethod & SPROTO_METHOD_TYPE_RESPONSE) != 0);
    if (bResponse){
        nMethod = nMethod & ~SPROTO_METHOD_TYPE_RESPONSE;
        switch (nMethod){
        case SPROTO_METHOD_SKYNET_Register:{
            SkynetPlusRegisterResponse response;
            IsErrorHapper(InsToSprotoStructDefaultStack(response, pDefaultStack),
				ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) parse register error, Close", __FUNCTION__, __FILE__, __LINE__); ErrorClose(); return 0);
            if (response.m_nError != 0){
                ASSERT(0);
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) register ret error(%d), Close", __FUNCTION__, __FILE__, __LINE__, response.m_nError);
                ErrorClose();
            }
            else{
                CCFrameSCBasicLogEvent("Register GlobalServer Success!");
                //重新发送所有的注册
                if (m_map.size() > 0){
                    SkynetPlusCreateNameToCtxID request;
                    request.m_mapNameToCtxID = m_map;
                    SendWithSprotoStruct(m_pClient, request, m_ins);
                }
            }
        }
                                           break;
        case SPROTO_METHOD_SKYNET_CreateNameToCtxID:{
            SkynetPlusCreateNameToCtxIDResponse response;
            IsErrorHapper(InsToSprotoStructDefaultStack(response, pDefaultStack), ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) parse createnametoctxid error, Close", __FUNCTION__, __FILE__, __LINE__); return 0);
            if (response.m_nError != 0){
                ASSERT(0);
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) createNameToCtxid ret error(%d), Close", __FUNCTION__, __FILE__, __LINE__, response.m_nError);
                ErrorClose();
            }
        }
                                                    break;
        case SPROTO_METHOD_SKYNET_DeleteNameToCtxID:{
            SkynetPlusDeleteNameToCtxIDResponse response;
            IsErrorHapper(InsToSprotoStructDefaultStack(response, pDefaultStack), ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) parse delnametoctxid error", __FUNCTION__, __FILE__, __LINE__); return 0);
            if (response.m_nError != 0){
                ASSERT(0);
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) deleteNameToCtxid ret error(%d), Close", __FUNCTION__, __FILE__, __LINE__, response.m_nError);
                ErrorClose();
            }
        }
                                                    break;
        case SPROTO_METHOD_SKYNET_GetCtxIDByName:{
            SkynetPlusGetCtxIDByNameResponse response;
            IsErrorHapper(InsToSprotoStructDefaultStack(response, pDefaultStack), ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) parse delnametoctxid error, Close", __FUNCTION__, __FILE__, __LINE__); ErrorClose(); return 0);
            //notify
            KernelRequestStoreData* pRequest = m_vtRequest.FrontData();
            IsErrorHapper(nullptr != pRequest, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) NoRequest Error, Close", __FUNCTION__, __FILE__, __LINE__); ErrorClose(); break);
            ctx_message sendCtxMsg;
            sendCtxMsg.m_session = (intptr_t)&response;
            IsErrorHapper(WaitResumeCoroutineCtx(pRequest->m_pCorutine, this, &sendCtxMsg), ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) WaitResumeCoroutineCtx Fail,Close", __FUNCTION__, __FILE__, __LINE__); ErrorClose(); break);
        }
                                                 break;
        case SPROTO_METHOD_SKYNETCHILD_Request:{
            Net_CBasicBitstream ins;
            SkynetPlusServerRequestResponse response;
            response.m_pIns = &ins;
            IsErrorHapper2(InsToSprotoStructDefaultStack(response, pDefaultStack),
                ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) parse delnametoctxid error", __FUNCTION__, __FILE__, __LINE__),
                return 0);
            auto& iter = m_mapRequest.find(response.m_head.m_nSession);
            if (iter != m_mapRequest.end()){
                ctx_message sendCtxMsg;
                sendCtxMsg.m_session = (intptr_t)&response;
                IsErrorHapper(WaitResumeCoroutineCtx(iter->second.m_pCorutine, this, &sendCtxMsg), ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) WaitResumeCoroutineCtx Fail,Close", __FUNCTION__, __FILE__, __LINE__); ErrorClose(); break);
            }
            else{
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) no sessionid find %d", __FUNCTION__, __FILE__, __LINE__, response.m_head.m_nSession);
            }
        }
                                               break;
        default:
            CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow method %x", __FUNCTION__, __FILE__, __LINE__, nMethod);
            break;
        }
    }
    else{
        switch (nMethod){
        case SPROTO_METHOD_SKYNETCHILD_ServerRequest:{
            Net_CBasicBitstream childRequest;
            SkynetPlusServerRequestTransfer request;
            request.m_pIns = &childRequest;
            SkynetPlusServerRequestTransferResponse response;
            IsErrorHapper2(InsToSprotoStructDefaultStack(request, pDefaultStack),
                ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) parse ServerRequest error", __FUNCTION__, __FILE__, __LINE__),
                return 0);
            //拷贝session,ctxid
            response.m_head.m_nSession = request.m_head.m_nSession;
            response.m_nCtxID = request.m_nCtxID;
            do{
#ifdef _DEBUG
				CCFrameSCBasicLogEventErrorV("bussiness receive request %d:%d", request.m_head.m_nSession, request.m_nCtxID);
#endif
                SkynetPlusServerRequest realRequest;
                IsErrorHapper2(InsToSprotoStructSMBuffer(realRequest, childRequest),
                    ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) parsechild ServerRequest error", __FUNCTION__, __FILE__, __LINE__),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
                IsErrorHapper2(CCoroutineCtxHandle::GetInstance()->IsRemoteServerCtxID(realRequest.m_request.m_nDstCtxID) == false,
                    ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) receive remotectx(%x) type(%d) error", __FUNCTION__, __FILE__, __LINE__, realRequest.m_request.m_nDstCtxID, realRequest.m_request.m_nType),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_RoadErr; break);
                auto& iterCtxMap = m_mapRegister.find(realRequest.m_request.m_nDstCtxID);
                IsErrorHapper2(iterCtxMap != m_mapRegister.end(),
                    ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) receive no serialize remotectx(%x) type(%d) error", __FUNCTION__, __FILE__, __LINE__, realRequest.m_request.m_nDstCtxID, realRequest.m_request.m_nType),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_NoSeria; break);
                MapSerialize& mapSerialize = iterCtxMap->second;
                auto& iter = mapSerialize.find(realRequest.m_request.m_nType);
                IsErrorHapper2(iter != mapSerialize.end(),
                    ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) receive no serialize remotectx(%x) type(%d) error", __FUNCTION__, __FILE__, __LINE__, realRequest.m_request.m_nDstCtxID, realRequest.m_request.m_nType),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_NoSeria; break);
                ServerCommuSerialize& funcData = iter->second;
                bool bRetUnSerializeRequest = funcData.m_requestUnSerialize(realRequest.m_ins, [&](int nParam, void** pParam)->bool{
                    IsErrorHapper2(nParam == realRequest.m_cParamCount,
                        ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) param count error remotectx(%x) type(%d) %d != %d", __FUNCTION__, __FILE__, __LINE__, realRequest.m_request.m_nDstCtxID, realRequest.m_request.m_nType, nParam, realRequest.m_cParamCount),
                        return false);
                    //去本机ctxID
                    uint32_t nDstCtxID = realRequest.m_request.m_nDstCtxID & HANDLE_ID_ALLOCATE_LOG;
                    CCoroutineCtx* pCurrentCtx = nullptr;
                    ctx_message* pCurrentMsg = nullptr;
                    int nRetValue = 0;
                    bool bRetSerializeResponse = funcData.m_responseSerialize([&](void* pRetPacket)->bool{
						IsErrorHapper2(WaitExeCoroutineToCtxBussiness(pCorutine, m_ctxID, nDstCtxID, realRequest.m_request.m_nType, nParam, pParam, pRetPacket, nRetValue),
							ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) WaitExeCoroutineToCtxBussiness error remotectx(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, realRequest.m_request.m_nDstCtxID, realRequest.m_request.m_nType),
							return false);
                        return true;
                    }, response.m_ins);
                    IsErrorHapper2(nRetValue == 0 && bRetSerializeResponse,
                        CCFrameSCBasicLogEventErrorV("%s(%s:%d) DispathBussinessMsg error remotectx(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, realRequest.m_request.m_nDstCtxID, realRequest.m_request.m_nType),
                        return false);
                    return true;
                });
                IsErrorHapper2(bRetUnSerializeRequest,
                    ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) m_requestUnSerialize error remotectx(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, realRequest.m_request.m_nDstCtxID, realRequest.m_request.m_nType),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_SeriaErr; break);
            } while (false);
#ifdef _DEBUG
			CCFrameSCBasicLogEventErrorV("bussiness receive request send %d:%d:%d", request.m_head.m_nSession, request.m_nCtxID, response.m_nError);
#endif
            SendWithSprotoStruct(m_pClient, response, m_ins);
        }
			break;
        default:
            CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow method %x", __FUNCTION__, __FILE__, __LINE__, nMethod);
            break;
        }
    }
    
    return 0;
}

long CCoroutineCtx_NetClientServerCommu::DispathBussinessMsg_CreateMap(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 2, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    if (!IsTransmit())
        return -2;
    const char* pCtxName = (const char*)pParam[0];
    uint32_t uCtxID = *(uint32_t*)pParam[1];
    auto& iter = m_map.find(pCtxName);
    if (iter != m_map.end()){
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) CreateNameToCtxID exist name %s(%d:%d)", __FUNCTION__, __FILE__, __LINE__, pCtxName, uCtxID, iter->second);
    }
    m_map[pCtxName] = uCtxID;
    SkynetPlusCreateNameToCtxID request;
    request.m_mapNameToCtxID[pCtxName] = uCtxID;
    SendWithSprotoStruct(m_pClient, request, m_ins);
    return 0;
}
long CCoroutineCtx_NetClientServerCommu::DispathBussinessMsg_DelMap(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    if (!IsTransmit())
        return -2;
    const char* pCtxName = (const char*)pParam[0];
    auto& iter = m_map.find(pCtxName);
    if (iter == m_map.end()){
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) DeleteNameToCtxID no exist name %s", __FUNCTION__, __FILE__, __LINE__, pCtxName);
        return 0;
    }
    m_map.erase(iter);
    SkynetPlusDeleteNameToCtxID request;
    request.m_strName = pCtxName;
    SendWithSprotoStruct(m_pClient, request, m_ins);
    return 0;
}

long CCoroutineCtx_NetClientServerCommu::DispathBussinessMsg_GetMap(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    const char* pCtxName = (const char*)pParam[0];
    if (!IsTransmit())
        return -2;
    CCoroutineCtx* pCurrentCtx = nullptr;
    ctx_message* pCurrentMsg = nullptr;
    //! 保存
    CKernelMsgQueueRequestMgr requestMgr(m_vtRequest, pCorutine);
    SkynetPlusGetCtxIDByName request;
    request.m_strName = pCtxName;
    SendWithSprotoStruct(m_pClient, request, m_ins);
    //截断协程由子协程唤醒(认为结束了 自己保存)
    if (!YieldCorutineToCtx(pCorutine, 0, pCurrentCtx, pCurrentMsg)){
        //代表进来的时候sourcectxid已经不存在了
        return -4;
    }
    SkynetPlusGetCtxIDByNameResponse* pResponse = (SkynetPlusGetCtxIDByNameResponse*)pRetPacket;
    *pResponse = *(SkynetPlusGetCtxIDByNameResponse*)pCurrentMsg->m_session;
    return 0;
}

long CCoroutineCtx_NetClientServerCommu::DispathBussinessMsg_Request(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 4, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    if (!IsTransmit())
        return -2;
    uint32_t nDstCtxID = *(uint32_t*)pParam[0];
    uint32_t nType = *(uint32_t*)pParam[1];
    int nRequestParam = *(int*)pParam[2];
    void** pRequestParam = (void**)pParam[3];

    auto& iterCtxMap = m_mapRegister.find(nDstCtxID);
    if (iterCtxMap == m_mapRegister.end())
        return 1;
    MapSerialize& mapSerialize = iterCtxMap->second;
    auto& iter = mapSerialize.find(nType);
    if (iter == mapSerialize.end())
        return 2;
    ServerCommuSerialize& funcData = iter->second;
    
    SkynetPlusServerRequest request;
    request.m_head.m_nSession = GetNewSessionID();
    request.m_request.m_nDstCtxID = nDstCtxID;
    request.m_request.m_nType = nType;
    request.m_cParamCount = (Net_UChar)nRequestParam;
    IsErrorHapper(funcData.m_requestSerialize(nRequestParam, pRequestParam, request.m_ins), ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) m_requestSerialize error ctxid(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, nDstCtxID, nType); return 3);
    //! 保存
    CKernelMapRequestMgr mapMgr(m_mapRequest, pCorutine, request.m_head.m_nSession);
    //发送
    SendWithSprotoStruct(m_pClient, request, m_ins);

    CCoroutineCtx* pCurrentCtx = nullptr;
    ctx_message* pCurrentMsg = nullptr;
    //截断协程由子协程唤醒(认为结束了 自己保存)
    if (!YieldCorutineToCtx(pCorutine, 0, pCurrentCtx, pCurrentMsg)){
        //代表进来的时候sourcectxid已经不存在了
        return -4;
    }
    SkynetPlusServerRequestResponse* pResponse = (SkynetPlusServerRequestResponse*)pCurrentMsg->m_session;
    IsErrorHapper(pResponse->m_nError == 0, return 4);
    IsErrorHapper(funcData.m_responseUnSerialize(*pResponse->m_pIns, pRetPacket), 
        ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) m_responseUnSerialize error ctxid(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, nDstCtxID, nType); return 5);
    return 0;
}


