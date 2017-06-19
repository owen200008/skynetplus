#include "ccframeserver.h"
#include "../coroutinedata/coroutinestackdata.h"

CCFrameServer::CCFrameServer(Net_UInt nSessionID) : CNetServerControl(nSessionID){
}

CCFrameServer::~CCFrameServer(){
}

//�ṩDebug receive�ӿ�
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

//����ctx
bool CCFrameServerSession::CreateCtx(const char* pCtxName, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func)
{
    //����ctx
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

//�жϿͻ����Ƿ�̫��Ƶ��
BOOL CCFrameServerSession::IsTooBusy()
{
	m_dwReceiveTimes++;
	if (m_dwReceiveTimes % 100 == 90)
	{
		//ÿ50�����ж�һ��
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
	m_bLua = false;
}
CMoniFrameServerSession::CMoniFrameServerSession(bool bLua) : CCFrameServerSession(0, nullptr) {
	m_bLua = bLua;
}
CMoniFrameServerSession::~CMoniFrameServerSession(){
	if (m_bLua) {
		//����Ҫ�Լ�����
		m_refSelf->KnowDelRef();
	}
}

int32_t CMoniFrameServerSession::Send(void *pData, int32_t cbData, uint32_t dwFlag){
	//�л���lua�߳�
	const int nSetParam = 3;
	void* pSetParam[nSetParam] = { this, pData, &cbData };
	CreateResumeCoroutineCtx([](CCorutinePlus* pCorutine)->void {
		int nGetParamCount = 0;
		void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
		CCorutineStackDataDefault stackData;
		CMoniFrameServerSession* pSession = (CMoniFrameServerSession*)pGetParam[0];
		stackData.CopyData((const char*)pGetParam[1], *(Net_Int*)pGetParam[2]);//������������
		uint32_t nDefaultHttpCtxID = CCtx_ThreadPool::GetThreadPool()->GetDefaultHttp();
		CCoroutineCtx* pCtx = nullptr;
		ctx_message* pMsg = nullptr;
		if (!YieldCorutineToCtx(pCorutine, nDefaultHttpCtxID, pCtx, pMsg)){
			CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nGetParamCount);
			ASSERT(0);
			return;
		}
		int nStackSize = 0;
		const char* pStack = stackData.GetData(nStackSize);
		basiclib::CBasicBitstream ins;
		ins.BindOutData((char*)pStack, nStackSize);
		if (pSession->m_sendfunc) {
			pSession->m_sendfunc((uintptr_t)pSession, &ins);
		}
	}, 0, nSetParam, pSetParam);
	return cbData;
}
int32_t CMoniFrameServerSession::Send(basiclib::CBasicSmartBuffer& smBuf, uint32_t dwFlag){
	//�л���lua�߳�
	return Send(smBuf.GetDataBuffer(), smBuf.GetDataLength(), dwFlag);
}

