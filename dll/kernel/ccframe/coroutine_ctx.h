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

/////////////////////////////////////////////////////////////////////////////////////////////////////
//���巵�ص�ͨ�ô���
#define DEFINECTX_RET_TYPE_SUCCESS							0
#define DEFINECTX_RET_TYPE_COROUTINEERR						-1			//Э�̴���
#define DEFINECTX_RET_TYPE_ParamErr							-2			//��������
#define DEFINECTX_RET_TYPE_NoNet							-3			//�����쳣
#define DEFINECTX_RET_TYPE_ReceDataErr						-4			//�յ������쳣
#define DEFINECTX_RET_TYPE_NoCtxID							-5			//�Ҳ�����Ӧ����Դ
#define DEFINECTX_RET_TYPE_NoSeriaze						-6			//�Ҳ�����Ӧ�����л�����
#define DEFINECTX_RET_TYPE_NetResponseError					-7			//���緵�ش���
#define DEFINECTX_RET_TYPE_SeriazeError						-8			//���л�����
#define DEFINECTX_RET_TYPE_RETPACKETTYPERROR				-9			//�����������ʹ�����߿ռ䲻��
#define DEFINECTX_RET_TYPE_UseDefintTypeErrorBegin			1000		//���������ʼ����
/////////////////////////////////////////////////////////////////////////////////////////////////////
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

#define PacketDealType_NoState_Mask		0x0000000F
#define PacketDealType_NoState_NoState	0x00000001

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)
typedef fastdelegate::FastDelegate0<void> HandleOnTimer;			//������Ϣ
typedef void(*pCallOnTimerFunc)();
class CCoroutineCtx;
class CCorutinePlusThreadData;
typedef void(*pCallbackOnTimerFunc)(CCoroutineCtx* pCtx);
struct lua_State;
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

	//! �Ƿ���״̬ctx,�ڴ������ʱ���������������ڴ���һ���Żض��е�����
	bool IsNoStateCtx() { return PacketDealType_NoState_Mask & m_ctxPacketDealType; }

	//! yield by resume self�������Լ����ѵ�yield��
	bool YieldAndResumeSelf(CCorutinePlus* pCorutine);

	//! ���Խӿ�
	virtual bool DebugInterface(const char* pDebugInterface, uint32_t nType, lua_State* L);
public:
    //! ������ȫ�ֲ��������ַ���,ֻ����ָ��
    CCoroutineCtx(const char* pName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx));

	virtual ~CCoroutineCtx();

    //! ����Ҫ�Լ�delete��ֻҪ����release
    virtual void ReleaseCtx();

	//! ǿ��push��ȫ�ֶ���
	void PushGlobalQueue();

    //! �ж��Ƿ��Ѿ�release
    bool IsReleaseCtx(){ return m_bRelease; }

	//! ��ʼ��0����ɹ�
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

	//!	��������
    virtual bool DispatchMsg(ctx_message& msg, CCorutinePlusThreadData* pCurrentData);

    //! Э���������Bussiness��Ϣ
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket);

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
	//����ģʽ
	uint32_t									m_ctxPacketDealType;

	//ͳ���ܹ�������������
	static uint32_t				m_nTotalCtx;
	
	friend class CCoroutineCtxHandle;
};
typedef basiclib::CBasicRefPtr<CCoroutineCtx> CRefCoroutineCtx;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef void(*coroutine_funcframe)(CCorutinePlus* pCorutinePlus);
class _SKYNET_KERNEL_DLL_API CCorutinePlus : public CCorutinePlusBase{
public:
	CCorutinePlus();
	virtual ~CCorutinePlus();

	//! ��ʼ��
	void ReInit(coroutine_funcframe pFunc);

	//! ��������ctx
	void SetStore(CCoroutineCtx* pCtx){
		m_vtStoreCtx.push_back(pCtx);
	}
	//! ���õ�ǰctx
	void SetCurrentCtx(CCoroutineCtx* pCtx){
		m_pCurrentCtx = pCtx;
	}
	bool IsSameCurrentCtx(uint32_t nNextCtxID){
		if(m_pCurrentCtx && nNextCtxID != 0){
			return m_pCurrentCtx->GetCtxID() == nNextCtxID;
		}
		return false;
	}
	CCoroutineCtx* GetCurrentCtx(){ return m_pCurrentCtx; }
protected:
	CCoroutineCtx*		m_pCurrentCtx;
	typedef basiclib::basic_vector<CRefCoroutineCtx> VTRefCoroutineCtx;
	VTRefCoroutineCtx	m_vtStoreCtx;

	friend class CCorutinePlusPool;
};
////////////////////////////////////////////////////////////////////////////////
class CCorutinePlusPool : public CCorutinePlusPoolBase{
public:
	CCorutinePlusPool();
	virtual ~CCorutinePlusPool();

	inline CCorutinePlus* GetCorutine(bool bGlobal = true){
		return (CCorutinePlus*)CCorutinePlusPoolBase::GetCorutine(bGlobal);
	}
protected:
	virtual void FinishFunc(CCorutinePlusBase* pCorutine);

	virtual CCorutinePlusBase* ConstructCorutine(){ return new CCorutinePlus(); }
};
////////////////////////////////////////////////////////////////////////////////
class CCorutinePlusThreadData : public CCorutinePlusThreadDataBase{
public:
	CCorutinePlusThreadData();
	virtual ~CCorutinePlusThreadData();
	CCorutinePlusPool* GetCorutinePlusPool(){ return (CCorutinePlusPool*)m_pPool; }
protected:
	virtual CCorutinePlusPoolBase* CreatePool(){ return new CCorutinePlusPool(); }
};
#pragma warning (pop)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! ����Э��ִ��ʧ��
_SKYNET_KERNEL_DLL_API void Ctx_ResumeCoroutine_Fail(CCorutinePlus* pCorutine);

_SKYNET_KERNEL_DLL_API bool Ctx_DispathCoroutine(CCorutinePlus* pCorutine, uint32_t nSrcCtxID, uint32_t nDstCtxID);

template<class... _Types>
bool Ctx_CreateCoroutine(uint32_t nSourceCtxID, coroutine_funcframe func, _Types... _Args){
	CCorutinePlusThreadData* pCurrentData = CCtx_ThreadPool::GetOrCreateSelfThreadData();
	CCorutinePlus* pCorutine = pCurrentData->GetCorutinePlusPool()->GetCorutine();
	pCorutine->ReInit(func);
	if(pCorutine->Resume(pCurrentData->GetCorutinePlusPool(), std::forward<_Types>(_Args)...) == CoroutineState_Suspend){
		uint32_t nNextCtxID = pCorutine->GetParam<uint32_t>(0);
		if(nNextCtxID != 0){
			return Ctx_DispathCoroutine(pCorutine, nSourceCtxID, nNextCtxID);
		}
	}
	return true;
}

//! Ĭ�ϻ��Ѻ���
_SKYNET_KERNEL_DLL_API bool Ctx_ResumeCoroutine(CCorutinePlus* pCorutine, CCoroutineCtx* pCtx, CCorutinePlusThreadData* pCurrentData);

//! Ĭ��Ͷ�ݺ���
_SKYNET_KERNEL_DLL_API bool Ctx_YieldCoroutine(CCorutinePlus* pCorutine, uint32_t nNextCtxID);

//! ���Ѻ���
template<class... _Types>
bool Ctx_ResumeCoroutineNormal(CCorutinePlus* pCorutine, CCoroutineCtx* pCtx, CCorutinePlusThreadData* pCurrentData, _Types... _Args){
	static WakeUpCorutineType wakeUpType = WakeUpCorutineType_Success;
	//ռ�е�ǰ��Դ
	pCorutine->SetCurrentCtx(pCtx);
	if(pCorutine->Resume(pCurrentData->GetCorutinePlusPool(), &wakeUpType, std::forward<_Types>(_Args)...) == CoroutineState_Suspend){
		pCorutine->SetCurrentCtx(nullptr);
		uint32_t nNextCtxID = pCorutine->GetParam<uint32_t>(0);
		if(nNextCtxID != 0){
			return Ctx_DispathCoroutine(pCorutine, pCtx->GetCtxID(), nNextCtxID);
		}
	}
	return true;
}

//! ִ��DstCtxID��nType����,����false��Ҫ��֤pCorutine�߳��˳�����Ȼ��������Э��й©, �ṩ�������л��ͷ����л�����
_SKYNET_KERNEL_DLL_API int Ctx_ExeCoroutine(CCorutinePlus* pCorutine, uint32_t nSourceCtxID, uint32_t nDstCtxID, uint32_t nType, int nParam, void** pParam, void* pRetPacket);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MACRO_ResumeToCtx(pCorutine, pCtx, pCurrentData, afterexp)\
	IsErrorHapper(Ctx_ResumeCoroutine(pCorutine, pCtx, pCurrentData), ASSERT(0);CCFrameSCBasicLogEventErrorV("%s(%s:%d) Ctx_ResumeCoroutine error", __FUNCTION__, __FILE__, __LINE__);afterexp)
#define MACRO_ResumeToCtxNormal(pCorutine, pCtx, pCurrentData, afterexp, ...)\
	IsErrorHapper(Ctx_ResumeCoroutineNormal(pCorutine, pCtx, pCurrentData, __VA_ARGS__), ASSERT(0);CCFrameSCBasicLogEventErrorV("%s(%s:%d) Ctx_ResumeCoroutineNormal error", __FUNCTION__, __FILE__, __LINE__);afterexp)
#define MACRO_YieldToCtx(pCorutine, nNextCtxID, afterexp) \
	IsErrorHapper(Ctx_YieldCoroutine(pCorutine, nNextCtxID), ASSERT(0);CCFrameSCBasicLogEventErrorV("%s(%s:%d) YieldCorutineToCtx error", __FUNCTION__, __FILE__, __LINE__);afterexp)
#define MACRO_ExeToCtx(pCorutine, nSourceCtxID, nDstCtxID, nType, nParam, pParam, pRetPacket, afterexp){\
		int nRetExeToCtxType = Ctx_ExeCoroutine(pCorutine, nSourceCtxID, nDstCtxID, nType, nParam, pParam, pRetPacket);\
		if(nRetExeToCtxType != DEFINECTX_RET_TYPE_SUCCESS){\
			if(nRetExeToCtxType == DEFINECTX_RET_TYPE_COROUTINEERR){\
				ASSERT(0);CCFrameSCBasicLogEventErrorV("%s(%s:%d) Ctx_ExeCoroutine error", __FUNCTION__, __FILE__, __LINE__);\
			}\
			afterexp;\
		}\
	}
#define MACRO_ExeToCtxParam1(pCorutine, nSourceCtxID, nDstCtxID, nType, param1, pRetPacket, afterexp){\
		const int nSetParam = 1;\
		void* pSetParam[nSetParam] = { param1 };\
		MACRO_ExeToCtx(pCorutine, nSourceCtxID, nDstCtxID, nType, nSetParam, pSetParam, pRetPacket, afterexp)\
	}
#define MACRO_ExeToCtxParam2(pCorutine, nSourceCtxID, nDstCtxID, nType, param1, param2, pRetPacket, afterexp){\
		const int nSetParam = 2;\
		void* pSetParam[nSetParam] = { param1, param2 };\
		MACRO_ExeToCtx(pCorutine, nSourceCtxID, nDstCtxID, nType, nSetParam, pSetParam, pRetPacket, afterexp)\
	}
#define MACRO_ExeToCtxParam3(pCorutine, nSourceCtxID, nDstCtxID, nType, param1, param2, param3, pRetPacket, afterexp){\
		const int nSetParam = 3;\
		void* pSetParam[nSetParam] = { param1, param2, param3 };\
		MACRO_ExeToCtx(pCorutine, nSourceCtxID, nDstCtxID, nType, nSetParam, pSetParam, pRetPacket, afterexp)\
	}
#define MACRO_ExeToCtxParam4(pCorutine, nSourceCtxID, nDstCtxID, nType, param1, param2, param3, param4, pRetPacket, afterexp){\
		const int nSetParam = 4;\
		void* pSetParam[nSetParam] = { param1, param2, param3, param4 };\
		MACRO_ExeToCtx(pCorutine, nSourceCtxID, nDstCtxID, nType, nSetParam, pSetParam, pRetPacket, afterexp)\
	}
#define MACRO_DispatchCheckParam1(Param1Type, Param1TransferType)\
	IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return DEFINECTX_RET_TYPE_ParamErr);\
	Param1Type = Param1TransferType pParam[0];
#define MACRO_DispatchCheckParam2(Param1Type, Param1TransferType, Param2Type, Param2TransferType)\
	IsErrorHapper(nParam == 2, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return DEFINECTX_RET_TYPE_ParamErr);\
	Param1Type = Param1TransferType pParam[0];\
	Param2Type = Param2TransferType pParam[1];
#define MACRO_DispatchCheckParam3(Param1Type, Param1TransferType, Param2Type, Param2TransferType, Param3Type, Param3TransferType)\
	IsErrorHapper(nParam == 3, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return DEFINECTX_RET_TYPE_ParamErr);\
	Param1Type = Param1TransferType pParam[0];\
	Param2Type = Param2TransferType pParam[1];\
	Param3Type = Param3TransferType pParam[2];
#define MACRO_DispatchCheckParam4(Param1Type, Param1TransferType, Param2Type, Param2TransferType, Param3Type, Param3TransferType, Param4Type, Param4TransferType)\
	IsErrorHapper(nParam == 4, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return DEFINECTX_RET_TYPE_ParamErr);\
	Param1Type = Param1TransferType pParam[0];\
	Param2Type = Param2TransferType pParam[1];\
	Param3Type = Param3TransferType pParam[2];\
	Param4Type = Param4TransferType pParam[3];

#define MACRO_GetYieldParam1(Param1Type, Param1TransferType)\
	Param1Type = pCorutine->GetParamPoint<Param1TransferType>(1);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//!Э�����
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! ����timer
_SKYNET_KERNEL_DLL_API bool CoroutineCtxAddOnTimer(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback);
_SKYNET_KERNEL_DLL_API bool CoroutineCtxAddOnTimeOut(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback);
_SKYNET_KERNEL_DLL_API void CoroutineCtxDelTimer(pCallbackOnTimerFunc pCallback);
//! ��ʱ����Э��
_SKYNET_KERNEL_DLL_API bool OnTimerToWakeUpCorutine(uint32_t nCtxID, int nTimes, CCorutinePlus* pCorutine);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
#endif