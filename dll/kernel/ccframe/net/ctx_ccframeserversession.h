#ifndef SCBASIC_CCFRAME_SERVERSESSION_H
#define SCBASIC_CCFRAME_SERVERSESSION_H

#include "ccframeserver.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

class _SKYNET_KERNEL_DLL_API CCoroutineCtx_CCFrameServerSession : public CCoroutineCtx
{
public:
    CCoroutineCtx_CCFrameServerSession(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx_CCFrameServerSession));
    virtual ~CCoroutineCtx_CCFrameServerSession();

    CreateTemplateHeader(CCoroutineCtx_CCFrameServerSession);

    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);
protected:
    //使用之前需要判断自己是否 IsRelease()
    RefCCFrameServerSession m_pSession;
};

#pragma warning (pop)

#endif

