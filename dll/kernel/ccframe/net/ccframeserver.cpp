#include "ccframeserver.h"

CCFrameServer::CCFrameServer(Net_UInt nSessionID) : CNetServerControl(nSessionID){
}

CCFrameServer::~CCFrameServer(){
}

//提供Debug receive接口
void CCFrameServer::DebugReceiveData(CCFrameServerSession* pSession, Net_Int cbData, const char* pszData){
    if (m_funcReceive)
        m_funcReceive(pSession, 0, cbData, pszData);
}

basiclib::CBasicSessionNetClient* CCFrameServer::ConstructSession(Net_UInt nSessionID)
{
    return CCFrameServerSession::CreateCCFrameServerSession(nSessionID, this);
}
///////////////////////////////////////////////////////////////////////////////////////////
CCFrameServerSession::CCFrameServerSession(Net_UInt nSessionID, CRefNetServerControl pServer) : CNetServerControlClient(nSessionID, pServer)
{
	m_ctxID = 0;
	m_dwLastTick = 0;
	m_dwReceiveTimes = 0;
}

CCFrameServerSession::~CCFrameServerSession()
{
    if (m_ctxID != 0){
        CCtx_ThreadPool::GetThreadPool()->ReleaseObjectCtxByCtxID(m_ctxID);
    }
}

//创建ctx
bool CCFrameServerSession::CreateCtx(const char* pCtxName, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func)
{
    //创建ctx
    m_ctxID = CCtx_ThreadPool::GetThreadPool()->CreateTemplateObjectCtx(pCtxName, nullptr, [&](InitGetParamType type, const char* pKey, const char* pDefault)->const char*{
        switch (type){
        case InitGetParamType_Config:
            return CCtx_ThreadPool::GetThreadPool()->GetCtxInitString(type, pKey, pDefault);
            break;
        case InitGetParamType_CreateUser:
        {
            if (strcmp(pKey, "object") == 0){
                return (const char*)this;
            }
        }
            break;
        case InitGetParamType_Init:
            func(type, pKey, pDefault);
            break;
        }
        return pDefault;
    });
    return m_ctxID != 0;
}

//判断客户端是否太过频繁
BOOL CCFrameServerSession::IsTooBusy()
{
	m_dwReceiveTimes++;
	if (m_dwReceiveTimes % 100 == 90)
	{
		//每50个包判断一次
		DWORD dwNow = basiclib::BasicGetTickTime();
		if (dwNow - m_dwLastTick < 10000)
		{
			return TRUE;
		}
		m_dwLastTick = dwNow;
	}
	return FALSE;
}

int32_t CCFrameServerSession::OnDisconnect(uint32_t dwNetCode){
    if (m_ctxID != 0){
        CCtx_ThreadPool::GetThreadPool()->ReleaseObjectCtxByCtxID(m_ctxID);
        m_ctxID = 0;
    }
    return CNetServerControlClient::OnDisconnect(dwNetCode);
}
//////////////////////////////////////////////////////////////////////////////////////////////
CMoniFrameServerSession::CMoniFrameServerSession() : CCFrameServerSession(0, nullptr){

}
CMoniFrameServerSession::~CMoniFrameServerSession(){

}

int32_t CMoniFrameServerSession::Send(void *pData, int32_t cbData, uint32_t dwFlag){
    m_sendData.AppendData((char*)pData, cbData);
    return cbData;
}
int32_t CMoniFrameServerSession::Send(basiclib::CBasicSmartBuffer& smBuf, uint32_t dwFlag){
    m_sendData.AppendData(smBuf.GetDataBuffer(), smBuf.GetDataLength());
    return smBuf.GetDataLength();
}
void CMoniFrameServerSession::ClearSendBuffer(){
    m_sendData.SetDataLength(0);
}
