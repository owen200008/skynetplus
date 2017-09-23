#ifndef SCBASIC_CCFRAME_SERVERSESSION_H
#define SCBASIC_CCFRAME_SERVERSESSION_H

#include "ccframeserver.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

#define CCoroutineCtx_CCFrameServerSession_Init				0
#define CCoroutineCtx_CCFrameServerSession_SessionRelease	1
#define CCoroutineCtx_CCFrameServerSession_ReceiveData		2
class _SKYNET_KERNEL_DLL_API CCoroutineCtx_CCFrameServerSession : public CCoroutineCtx
{
public:
    CCoroutineCtx_CCFrameServerSession(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx_CCFrameServerSession));
    virtual ~CCoroutineCtx_CCFrameServerSession();

    CreateTemplateHeader(CCoroutineCtx_CCFrameServerSession);

    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

	//! 协程里面调用Bussiness消息
	virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket);
protected:
	////////////////////DispathBussinessMsg
	//! 发送请求
	virtual long DispathBussinessMsg_0_Init(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	virtual long DispathBussinessMsg_1_SessionRelease(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	virtual long DispathBussinessMsg_2_ReceiveData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
    //使用之前需要判断自己是否 IsRelease()
    RefCCFrameServerSession m_pSession;
};

#pragma warning (pop)

#endif

