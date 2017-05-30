#include "ctx_redis.h"
#include "../ctx_threadpool.h"
#include "../log/ctx_log.h"
#include "../scbasic/redis/read.h"

extern redisReplyObjectFunctions g_defaultResidFunc;

using namespace basiclib;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CNet_RedisInterface::CNet_RedisInterface(){
    m_pClient = nullptr;
    m_netBuffCache.SetDataLength(10240);
    m_netBuffCache.SetDataLength(0);
    m_pRedisReader = nullptr;
    m_pInterface = nullptr;
}
CNet_RedisInterface::~CNet_RedisInterface(){
    if (m_pRedisReader){
        redisReaderFree(m_pRedisReader);
        m_pRedisReader = nullptr;
    }
    if (m_pClient){
        m_pClient->Release();
    }
}

void CNet_RedisInterface::CheckConnect(){
    m_pClient->DoConnect();
}
void CNet_RedisInterface::ErrorClose(){
    m_pClient->Close(0);
}
//! 判断是否已经认证成功
bool CNet_RedisInterface::IsTransmit(){
    return m_pClient->IsTransmit();
}
//! 发送数据
void CNet_RedisInterface::SendRedisData(basiclib::CBasicSmartBuffer& smBuf){
    m_pClient->Send(smBuf);
}

bool CNet_RedisInterface::InitNet(CCoroutineCtx_RedisInterface* pInterface, const char* pAddress, const char* pPwd, const char* pDB){
    m_pInterface = pInterface;
    m_strAddress = pAddress;
    if (pPwd)
        m_strPwd = pPwd;
    if (pDB)
        m_strDB = pDB;
    m_pClient = CCommonClientSession::CreateCCommonClientSession();
    m_pClient->bind_connect(MakeFastFunction(this, &CNet_RedisInterface::OnConnect));
    m_pClient->bind_disconnect(MakeFastFunction(this, &CNet_RedisInterface::OnDisconnect));
    m_pClient->bind_idle(MakeFastFunction(this, &CNet_RedisInterface::OnIdle));
    m_pClient->Connect(m_strAddress.c_str());
    return true;
}

bool CNet_RedisInterface::CheckVerifyPwd(basiclib::CBasicSessionNetClient* pNotify, basiclib::CBasicSmartBuffer& smBuf)
{
    if (!m_strPwd.IsEmpty()){
        CRedisSendPacket packetAuth;
        basiclib::CNetBasicValue setValue[2];
        setValue[0].SetString("AUTH");
        packetAuth.PushBasicValue(setValue[0]);
        setValue[1].SetString(m_strPwd.c_str());
        packetAuth.PushBasicValue(setValue[1]);
        packetAuth.DataToSmartBuffer(smBuf);
        return true;
    }
    return false;
}
bool CNet_RedisInterface::CheckVerifyDB(basiclib::CBasicSessionNetClient* pNotify, basiclib::CBasicSmartBuffer& smBuf)
{
    if (!m_strDB.IsEmpty()){
        CRedisSendPacket packetAuth;
        basiclib::CNetBasicValue setValue[2];
        setValue[0].SetString("SELECT");
        packetAuth.PushBasicValue(setValue[0]);
        setValue[1].SetString(m_strDB.c_str());
        packetAuth.PushBasicValue(setValue[1]);
        packetAuth.DataToSmartBuffer(smBuf);
        return true;
    }
    return false;
}

void CNet_RedisInterface::OnNetSuccessVerify(){
    m_pClient->bind_rece(MakeFastFunction(this, &CNet_RedisInterface::OnReceive));
    m_pInterface->OnNetVerifySuccess();
}

int32_t CNet_RedisInterface::OnConnect(CBasicSessionNetClient* pClient, uint32_t nCode)
{
    if (m_pRedisReader){
        redisReaderFree(m_pRedisReader);
        m_pRedisReader = nullptr;
    }
    m_pRedisReader = redisReaderCreateWithFunctions(&g_defaultResidFunc);
    //网络层线程专用，需要保证线程安全
    if (CheckVerifyPwd(pClient, m_netBuffCache)){
        m_pClient->bind_rece(MakeFastFunction(this, &CNet_RedisInterface::OnReceiveVerify));
        pClient->Send(m_netBuffCache);
        return BASIC_NET_HC_RET_HANDSHAKE;
    }
    else if (CheckVerifyDB(pClient, m_netBuffCache)){
        m_pClient->bind_rece(MakeFastFunction(this, &CNet_RedisInterface::OnReceiveVerifyDB));
        pClient->Send(m_netBuffCache);
        return BASIC_NET_HC_RET_HANDSHAKE;
    }
    else{
        OnNetSuccessVerify();
    }
    return BASIC_NET_OK;
}
int32_t CNet_RedisInterface::OnReceiveVerify(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    IsErrorHapper(redisReaderFeed(m_pRedisReader, pszData, cbData) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) Redis Verify Feed Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
    void* pReply = nullptr;
    IsErrorHapper(redisReaderGetReply(m_pRedisReader, &pReply) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) Redis Verify GetReply Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);

    int32_t nRet = BASIC_NET_OK;
    if (pReply){
        redisReply* pReplyData = (redisReply*)pReply;
        if (pReplyData->type == REDIS_REPLY_STATUS){
            if (CheckVerifyDB(pNotify, m_netBuffCache)){
                m_pClient->bind_rece(MakeFastFunction(this, &CNet_RedisInterface::OnReceiveVerifyDB));
                pNotify->Send(m_netBuffCache);
                nRet = BASIC_NET_OK;
            }
            else{
                OnNetSuccessVerify();
                nRet = BASIC_NET_HR_RET_HANDSHAKE;
            }
        }
        else if (pReplyData->type == REDIS_REPLY_ERROR){
            CCFrameSCBasicLogEventErrorV(nullptr, "Redis OnReceiveVerify Fail %s, Close", pReplyData->str);
            pNotify->Close(0);
            return BASIC_NET_GENERIC_ERROR;
        }
        freeReplyObject(pReply);
    }
    return nRet;
}
int32_t CNet_RedisInterface::OnReceiveVerifyDB(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData)
{
    IsErrorHapper(redisReaderFeed(m_pRedisReader, pszData, cbData) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) Redis VerifyDB Feed Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
    void* pReply = nullptr;
    IsErrorHapper(redisReaderGetReply(m_pRedisReader, &pReply) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) Redis VerifyDB GetReply Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);

    int32_t nRet = BASIC_NET_OK;
    if (pReply){
        redisReply* pReplyData = (redisReply*)pReply;
        if (pReplyData->type == REDIS_REPLY_STATUS){
            OnNetSuccessVerify();
            nRet = BASIC_NET_HR_RET_HANDSHAKE;
        }
        else if (pReplyData->type == REDIS_REPLY_ERROR){
            CCFrameSCBasicLogEventErrorV(nullptr, "Redis OnReceiveVerifyDB Fail %s, Close", pReplyData->str);
            pNotify->Close(0);
            return BASIC_NET_GENERIC_ERROR;
        }
        freeReplyObject(pReply);
    }
    return nRet;
}
int32_t CNet_RedisInterface::OnReceive(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData)
{
    IsErrorHapper(redisReaderFeed(m_pRedisReader, pszData, cbData) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) Redis OnReceive Feed Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
    bool bContinueFindReply = false;
    do{
        bContinueFindReply = false;
        void* pReply = nullptr;
        IsErrorHapper(redisReaderGetReply(m_pRedisReader, &pReply) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) Redis OnReceive GetReply Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
        if (pReply){
            //默认第一个
            ctx_message ctxMsg(0, CNet_RedisInterface::Func_ReceiveRedisReply, (intptr_t)pReply);
            IsErrorHapper(CCoroutineCtx::PushMessageByID(m_pInterface->GetCtxID(), ctxMsg), ASSERT(0); CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) Redis OnReceive PushMessageByID Error, Close", __FUNCTION__, __FILE__, __LINE__); freeReplyObject(pReply); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
            bContinueFindReply = true;
        }
    } while (bContinueFindReply);

    return BASIC_NET_OK;
}
int32_t CNet_RedisInterface::OnDisconnect(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode)
{
    if (m_pRedisReader){
        redisReaderFree(m_pRedisReader);
        m_pRedisReader = nullptr;
    }
    //执行
    ctx_message ctxMsg(0, CNet_RedisInterface::Func_ReceiveRedisDisconnect);
    CCoroutineCtx::PushMessageByID(m_pInterface->GetCtxID(), ctxMsg);
    return BASIC_NET_OK;
}

int32_t CNet_RedisInterface::OnIdle(basiclib::CBasicSessionNetClient*, uint32_t nIdle)
{
    if (nIdle % 15 == 14){
        m_pInterface->OnNetIdle();
    }
    return BASIC_NET_OK;
}

void CNet_RedisInterface::Func_ReceiveRedisReply(CCoroutineCtx* pCtx, ctx_message* pMsg, CCtx_CorutinePlusThreadData* pData)
{
    CCoroutineCtx_RedisInterface* pRedis = (CCoroutineCtx_RedisInterface*)pCtx;
    if (!pRedis->ReceiveRedisReply(pMsg, pData)){
        freeReplyObject((redisReply*)pMsg->m_session);
    }
}
void CNet_RedisInterface::Func_ReceiveRedisDisconnect(CCoroutineCtx* pCtx, ctx_message* pMsg, CCtx_CorutinePlusThreadData* pData)
{
    CCoroutineCtx_RedisInterface* pRedis = (CCoroutineCtx_RedisInterface*)pCtx;
    pRedis->ReceiveRedisDisconnect(pMsg, pData);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CCoroutineCtx_CRedis)

CCoroutineCtx_CRedis::CCoroutineCtx_CRedis(const char* pKeyName, const char* pClassName) : CCoroutineCtx_RedisInterface(pKeyName, pClassName)
{
    m_ctxInsCache.SetDataLength(10240);
    m_ctxInsCache.SetDataLength(0);
    m_nDefaultTimeoutRequest = 5;
}

CCoroutineCtx_CRedis::~CCoroutineCtx_CRedis()
{

}

int CCoroutineCtx_CRedis::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
    IsErrorHapper(nRet == 0, return nRet);
    char szBuf[64] = { 0 };
    sprintf(szBuf, "%s_Address", m_pCtxName);
    const char* pAddress = func(InitGetParamType_Config, szBuf, "");
    if (pAddress == nullptr || strlen(pAddress) == 0){
        return 2;
    }
    sprintf(szBuf, "%s_AddressPwd", m_pCtxName);
    const char* pPwd = func(InitGetParamType_Config, szBuf, "");
    sprintf(szBuf, "%s_AddressDB", m_pCtxName);
    const char* pDB = func(InitGetParamType_Config, szBuf, "");
    sprintf(szBuf, "%s_timeout", m_pCtxName);
    m_nDefaultTimeoutRequest = atol(func(InitGetParamType_Config, szBuf, "5"));
    if (!m_netRedis.InitNet(this, pAddress, pPwd, pDB)){
        return 2;
    }
    //1s检查一次
    CoroutineCtxAddOnTimer(m_ctxID, 100, OnTimer);
    return 0;
}

//! 不需要自己delete，只要调用release
void CCoroutineCtx_CRedis::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimer);
    CCoroutineCtx::ReleaseCtx();
}

//! 协程里面调用Bussiness消息
int CCoroutineCtx_CRedis::DispathBussinessMsg(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    switch (nType){
    case 0:
        return DispathBussinessMsg_0_SendRequest(pCorutine, pData, nParam, pParam, pRetPacket);
    case 1:
        return DispathBussinessMsg_1_SendRequest(pCorutine, pData, nParam, pParam, pRetPacket);
    default:
        CCFrameSCBasicLogEventErrorV(pData, "CCoroutineCtx_CRedis::DispathBussinessMsg unknow type(%d) nParam(%d)", nType, nParam);
        break;
    }
    return -99;
}

//! 发送请求
long CCoroutineCtx_CRedis::DispathBussinessMsg_0_SendRequest(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCoroutineCtx* pCurrentCtx = nullptr;
    CCorutinePlusThreadData* pCurrentData = nullptr;
    ctx_message* pCurrentMsg = nullptr;

    if (!m_netRedis.IsTransmit())
        return -2;
    
    CRedisSendPacket* pSendPacket = (CRedisSendPacket*)pParam[0];
    CRedisReplyPacket* pReplyPacket = (CRedisReplyPacket*)pRetPacket;
    do{
        //! 保存
        CKernelMsgQueueRequestMgr requestMgr(m_vtRequest, pCorutine);

        //发送保存协程
        pSendPacket->DataToSmartBuffer(m_ctxInsCache);
        m_netRedis.SendRedisData(m_ctxInsCache);

        //截断协程由子协程唤醒(认为结束了 自己保存)
        if (!YieldCorutineToCtx(pCorutine, 0, pCurrentCtx, pCurrentData, pCurrentMsg)){
            //代表进来的时候sourcectxid已经不存在了
            return -4;
        }
        redisReply* pGetRedisReply = (redisReply*)pCurrentMsg->m_session;
        pReplyPacket->BindReply(pGetRedisReply);
        if (pGetRedisReply->type == REDIS_REPLY_ERROR){
            return 1;
        }
        pSendPacket = pSendPacket->GetNextSendPacket();
        pReplyPacket = pReplyPacket->GetNextReplyPacket();
    } while (pSendPacket && pReplyPacket);
    return 0;
}

//! 发送请求
long CCoroutineCtx_CRedis::DispathBussinessMsg_1_SendRequest(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCoroutineCtx* pCurrentCtx = nullptr;
    CCorutinePlusThreadData* pCurrentData = nullptr;
    ctx_message* pCurrentMsg = nullptr;

    if (!m_netRedis.IsTransmit())
        return -2;

    basiclib::CBasicStringArray* pArrayRequest = (basiclib::CBasicStringArray*)pParam[0];
    CRedisReplyPacket* pReplyPacket = (CRedisReplyPacket*)pRetPacket;
    for (int i = 0; i < pArrayRequest->GetSize() && pReplyPacket; i++){
        //! 
        CKernelMsgQueueRequestMgr requestMgr(m_vtRequest, pCorutine);

        CRedisSendPacket sendPacket;
        basiclib::CNetBasicValue netValue[32];
        int nIndex = 0;
        basiclib::BasicSpliteString(pArrayRequest->GetAt(i).c_str(), " ", [&](const char* pValue)->void{
            basiclib::CNetBasicValue& setValue = netValue[nIndex % 32];
            setValue.SetString(pValue);
            sendPacket.PushBasicValue(setValue);
            nIndex++;
        });
        //发送保存协程
        sendPacket.DataToSmartBuffer(m_ctxInsCache);
        m_netRedis.SendRedisData(m_ctxInsCache);

        //截断协程由子协程唤醒(认为结束了 自己保存)
        if (!YieldCorutineToCtx(pCorutine, 0, pCurrentCtx, pCurrentData, pCurrentMsg)){
            //代表进来的时候sourcectxid已经不存在了
            return -4;
        }
        redisReply* pGetRedisReply = (redisReply*)pCurrentMsg->m_session;
        pReplyPacket->BindReply(pGetRedisReply);
        if (pGetRedisReply->type == REDIS_REPLY_ERROR){
            return 1;
        }
        pReplyPacket = pReplyPacket->GetNextReplyPacket();
    }
    return 0;
}


void CCoroutineCtx_CRedis::OnTimer(CCoroutineCtx* pCtx, CCtx_CorutinePlusThreadData* pData)
{
    CCoroutineCtx_CRedis* pRedisCtx = (CCoroutineCtx_CRedis*)pCtx;
    time_t tmNow = time(NULL);
    
    KernelRequestStoreData* pRequestStore = pRedisCtx->m_vtRequest.FrontData();
    IsErrorHapper(pRequestStore == nullptr || !pRequestStore->IsTimeOut(tmNow, pRedisCtx->m_nDefaultTimeoutRequest), CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) OnTimerRedis Timeout, Close", __FUNCTION__, __FILE__, __LINE__); pRedisCtx->m_netRedis.ErrorClose(); return);
    pRedisCtx->m_netRedis.CheckConnect();
}

bool CCoroutineCtx_CRedis::ReceiveRedisReply(ctx_message* pMsg, CCtx_CorutinePlusThreadData* pData){
    KernelRequestStoreData* pRequestStore = m_vtRequest.FrontData();
    IsErrorHapper(pRequestStore, CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) ReceiveRedisReply NoRequest, Close", __FUNCTION__, __FILE__, __LINE__); m_netRedis.ErrorClose(); return false);
    IsErrorHapper(WaitResumeCoroutineCtx(pRequestStore->m_pCorutine, this, pData, pMsg), ASSERT(0); CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) WaitResumeCoroutineCtx Fail", __FUNCTION__, __FILE__, __LINE__); return false);
    return true;
}

void CCoroutineCtx_CRedis::ReceiveRedisDisconnect(ctx_message* pMsg, CCtx_CorutinePlusThreadData* pData){
    //发送所有redis消息断开了
    m_vtRequest.ForeachData([&](KernelRequestStoreData* pRequest)->void{
        WaitResumeCoroutineCtxFail(pRequest->m_pCorutine, pData);
    });
    if (m_vtRequest.GetMQLength() != 0){
        ASSERT(0);
        CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) vtrequest no empty", __FUNCTION__, __FILE__, __LINE__);
        m_vtRequest.Drop_Queue([&](KernelRequestStoreData* pRequest)->void{});
    }
}

void CCoroutineCtx_CRedis::Corutine_OnIdleSendPing(CCorutinePlus* pCorutine)
{
    //外部变量一定要谨慎，线程安全问题
    CCoroutineCtx* pCurrentCtx = nullptr;
    CCorutinePlusThreadData* pCurrentData = nullptr;
    ctx_message* pCurrentMsg = nullptr;
    {
        int nGetParamCount = 0;
        void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
        IsErrorHapper(nGetParamCount == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV(pCurrentData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nGetParamCount); return);
        CCoroutineCtx_CRedis* pRedis = (CCoroutineCtx_CRedis*)pGetParam[0];

        CRedisSendPacket sendPacket;
        basiclib::CNetBasicValue setValue;
        setValue.SetString("PING");
        sendPacket.PushBasicValue(setValue);

        CRedisReplyPacket replyPacket;
        IsErrorHapper(CCFrameRedisRequest(pRedis->GetCtxID(), pRedis->GetCtxID(), pCorutine, sendPacket, replyPacket), return);
        redisReply* pReply = replyPacket.GetReply();
        IsErrorHapper(pReply->type == REDIS_REPLY_STATUS, CCFrameSCBasicLogEventErrorV(pCurrentData, "%s(%s:%d) Idle Receive No REDIS_REPLY_STATUS %d, Close", __FUNCTION__, __FILE__, __LINE__, pReply->type); pRedis->m_netRedis.ErrorClose(); return);
        IsErrorHapper(strcmp(pReply->str, "PONG") == 0, CCFrameSCBasicLogEventErrorV(pCurrentData, "%s(%s:%d) Idle Receive No PONG, Close", __FUNCTION__, __FILE__, __LINE__); pRedis->m_netRedis.ErrorClose(); return);
    }
}

void CCoroutineCtx_CRedis::OnNetIdle(){
    //15s 如果没有发送数据, 就发送ping命令
    const int nSetParam = 1;
    void* pSetParam[nSetParam] = { this };
    CreateResumeCoroutineCtx(CCoroutineCtx_CRedis::Corutine_OnIdleSendPing, CCtx_ThreadPool::GetOrCreateSelfThreadData(), 0, nSetParam, pSetParam);
}

void CCoroutineCtx_CRedis::OnNetVerifySuccess(){

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CCoroutineCtx_CRedisWatch)

CCoroutineCtx_CRedisWatch::CCoroutineCtx_CRedisWatch(const char* pKeyName, const char* pClassName) : CCoroutineCtx_RedisInterface(pKeyName, pClassName)
{
    m_ctxInsCache.SetDataLength(10240);
    m_ctxInsCache.SetDataLength(0);
}

CCoroutineCtx_CRedisWatch::~CCoroutineCtx_CRedisWatch()
{

}

int CCoroutineCtx_CRedisWatch::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
    IsErrorHapper(nRet == 0, return nRet);

    char szBuf[64] = { 0 };
    sprintf(szBuf, "%s_Address", m_pCtxName);
    const char* pAddress = func(InitGetParamType_Config, szBuf, "");
    if (pAddress == nullptr || strlen(pAddress) == 0){
        return 2;
    }
    sprintf(szBuf, "%s_AddressPwd", m_pCtxName);
    const char* pPwd = func(InitGetParamType_Config, szBuf, "");
    sprintf(szBuf, "%s_AddressDB", m_pCtxName);
    const char* pDB = func(InitGetParamType_Config, szBuf, "");
    if (!m_netRedis.InitNet(this, pAddress, pPwd, pDB)){
        return 2;
    }
    //1s检查一次
    CoroutineCtxAddOnTimer(m_ctxID, 100, OnTimer);
    return 0;
}

//! 不需要自己delete，只要调用release
void CCoroutineCtx_CRedisWatch::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimer);
    CCoroutineCtx::ReleaseCtx();
}


//! 协程里面调用Bussiness消息
int CCoroutineCtx_CRedisWatch::DispathBussinessMsg(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    switch (nType){
    case 0:
        return DispathBussinessMsg_0_subscribe(pCorutine, pData, nParam, pParam);
    case 1:
        return DispathBussinessMsg_1_psubscribe(pCorutine, pData, nParam, pParam);
    case 2:
        return DispathBussinessMsg_2_unsubscribe(pCorutine, pData, nParam, pParam);
    case 3:
        return DispathBussinessMsg_3_punsubscribe(pCorutine, pData, nParam, pParam);
    default:
        CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
        break;
    }
    return -99;
}

//! 发送请求
int CCoroutineCtx_CRedisWatch::DispathBussinessMsg_0_subscribe(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCoroutineCtx* pCurrentCtx = nullptr;
    CCorutinePlusThreadData* pCurrentData = nullptr;
    ctx_message* pCurrentMsg = nullptr;
    if (!m_netRedis.IsTransmit())
        return -2;
    basiclib::CBasicString* pStrSend = (basiclib::CBasicString*)pParam[0];
    if (m_mapSub.find(*pStrSend) != m_mapSub.end()){
        //已经存在
        return -3;
    }
    //发送
    CRedisSendPacket sendPacket;
    basiclib::CNetBasicValue setValue[2];
    setValue[0].SetString("SUBSCRIBE");
    setValue[1].SetString(pStrSend->c_str());
    sendPacket.PushBasicValue(setValue[0]);
    sendPacket.PushBasicValue(setValue[1]);
    RedisWatchStore& storeWatch = m_mapSub[*pStrSend];
    storeWatch.m_nSourceCtxID = *(uint32_t*)pParam[1];
    storeWatch.m_nType = *(uint32_t*)pParam[2];

    sendPacket.DataToSmartBuffer(m_ctxInsCache);
    m_netRedis.SendRedisData(m_ctxInsCache);
    return 0;
}

int CCoroutineCtx_CRedisWatch::DispathBussinessMsg_1_psubscribe(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCoroutineCtx* pCurrentCtx = nullptr;
    CCorutinePlusThreadData* pCurrentData = nullptr;
    ctx_message* pCurrentMsg = nullptr;
    if (!m_netRedis.IsTransmit())
        return -2;
    basiclib::CBasicString* pStrSend = (basiclib::CBasicString*)pParam[0];
    if (m_mapPSub.find(*pStrSend) != m_mapPSub.end()){
        //已经存在
        return -3;
    }
    //发送
    CRedisSendPacket sendPacket;
    basiclib::CNetBasicValue setValue[2];
    setValue[0].SetString("PSUBSCRIBE");
    setValue[1].SetString(pStrSend->c_str());
    sendPacket.PushBasicValue(setValue[0]);
    sendPacket.PushBasicValue(setValue[1]);
    RedisWatchStore& storeWatch = m_mapPSub[*pStrSend];
    storeWatch.m_nSourceCtxID = *(uint32_t*)pParam[1];
    storeWatch.m_nType = *(uint32_t*)pParam[2];

    sendPacket.DataToSmartBuffer(m_ctxInsCache);
    m_netRedis.SendRedisData(m_ctxInsCache);
    return 0;
}
int CCoroutineCtx_CRedisWatch::DispathBussinessMsg_2_unsubscribe(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCoroutineCtx* pCurrentCtx = nullptr;
    CCorutinePlusThreadData* pCurrentData = nullptr;
    ctx_message* pCurrentMsg = nullptr;
    if (!m_netRedis.IsTransmit())
        return -2;
    basiclib::CBasicString* pStrSend = (basiclib::CBasicString*)pParam[0];
    if (m_mapSub.find(*pStrSend) == m_mapSub.end()){
        //不存在
        return -3;
    }
    //发送
    CRedisSendPacket sendPacket;
    basiclib::CNetBasicValue setValue[2];
    setValue[0].SetString("UNSUBSCRIBE");
    setValue[1].SetString(pStrSend->c_str());
    sendPacket.PushBasicValue(setValue[0]);
    sendPacket.PushBasicValue(setValue[1]);

    sendPacket.DataToSmartBuffer(m_ctxInsCache);
    m_netRedis.SendRedisData(m_ctxInsCache);
    return 0;
}

int CCoroutineCtx_CRedisWatch::DispathBussinessMsg_3_punsubscribe(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
    CCoroutineCtx* pCurrentCtx = nullptr;
    CCorutinePlusThreadData* pCurrentData = nullptr;
    ctx_message* pCurrentMsg = nullptr;
    if (!m_netRedis.IsTransmit())
        return -2;
    basiclib::CBasicString* pStrSend = (basiclib::CBasicString*)pParam[0];
    if (m_mapPSub.find(*pStrSend) == m_mapPSub.end()){
        //不存在
        return -3;
    }
    //发送
    CRedisSendPacket sendPacket;
    basiclib::CNetBasicValue setValue[2];
    setValue[0].SetString("PUNSUBSCRIBE");
    setValue[1].SetString(pStrSend->c_str());
    sendPacket.PushBasicValue(setValue[0]);
    sendPacket.PushBasicValue(setValue[1]);

    sendPacket.DataToSmartBuffer(m_ctxInsCache);
    m_netRedis.SendRedisData(m_ctxInsCache);
    return 0;
}

void CCoroutineCtx_CRedisWatch::OnTimer(CCoroutineCtx* pCtx, CCtx_CorutinePlusThreadData* pData)
{
    CCoroutineCtx_CRedisWatch* pRedisCtx = (CCoroutineCtx_CRedisWatch*)pCtx;
    pRedisCtx->m_netRedis.CheckConnect();
}

void CCoroutineCtx_CRedisWatch::Corutine_Message(CCorutinePlus* pCorutine){
    int nGetParamCount = 0;
    void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
    redisReply* pReply = (redisReply*)pGetParam[0];
    RedisWatchStore* pWatch = (RedisWatchStore*)pGetParam[1];

    void* pReplyPacket = nullptr;
    int nRetValue = 0;
    WaitExeCoroutineToCtxBussiness(pCorutine, 0, pWatch->m_nSourceCtxID, pWatch->m_nType, nGetParamCount, pGetParam, pReplyPacket, nRetValue);
    freeReplyObject(pReply);
}

bool CCoroutineCtx_CRedisWatch::ReceiveRedisReply(ctx_message* pMsg, CCtx_CorutinePlusThreadData* pData){
    redisReply* pReply = (redisReply*)pMsg->m_session;
    if (pReply->type == REDIS_REPLY_STATUS){
        //! ping返回,不做处理
    }
    else if (pReply->type == REDIS_REPLY_ARRAY){
        IsErrorHapper(pReply->elements > 0, ASSERT(0); CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) reply aysize error(%d)", __FUNCTION__, __FILE__, __LINE__, pReply->elements); m_netRedis.ErrorClose(); return false);
        redisReply* pMessage = pReply->element[0];
        IsErrorHapper(pMessage->type == REDIS_REPLY_STRING, CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) reply messagetype error(%d)", __FUNCTION__, __FILE__, __LINE__, pMessage->type); m_netRedis.ErrorClose(); return false);
        if (strcmp(pMessage->str, "message") == 0){
            MapRequest::iterator iter = m_mapSub.find(pReply->element[1]->str);
            if (iter->second.m_bInitSuccess){
                const int nSetParam = 2;
                void* pSetParam[nSetParam] = { pReply, &iter->second };
                CreateResumeCoroutineCtx(CCoroutineCtx_CRedisWatch::Corutine_Message, pData, m_ctxID, nSetParam, pSetParam);
            }
            else{
                CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) Redis message noinit(%s)", __FUNCTION__, __FILE__, __LINE__, pReply->element[1]->str);
            }
        }
        else if (strcmp(pMessage->str, "pmessage") == 0){
            MapRequest::iterator iter = m_mapPSub.find(pReply->element[1]->str);
            if (iter->second.m_bInitSuccess){
                const int nSetParam = 2;
                void* pSetParam[nSetParam] = { pReply, &iter->second };
                CreateResumeCoroutineCtx(CCoroutineCtx_CRedisWatch::Corutine_Message, pData, m_ctxID, nSetParam, pSetParam);
            }
            else{
                CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) Redis pmessage noinit(%s)", __FUNCTION__, __FILE__, __LINE__, pReply->element[1]->str);
            }
        }
        else if (strcmp(pMessage->str, "subscribe") == 0){
            MapRequest::iterator iter = m_mapSub.find(pReply->element[1]->str);
            if (iter != m_mapSub.end()){
                iter->second.m_bInitSuccess = true;
            }
            else{
                CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) Redis subscribe no(%s)", __FUNCTION__, __FILE__, __LINE__, pReply->element[1]->str);
            }
        }
        else if (strcmp(pMessage->str, "psubscribe") == 0){
            MapRequest::iterator iter = m_mapPSub.find(pReply->element[1]->str);
            if (iter != m_mapPSub.end()){
                iter->second.m_bInitSuccess = true;
            }
            else{
                CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) Redis psubscribe no(%s)", __FUNCTION__, __FILE__, __LINE__, pReply->element[1]->str);
            }
        }
        else if (strcmp(pMessage->str, "unsubscribe") == 0){
            m_mapSub.erase(pReply->element[1]->str);
        }
        else if (strcmp(pMessage->str, "punsubscribe") == 0){
            m_mapPSub.erase(pReply->element[1]->str);
        }
        else{
            CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) unknowmessage(%s)", __FUNCTION__, __FILE__, __LINE__, pMessage->str);
        }
    }
    else{
        CCFrameSCBasicLogEventErrorV(pData, "%s(%s:%d) reply type error(%d)", __FUNCTION__, __FILE__, __LINE__, pReply->type);
        m_netRedis.ErrorClose();
    }
    return false;
}

void CCoroutineCtx_CRedisWatch::ReceiveRedisDisconnect(ctx_message* pMsg, CCtx_CorutinePlusThreadData* pData){
    //! disconnect 不做
}

void CCoroutineCtx_CRedisWatch::OnNetIdle(){

}

void CCoroutineCtx_CRedisWatch::OnNetVerifySuccess(){

}


bool CCFrameRedisRequest(uint32_t nResCtxID, uint32_t nRedisCtxID, CCorutinePlus* pCorutine, CRedisSendPacket& request, CRedisReplyPacket& replyPacket){
    const int nSetParam = 1;
    void* pSetParam[nSetParam] = { &request };
    int nRetValue = 0;
    if (!WaitExeCoroutineToCtxBussiness(pCorutine, nResCtxID, nRedisCtxID, 0, nSetParam, pSetParam, &replyPacket, nRetValue)){
        ASSERT(0);
        CCFrameSCBasicLogEventErrorV(nullptr, "%s(%s:%d) WaitExeCoroutineToCtxBussiness FAIL ret(%d)", __FUNCTION__, __FILE__, __LINE__, nRetValue);
        return false;
    }
    return true;
}

