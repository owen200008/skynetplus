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

    //! ����Ҫ�Լ�delete��ֻҪ����release
    virtual void ReleaseCtx();

    //! Э���������Bussiness��Ϣ
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

	//! ��ȡctx
	static uint32_t GetLogCtxID();

	//! ���ÿ����˳��ı�ʾ
	void SetCanExit() { m_bCanExit = true; }
	static bool IsCanExit();
public:
	virtual void LogEvent(int nChannel, const char* pszLog);
public:
	////////////////////////////////////////////////////////////////////////////////////////
	//ҵ����, ȫ��ʹ�þ�̬����
    static void OnTimerBasicLog(CCoroutineCtx* pCtx);
protected:
    static void OnLogEventCtx(CCorutinePlus* pCorutine);
protected:
	bool	m_bCanExit;
};

//!�¼���¼
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventV(const char* pszLog, ...);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventErrorV(const char* pszLog, ...);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEvent(const char* pszLog);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventError(const char* pszLog);


#endif