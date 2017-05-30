#ifndef SKYNETPLUS_INTERATIONCENTER_H
#define SKYNETPLUS_INTERATIONCENTER_H

#include "../kernel/ccframe/initbussinesshead.h"
#include "../kernel/ccframe/coroutinedata/coroutinestackdata.h"
#include "../kernel/ccframe/net/ccframeserver_frame.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

#define DEFINEDISPATCH_CORUTINE_InterationCenterCtx_Packet          0
#define DEFINEDISPATCH_CORUTINE_InterationCenterCtx_GetByID         1
#define DEFINEDISPATCH_CORUTINE_InterationCenterCtx_SelfCtxRegister 2
#define DEFINEDISPATCH_CORUTINE_InterationCenterCtx_SelfCtxDelete   3
struct InterationCenterCtxStoreInfo{
    uint32_t                m_nSelfCtxID;
    basiclib::CBasicString  m_strServerKey;
    uint32_t                m_nSessionID;
    InterationCenterCtxStoreInfo(){
        m_nSelfCtxID = 0;
        m_nSessionID = 0;
    }
};

typedef basiclib::basic_map<uint32_t, ServerCommuSerialize>  MapSerialize;//type->func
typedef basiclib::basic_map<uint32_t, MapSerialize>     MapCtxIDToMap;//ctxid->map
class _SKYNET_BUSSINESS_DLL_API CInterationCenterCtx : public CCoroutineCtx
{
public:
    static CInterationCenterCtx* GetInterationCenterCtx();
public:
	CInterationCenterCtx(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CInterationCenterCtx));
	virtual ~CInterationCenterCtx();

	CreateTemplateHeader(CInterationCenterCtx);

	virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

    //! 共享网络层
    CCFrameServer_Frame& GetServer(){ return m_server; }
    //! 下层应用注册序列化
    MapCtxIDToMap& GetRegister(){ return m_mapRegister; }
    //! 启动的时候注册
    void RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func);
protected:
    long DispathBussinessMsg_Packet(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_GetByID(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_SelfCtxRegister(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_SelfCtxDelete(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam, void* pRetPacket);
protected:
    virtual int32_t OnReceive(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
    virtual int32_t OnDisconnect(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode);
protected:
    CCFrameServer_Frame                         m_server;
protected:
    //ctx内部安全
    basiclib::CBasicBitstream                   m_insCtxSafe;
    typedef basiclib::basic_map<Net_UChar, InterationCenterCtxStoreInfo>   MapGlobalKeyToSessionCtxID;
    MapGlobalKeyToSessionCtxID                  m_mapGlobalKeyToSelfCtxID;

    typedef basiclib::basic_map<basiclib::CBasicString, uint32_t>   MapGlobalNameToCtxID;
    MapGlobalNameToCtxID                        m_mapNameToCtxID;

    MapCtxIDToMap                               m_mapRegister;
};

//
bool CCFrameGetInterationCenterCtxStoreInfoByCIndex(uint32_t nResCtxID, CCorutinePlus* pCorutine, Net_UChar& cIndex, InterationCenterCtxStoreInfo& replyPacket);

#pragma warning (pop)

#endif
