#ifndef SKYNETPLUS_INTERATIONCENTER_H
#define SKYNETPLUS_INTERATIONCENTER_H

#include "../kernel/ccframe/initbussinesshead.h"
#include "../kernel/ccframe/coroutinedata/coroutinestackdata.h"
#include "../kernel/ccframe/net/ccframeserver_frame.h"
#include "ctx_gateinteration.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

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

    //! 共享网络层
    CCFrameServer_Frame& GetServer(){ return m_server; }
    //! 下层应用注册序列化
    MapCtxIDToMap& GetRegister(){ return m_mapRegister; }
    //! 启动的时候注册
    void RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func);
protected:
    CCFrameServer_Frame                         m_server;
protected:
    MapCtxIDToMap                               m_mapRegister;
};

//
bool CCFrameGetInterationCenterCtxStoreInfoByCIndex(uint32_t nResCtxID, CCorutinePlus* pCorutine, Net_UChar& cIndex, InterationCenterCtxStoreInfo& replyPacket);

#pragma warning (pop)

#endif
