#ifndef SCBASIC_CCFRAME_LOG_H
#define SCBASIC_CCFRAME_LOG_H

#include "../dllmodule.h"

class _SKYNET_KERNEL_DLL_API CCoroutineCtx_Log : public CCoroutineCtx
{
public:
    CCoroutineCtx_Log(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx_Log));
	virtual ~CCoroutineCtx_Log();

	CreateTemplateHeader(CCoroutineCtx_Log);

    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! 不需要自己delete，只要调用release
    virtual void ReleaseCtx();

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

	//! 获取ctx
	static uint32_t GetLogCtxID();

	//! 设置可以退出的表示
	void SetCanExit() { m_bCanExit = true; }
	static bool IsCanExit();
public:
	virtual void LogEvent(int nChannel, const char* pszLog);
public:
	////////////////////////////////////////////////////////////////////////////////////////
	//业务类, 全部使用静态函数
    static void OnTimerBasicLog(CCoroutineCtx* pCtx);
protected:
    static void OnLogEventCtx(CCorutinePlus* pCorutine);
protected:
	bool	m_bCanExit;
};

//!事件记录
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventV(const char* pszLog, ...);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventErrorV(const char* pszLog, ...);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEvent(const char* pszLog);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventError(const char* pszLog);


#endif