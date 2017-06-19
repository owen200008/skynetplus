#ifndef SCBASIC_CCFRAME_REDIS_H
#define SCBASIC_CCFRAME_REDIS_H

#include "../../ccframe/dllmodule.h"
#include "../../kernel_head.h"
#include "../scbasic/commu/basicclient.h"
#include "../scbasic/redis/redisprotocal.h"
#include "../public/kernelrequeststoredata.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

//定义redis请求的返回值
enum RedisRequestCallRetType{
    RedisRequestCallRetType_ResumeFail = -99,
    RedisRequestCallRetType_TimeOut = -2,
    RedisRequestCallRetType_NoConnect = -1,
    RedisRequestCallRetType_Success = 0,
};

struct redisReader;
class _SKYNET_KERNEL_DLL_API CCoroutineCtx_RedisInterface : public CCoroutineCtx
{
public:
    CCoroutineCtx_RedisInterface(const char* pName, const char* pClassName) : CCoroutineCtx(pName, pClassName){}
    virtual ~CCoroutineCtx_RedisInterface(){}

    virtual bool ReceiveRedisReply(ctx_message* pMsg) = 0;
    virtual void ReceiveRedisDisconnect(ctx_message* pMsg) = 0;
public:
    virtual void OnNetVerifySuccess() = 0;
    virtual void OnNetIdle() = 0;
};

class _SKYNET_KERNEL_DLL_API CNet_RedisInterface : basiclib::CBasicObject
{
public:
    CNet_RedisInterface();
    virtual ~CNet_RedisInterface();

    //! 初始化
    bool InitNet(CCoroutineCtx_RedisInterface* pInterface, const char* pAddress, const char* pPwd, const char* pDB = nullptr);
    //! 判断是否连接，如果没有连接发起连接
    void CheckConnect();
    //! 错误关闭函数
    void ErrorClose();
    //! 判断是否已经认证成功
    bool IsTransmit();
    //! 发送数据
    void SendRedisData(basiclib::CBasicSmartBuffer& smBuf);
protected:
    int32_t OnConnect(basiclib::CBasicSessionNetClient* pClient, uint32_t nCode);
    int32_t OnReceiveVerify(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
    int32_t OnReceiveVerifyDB(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
    int32_t OnReceive(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
    int32_t OnDisconnect(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode);
    int32_t OnIdle(basiclib::CBasicSessionNetClient*, uint32_t);
protected:
    bool CheckVerifyPwd(basiclib::CBasicSessionNetClient* pNotify, basiclib::CBasicSmartBuffer& smBuf);
    bool CheckVerifyDB(basiclib::CBasicSessionNetClient* pNotify, basiclib::CBasicSmartBuffer& smBuf);
protected:
    //! 执行函数
    static void Func_ReceiveRedisReply(CCoroutineCtx* pCtx, ctx_message* pMsg);
    static void Func_ReceiveRedisDisconnect(CCoroutineCtx* pCtx, ctx_message* pMsg);
protected:
    void OnNetSuccessVerify();
protected:
    CCoroutineCtx_RedisInterface*   m_pInterface;
    //网络层的smartbuffer,线程安全，只能在网络层的线程中使用
    basiclib::CBasicSmartBuffer     m_netBuffCache;
    redisReader*                    m_pRedisReader;

    CCommonClientSession*           m_pClient;
    basiclib::CBasicString          m_strAddress;
    basiclib::CBasicString          m_strPwd;
    basiclib::CBasicString          m_strDB;
};

class _SKYNET_KERNEL_DLL_API CCoroutineCtx_CRedis : public CCoroutineCtx_RedisInterface
{
public:
    //必须是全局不析构的字符串
    CCoroutineCtx_CRedis(const char* pKeyName = "redis", const char* pClassName = GlobalGetClassName(CCoroutineCtx_CRedis));
    virtual ~CCoroutineCtx_CRedis();

    CreateTemplateHeader(CCoroutineCtx_CRedis);

    //! 初始化0代表成功
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! 不需要自己delete，只要调用release
    virtual void ReleaseCtx();

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

    ////////////////////////////////////////////////////////////////////////////////////////
    //业务类, 全部使用静态函数, 这样可以保证动态库函数可以替换,做到动态更新
    static void OnTimer(CCoroutineCtx* pCtx);
protected:
    ////////////////////DispathBussinessMsg
    //! 发送请求
    long DispathBussinessMsg_0_SendRequest(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_1_SendRequest(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
    //! 创建协程
    static void Corutine_OnIdleSendPing(CCorutinePlus* pCorutine);
protected:
    virtual bool ReceiveRedisReply(ctx_message* pMsg);
    virtual void ReceiveRedisDisconnect(ctx_message* pMsg);
protected:
    virtual void OnNetIdle();
    virtual void OnNetVerifySuccess();
protected:
    CNet_RedisInterface         m_netRedis;
    basiclib::CBasicBitstream   m_ctxInsCache;
    //在work线程安全
    MsgQueueRequestStoreData    m_vtRequest;
    int                         m_nDefaultTimeoutRequest;
};

struct RedisWatchStore{
    uint32_t        m_nSourceCtxID;
    uint32_t        m_nType;
    bool            m_bInitSuccess;
    RedisWatchStore(){
        memset(this, 0, sizeof(RedisWatchStore));
    }
};
class _SKYNET_KERNEL_DLL_API CCoroutineCtx_CRedisWatch : public CCoroutineCtx_RedisInterface
{
public:
    //必须是全局不析构的字符串
    CCoroutineCtx_CRedisWatch(const char* pKeyName = "rediswatch", const char* pClassName = GlobalGetClassName(CCoroutineCtx_CRedisWatch));
    virtual ~CCoroutineCtx_CRedisWatch();

    CreateTemplateHeader(CCoroutineCtx_CRedisWatch);

    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! 不需要自己delete，只要调用release
    virtual void ReleaseCtx();

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

    ////////////////////////////////////////////////////////////////////////////////////////
    //业务类, 全部使用静态函数, 这样可以保证动态库函数可以替换,做到动态更新
    static void OnTimer(CCoroutineCtx* pCtx);
protected:
    ////////////////////DispathBussinessMsg
    //! 发送请求 订阅
    //! 1:basiclib::CBasicString 2:sourcectxid 3:nType
    int DispathBussinessMsg_0_subscribe(CCorutinePlus* pCorutine, int nParam, void** pParam);
    //! 1:basiclib::CBasicString 2:sourcectxid 3:nType
    int DispathBussinessMsg_1_psubscribe(CCorutinePlus* pCorutine, int nParam, void** pParam);
    //! param1 basiclib::CBasicString
    int DispathBussinessMsg_2_unsubscribe(CCorutinePlus* pCorutine, int nParam, void** pParam);
    //! param1 basiclib::CBasicString
    int DispathBussinessMsg_3_punsubscribe(CCorutinePlus* pCorutine, int nParam, void** pParam);
protected:
    static void Corutine_Message(CCorutinePlus* pCorutine);
    static void Corutine_PMessage(CCorutinePlus* pCorutine);
protected:
    virtual bool ReceiveRedisReply(ctx_message* pMsg);
    virtual void ReceiveRedisDisconnect(ctx_message* pMsg);
protected:
    virtual void OnNetIdle();
    virtual void OnNetVerifySuccess();
protected:
    CNet_RedisInterface         m_netRedis;
    basiclib::CBasicBitstream   m_ctxInsCache;
    //在work线程安全
    typedef basiclib::basic_map<basiclib::CBasicString, RedisWatchStore> MapRequest;
    MapRequest                  m_mapSub;
    MapRequest                  m_mapPSub;
};

//实现全局安全的函数封装使用redis（不是watch）
_SKYNET_KERNEL_DLL_API bool CCFrameRedisRequest(uint32_t nResCtxID, uint32_t nRedisCtxID, CCorutinePlus* pCorutine, CRedisSendPacket& request, CRedisReplyPacket& replyPacket);

#pragma warning (pop)

#endif

