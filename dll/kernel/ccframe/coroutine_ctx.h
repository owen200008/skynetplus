/***********************************************************************************************
// �ļ���:     coroutine_ctx.h
// ������:     ������
// Email:      zqcai@w.cn
// ��������:   Э����Դ����
// �汾��Ϣ:   1.0V
************************************************************************************************/
#ifndef SCBASIC_COROUTINE_CTX_H
#define SCBASIC_COROUTINE_CTX_H

#include "ctx_msgqueue.h"

enum InitGetParamType
{
    InitGetParamType_Init = 0,//Ĭ�ϻص�һ��init pKey����CCoroutineCtx
	InitGetParamType_Config = 1,
    InitGetParamType_CreateUser = 2,
};

enum WakeUpCorutineType
{
    WakeUpCorutineType_Fail = -1,
    WakeUpCorutineType_Success = 0,
};

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)
typedef fastdelegate::FastDelegate0<void> HandleOnTimer;			//������Ϣ
typedef void(*pCallOnTimerFunc)();
class CCoroutineCtx;
class CCtx_CorutinePlusThreadData;
typedef void(*pCallbackOnTimerFunc)(CCoroutineCtx* pCtx, CCtx_CorutinePlusThreadData* pData);
class _SKYNET_KERNEL_DLL_API CCoroutineCtx : public basiclib::CBasicObject, public basiclib::EnableRefPtr<CCoroutineCtx>
{
public:
    //! ��ȡctx����
    static uint32_t& GetTotalCreateCtx();

    //! Push����
    static bool PushMessageByID(uint32_t nDstCtxID, ctx_message& msg);
    //! ɾ��ĳ��ctx
    static void ReleaseCtxByID(uint32_t nDstCtxID);
public:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //�̰߳�ȫ�ĺ���
    //! Push����
    void PushMessage(ctx_message& msg);

    //! ��ȡΨһID
    uint32_t GetCtxID(){ return m_ctxID; }

    //! ��ȡ����������
    const char* GetCtxName(){ return m_pCtxName; }

    //! ��ȡʵ������
    const char* GetCtxClassName(){ return m_pClassName; }

    //! ��ȡ״̬
    virtual void GetState(basiclib::CBasicSmartBuffer& smBuf);
public:
    //! ������ȫ�ֲ��������ַ���,ֻ����ָ��
    CCoroutineCtx(const char* pName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx));

	virtual ~CCoroutineCtx();

    //! ����Ҫ�Լ�delete��ֻҪ����release
    virtual void ReleaseCtx();

    //! �ж��Ƿ��Ѿ�release
    bool IsReleaseCtx(){ return m_bRelease; }

	//! ��ʼ��0����ɹ�
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

	//!	��������
    virtual void DispatchMsg(ctx_message& msg, CCtx_CorutinePlusThreadData* pData);

    //! Э���������Bussiness��Ϣ
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

protected:
    //! ����һ���µ�sessionid
    int32_t GetNewSessionID();
protected:
	bool										m_bRelease;
    uint8_t                                     m_bFixCtxID;
    bool                                        m_bUseForServer;    //ע���interationcenter��ctx�ṩ���ӽڵ����
	uint32_t									m_ctxID;
	int32_t										m_sessionID_Index;
	const char*						            m_pCtxName;
    const char*                                 m_pClassName;
	CCtxMessageQueue							m_ctxMsgQueue;
	moodycamel::ConsumerToken					m_Ctoken;		//�Ż���ȡ

	//ͳ���ܹ�������������
	static uint32_t				m_nTotalCtx;
	
	friend class CCoroutineCtxHandle;
};
typedef basiclib::CBasicRefPtr<CCoroutineCtx> CRefCoroutineCtx;
#pragma warning (pop)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//!Э�����
//! ����Э��,����false����Ҫ�����Ŀ��ctxid�Ѿ�������
_SKYNET_KERNEL_DLL_API bool CreateResumeCoroutineCtx(coroutine_func func, CCorutinePlusThreadData* pData, uint32_t nSourceCtxID, int nParam, void* pParam);

//! ����Э��ִ��
_SKYNET_KERNEL_DLL_API bool WaitResumeCoroutineCtx(CCorutinePlus* pCorutine, CCoroutineCtx* pCtx, CCorutinePlusThreadData* pData, ctx_message* pMsg);

//! ����Э��ִ��ʧ��
_SKYNET_KERNEL_DLL_API void WaitResumeCoroutineCtxFail(CCorutinePlus* pCorutine, CCorutinePlusThreadData* pData);

//! ִ��DstCtxID��nType����,����false��Ҫ��֤pCorutine�߳��˳�����Ȼ��������Э��й©, �ṩ�������л��ͷ����л�����
_SKYNET_KERNEL_DLL_API bool WaitExeCoroutineToCtxBussiness(CCorutinePlus* pCorutine, uint32_t nSourceCtxID, uint32_t nDstCtxID, uint32_t nType, int nParam, void** pParam, void* pRetPacket, int& nRetValue);
//�ٽ�һ��Ӧ�ñ�ɿ�����ʵ�ʱ������ע�����л��ͷ����л�����
typedef bool(*pRegisterSerializeRequest)(int nParam, void** pParam, basiclib::CBasicBitstream& ins);
typedef bool(*pRegisterSerializeResponse)(const std::function<bool(void*)>& func, basiclib::CBasicBitstream& ins);
typedef bool(*pRegisterUnSerializeRequest)(basiclib::CBasicBitstream& os, const std::function<bool(int, void**)>& func);
typedef bool(*pRegisterUnSerializeResponse)(basiclib::CBasicBitstream& os, void* pRetPacket);
struct ServerCommuSerialize{
    pRegisterSerializeRequest       m_requestSerialize;
    pRegisterUnSerializeRequest     m_requestUnSerialize;
    pRegisterSerializeResponse      m_responseSerialize;
    pRegisterUnSerializeResponse    m_responseUnSerialize;
    ServerCommuSerialize(){
        memset(this, 0, sizeof(ServerCommuSerialize));
    }
};
_SKYNET_KERNEL_DLL_API void RegisterSerializeAndUnSerialize(uint32_t nDstCtxID, uint32_t nType, ServerCommuSerialize& func, bool bSelfCtx = false);

//! yieldЭ��
_SKYNET_KERNEL_DLL_API bool YieldCorutineToCtx(CCorutinePlus* pCorutine, uint32_t nNextCtxID, CCoroutineCtx*& pCtx, CCorutinePlusThreadData*& pData, ctx_message*& pMsg);

//! ��ȡ���������Ͳ���,ֻ����resume֮�����
_SKYNET_KERNEL_DLL_API void** GetCoroutineCtxParamInfo(CCorutinePlus* pCorutine, int& nCount);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! ����timer
_SKYNET_KERNEL_DLL_API bool CoroutineCtxAddOnTimer(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback);
_SKYNET_KERNEL_DLL_API bool CoroutineCtxAddOnTimeOut(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback);
_SKYNET_KERNEL_DLL_API void CoroutineCtxDelTimer(pCallbackOnTimerFunc pCallback);
//! ��ʱ����Э��
_SKYNET_KERNEL_DLL_API bool OnTimerToWakeUpCorutine(uint32_t nCtxID, int nTimes, CCorutinePlus* pCorutine);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif