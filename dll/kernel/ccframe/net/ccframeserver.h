#ifndef SCBASIC_CCFRAMESERVER_H
#define SCBASIC_CCFRAMESERVER_H

#include "../scbasic/commu/servertemplate.h"
#include "../ctx_threadpool.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

class CCFrameServerSession;
class _SKYNET_KERNEL_DLL_API CCFrameServer : public CNetServerControl
{
public:
    static CCFrameServer* CreateCCFrameServer(){ return new CCFrameServer(basiclib::CBasicSessionNet::GetDefaultCreateSessionID()); }
    //提供Debug receive接口
    void DebugReceiveData(CCFrameServerSession* pSession, Net_Int cbData, const char* pszData);
protected:
    CCFrameServer(Net_UInt nSessionID);
    virtual ~CCFrameServer();

	virtual basiclib::CBasicSessionNetClient* ConstructSession(Net_UInt nSessionID);
};
/////////////////////////////////////////////////////////////////////////////////////////////////////
typedef basiclib::basic_map<uint32_t, basiclib::CNetBasicValue> MapContain;
class _SKYNET_KERNEL_DLL_API CCFrameServerSession : public CNetServerControlClient
{
public:
    static CCFrameServerSession* CreateCCFrameServerSession(Net_UInt nSessionID, CRefNetServerControl pServer){ return new CCFrameServerSession(nSessionID, pServer); }
protected:
    CCFrameServerSession(Net_UInt nSessionID, CRefNetServerControl pServer);
    virtual ~CCFrameServerSession();
public:
    //创建ctx
    bool CreateCtx(const char* pCtxName, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    uint32_t GetCtxSessionID(){ return m_ctxID; }

	//判断客户端是否太过频繁
	BOOL IsTooBusy();

    MapContain& GetServerSessionMapRevert(){ return m_mapRevert; }
protected:
    virtual int32_t OnDisconnect(uint32_t dwNetCode);
protected:
	uint32_t		m_ctxID;
	DWORD           m_dwLastTick;
	DWORD           m_dwReceiveTimes;

    MapContain      m_mapRevert;
};
typedef basiclib::CBasicRefPtr<CCFrameServerSession> RefCCFrameServerSession;


//模拟客户端
class _SKYNET_KERNEL_DLL_API CMoniFrameServerSession : public CCFrameServerSession
{
public:
    CMoniFrameServerSession();
    virtual ~CMoniFrameServerSession();

    virtual int32_t Send(void *pData, int32_t cbData, uint32_t dwFlag = 0);
    virtual int32_t Send(basiclib::CBasicSmartBuffer& smBuf, uint32_t dwFlag = 0);
    void ClearSendBuffer();
    basiclib::CBasicBitstream* GetSendBuffer(){ return &m_sendData; }
protected:
    basiclib::CBasicBitstream m_sendData;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma warning (pop)

#endif