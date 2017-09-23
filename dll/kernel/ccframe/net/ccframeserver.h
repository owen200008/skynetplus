#ifndef SCBASIC_CCFRAMESERVER_H
#define SCBASIC_CCFRAMESERVER_H

#include "../scbasic/commu/servertemplate.h"
#include "../ctx_threadpool.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

class CCFrameServerSession;
class CMoniFrameServerSessionLua;
class _SKYNET_KERNEL_DLL_API CCFrameServer : public CNetServerControl
{
	DefineCreateNetServerDefault(CCFrameServer)
public:
    //�ṩDebug receive�ӿ�
    void DebugReceiveData(CCFrameServerSession* pSession, Net_Int cbData, const char* pszData);
	void DebugReceiveDataLua(CMoniFrameServerSessionLua* pSession, Net_Int cbData, const char* pszData);
protected:
	virtual basiclib::CBasicSessionNetServerSession* ConstructSession(uint32_t nSessionID);
};
/////////////////////////////////////////////////////////////////////////////////////////////////////
typedef basiclib::basic_map<uint32_t, basiclib::CNetBasicValue> MapContain;
class _SKYNET_KERNEL_DLL_API CCFrameServerSession : public CNetServerControlSession
{
	DefineCreateNetServerSessionWithServer(CCFrameServerSession)
public:
    //����ctx
    void BindCtxID(uint32_t nCtxID, int64_t nUniqueKey);

    uint32_t GetCtxSessionID(){ return m_ctxID; }
	//reset ctxsession
	void ResetSessionID() { m_ctxID = 0; }
	int64_t GetUniqueKey() { return m_nUniqueKey; }

	//! 100��������С��Ҫ��ʱ��, �жϿͻ����Ƿ�̫��Ƶ����Ĭ��10s
	BOOL IsTooBusy(int nUseTime = 10000);

    MapContain& GetServerSessionMapRevert(){ return m_mapRevert; }
protected:
	CCFrameServerSession(){
		m_ctxID = 0;
		m_nUniqueKey = 0;
		m_dwLastTick = 0;
		m_dwReceiveTimes = 0;
	}
	virtual ~CCFrameServerSession(){
		ASSERT(m_ctxID == 0);
	}
protected:
	uint32_t		m_ctxID;
	int64_t			m_nUniqueKey;
	DWORD           m_dwLastTick;
	DWORD           m_dwReceiveTimes;

    MapContain      m_mapRevert;
};
typedef basiclib::CBasicRefPtr<CCFrameServerSession> RefCCFrameServerSession;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//ģ��ͻ���
class _SKYNET_KERNEL_DLL_API CMoniFrameServerSession : public CCFrameServerSession{
	DefineCreateNetServerSessionWithServerDefault(CMoniFrameServerSession)
public:
	virtual int32_t Send(void *pData, int32_t cbData, uint32_t dwFlag = 0);
	virtual int32_t Send(basiclib::CBasicSmartBuffer& smBuf, uint32_t dwFlag = 0);
	//! bind send callback
	void BindSendCallback(const std::function<void(uintptr_t nUniqueID, basiclib::CBasicBitstream* pSMBuf)>& func){
		m_sendfunc = func;
	}
public:
	std::function<void(uintptr_t nUniqueID, basiclib::CBasicBitstream* pSMBuf)>					m_sendfunc;
};

//ģ��ͻ���
class _SKYNET_KERNEL_DLL_API CMoniFrameServerSessionLua : public basiclib::CBasicObject
{
public:
	CMoniFrameServerSessionLua();
	virtual ~CMoniFrameServerSessionLua();

	uintptr_t GetUniqueID() { return (uintptr_t)m_pSession; }

	int32_t SendForLua(basiclib::CBasicSmartBuffer* pBuffer) { return m_pSession->Send(*pBuffer); }
	int32_t SendData(void *pData, int32_t cbData){ return m_pSession->Send(pData, cbData); }
	//! bind send callback
	void BindSendCallback(const std::function<void(uintptr_t nUniqueID, basiclib::CBasicBitstream* pSMBuf)>& func) {
		m_pSession->BindSendCallback(func);
	}
	CMoniFrameServerSession* GetSession(){ return m_pSession; }
protected:
	CMoniFrameServerSession*																	m_pSession;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning (pop)

#endif