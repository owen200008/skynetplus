#ifndef SCBASIC_CTX_GATEINTERATION_H
#define SCBASIC_CTX_GATEINTERATION_H

#include "../kernel/ccframe/initbussinesshead.h"
#include "../kernel/ccframe/net/ctx_gate.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

struct InterationCenterCtxStoreInfo {
	uint32_t                m_nSelfCtxID;
	basiclib::CBasicString  m_strServerKey;
	uint32_t                m_nSessionID;
	InterationCenterCtxStoreInfo() {
		m_nSelfCtxID = 0;
		m_nSessionID = 0;
	}
};

#define CCoroutineCtx_GateInteration_GetByID         1000
#define CCoroutineCtx_GateInteration_SelfCtxRegister 1001
#define CCoroutineCtx_GateInteration_SelfCtxDelete   1002
class _SKYNET_BUSSINESS_DLL_API CCoroutineCtx_GateInteration : public CCoroutineCtx_CCFrameServerGate
{
public:
	CCoroutineCtx_GateInteration(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx_GateInteration));
	virtual ~CCoroutineCtx_GateInteration();

	CreateTemplateHeader(CCoroutineCtx_GateInteration);

	//! 协程里面调用Bussiness消息
	virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket);
	virtual int32_t OnReceive(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
protected:
	////////////////////DispathBussinessMsg
	//! 发送请求
	virtual long DispathBussinessMsg_4_ReceiveData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
	long DispathBussinessMsg_GetByID(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	long DispathBussinessMsg_SelfCtxRegister(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	long DispathBussinessMsg_SelfCtxDelete(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
	virtual bool SessionRelease(CCorutinePlus* pCorutine, CCFrameServerSession* pSession);
protected:
	//ctx内部安全
	basiclib::CBasicBitstream                   m_insCtxSafe;
	typedef basiclib::basic_map<Net_UChar, InterationCenterCtxStoreInfo>   MapGlobalKeyToSessionCtxID;
	MapGlobalKeyToSessionCtxID                  m_mapGlobalKeyToSelfCtxID;

	typedef basiclib::basic_map<basiclib::CBasicString, uint32_t>   MapGlobalNameToCtxID;
	MapGlobalNameToCtxID                        m_mapNameToCtxID;
};

#pragma warning (pop)

#endif