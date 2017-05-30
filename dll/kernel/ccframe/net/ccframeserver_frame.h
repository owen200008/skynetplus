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

    //! ��֤ͨ������serversession
    virtual bool VerifySuccessCreateServerSession(basiclib::CBasicSessionNetClient* pClient);

    //! ��ȡserver
    CCFrameServer* GetServer(){ return m_pServer; }
protected:
    virtual CCFrameServer* CreateServer(const char* pKey, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);
protected:
    const char*                 m_pListenAddress;
    const char*                 m_pIPTrust;
    CCFrameServer*              m_pServer;
    const char*                 m_pServerSessionCtx;
};

#endif