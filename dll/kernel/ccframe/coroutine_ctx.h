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

    //! 判断是否已经release
    bool IsReleaseCtx(){ return m_bRelease; }

	//! 初始化0代表成功
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

	//!	分配任务
    virtual void DispatchMsg(ctx_message& msg);

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

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
#pragma warning (pop)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//!协程相关
//! 创建协程,返回false代表要传输的目的ctxid已经不存在
_SKYNET_KERNEL_DLL_API bool CreateResumeCoroutineCtx(coroutine_func func, uint32_t nSourceCtxID, int nParam, void* pParam);

//! 唤醒协程执行
_SKYNET_KERNEL_DLL_API bool WaitResumeCoroutineCtx(CCorutinePlus* pCorutine, CCoroutineCtx* pCtx, ctx_message* pMsg);

//! 唤醒协程执行失败
_SKYNET_KERNEL_DLL_API void WaitResumeCoroutineCtxFail(CCorutinePlus* pCorutine);

//! 执行DstCtxID的nType函数,返回false需要保证pCorutine线程退出，不然会出现这个协程泄漏, 提供参数序列化和反序列化方法
_SKYNET_KERNEL_DLL_API bool WaitExeCoroutineToCtxBussiness(CCorutinePlus* pCorutine, uint32_t nSourceCtxID, uint32_t nDstCtxID, uint32_t nType, int nParam, void** pParam, void* pRetPacket, int& nRetValue);
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

//! yield协程
_SKYNET_KERNEL_DLL_API bool YieldCorutineToCtx(CCorutinePlus* pCorutine, uint32_t nNextCtxID, CCoroutineCtx*& pCtx, ctx_message*& pMsg);

//! 获取参数个数和参数,只能在resume之后调用
_SKYNET_KERNEL_DLL_API void** GetCoroutineCtxParamInfo(CCorutinePlus* pCorutine, int& nCount);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! 加入timer
_SKYNET_KERNEL_DLL_API bool CoroutineCtxAddOnTimer(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback);
_SKYNET_KERNEL_DLL_API bool CoroutineCtxAddOnTimeOut(uint32_t nCtxID, int nTimes, pCallbackOnTimerFunc pCallback);
_SKYNET_KERNEL_DLL_API void CoroutineCtxDelTimer(pCallbackOnTimerFunc pCallback);
//! 定时唤醒协程
_SKYNET_KERNEL_DLL_API bool OnTimerToWakeUpCorutine(uint32_t nCtxID, int nTimes, CCorutinePlus* pCorutine);
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif