#ifndef SCBASIC_CCFRAME_GATE_H
#define SCBASIC_CCFRAME_GATE_H

#include "../dllmodule.h"
#include "ccframeserver.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

struct UniqueCtxData {
	uint32_t				m_nCtxID;
	time_t					m_releaseTime;
	CCFrameServerSession*	m_pSession;
	UniqueCtxData() {
		m_nCtxID = 0;
		m_releaseTime = 0;
		m_pSession = nullptr;
	}
	void Create(uint32_t nCtxID, CCFrameServerSession* pSession) {
		m_nCtxID = nCtxID;
		m_releaseTime = 0;
		m_pSession = pSession;
	}
	bool IsHaveReleaseTime() {
		return m_releaseTime == 0;
	}
	bool IsTimeToRelease(time_t tmNow, uint32_t nDelayTime) {
		return tmNow > m_releaseTime + nDelayTime;
	}
};

#define CCoroutineCtx_CCFrameServerGate_Find				1
#define CCoroutineCtx_CCFrameServerGate_Forward				2
#define CCoroutineCtx_CCFrameServerGate_SessionRelease		3
#define CCoroutineCtx_CCFrameServerGate_ReceiveData			4
class _SKYNET_KERNEL_DLL_API CCoroutineCtx_CCFrameServerGate : public CCoroutineCtx
{
public:
	CCoroutineCtx_CCFrameServerGate(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx_CCFrameServerGate));
	virtual ~CCoroutineCtx_CCFrameServerGate();

	CreateTemplateHeader(CCoroutineCtx_CCFrameServerGate);

	virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

	//! 不需要自己delete，只要调用release
	virtual void ReleaseCtx();

	//! 协程里面调用Bussiness消息
	virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket);
	////////////////////////////////////////////////////////////////////////////////////////
	//业务类, 全部使用静态函数, 这样可以保证动态库函数可以替换,做到动态更新
	static void OnTimer(CCoroutineCtx* pCtx);
	virtual void OnTimerChild(unsigned int nTick){}
public:
	virtual int32_t OnReceive(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
	virtual int32_t OnDisconnect(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode);
protected:
	////////////////////DispathBussinessMsg
	//! 发送请求
	long DispathBussinessMsg_1_Find(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	long DispathBussinessMsg_2_Forward(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	long DispathBussinessMsg_3_SessionRelease(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	virtual long DispathBussinessMsg_4_ReceiveData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
	virtual bool Create(CCorutinePlus* pCorutine, CCFrameServerSession* pSession, int64_t nUniqueKey, const std::function<void()>& funcBeforeInit);
	virtual bool SessionRelease(CCorutinePlus* pCorutine, CCFrameServerSession* pSession);
protected:
	void DealWithOnReceiveCtx(CCFrameServerSession* pSession, Net_Int cbData, const char* pszData);
	void DealWithOnReceiveToUniqueCtx(CCFrameServerSession* pSession, Net_Int cbData, const char* pszData);
protected:
	const char*                 m_pServerSessionCtx;
	uint32_t					m_nDelayDeleteTime;
	typedef basiclib::basic_map<int64_t, UniqueCtxData>	MapUniqueKeyToCtxID;
	MapUniqueKeyToCtxID			m_mapKeyToCtxID;
	//延迟删除的队列
	typedef basiclib::basic_vector<int64_t>	VTDelayRelease;
	VTDelayRelease				m_vtQueue;

	unsigned int				m_nTickOnTimer;
};

#pragma warning (pop)

#endif