/***********************************************************************************************
// �ļ���:     ctx_msgqueue.h
// ������:     ������
// Email:      zqcai@w.cn
// ��������:   ����Э�̺��첽��Э�̳�
// �汾��Ϣ:   1.0V
************************************************************************************************/

#ifndef SCBASIC_CTX_THREADPOOL_H
#define SCBASIC_CTX_THREADPOOL_H

#include "ctx_handle.h"
#include "dllmodule.h"
#include "log/ctx_log.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)
//call in the thread create
class basiclib::CBasicThreadTLS;
class CCtx_ThreadPool;
class CCFrameServer_Frame;

class _SKYNET_KERNEL_DLL_API CCtx_CorutinePlusThreadData : public CCorutinePlusThreadData
{
public:
    CCtx_CorutinePlusThreadData(CCtx_ThreadPool* pThreadPool);
    virtual ~CCtx_CorutinePlusThreadData();

	//ִ��
	void ExecThreadOnWork();
public:
	CCtx_ThreadPool*									m_pThreadPool;
	//ȫ����Ϣ������token
	moodycamel::ConsumerToken							m_globalCToken;

	CMQMgr												m_threadMgrQueue;
    moodycamel::ConsumerToken							m_threadCToken;
};

typedef fastdelegate::FastDelegate0<void*> OnCreateUDData;
class _SKYNET_KERNEL_DLL_API CCtx_ThreadPool : public basiclib::CBasicObject
{
public:
	static CCtx_ThreadPool* GetThreadPool();
	static CCorutinePlusThreadData* GetOrCreateSelfThreadData();
	static void CreateThreadPool(CCtx_ThreadPool* pPool);
public:
	//����ʵ�ֵ��麯��
	virtual const char* GetCtxInitString(InitGetParamType nType, const char* pParam, const char* pDefault) = 0;
public:
	CCtx_ThreadPool();
	virtual ~CCtx_ThreadPool();

	//! ��ʼ��
	virtual bool Init(const std::function<void*(CCorutinePlusThreadData*)>& pCreateFunc, 
		const std::function<void(void*)>& pReleaseParamFunc);
	//! �ȴ��˳�
	void Wait();

    //! ע������ýӿ�
    void RegisterServerForDebug(const char* pName, CCFrameServer_Frame* pRegister);
    CCFrameServer_Frame* GetRegisterServer(const char* pName);
public:
	basiclib::CBasicThreadTLS& GetTLS(){ return m_threadIndexTLS; }
    CBasicOnTimer& GetOnTimerModule(){ return m_ontimerModule; }
	CMQMgr& GetGlobalMQMgr(){ return m_globalMQMgrModule; }
public:
    //����ctx���
	CDllRegisterCtxTemplateMgr& GetCtxTemplateRegister(){ return m_mgtDllCtx; }
    uint32_t CreateTemplateObjectCtx(const char* pName, const char* pKeyName, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func, CMQMgr* pMQMgr = nullptr);
    void ReleaseObjectCtxByCtxID(uint32_t nCtxID);
protected:
	friend THREAD_RETURN ThreadOnWork(void*);
	
	friend THREAD_RETURN ThreadOnMonitor(void*);
	void ExecThreadOnMonitor();
protected:
    std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)> m_defaultFuncGetConfig;
	bool														m_bRunning;
	basiclib::CBasicThreadTLS									m_threadIndexTLS;
	CBasicOnTimer												m_ontimerModule;			//ִ��ontimer callback
	basiclib::basic_vector<HANDLE>								m_vtHandle;
	HANDLE														m_hMonitor;

	CMQMgr														m_globalMQMgrModule;
	uint32_t													m_nDPacketNumPerTime;

	std::function<void*(CCorutinePlusThreadData*)>				m_pCreateFunc;
	std::function<void(void*)>									m_pReleaseFunc;
	//ctx����
	CDllRegisterCtxTemplateMgr									m_mgtDllCtx;
protected:
	//�����ͷŵ�ctx
	CCoroutineCtx_Log*											m_pLog;
    typedef basiclib::basic_map<basiclib::CBasicString, CCFrameServer_Frame*> MapServerFrame;
    MapServerFrame                                              m_mapServerFrame;
    basiclib::SpinLock                                          m_Debuglock;

    friend class CCoroutineCtxHandle;
	friend class CCoroutineCtx;
	friend class CCtx_CorutinePlusThreadData;
};
#pragma warning (pop)

#endif