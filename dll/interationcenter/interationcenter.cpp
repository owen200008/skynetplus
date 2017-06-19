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
    pServer->bind_rece(MakeFastFunction(this, &CInterationCenterCtx::OnReceive));
    pServer->bind_disconnect(MakeFastFunction(this, &CInterationCenterCtx::OnDisconnect));
    CCCFrameDefaultFilter filter;
    m_server.StartServer(&filter);
	return 0;
}

int32_t CInterationCenterCtx::OnReceive(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    CCFrameServerSession* pSession = (CCFrameServerSession*)pNotify;
    uint32_t nCtxID = pSession->GetCtxSessionID();
    if (cbData < sizeof(SProtoHead)){
        CCFrameSCBasicLogEventErrorV("CCoroutineCtx_XKServer CtxID(%d)收到的包大小错误%d,断开连接", nCtxID, cbData);
        pSession->Close();
        return BASIC_NET_GENERIC_ERROR;
    }
    Net_UInt nMethod = 0;
    basiclib::UnSerializeUInt((unsigned char*)pszData, nMethod);
    Net_UInt nFieldType = nMethod & SPROTO_METHOD_FIELD_MASK;
    if (nFieldType == SPROTO_METHOD_FIELD_SKYNET){
        if (nMethod == SPROTO_METHOD_SKYNET_Ping)
            return BASIC_NET_OK;
        const int nSetParam = 4;
        void* pSetParam[nSetParam] = { this, pSession, (void*)pszData, &cbData };
        CreateResumeCoroutineCtx([](CCorutinePlus* pCorutine)->void{
            int nGetParamCount = 0;
            void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
            IsErrorHapper(nGetParamCount == 4, 
				ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nGetParamCount); return);
            CCorutineStackDataDefault stackData;
            CInterationCenterCtx* pCtx = (CInterationCenterCtx*)pGetParam[0];
            RefCCFrameServerSession pSession = (CCFrameServerSession*)pGetParam[1];//智能指针保证不会析构
            stackData.CopyData((const char*)pGetParam[2], *(Net_Int*)pGetParam[3]);//拷贝到堆数据

            const int nSetParam = 2;
            void* pSetParam[nSetParam] = { pGetParam[1], &stackData };
            int nRetValue = 0;
            if (!WaitExeCoroutineToCtxBussiness(pCorutine, 0, pCtx->GetCtxID(), DEFINEDISPATCH_CORUTINE_InterationCenterCtx_Packet, nSetParam, pSetParam, nullptr, nRetValue)){
                ASSERT(0);
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) WaitExeCoroutineToCtxBussiness FAIL ret(%d)", __FUNCTION__, __FILE__, __LINE__, nRetValue);
            }
        }, 0, nSetParam, pSetParam);
    }
    else{
#ifdef _DEBUG
		CCFrameSCBasicLogEventErrorV("%x:%d", nMethod, cbData);
#endif
        if (nCtxID != 0){
            const int nSetParam = 4;
            void* pSetParam[nSetParam] = { this, pSession, (void*)pszData, &cbData };
            CreateResumeCoroutineCtx([](CCorutinePlus* pCorutine)->void{
                CCoroutineCtx* pCurrentCtx = nullptr;
                ctx_message* pCurrentMsg = nullptr;
                CCorutineStackDataDefault stackData;
                {
                    int nGetParamCount = 0;
                    void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
                    IsErrorHapper(nGetParamCount == 4, 
						ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nGetParamCount); return);
                    CInterationCenterCtx* pCtx = (CInterationCenterCtx*)pGetParam[0];
                    RefCCFrameServerSession pSession = (CCFrameServerSession*)pGetParam[1];//智能指针保证不会析构
                    stackData.CopyData((const char*)pGetParam[2], *(Net_Int*)pGetParam[3]);//拷贝到堆数据

                    const int nSetParam = 2;
                    void* pSetParam[nSetParam] = { pGetParam[1], &stackData };
                    int nRetValue = 0;
                    if (!WaitExeCoroutineToCtxBussiness(pCorutine, 0, pSession->GetCtxSessionID(), DEFINEDISPATCH_CORUTINE_InterationCenterServerSessionCtx_RequestPacket, nSetParam, pSetParam, nullptr, nRetValue)){
                        ASSERT(0);
                        CCFrameSCBasicLogEventErrorV("%s(%s:%d) WaitExeCoroutineToCtxBussiness FAIL ret(%d)", __FUNCTION__, __FILE__, __LINE__, nRetValue);
                    }
                }
            }, 0, nSetParam, pSetParam);
        }
        else{
            CCFrameSCBasicLogEventErrorV("%s(%s:%d) method error no ctxid method(%x)", __FUNCTION__, __FILE__, __LINE__, nMethod);
            pSession->Close(0);
        }
    }
    return BASIC_NET_OK;
}

int32_t CInterationCenterCtx::OnDisconnect(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode){
    CCFrameServerSession* pSession = (CCFrameServerSession*)pNotify;
    MapContain& mapRevert = pSession->GetServerSessionMapRevert();
    auto& iter = mapRevert.find(ServerSessionRevert_Key_RemoteID);
    if (iter != mapRevert.end()){
        ctx_message msg(GetCtxID(), [](CCoroutineCtx* pCtx, ctx_message* pMsg)->void{
            CInterationCenterCtx* pRealCtx = (CInterationCenterCtx*)pCtx;
            Net_UChar cIndex = pMsg->m_session;
            auto& iter = pRealCtx->m_mapGlobalKeyToSelfCtxID.find(cIndex);
            if (iter == pRealCtx->m_mapGlobalKeyToSelfCtxID.end()){
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) disconnect no find cindex(%d)", __FUNCTION__, __FILE__, __LINE__, cIndex);
                return;
            }
            ReleaseCtxByID(iter->second.m_nSelfCtxID);
            pRealCtx->m_mapGlobalKeyToSelfCtxID.erase(iter);
        }, iter->second.GetLong());
    }
    return BASIC_NET_OK;
}

//! 协程里面调用Bussiness消息
int CInterationCenterCtx::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    switch (nType){
    case DEFINEDISPATCH_CORUTINE_InterationCenterCtx_Packet:
        return DispathBussinessMsg_Packet(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_InterationCenterCtx_GetByID:
        return DispathBussinessMsg_GetByID(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_InterationCenterCtx_SelfCtxRegister:
        return DispathBussinessMsg_SelfCtxRegister(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_InterationCenterCtx_SelfCtxDelete:
        return DispathBussinessMsg_SelfCtxDelete(pCorutine, nParam, pParam, pRetPacket);
    default:
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
        break;
    }
    return -99;
}

long CInterationCenterCtx::DispathBussinessMsg_Packet(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 2, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCFrameServerSession* pSession = (CCFrameServerSession*)pParam[0];
    CCorutineStackDataDefault* pDefaultStack = (CCorutineStackDataDefault*)pParam[1];
    if (pSession->GetCtxSessionID() == 0){
        SkynetPlusRegister request;
        SkynetPlusRegisterResponse response;
        do{
            IsErrorHapper(InsToSprotoStructDefaultStack(request, pDefaultStack), 
				response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
            IsErrorHapper(request.m_head.m_nMethod == SPROTO_METHOD_SKYNET_Register, 
				ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) method error %x", __FUNCTION__, __FILE__, __LINE__, request.m_head.m_nMethod); response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
            if (request.m_cIndex == 0 || request.m_cIndex == 0xFF){
                //0代表自己，FF代表中心服务器
                ASSERT(0); 
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) method error %x", __FUNCTION__, __FILE__, __LINE__, request.m_head.m_nMethod);
                response.m_nError = SPROTO_ERRORID_SKYNETPLUS_REGISTER_ERRORREGISTERKEY;
                break;
            }
            auto& iter = m_mapGlobalKeyToSelfCtxID.find(request.m_cIndex);
            if (iter != m_mapGlobalKeyToSelfCtxID.end()){
                if (iter->second.m_strServerKey.Compare(request.m_strKeyName.c_str()) != 0){
                    //已经存在，并且key不相同
                    CCFrameSCBasicLogEventErrorV("%s(%s:%d) exist key no same (%d:%s:%s)", __FUNCTION__, __FILE__, __LINE__, request.m_cIndex, iter->second.m_strServerKey.c_str(), request.m_strKeyName.c_str());
                    response.m_nError = SPROTO_ERRORID_SKYNETPLUS_REGISTER_KEYEXIST;
                    break;
                }
                //析构原来的ctx
                ReleaseCtxByID(iter->second.m_nSelfCtxID);
            }
            InterationCenterCtxStoreInfo& storeInfo = m_mapGlobalKeyToSelfCtxID[request.m_cIndex];
            storeInfo.m_strServerKey = request.m_strKeyName;
            IsErrorHapper(m_server.VerifySuccessCreateServerSession(pSession), response.m_nError = SPROTO_ERRORID_SKYNETPLUS_REGISTER_CREATECTXERR; break);
            MapContain& mapRevert = pSession->GetServerSessionMapRevert();
            mapRevert[ServerSessionRevert_Key_RemoteID] = request.m_cIndex;
            mapRevert[ServerSessionRevert_Key_RemoteKey].SetString(request.m_strKeyName.c_str());
            storeInfo.m_nSelfCtxID = pSession->GetCtxSessionID();
            storeInfo.m_nSessionID = pSession->GetSessionID();
			CCFrameSCBasicLogEventV("RegisterChildServer %d %s", request.m_cIndex, request.m_strKeyName.c_str());
        } while (false);
        SendWithSprotoStruct(pSession, response, m_insCtxSafe);
        if (response.m_nError != 0){
            pSession->Close();
        }
        return 0;
    }
    Net_UInt nMethod = GetDefaultStackMethod(pDefaultStack);
    switch (nMethod){
    case SPROTO_METHOD_SKYNET_CreateNameToCtxID:{
        SkynetPlusCreateNameToCtxID request;
        SkynetPlusCreateNameToCtxIDResponse response;
        do{
            MapContain& mapRevert = pSession->GetServerSessionMapRevert();
            Net_UChar cIndex = mapRevert[ServerSessionRevert_Key_RemoteID].GetLong();
            IsErrorHapper(InsToSprotoStructDefaultStack(request, pDefaultStack), response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
            for (auto& key : request.m_mapNameToCtxID){
                uint32_t nSetCtxID = cIndex << 24 | key.second;
                auto& iter = m_mapNameToCtxID.find(key.first);
                if (iter != m_mapNameToCtxID.end()){
                    if (iter->second != nSetCtxID){
                        CCFrameSCBasicLogEventErrorV("%s(%s:%d) CreateNameToCtxID exist name %s(%d:%d)", __FUNCTION__, __FILE__, __LINE__, key.first.c_str(), nSetCtxID, iter->second);
                    }
                }
                m_mapNameToCtxID[key.first] = nSetCtxID;
            }
            response.m_mapNameToCtxID = request.m_mapNameToCtxID;
        } while (false);
        SendWithSprotoStruct(pSession, response, m_insCtxSafe);
    }
        break;
    case SPROTO_METHOD_SKYNET_DeleteNameToCtxID:{
        SkynetPlusDeleteNameToCtxID request;
        SkynetPlusDeleteNameToCtxIDResponse response;
        do{
            IsErrorHapper(InsToSprotoStructDefaultStack(request, pDefaultStack), 
				response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
            m_mapNameToCtxID.erase(request.m_strName);
            response.m_strName = request.m_strName;
        } while (false);
        SendWithSprotoStruct(pSession, response, m_insCtxSafe);
    }
        break;
    case SPROTO_METHOD_SKYNET_GetCtxIDByName:{
        SkynetPlusGetCtxIDByName request;
        SkynetPlusGetCtxIDByNameResponse response;
        do{
            IsErrorHapper(InsToSprotoStructDefaultStack(request, pDefaultStack), 
				response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
            auto& iter = m_mapNameToCtxID.find(request.m_strName);
            IsErrorHapper(iter != m_mapNameToCtxID.end(), response.m_nError = SPROTO_ERRORID_SKYNETPLUS_GetNameToCtx_NoName; break);
            response.m_nCtxID = iter->second;
        } while (false);
        SendWithSprotoStruct(pSession, response, m_insCtxSafe);
    }
        break;
    default:{
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow method %x", __FUNCTION__, __FILE__, __LINE__, nMethod);
    }
        break;
    }
    return 0;
}

long CInterationCenterCtx::DispathBussinessMsg_GetByID(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    Net_UChar cIndex = *(Net_UChar*)pParam[0];
    auto& iter = m_mapGlobalKeyToSelfCtxID.find(cIndex);
    if (iter == m_mapGlobalKeyToSelfCtxID.end())
        return -1;
    InterationCenterCtxStoreInfo* pCtxStoreInfo = (InterationCenterCtxStoreInfo*)pRetPacket;
    *pCtxStoreInfo = iter->second;
    return 0;
}

long CInterationCenterCtx::DispathBussinessMsg_SelfCtxRegister(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 2, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    const char* pCtxName = (const char*)pParam[0];
    uint32_t uCtxID = *(uint32_t*)pParam[1];
    auto& iter = m_mapNameToCtxID.find(pCtxName);
    if (iter != m_mapNameToCtxID.end()){
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) CreateNameToCtxID exist name %s(%d:%d)", __FUNCTION__, __FILE__, __LINE__, pCtxName, uCtxID, iter->second);
    }
    m_mapNameToCtxID[pCtxName] = (0xFF000000 | uCtxID);
    return 0;
}

long CInterationCenterCtx::DispathBussinessMsg_SelfCtxDelete(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    const char* pCtxName = (const char*)pParam[0];
    auto& iter = m_mapNameToCtxID.find(pCtxName);
    if (iter == m_mapNameToCtxID.end()){
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) DeleteNameToCtxID no exist name %s", __FUNCTION__, __FILE__, __LINE__, pCtxName);
        return 0;
    }
    m_mapNameToCtxID.erase(iter);
    return 0;
}

//! 启动的时候注册
void CInterationCenterCtx::RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func){
    m_mapRegister[(nDstCtxID | 0xFF000000)][nType] = func;
}

bool CCFrameGetInterationCenterCtxStoreInfoByCIndex(uint32_t nResCtxID, CCorutinePlus* pCorutine, Net_UChar& cIndex, InterationCenterCtxStoreInfo& replyPacket){
    const int nSetParam = 1;
    void* pSetParam[nSetParam] = { &cIndex };
    int nRetValue = 0;
    if (!WaitExeCoroutineToCtxBussiness(pCorutine, nResCtxID, g_pCenterCtx->GetCtxID(), DEFINEDISPATCH_CORUTINE_InterationCenterCtx_GetByID, nSetParam, pSetParam, &replyPacket, nRetValue)){
        ASSERT(0);
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) WaitExeCoroutineToCtxBussiness FAIL ret(%d)", __FUNCTION__, __FILE__, __LINE__, nRetValue);
        return false;
    }
    return true;
}


