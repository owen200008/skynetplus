#include "interationcenterserversessionctx.h"
#include "../kernel/ccframe/log/ctx_log.h"
#include "../kernel/ccframe/net/ccframeserver_frame.h"
#include "../kernel/ccframe/coroutinedata/coroutinestackdata.h"
#include "interationcenter.h"
#include "../skynetplusprotocal/skynetplusprotocal_function.h"
#include "../skynetplusprotocal/skynetplusprotocal_error.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CInterationCenterServerSessionCtx)
CInterationCenterServerSessionCtx::CInterationCenterServerSessionCtx(const char* pKeyName, const char* pClassName) : CCoroutineCtx_CCFrameServerSession(pKeyName, pClassName){
    m_nDefaultTimeoutRequest = 5;
}
CInterationCenterServerSessionCtx::~CInterationCenterServerSessionCtx(){
}

int CInterationCenterServerSessionCtx::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func)
{
    int nRet = CCoroutineCtx_CCFrameServerSession::InitCtx(pMQMgr, func);
    if (nRet != 0)
        return nRet;
    //默认timeout时间检查两次
    m_nDefaultTimeoutRequest = atol(func(InitGetParamType_Config, "requesttimeout", "5"));

    CoroutineCtxAddOnTimer(m_ctxID, m_nDefaultTimeoutRequest * 100 / 2, OnTimer);
    return 0;
}

void CInterationCenterServerSessionCtx::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimer);
    CCoroutineCtx_CCFrameServerSession::ReleaseCtx();
}

void CInterationCenterServerSessionCtx::OnTimer(CCoroutineCtx* pCtx){
    CInterationCenterServerSessionCtx* pRealCtx = (CInterationCenterServerSessionCtx*)pCtx;

    time_t tmNow = time(NULL);
	basiclib::basic_vector<KernelRequestStoreData> vtDelete;
    for (auto& data : pRealCtx->m_mapRequest){
#ifndef _DEBUG
        if (data.second.IsTimeOut(tmNow, pRealCtx->m_nDefaultTimeoutRequest)){
            //超时
            CCFrameSCBasicLogEventErrorV("%s(%s:%d) Timeout, Close", __FUNCTION__, __FILE__, __LINE__);
			vtDelete.push_back(data.second);
        }
#endif
    }
	for (auto& delData : vtDelete) {
		WaitResumeCoroutineCtxFail(delData.m_pCorutine);
	}
}

//! 协程里面调用Bussiness消息
int CInterationCenterServerSessionCtx::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    switch (nType){
    case DEFINEDISPATCH_CORUTINE_InterationCenterServerSessionCtx_RequestPacket:
        return DispathBussinessMsg_RequestPacket(pCorutine, nParam, pParam, pRetPacket);
	case DEFINEDISPATCH_CORUTINE_InterationCenterServerSessionCtx_WakeUp:
		return DispathBussinessMsg_Wakeup(pCorutine, nParam, pParam, pRetPacket);
    default:
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
        break;
    }
    return -99;
}

long CInterationCenterServerSessionCtx::DispathBussinessMsg_RequestPacket(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 2, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCFrameServerSession* pSession = (CCFrameServerSession*)pParam[0];
    CCorutineStackDataDefault* pDefaultStack = (CCorutineStackDataDefault*)pParam[1];
	int nStackDataLength = 0;
	const char* pStackData = pDefaultStack->GetData(nStackDataLength);
    Net_UInt nMethod = GetDefaultStackMethod(pDefaultStack);
    switch (nMethod){
    case SPROTO_METHOD_SKYNETCHILD_Request:{
        SkynetPlusServerRequest         request;//到这里为止为止不能变动，server只解析这两个
        Net_CBasicBitstream defaultIns;
        SkynetPlusServerRequestResponse response;
        response.m_pIns = &defaultIns;
        do{
            basiclib::CBasicBitstream os;
            os.BindOutData((char*)pStackData, nStackDataLength);
            os >> request.m_head;
            os >> request.m_request;
            IsErrorHapper(os.IsReadError() == false, response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
			response.m_head.m_nSession = request.m_head.m_nSession;
            //发送给对应的
            Net_UChar cIndex = (request.m_request.m_nDstCtxID >> 24);
            if (cIndex == 0xFF){
                MapCtxIDToMap& mapCtxID = CInterationCenterCtx::GetInterationCenterCtx()->GetRegister();
                auto& iterCtxMap = mapCtxID.find(request.m_request.m_nDstCtxID);
                IsErrorHapper2(iterCtxMap != mapCtxID.end(),
                    ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) receive no serialize remotectx(%x) type(%d) error", __FUNCTION__, __FILE__, __LINE__, request.m_request.m_nDstCtxID, request.m_request.m_nType),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_RequestCtx_ResponseErr; break);
                MapSerialize& mapSerialize = iterCtxMap->second;
                auto& iter = mapSerialize.find(request.m_request.m_nType);
                IsErrorHapper2(iter != mapSerialize.end(),
                    ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) receive no serialize remotectx(%x) type(%d) error", __FUNCTION__, __FILE__, __LINE__, request.m_request.m_nDstCtxID, request.m_request.m_nType),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_RequestCtx_ResponseErr; break);
                ServerCommuSerialize& funcData = iter->second;
                bool bRetUnSerializeRequest = funcData.m_requestUnSerialize(request.m_ins, [&](int nParam, void** pParam)->bool{
                    IsErrorHapper2(nParam == request.m_cParamCount,
                        ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) param count error remotectx(%x) type(%d) %d != %d", __FUNCTION__, __FILE__, __LINE__, request.m_request.m_nDstCtxID, request.m_request.m_nType, nParam, request.m_cParamCount),
                        return false);
                    //去本机ctxID
                    uint32_t nDstCtxID = request.m_request.m_nDstCtxID & HANDLE_ID_ALLOCATE_LOG;
                    CCoroutineCtx* pCurrentCtx = nullptr;
                    ctx_message* pCurrentMsg = nullptr;
                    int nRetValue = 0;
                    bool bRetSerializeResponse = funcData.m_responseSerialize([&](void* pRetPacket)->bool{
                        IsErrorHapper2(YieldCorutineToCtx(pCorutine, nDstCtxID, pCurrentCtx, pCurrentMsg),
                            ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) YieldCorutineToCtx error remotectx(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, request.m_request.m_nDstCtxID, request.m_request.m_nType),
                            return false);
                        nRetValue = pCurrentCtx->DispathBussinessMsg(pCorutine, request.m_request.m_nType, nParam, pParam, pRetPacket, pCurrentMsg);
                        IsErrorHapper2(YieldCorutineToCtx(pCorutine, GetCtxID(), pCurrentCtx, pCurrentMsg),
                            ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) YieldCorutineToCtx error remotectx(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, request.m_request.m_nDstCtxID, request.m_request.m_nType),
                            return false);
                        return true;
                    }, defaultIns);
                    IsErrorHapper2(nRetValue == 0 && bRetSerializeResponse,
                        CCFrameSCBasicLogEventErrorV("%s(%s:%d) DispathBussinessMsg error remotectx(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, request.m_request.m_nDstCtxID, request.m_request.m_nType),
                        return false);
                    return true;
                });
                IsErrorHapper2(bRetUnSerializeRequest,
                    CCFrameSCBasicLogEventErrorV("%s(%s:%d) m_requestUnSerialize error remotectx(%x) type(%d)", __FUNCTION__, __FILE__, __LINE__, request.m_request.m_nDstCtxID, request.m_request.m_nType),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_SeriaErr; break);
            }
            else{
#ifdef _DEBUG
				CCFrameSCBasicLogEventErrorV("center receive request %d:%x:%d", request.m_head.m_nSession, request.m_request.m_nDstCtxID, request.m_request.m_nType);
#endif
                InterationCenterCtxStoreInfo ctxStoreInfo;
                IsErrorHapper(CCFrameGetInterationCenterCtxStoreInfoByCIndex(GetCtxID(), pCorutine, cIndex, ctxStoreInfo), response.m_nError = SPROTO_ERRORID_SKYNETPLUS_RequestCtx_NoServer; break);
                basiclib::CRefBasicSessionNetClient pDstSession = CInterationCenterCtx::GetInterationCenterCtx()->GetServer().GetServer()->GetClientBySessionID(ctxStoreInfo.m_nSessionID);
                IsErrorHapper(pDstSession != nullptr, response.m_nError = SPROTO_ERRORID_SKYNETPLUS_RequestCtx_NoServer; break);
                SkynetPlusServerRequestTransfer requestTransfer;
                requestTransfer.m_head.m_nSession = request.m_head.m_nSession;
                requestTransfer.m_nCtxID = GetCtxID();
                Net_CBasicBitstream ins;
                ins.BindOutData((char*)pStackData, nStackDataLength);
                requestTransfer.m_pIns = &ins;
                //!保存
                CKernelMapRequestMgr mapMgr(m_mapRequest, pCorutine, request.m_head.m_nSession);
                //!发送
                SendWithSprotoStruct(pDstSession, requestTransfer, m_insCtxSafe);

                CCoroutineCtx* pCurrentCtx = nullptr;
                ctx_message* pCurrentMsg = nullptr;
                //截断协程由子协程唤醒(认为结束了 自己保存)
                IsErrorHapper(YieldCorutineToCtx(pCorutine, 0, pCurrentCtx, pCurrentMsg), 
					response.m_nError = SPROTO_ERRORID_SKYNETPLUS_RequestCtx_ServerYieldErr; break);
                SkynetPlusServerRequestTransferResponse* pGetResponse = (SkynetPlusServerRequestTransferResponse*)pCurrentMsg->m_session;
                IsErrorHapper2(pGetResponse->m_nError == 0,
                    CCFrameSCBasicLogEventErrorV("%s(%s:%d) response error(%x)", __FUNCTION__, __FILE__, __LINE__, pGetResponse->m_nError),
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_RequestCtx_ResponseErr; break);
                //最后reply部分
                response.m_pIns = &(pGetResponse->m_ins);
#ifdef _DEBUG
				CCFrameSCBasicLogEventErrorV("center receive wakeup %d:%x:%d", request.m_head.m_nSession, request.m_request.m_nDstCtxID, request.m_request.m_nType);
#endif
            }
        } while (false);
        SendWithSprotoStruct(pSession, response, m_insCtxSafe);
    }
        break;
    case (SPROTO_METHOD_SKYNETCHILD_ServerRequest | SPROTO_METHOD_TYPE_RESPONSE):{
        SkynetPlusServerRequestTransferResponse response;
        do{
            //处理ctx
			IsErrorHapper(InsToSprotoStructDefaultStack(response, pDefaultStack),
				ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) parsechild ServerRequest error", __FUNCTION__, __FILE__, __LINE__); break);
			const int nSetParam = 1;
			void* pSetParam[nSetParam] = { &response };
			int nRetValue = 0;
            IsErrorHapper(WaitExeCoroutineToCtxBussiness(pCorutine, m_ctxID, response.m_nCtxID, DEFINEDISPATCH_CORUTINE_InterationCenterServerSessionCtx_WakeUp, nSetParam, pSetParam, nullptr, nRetValue),
                ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) serverrequest response ctx nofind(%d)", __FUNCTION__, __FILE__, __LINE__, response.m_nCtxID); break);
        } while (false);
    }
        break;
    default:{
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow method %x", __FUNCTION__, __FILE__, __LINE__, nMethod);
    }
        break;
    }
    return 0;
}

long CInterationCenterServerSessionCtx::DispathBussinessMsg_Wakeup(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
	//切换到发送ctx,准备唤醒发送的协程
	do {
		SkynetPlusServerRequestTransferResponse& response = *(SkynetPlusServerRequestTransferResponse*)pParam[0];
		auto& iter = m_mapRequest.find(response.m_head.m_nSession);
		IsErrorHapper(iter != m_mapRequest.end(),
			CCFrameSCBasicLogEventErrorV("%s(%s:%d) serverrequest response session nofind(%d)", __FUNCTION__, __FILE__, __LINE__, response.m_head.m_nSession); break);
		ctx_message sendCtxMsg;
		sendCtxMsg.m_session = (intptr_t)&response;
		IsErrorHapper(WaitResumeCoroutineCtx(iter->second.m_pCorutine, this, &sendCtxMsg),
			ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) WaitResumeCoroutineCtx Fail", __FUNCTION__, __FILE__, __LINE__); break);
	} while (false);
	return 0;
}

