#ifndef SCBASIC_CCFRAME_SERVER_FRAME_H
#define SCBASIC_CCFRAME_SERVER_FRAME_H

#include "ccframeserver.h"

class _SKYNET_KERNEL_DLL_API CCFrameServer_Frame : public basiclib::CBasicObject
{
public:
    CCFrameServer_Frame();
    virtual ~CCFrameServer_Frame();

    //! 初始化
    virtual bool InitServer(const char* pKey, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! 开始服务
    virtual void StartServer(basiclib::CBasicPreSend* pPreSend = nullptr);

    //! 获取server
    CCFrameServer* GetServer(){ return m_pServer; }

	//! 获取gatectxid
	uint32_t GetGateCtxID() { return m_nCtxIDGate; }
protected:
    virtual CCFrameServer* CreateInitServer(const char* pKey, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);
	virtual CCFrameServer* CreateServer();
protected:
	uint32_t					m_nCtxIDGate;
    const char*                 m_pListenAddress;
    const char*                 m_pIPTrust;
    CCFrameServer*              m_pServer;
};

#endif