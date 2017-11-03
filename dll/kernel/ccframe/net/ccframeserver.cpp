#include "ccframeserver.h"
#include "../coroutinedata/coroutinestackdata.h"

std::function<void(CCFrameServerSession* pSession)>	CCFrameServerSession::g_notifyOnTime30NoCtxID;

//提供Debug receive接口
void CCFrameServer::DebugReceiveData(CCFrameServerSession* pSession, Net_Int cbData, const char* pszData){
    if (m_funcReceive)
        m_funcReceive(pSession, 0, cbData, pszData);
}
void CCFrameServer::DebugReceiveDataLua(CMoniFrameServerSessionLua* pSession, Net_Int cbData, const char* pszData){
	DebugReceiveData(pSession->GetSession(), cbData, pszData);
}

basiclib::CBasicSessionNetServerSession* CCFrameServer::ConstructSession(uint32_t nSessionID){
    return CCFrameServerSession::CreateNetServerSessionWithServer(nSessionID, m_usRecTimeout, this);
}
///////////////////////////////////////////////////////////////////////////////////////////

//绑定ctx
void CCFrameServerSession::BindCtxID(uint32_t nCtxID, int64_t nUniqueKey){
	m_ctxID = nCtxID;
	m_nUniqueKey = nUniqueKey;
}

//判断客户端是否太过频繁
BOOL CCFrameServerSession::IsTooBusy(int nUseTime){
	m_dwReceiveTimes++;
	if (m_dwReceiveTimes % 100 == 99){
		//每50个包判断一次
		DWORD dwNow = basiclib::BasicGetTickTime();
		if (dwNow - m_dwLastTick < nUseTime){
			return TRUE;
		}
		m_dwLastTick = dwNow;
	}
	return FALSE;
}
//定时器,ontimer线程
void CCFrameServerSession::OnTimer(unsigned int nTick){
	CNetServerControlSession::OnTimer(nTick);
	m_dwNoCtxIDTime++;

	if(m_dwNoCtxIDTime > 30){
		m_dwNoCtxIDTime = 0;
		//30s还没初始化的话需要通知上层
		if(g_notifyOnTime30NoCtxID != nullptr)
			g_notifyOnTime30NoCtxID(this);
	}
}
//////////////////////////////////////////////////////////////////////////////////////////////
int32_t CMoniFrameServerSession::Send(void *pData, int32_t cbData, uint32_t dwFlag){
	//切换到lua线程
	Ctx_CreateCoroutine(0, [](CCorutinePlus* pCorutine){
		CCorutineStackDataDefault stackData;
		CMoniFrameServerSession* pSession = pCorutine->GetParamPoint<CMoniFrameServerSession>(0);
		stackData.CopyData(pCorutine->GetParamPoint<const char>(1), pCorutine->GetParam<Net_Int>(2));//拷贝到堆数据
		uint32_t nDefaultHttpCtxID = CCtx_ThreadPool::GetThreadPool()->GetDefaultHttp();
		MACRO_YieldToCtx(pCorutine, nDefaultHttpCtxID,
						 return);
		int nStackSize = 0;
		const char* pStack = stackData.GetData(nStackSize);
		basiclib::CBasicBitstream ins;
		ins.BindOutData((char*)pStack, nStackSize);
		if(pSession->m_sendfunc){
			pSession->m_sendfunc((uintptr_t)pSession, &ins);
		}
	}, this, pData, &cbData);
	return cbData;
}
int32_t CMoniFrameServerSession::Send(basiclib::CBasicSmartBuffer& smBuf, uint32_t dwFlag){
	//切换到lua线程
	return Send(smBuf.GetDataBuffer(), smBuf.GetDataLength(), dwFlag);
}
//////////////////////////////////////////////////////////////////////////////////////////////
CMoniFrameServerSessionLua::CMoniFrameServerSessionLua(){
	m_pSession = CMoniFrameServerSession::CreateNetServerSessionWithServer(0, 0, nullptr);
}
CMoniFrameServerSessionLua::~CMoniFrameServerSessionLua(){
	if(m_pSession){
		m_pSession->SafeDelete();
	}
}
