#ifndef SCBASIC_CCFRAME_NETCLIENTSERVERCOMMU_H
#define SCBASIC_CCFRAME_NETCLIENTSERVERCOMMU_H

#include "ctx_netclient.h"
#include "../../../skynetplusprotocal/skynetplusprotocal_function.h"
#include "../public/kernelrequeststoredata.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

#define DEFINEDISPATCH_CORUTINE_NetClient_CreateMap     10
#define DEFINEDISPATCH_CORUTINE_NetClient_DeleteMap     11
#define DEFINEDISPATCH_CORUTINE_NetClient_GetMap        12
#define DEFINEDISPATCH_CORUTINE_NetClient_Request       20
class _SKYNET_KERNEL_DLL_API CCoroutineCtx_NetClientServerCommu : public CCoroutineCtx_NetClient
{
public:
    CCoroutineCtx_NetClientServerCommu(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx_NetClientServerCommu));
    virtual ~CCoroutineCtx_NetClientServerCommu();

    CreateTemplateHeader(CCoroutineCtx_NetClientServerCommu);

    //! 初始化0代表成功
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! 不需要自己delete，只要调用release
    virtual void ReleaseCtx();

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

    //! 获取自己的服务器ID
    Net_UChar GetSelfServerID(){ return m_request.m_cIndex; }
    //! 启动的时候注册
    void RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func, bool bSelf = false);

    static void OnTimerCheckTimeout(CCoroutineCtx* pCtx);
protected:
    virtual long DispathBussinessMsg_Receive(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);
    long DispathBussinessMsg_CreateMap(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_DelMap(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_GetMap(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_Request(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
    virtual int32_t OnConnect(basiclib::CBasicSessionNetClient* pClient, uint32_t nCode);
    virtual int32_t OnIdle(basiclib::CBasicSessionNetClient*, uint32_t);
    virtual int32_t OnDisconnect(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode);
protected:
    static void Func_ReceiveCommuDisconnect(CCoroutineCtx* pCtx, ctx_message* pMsg);
protected:
    //! 创建协程
    static void Corutine_ReceiveRequest(CCorutinePlus* pCorutine);
protected:
    void ReceiveDisconnect(ctx_message* pMsg);
protected:
    bool                                                m_bVerifySuccess;
    SkynetPlusRegister                                  m_request;
    SkynetPlusPing                                      m_ping;
    basiclib::CBasicBitstream                           m_insServer;

    MapNameToCtxID                                      m_map;
    basiclib::CBasicBitstream                           m_ins;
    //请求串
    MsgQueueRequestStoreData                            m_vtRequest;//get
    int                                                 m_nDefaultTimeoutRequest;

    MapKernelRequestStoreData                           m_mapRequest;//request

    typedef basiclib::basic_map<uint32_t, ServerCommuSerialize>  MapSerialize;//type->func
    typedef basiclib::basic_map<uint32_t, MapSerialize>     MapCtxIDToMap;//ctxid->map
    MapCtxIDToMap                                       m_mapRegister;
};

#pragma warning (pop)

#endif