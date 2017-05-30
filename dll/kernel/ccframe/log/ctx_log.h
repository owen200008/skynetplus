#ifndef SCBASIC_CCFRAME_LOG_H
#define SCBASIC_CCFRAME_LOG_H

#include "../dllmodule.h"

class CCorutinePlusThreadData;
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
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

    //! �Ƿ����
    static bool IsExist();
public:
	void LogEvent(CCorutinePlusThreadData* pThreadData, int nChannel, const char* pszLog);
public:
	////////////////////////////////////////////////////////////////////////////////////////
	//ҵ����, ȫ��ʹ�þ�̬����
    static void OnTimerBasicLog(CCoroutineCtx* pCtx, CCtx_CorutinePlusThreadData* pData);
protected:
    static void OnLogEventCtx(CCorutinePlus* pCorutine);
};

//!�¼���¼
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventV(CCorutinePlusThreadData* pThreadData, const char* pszLog, ...);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventErrorV(CCorutinePlusThreadData* pThreadData, const char* pszLog, ...);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEvent(CCorutinePlusThreadData* pThreadData, const char* pszLog);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventError(CCorutinePlusThreadData* pThreadData, const char* pszLog);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEvent(const char* pszLog);
_SKYNET_KERNEL_DLL_API void CCFrameSCBasicLogEventError(const char* pszLog);


#endif