/***********************************************************************************************
// 文件名:     ctx_msgqueue.h
// 创建者:     蔡振球
// Email:      zqcai@w.cn
// 内容描述:   用于协程和异步的协程池
// 版本信息:   1.0V
************************************************************************************************/

#ifndef SCBASIC_CTX_THREADPOOL_H
#define SCBASIC_CTX_THREADPOOL_H

#include "ctx_handle.h"
#include "dllmodule.h"
#include "log/ctx_log.h"
#include "../scbasic/encode/rsaencode.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

class _SKYNET_KERNEL_DLL_API CCtx_ThreadPool : public basiclib::CBasicObject
{
public:
	static CCtx_ThreadPool* GetThreadPoolOrCreate();
	static CCtx_ThreadPool* GetThreadPool();
	static CCorutinePlusThreadData* GetOrCreateSelfThreadData();
public:
	std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)> m_defaultFuncGetConfig;
public:
	CCtx_ThreadPool();
	virtual ~CCtx_ThreadPool();

	//! 初始化
	virtual bool Init();
	//! 等待退出
	void Wait();
	//! 主动设置退出
	void SetClose();
	//获取默认的dll
	uint32_t GetDefaultHttp() { return m_nDefaultHttpCtxID; }
	const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& GetDefaultConfigFunc(){ return m_defaultFuncGetConfig; }
public:
    CBasicOnTimer& GetOnTimerModule(){ return m_ontimerModule; }
	CMQMgr& GetGlobalMQMgr(){ return m_globalMQMgrModule; }
	CXKRSAManager& GetRSAMgr() { return m_rsaMgr; }
	bool GetSetClose() { return m_bSetClose; }
public:
    //创建ctx相关
	CDllRegisterCtxTemplateMgr& GetCtxTemplateRegister(){ return m_mgtDllCtx; }
    uint32_t CreateTemplateObjectCtx(const char* pName, const char* pKeyName, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func, CMQMgr* pMQMgr = nullptr);
    void ReleaseObjectCtxByCtxID(uint32_t nCtxID);
protected:
	friend THREAD_RETURN ThreadOnWork(void*);
	
	friend THREAD_RETURN ThreadOnMonitor(void*);
	void ExecThreadOnMonitor();
	friend void ExecThreadOnWork(CCtx_ThreadPool* pThreadPool);
protected:
	bool														m_bSetClose;
	bool														m_bRunning;
	CBasicOnTimer												m_ontimerModule;			//执行ontimer callback
	basiclib::basic_vector<HANDLE>								m_vtHandle;
	HANDLE														m_hMonitor;

	CMQMgr														m_globalMQMgrModule;
	uint32_t													m_nDPacketNumPerTime;

	//ctxhandle
	CCoroutineCtxHandle											m_gHandle;
	//ctx管理
	CDllRegisterCtxTemplateMgr									m_mgtDllCtx;
	//rsa
	CXKRSAManager												m_rsaMgr;
	//默认的http服务
	uint32_t													m_nDefaultHttpCtxID;

	//存放线程数据
	basiclib::CBasicThreadTLS									m_tls;
protected:
	//不会释放的ctx
	CCoroutineCtx_Log*											m_pLog;

    friend class CCoroutineCtxHandle;
	friend class CCoroutineCtx;
	friend class CCtx_CorutinePlusThreadData;
};

#pragma warning (pop)

#endif