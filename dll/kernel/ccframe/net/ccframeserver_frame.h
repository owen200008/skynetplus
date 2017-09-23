#ifndef SCBASIC_CCFRAME_SERVER_FRAME_H
#define SCBASIC_CCFRAME_SERVER_FRAME_H

#include "ccframeserver.h"

class _SKYNET_KERNEL_DLL_API CCFrameServer_Frame : public basiclib::CBasicObject
{
public:
    CCFrameServer_Frame();
    virtual ~CCFrameServer_Frame();

    //! ��ʼ��
    virtual bool InitServer(const char* pKey, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! ��ʼ����
    virtual void StartServer(basiclib::CBasicPreSend* pPreSend = nullptr);

    //! ��ȡserver
    CCFrameServer* GetServer(){ return m_pServer; }

	//! ��ȡgatectxid
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