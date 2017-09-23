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
        m_pClient->SafeDelete();
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
    m_pClient = CCommonClientSession::CreateNetClient();
    m_pClient->bind_connect(MakeFastFunction(this, &CNet_RedisInterface::OnConnect));
    m_pClient->bind_disconnect(MakeFastFunction(this, &CNet_RedisInterface::OnDisconnect));
    m_pClient->bind_idle(MakeFastFunction(this, &CNet_RedisInterface::OnIdle));
    m_pClient->Connect(m_strAddress.c_str());
    return true;
}

bool CNet_RedisInterface::CheckVerifyPwd(basiclib::CBasicSessionNetNotify* pNotify, basiclib::CBasicSmartBuffer& smBuf){
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
bool CNet_RedisInterface::CheckVerifyDB(basiclib::CBasicSessionNetNotify* pNotify, basiclib::CBasicSmartBuffer& smBuf){
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

int32_t CNet_RedisInterface::OnConnect(CBasicSessionNetNotify* pClient, uint32_t nCode){
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
int32_t CNet_RedisInterface::OnReceiveVerify(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    IsErrorHapper(redisReaderFeed(m_pRedisReader, pszData, cbData) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis Verify Feed Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
    void* pReply = nullptr;
    IsErrorHapper(redisReaderGetReply(m_pRedisReader, &pReply) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis Verify GetReply Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);

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
            CCFrameSCBasicLogEventErrorV("Redis OnReceiveVerify Fail %s, Close", pReplyData->str);
            pNotify->Close(0);
            return BASIC_NET_GENERIC_ERROR;
        }
        freeReplyObject(pReply);
    }
    return nRet;
}
int32_t CNet_RedisInterface::OnReceiveVerifyDB(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    IsErrorHapper(redisReaderFeed(m_pRedisReader, pszData, cbData) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis VerifyDB Feed Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
    void* pReply = nullptr;
    IsErrorHapper(redisReaderGetReply(m_pRedisReader, &pReply) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis VerifyDB GetReply Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);

    int32_t nRet = BASIC_NET_OK;
    if (pReply){
        redisReply* pReplyData = (redisReply*)pReply;
        if (pReplyData->type == REDIS_REPLY_STATUS){
            OnNetSuccessVerify();
            nRet = BASIC_NET_HR_RET_HANDSHAKE;
        }
        else if (pReplyData->type == REDIS_REPLY_ERROR){
            CCFrameSCBasicLogEventErrorV("Redis OnReceiveVerifyDB Fail %s, Close", pReplyData->str);
            pNotify->Close(0);
            return BASIC_NET_GENERIC_ERROR;
        }
        freeReplyObject(pReply);
    }
    return nRet;
}
int32_t CNet_RedisInterface::OnReceive(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    IsErrorHapper(redisReaderFeed(m_pRedisReader, pszData, cbData) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis OnReceive Feed Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
    bool bContinueFindReply = false;
    do{
        bContinueFindReply = false;
        void* pReply = nullptr;
        IsErrorHapper(redisReaderGetReply(m_pRedisReader, &pReply) == REDIS_OK, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis OnReceive GetReply Error, Close", __FUNCTION__, __FILE__, __LINE__); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
        if (pReply){
            //默认第一个
            ctx_message ctxMsg(0, CNet_RedisInterface::Func_ReceiveRedisReply, (intptr_t)pReply);
            IsErrorHapper(CCoroutineCtx::PushMessageByID(m_pInterface->GetCtxID(), ctxMsg), ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis OnReceive PushMessageByID Error, Close", __FUNCTION__, __FILE__, __LINE__); freeReplyObject(pReply); pNotify->Close(0); return BASIC_NET_GENERIC_ERROR);
            bContinueFindReply = true;
        }
    } while (bContinueFindReply);

    return BASIC_NET_OK;
}
int32_t CNet_RedisInterface::OnDisconnect(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode){
    if (m_pRedisReader){
        redisReaderFree(m_pRedisReader);
        m_pRedisReader = nullptr;
    }
    //执行
    ctx_message ctxMsg(0, CNet_RedisInterface::Func_ReceiveRedisDisconnect);
    CCoroutineCtx::PushMessageByID(m_pInterface->GetCtxID(), ctxMsg);
    return BASIC_NET_OK;
}

int32_t CNet_RedisInterface::OnIdle(basiclib::CBasicSessionNetNotify*, uint32_t nIdle){
    if (nIdle % 15 == 14){
        m_pInterface->OnNetIdle();
    }
    return BASIC_NET_OK;
}

void CNet_RedisInterface::Func_ReceiveRedisReply(CCoroutineCtx* pCtx, ctx_message* pMsg){
    CCoroutineCtx_RedisInterface* pRedis = (CCoroutineCtx_RedisInterface*)pCtx;
    if (!pRedis->ReceiveRedisReply(pMsg)){
        freeReplyObject((redisReply*)pMsg->m_session);
    }
}
void CNet_RedisInterface::Func_ReceiveRedisDisconnect(CCoroutineCtx* pCtx, ctx_message* pMsg){
    CCoroutineCtx_RedisInterface* pRedis = (CCoroutineCtx_RedisInterface*)pCtx;
    pRedis->ReceiveRedisDisconnect(pMsg);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CCoroutineCtx_CRedis)

CCoroutineCtx_CRedis::CCoroutineCtx_CRedis(const char* pKeyName, const char* pClassName) : CCoroutineCtx_RedisInterface(pKeyName, pClassName){
    m_ctxInsCache.SetDataLength(10240);
    m_ctxInsCache.SetDataLength(0);
    m_nDefaultTimeoutRequest = 5;
}

CCoroutineCtx_CRedis::~CCoroutineCtx_CRedis(){
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
int CCoroutineCtx_CRedis::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket){
    switch (nType){
    case 0:
        return DispathBussinessMsg_0_SendRequest(pCorutine, nParam, pParam, pRetPacket);
    case 1:
        return DispathBussinessMsg_1_SendRequest(pCorutine, nParam, pParam, pRetPacket);
    default:
        CCFrameSCBasicLogEventErrorV("CCoroutineCtx_CRedis::DispathBussinessMsg unknow type(%d) nParam(%d)", nType, nParam);
        break;
    }
    return -99;
}

//! 发送请求
long CCoroutineCtx_CRedis::DispathBussinessMsg_0_SendRequest(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam1(CRedisSendPacket* pSendPacket, (CRedisSendPacket*));
	if(!m_netRedis.IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_netRedis.IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}
    
    CRedisReplyPacket* pReplyPacket = (CRedisReplyPacket*)pRetPacket;
    do{
        //! 保存
        CKernelMsgQueueRequestMgr requestMgr(m_vtRequest, pCorutine);

        //发送保存协程
        pSendPacket->DataToSmartBuffer(m_ctxInsCache);
        m_netRedis.SendRedisData(m_ctxInsCache);

        //截断协程由子协程唤醒(认为结束了 自己保存)
		MACRO_YieldToCtx(pCorutine, 0, 
						 return DEFINECTX_RET_TYPE_COROUTINEERR;);
		MACRO_GetYieldParam1(redisReply* pGetRedisReply, redisReply);
        pReplyPacket->BindReply(pGetRedisReply);
        if (pGetRedisReply->type == REDIS_REPLY_ERROR){
            return DEFINECTX_RET_TYPE_ReceDataErr;
        }
        pSendPacket = pSendPacket->GetNextSendPacket();
        pReplyPacket = pReplyPacket->GetNextReplyPacket();
    } while (pSendPacket && pReplyPacket);
    return DEFINECTX_RET_TYPE_SUCCESS;
}

//! 发送请求
long CCoroutineCtx_CRedis::DispathBussinessMsg_1_SendRequest(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
	MACRO_DispatchCheckParam1(basiclib::CBasicStringArray* pArrayRequest, (basiclib::CBasicStringArray*));
	if(!m_netRedis.IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_netRedis.IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}

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
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		MACRO_GetYieldParam1(redisReply* pGetRedisReply, redisReply);
        pReplyPacket->BindReply(pGetRedisReply);
        if (pGetRedisReply->type == REDIS_REPLY_ERROR){
            return DEFINECTX_RET_TYPE_ReceDataErr;
        }
        pReplyPacket = pReplyPacket->GetNextReplyPacket();
    }
    return DEFINECTX_RET_TYPE_SUCCESS;
}


void CCoroutineCtx_CRedis::OnTimer(CCoroutineCtx* pCtx){
    CCoroutineCtx_CRedis* pRedisCtx = (CCoroutineCtx_CRedis*)pCtx;
    time_t tmNow = time(NULL);
    
    KernelRequestStoreData* pRequestStore = pRedisCtx->m_vtRequest.FrontData();
    IsErrorHapper(pRequestStore == nullptr || !pRequestStore->IsTimeOut(tmNow, pRedisCtx->m_nDefaultTimeoutRequest), CCFrameSCBasicLogEventErrorV("%s(%s:%d) OnTimerRedis Timeout, Close", __FUNCTION__, __FILE__, __LINE__); pRedisCtx->m_netRedis.ErrorClose(); return);
    pRedisCtx->m_netRedis.CheckConnect();

	do{
		pRequestStore = pRedisCtx->m_vtWaitRequest.FrontData();
		if(pRequestStore && pRequestStore->IsTimeOut(tmNow, pRedisCtx->m_nDefaultTimeoutRequest)){
			MACRO_ResumeToCtx(pRequestStore->m_pCorutine, pRedisCtx, CCtx_ThreadPool::GetOrCreateSelfThreadData(),
							  CCFrameSCBasicLogEventErrorV("%s(%s:%d) OnTimerRedis timeout wait", __FUNCTION__, __FILE__, __LINE__); break);
		}
		else{
			break;
		}
	} while(pRequestStore);
}

bool CCoroutineCtx_CRedis::ReceiveRedisReply(ctx_message* pMsg){
    KernelRequestStoreData* pRequestStore = m_vtRequest.FrontData();
    IsErrorHapper(pRequestStore, CCFrameSCBasicLogEventErrorV("%s(%s:%d) ReceiveRedisReply NoRequest, Close", __FUNCTION__, __FILE__, __LINE__); m_netRedis.ErrorClose(); return false);
	MACRO_ResumeToCtxNormal(pRequestStore->m_pCorutine, this, CCtx_ThreadPool::GetOrCreateSelfThreadData(),
							return false,
							(void*)pMsg->m_session);
    return true;
}

void CCoroutineCtx_CRedis::ReceiveRedisDisconnect(ctx_message* pMsg){
    //发送所有redis消息断开了,不能保存指针不然指向同一个(最后一个)
	basiclib::basic_vector<KernelRequestStoreData> vtDeleteRequest;
	m_vtRequest.ForeachData([&](KernelRequestStoreData* pRequest)->void {
		vtDeleteRequest.push_back(*pRequest);
	});
	for (auto& deldata : vtDeleteRequest) {
		Ctx_ResumeCoroutine_Fail(deldata.m_pCorutine);
	}
    if (m_vtRequest.GetMQLength() != 0){
        ASSERT(0);
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) vtrequest no empty", __FUNCTION__, __FILE__, __LINE__);
        m_vtRequest.Drop_Queue([&](KernelRequestStoreData* pRequest)->void{});
    }
}

void CCoroutineCtx_CRedis::Corutine_OnIdleSendPing(CCorutinePlus* pCorutine){
    //外部变量一定要谨慎，线程安全问题
    {
		CCoroutineCtx_CRedis* pRedis = pCorutine->GetParamPoint<CCoroutineCtx_CRedis>(0);

        CRedisSendPacket sendPacket;
        basiclib::CNetBasicValue setValue;
        setValue.SetString("PING");
        sendPacket.PushBasicValue(setValue);

        CRedisReplyPacket replyPacket;
        IsErrorHapper(CCFrameRedisRequest(0, pRedis->GetCtxID(), pCorutine, sendPacket, replyPacket), return);
        redisReply* pReply = replyPacket.GetReply();
        IsErrorHapper(pReply->type == REDIS_REPLY_STATUS, CCFrameSCBasicLogEventErrorV("%s(%s:%d) Idle Receive No REDIS_REPLY_STATUS %d, Close", __FUNCTION__, __FILE__, __LINE__, pReply->type); pRedis->m_netRedis.ErrorClose(); return);
        IsErrorHapper(strcmp(pReply->str, "PONG") == 0, CCFrameSCBasicLogEventErrorV("%s(%s:%d) Idle Receive No PONG, Close", __FUNCTION__, __FILE__, __LINE__); pRedis->m_netRedis.ErrorClose(); return);
    }
}

void CCoroutineCtx_CRedis::OnNetIdle(){
    //15s 如果没有发送数据, 就发送ping命令
	Ctx_CreateCoroutine(0, CCoroutineCtx_CRedis::Corutine_OnIdleSendPing, this);
}

void CCoroutineCtx_CRedis::OnNetVerifySuccess(){
	//认证成功之后将等待队列的发送
	ctx_message msg(0, [](CCoroutineCtx* pCtx, ctx_message* pMsg)->void{
		CCoroutineCtx_CRedis* pRedisCtx = (CCoroutineCtx_CRedis*)pCtx;
		KernelRequestStoreData* pRequestStore = nullptr;
		do{
			pRequestStore = pRedisCtx->m_vtWaitRequest.FrontData();
			if(pRequestStore){
				MACRO_ResumeToCtx(pRequestStore->m_pCorutine, pRedisCtx, CCtx_ThreadPool::GetOrCreateSelfThreadData(),
									CCFrameSCBasicLogEventErrorV("%s(%s:%d) OnTimerMysql wait queue wakeup", __FUNCTION__, __FILE__, __LINE__); break);
			}
		} while(pRequestStore);
	});
	PushMessage(msg);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CCoroutineCtx_CRedisWatch)

CCoroutineCtx_CRedisWatch::CCoroutineCtx_CRedisWatch(const char* pKeyName, const char* pClassName) : CCoroutineCtx_RedisInterface(pKeyName, pClassName){
    m_ctxInsCache.SetDataLength(10240);
    m_ctxInsCache.SetDataLength(0);
	m_nDefaultTimeoutRequest = 5;
}

CCoroutineCtx_CRedisWatch::~CCoroutineCtx_CRedisWatch(){
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
void CCoroutineCtx_CRedisWatch::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimer);
    CCoroutineCtx::ReleaseCtx();
}


//! 协程里面调用Bussiness消息
int CCoroutineCtx_CRedisWatch::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket){
    switch (nType){
    case 0:
        return DispathBussinessMsg_0_subscribe(pCorutine, nParam, pParam);
    case 1:
        return DispathBussinessMsg_1_psubscribe(pCorutine, nParam, pParam);
    case 2:
        return DispathBussinessMsg_2_unsubscribe(pCorutine, nParam, pParam);
    case 3:
        return DispathBussinessMsg_3_punsubscribe(pCorutine, nParam, pParam);
    default:
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
        break;
    }
    return -99;
}

//! 发送请求
int CCoroutineCtx_CRedisWatch::DispathBussinessMsg_0_subscribe(CCorutinePlus* pCorutine, int nParam, void** pParam){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
	if(!m_netRedis.IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_netRedis.IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}
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

int CCoroutineCtx_CRedisWatch::DispathBussinessMsg_1_psubscribe(CCorutinePlus* pCorutine, int nParam, void** pParam){
	MACRO_DispatchCheckParam1(basiclib::CBasicString* pStrSend, (basiclib::CBasicString*))
    if (!m_netRedis.IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_netRedis.IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}
    if (m_mapPSub.find(*pStrSend) != m_mapPSub.end()){
        //已经存在
        return DEFINECTX_RET_TYPE_SUCCESS;
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
    return DEFINECTX_RET_TYPE_SUCCESS;
}
int CCoroutineCtx_CRedisWatch::DispathBussinessMsg_2_unsubscribe(CCorutinePlus* pCorutine, int nParam, void** pParam){
	MACRO_DispatchCheckParam1(basiclib::CBasicString* pStrSend, (basiclib::CBasicString*))
    if (!m_netRedis.IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_netRedis.IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}
    if (m_mapSub.find(*pStrSend) == m_mapSub.end()){
        //不存在
        return DEFINECTX_RET_TYPE_SUCCESS;
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
    return DEFINECTX_RET_TYPE_SUCCESS;
}

int CCoroutineCtx_CRedisWatch::DispathBussinessMsg_3_punsubscribe(CCorutinePlus* pCorutine, int nParam, void** pParam){
	MACRO_DispatchCheckParam1(basiclib::CBasicString* pStrSend, (basiclib::CBasicString*))
	if(!m_netRedis.IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_netRedis.IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}
    if (m_mapPSub.find(*pStrSend) == m_mapPSub.end()){
        //不存在
        return DEFINECTX_RET_TYPE_SUCCESS;
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
    return DEFINECTX_RET_TYPE_SUCCESS;
}

void CCoroutineCtx_CRedisWatch::OnTimer(CCoroutineCtx* pCtx){
    CCoroutineCtx_CRedisWatch* pRedisCtx = (CCoroutineCtx_CRedisWatch*)pCtx;
    pRedisCtx->m_netRedis.CheckConnect();
	time_t tmNow = time(nullptr);
	KernelRequestStoreData* pRequestStore = nullptr;
	do{
		pRequestStore = pRedisCtx->m_vtWaitRequest.FrontData();
		if(pRequestStore && pRequestStore->IsTimeOut(tmNow, pRedisCtx->m_nDefaultTimeoutRequest)){
			MACRO_ResumeToCtx(pRequestStore->m_pCorutine, pRedisCtx, CCtx_ThreadPool::GetOrCreateSelfThreadData(),
							  CCFrameSCBasicLogEventErrorV("%s(%s:%d) OnTimerRedis timeout wait", __FUNCTION__, __FILE__, __LINE__); break);
		}
		else{
			break;
		}
	} while(pRequestStore);
}

void CCoroutineCtx_CRedisWatch::Corutine_Message(CCorutinePlus* pCorutine){
	pCorutine->SetCurrentCtx(pCorutine->GetParamPoint<CCoroutineCtx>(2));
    redisReply* pReply = pCorutine->GetParamPoint<redisReply>(0);
    RedisWatchStore* pWatch = pCorutine->GetParamPoint<RedisWatchStore>(1);

    int nRetValue = 0;
	const int nSetParam = 2;
	void* pSetParam[nSetParam] = { pReply, pWatch };
	MACRO_ExeToCtxParam2(pCorutine, 0, pWatch->m_nSourceCtxID, pWatch->m_nType, pReply, pWatch, nullptr,
						 );
    freeReplyObject(pReply);
}

bool CCoroutineCtx_CRedisWatch::ReceiveRedisReply(ctx_message* pMsg){
    redisReply* pReply = (redisReply*)pMsg->m_session;
    if (pReply->type == REDIS_REPLY_STATUS){
        //! ping返回,不做处理
    }
    else if (pReply->type == REDIS_REPLY_ARRAY){
        IsErrorHapper(pReply->elements > 0, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) reply aysize error(%d)", __FUNCTION__, __FILE__, __LINE__, pReply->elements); m_netRedis.ErrorClose(); return false);
        redisReply* pMessage = pReply->element[0];
        IsErrorHapper(pMessage->type == REDIS_REPLY_STRING, CCFrameSCBasicLogEventErrorV("%s(%s:%d) reply messagetype error(%d)", __FUNCTION__, __FILE__, __LINE__, pMessage->type); m_netRedis.ErrorClose(); return false);
        if (strcmp(pMessage->str, "message") == 0){
            MapRequest::iterator iter = m_mapSub.find(pReply->element[1]->str);
            if (iter->second.m_bInitSuccess){
				Ctx_CreateCoroutine(m_ctxID, CCoroutineCtx_CRedisWatch::Corutine_Message, pReply, &iter->second, this);
            }
            else{
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis message noinit(%s)", __FUNCTION__, __FILE__, __LINE__, pReply->element[1]->str);
            }
        }
        else if (strcmp(pMessage->str, "pmessage") == 0){
            MapRequest::iterator iter = m_mapPSub.find(pReply->element[1]->str);
            if (iter->second.m_bInitSuccess){
				Ctx_CreateCoroutine(m_ctxID, CCoroutineCtx_CRedisWatch::Corutine_Message, pReply, &iter->second, this);
            }
            else{
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis pmessage noinit(%s)", __FUNCTION__, __FILE__, __LINE__, pReply->element[1]->str);
            }
        }
        else if (strcmp(pMessage->str, "subscribe") == 0){
            MapRequest::iterator iter = m_mapSub.find(pReply->element[1]->str);
            if (iter != m_mapSub.end()){
                iter->second.m_bInitSuccess = true;
            }
            else{
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis subscribe no(%s)", __FUNCTION__, __FILE__, __LINE__, pReply->element[1]->str);
            }
        }
        else if (strcmp(pMessage->str, "psubscribe") == 0){
            MapRequest::iterator iter = m_mapPSub.find(pReply->element[1]->str);
            if (iter != m_mapPSub.end()){
                iter->second.m_bInitSuccess = true;
            }
            else{
                CCFrameSCBasicLogEventErrorV("%s(%s:%d) Redis psubscribe no(%s)", __FUNCTION__, __FILE__, __LINE__, pReply->element[1]->str);
            }
        }
        else if (strcmp(pMessage->str, "unsubscribe") == 0){
            m_mapSub.erase(pReply->element[1]->str);
        }
        else if (strcmp(pMessage->str, "punsubscribe") == 0){
            m_mapPSub.erase(pReply->element[1]->str);
        }
        else{
            CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknowmessage(%s)", __FUNCTION__, __FILE__, __LINE__, pMessage->str);
        }
    }
    else{
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) reply type error(%d)", __FUNCTION__, __FILE__, __LINE__, pReply->type);
        m_netRedis.ErrorClose();
    }
    return false;
}

void CCoroutineCtx_CRedisWatch::ReceiveRedisDisconnect(ctx_message* pMsg){
    //! disconnect 不做
}

void CCoroutineCtx_CRedisWatch::OnNetIdle(){

}

void CCoroutineCtx_CRedisWatch::OnNetVerifySuccess(){
	//认证成功之后将等待队列的发送
	ctx_message msg(0, [](CCoroutineCtx* pCtx, ctx_message* pMsg){
		CCoroutineCtx_CRedisWatch* pRedisCtx = (CCoroutineCtx_CRedisWatch*)pCtx;
		KernelRequestStoreData* pRequestStore = nullptr;
		do{
			pRequestStore = pRedisCtx->m_vtWaitRequest.FrontData();
			if(pRequestStore){
				MACRO_ResumeToCtx(pRequestStore->m_pCorutine, pRedisCtx, CCtx_ThreadPool::GetOrCreateSelfThreadData(),
								  CCFrameSCBasicLogEventErrorV("%s(%s:%d) OnTimerMysql wait queue wakeup", __FUNCTION__, __FILE__, __LINE__); break);
			}
		} while(pRequestStore);
	});
	PushMessage(msg);
}


bool CCFrameRedisRequest(uint32_t nResCtxID, uint32_t nRedisCtxID, CCorutinePlus* pCorutine, CRedisSendPacket& request, CRedisReplyPacket& replyPacket){
	MACRO_ExeToCtxParam1(pCorutine, nResCtxID, nRedisCtxID, 0, &request, &replyPacket,
						 return false);
    return true;
}

