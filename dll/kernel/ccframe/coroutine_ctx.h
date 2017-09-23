/***********************************************************************************************
// 文件名:     coroutine_ctx.h
// 创建者:     蔡振球
// Email:      zqcai@w.cn
// 内容描述:   协程资源基类
// 版本信息:   1.0V
************************************************************************************************/
#ifndef SCBASIC_COROUTINE_CTX_H
#define SCBASIC_COROUTINE_CTX_H

#include "ctx_msgqueue.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
//定义返回的通用错误
#define DEFINECTX_RET_TYPE_SUCCESS							0
#define DEFINECTX_RET_TYPE_COROUTINEERR						-1			//协程错误
#define DEFINECTX_RET_TYPE_ParamErr							-2			//参数错误
#define DEFINECTX_RET_TYPE_NoNet							-3			//网络异常
#define DEFINECTX_RET_TYPE_ReceDataErr						-4			//收到数据异常
#define DEFINECTX_RET_TYPE_NoCtxID							-5			//找不到对应的资源
#define DEFINECTX_RET_TYPE_NoSeriaze						-6			//找不到对应的序列化函数
#define DEFINECTX_RET_TYPE_NetResponseError					-7			//网络返回错误
#define DEFINECTX_RET_TYPE_SeriazeError						-8			//序列化错误
#define DEFINECTX_RET_TYPE_RETPACKETTYPERROR				-9			//返回类型类型错误或者空间不足
#define DEFINECTX_RET_TYPE_UseDefintTypeErrorBegin			1000		//从这个数开始递增
/////////////////////////////////////////////////////////////////////////////////////////////////////
enum InitGetParamType
{
    InitGetParamType_Init = 0,//默认回调一次init pKey就是CCoroutineCtx
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
typedef fastdelegate::FastDelegate0<void> HandleOnTimer;			//错误消息
typedef void(*pCallOnTimerFunc)();
class CCoroutineCtx;
class CCorutinePlusThreadData;
typedef void(*pCallbackOnTimerFunc)(CCoroutineCtx* pCtx);
struct lua_State;
class _SKYNET_KERNEL_DLL_API CCoroutineCtx : public basiclib::CBasicObject, public basiclib::EnableRefPtr<CCoroutineCtx>
{
public:
    //! 获取ctx总数
    static uint32_t& GetTotalCreateCtx();

    //! Push任务
    static bool PushMessageByID(uint32_t nDstCtxID, ctx_message& msg);
    //! 删除某个ctx
    static void ReleaseCtxByID(uint32_t nDstCtxID);
public:
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //线程安全的函数
    //! Push任务
    void PushMessage(ctx_message& msg);

    //! 获取唯一ID
    uint32_t GetCtxID(){ return m_ctxID; }

    //! 获取可搜索名字
    const char* GetCtxName(){ return m_pCtxName; }

    //! 获取实际类名
    const char* GetCtxClassName(){ return m_pClassName; }

    //! 获取状态
    virtual void GetState(basiclib::CBasicSmartBuffer& smBuf);

	//! 是否无状态ctx,在处理包的时候连续处理，不存在处理一个放回队列的问题
	bool IsNoStateCtx() { return PacketDealType_NoState_Mask & m_ctxPacketDealType; }

	//! yield by resume self（必须自己唤醒的yield）
	bool YieldAndResumeSelf(CCorutinePlus* pCorutine);

	//! 测试接口
	virtual bool DebugInterface(const char* pDebugInterface, uint32_t nType, lua_State* L);
public:
    //! 必须是全局不析构的字符串,只保存指针
    CCoroutineCtx(const char* pName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx));

	virtual ~CCoroutineCtx();

    //! 不需要自己delete，只要调用release
    virtual void ReleaseCtx();

	//! 强制push到全局队列
	void PushGlobalQueue();

    //! 判断是否已经release
    bool IsReleaseCtx(){ return m_bRelease; }

	//! 初始化0代表成功
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

	//!	分配任务
    virtual bool DispatchMsg(ctx_message& msg, CCorutinePlusThreadData* pCurrentData);

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket);

protected:
    //! 分配一个新的sessionid
    int32_t GetNewSessionID();
protected:
	bool										m_bRelease;
    uint8_t                                     m_bFixCtxID;
    bool                                        m_bUseForServer;    //注册给interationcenter的ctx提供给子节点访问
	uint32_t									m_ctxID;
	int32_t										m_sessionID_Index;
	const char*						            m_pCtxName;
    const char*                                 m_pClassName;
	CCtxMessageQueue							m_ctxMsgQueue;
	moodycamel::ConsumerToken					m_Ctoken;		//优化读取
	//处理模式
	uint32_t									m_ctxPacketDealType;

	//统计总共创建的上下文
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

	//! 初始化
	void ReInit(coroutine_funcframe pFunc);

	//! 设置锁定ctx
	void SetStore(CCoroutineCtx* pCtx){
		m_vtStoreCtx.push_back(pCtx);
	}
	//! 设置当前ctx
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
//! 唤醒协程执行失败
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

//! 默认唤醒函数
_SKYNET_KERNEL_DLL_API bool Ctx_ResumeCoroutine(CCorutinePlus* pCorutine, CCoroutineCtx* pCtx, CCorutinePlusThreadData* pCurrentData);

//! 默认投递函数
_SKYNET_KERNEL_DLL_API bool Ctx_YieldCoroutine(CCorutinePlus* pCorutine, uint32_t nNextCtxID);

//! 唤醒函数
template<class... _Types>
bool Ctx_ResumeCoroutineNormal(CCorutinePlus* pCorutine, CCoroutineCtx* pCtx, CCorutinePlusThreadData* pCurrentData, _Types... _Args){
	static WakeUpCorutineType wakeUpType = WakeUpCorutineType_Success;
	//占有当前资源
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

//! 执行DstCtxID的nType函数,返回false需要保证pCorutine线程退出，不然会出现这个协程泄漏, 提供参数序列化和反序列化方法
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
//!协程相关
//再将一个应用变成跨服访问的时候，请先注册序列化和反序列化函数
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
//! 加入timer
_SKYNET_KERNEL_DLL_API bool CoroutineCtxAddOnTimer(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback);
_SKYNET_KERNEL_DLL_API bool CoroutineCtxAddOnTimeOut(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback);
_SKYNET_KERNEL_DLL_API void CoroutineCtxDelTimer(pCallbackOnTimerFunc pCallback);
//! 定时唤醒协程
_SKYNET_KERNEL_DLL_API bool OnTimerToWakeUpCorutine(uint32_t nCtxID, int nTimes, CCorutinePlus* pCorutine);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
#endif