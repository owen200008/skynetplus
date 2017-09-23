#include "ctx_gate.h"
#include "../log/ctx_log.h"
#include "ctx_ccframeserversession.h"
#include "../ctx_threadpool.h"
#include "../coroutinedata/coroutinestackdata.h"

CreateTemplateSrc(CCoroutineCtx_CCFrameServerGate)
CCoroutineCtx_CCFrameServerGate::CCoroutineCtx_CCFrameServerGate(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName) {
	m_pServerSessionCtx = nullptr;
	m_nDelayDeleteTime = 0;
}

CCoroutineCtx_CCFrameServerGate::~CCoroutineCtx_CCFrameServerGate() {

}


int CCoroutineCtx_CCFrameServerGate::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func) {
	int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
	IsErrorHapper(nRet == 0, return nRet);
	char szBuf[64] = { 0 };
	sprintf(szBuf, "%s_serversessionctx", m_pCtxName);
	m_pServerSessionCtx = func(InitGetParamType_Config, szBuf, "CCoroutineCtx_CCFrameServerSession");
	IsErrorHapper(strlen(m_pServerSessionCtx) != 0, CCFrameSCBasicLogEventErrorV("ServerListen createserversessionctx %s 0", szBuf); return -1);
	sprintf(szBuf, "%s_delaydeletectxtime", m_pCtxName);
	m_nDelayDeleteTime = atol(func(InitGetParamType_Config, szBuf, "0"));
	if (m_nDelayDeleteTime > 0 && m_nDelayDeleteTime != 0xFFFFFFFF) {
		//10s检查一次
		CoroutineCtxAddOnTimer(m_ctxID, m_nDelayDeleteTime * 10, OnTimer);
	}
	return 0;
}

//! 不需要自己delete，只要调用release
void CCoroutineCtx_CCFrameServerGate::ReleaseCtx() {
	CoroutineCtxDelTimer(OnTimer);
	CCoroutineCtx::ReleaseCtx();
}


void CCoroutineCtx_CCFrameServerGate::OnTimer(CCoroutineCtx* pCtx) {
	CCoroutineCtx_CCFrameServerGate* pRedisCtx = (CCoroutineCtx_CCFrameServerGate*)pCtx;
	time_t tmNow = time(NULL);
	VTDelayRelease& checkData = pRedisCtx->m_vtQueue;
	MapUniqueKeyToCtxID& mapData  =pRedisCtx->m_mapKeyToCtxID;
	while (checkData.size() > 0) {
		auto& iterCheck = checkData.begin();
		auto& iter = mapData.find(*iterCheck);
		if (iter != mapData.end()) {
			if (iter->second.IsHaveReleaseTime()) {
				if (!iter->second.IsTimeToRelease(tmNow, pRedisCtx->m_nDelayDeleteTime)) {
					break;
				}
				CCtx_ThreadPool::GetThreadPool()->ReleaseObjectCtxByCtxID(iter->second.m_nCtxID);
				mapData.erase(iter);
			}
		}
		checkData.erase(iterCheck);
	}
}

//! 协程里面调用Bussiness消息
int CCoroutineCtx_CCFrameServerGate::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket) {
	switch (nType) {
	case CCoroutineCtx_CCFrameServerGate_Find:
		return DispathBussinessMsg_1_Find(pCorutine, nParam, pParam, pRetPacket);
	case CCoroutineCtx_CCFrameServerGate_Forward:
		return DispathBussinessMsg_2_Forward(pCorutine, nParam, pParam, pRetPacket);
	case CCoroutineCtx_CCFrameServerGate_SessionRelease:
		return DispathBussinessMsg_3_SessionRelease(pCorutine, nParam, pParam, pRetPacket);
	case CCoroutineCtx_CCFrameServerGate_ReceiveData:
		return DispathBussinessMsg_4_ReceiveData(pCorutine, nParam, pParam, pRetPacket);
	default:
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
		break;
	}
	return -99;
}

bool CCoroutineCtx_CCFrameServerGate::Create(CCorutinePlus* pCorutine, CCFrameServerSession* pSession, int64_t nUniqueKey, const std::function<void()>& funcBeforeInit) {
	auto& iter = m_mapKeyToCtxID.find(nUniqueKey);
	uint32_t m_nSessionCtxID = 0;
	if (iter == m_mapKeyToCtxID.end()) {
		m_nSessionCtxID = CCtx_ThreadPool::GetThreadPool()->CreateTemplateObjectCtx(m_pServerSessionCtx, nullptr, [&](InitGetParamType type, const char* pKey, const char* pDefault)->const char* {
			switch (type) {
			case InitGetParamType_Config:
				return CCtx_ThreadPool::GetThreadPool()->m_defaultFuncGetConfig(type, pKey, pDefault);
				break;
			case InitGetParamType_CreateUser:
			{
				if (strcmp(pKey, "object") == 0) {
					return (const char*)pSession;
				}
			}
			break;
			}
			return pDefault;
		});
		if (m_nSessionCtxID == 0) {
			CCFrameSCBasicLogEvent("VerifySuccessCallback but createctx fail");
			return false;
		}
	}
	else {
		m_nSessionCtxID = iter->second.m_nCtxID;
	}
	pSession->BindCtxID(m_nSessionCtxID, nUniqueKey);
	m_mapKeyToCtxID[nUniqueKey].Create(m_nSessionCtxID, pSession);
	funcBeforeInit();
	MACRO_ExeToCtxParam2(pCorutine, m_ctxID, m_nSessionCtxID, CCoroutineCtx_CCFrameServerSession_Init, pSession, &nUniqueKey, nullptr,
						 return false);
	return true;
}

long CCoroutineCtx_CCFrameServerGate::DispathBussinessMsg_1_Find(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	MACRO_DispatchCheckParam1(int64_t nUniqueKey, *(int64_t*))
	auto& iter = m_mapKeyToCtxID.find(nUniqueKey);
	if (iter != m_mapKeyToCtxID.end()) {
		uint32_t* pRet = (uint32_t*)pRetPacket;
		*pRet = iter->second.m_nCtxID;
	}
	return 0;
}
long CCoroutineCtx_CCFrameServerGate::DispathBussinessMsg_2_Forward(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	MACRO_DispatchCheckParam4(int64_t nUniqueKey, *(int64_t*), uint32_t nType, *(uint32_t*), int nRequestParam, *(int*), void** pRequestParam, (void**));
	auto& iter = m_mapKeyToCtxID.find(nUniqueKey);
	if (iter != m_mapKeyToCtxID.end()) {
		uint32_t nCtxID = iter->second.m_nCtxID;
		MACRO_ExeToCtx(pCorutine, 0, nCtxID, nType, nRequestParam, pRequestParam, pRetPacket,
					   return nRetExeToCtxType);
		return 0;
	}
	return -2;
}

bool CCoroutineCtx_CCFrameServerGate::SessionRelease(CCorutinePlus* pCorutine, CCFrameServerSession* pSession) {
	//这里不能使用ctxid，因为session已经把ctxid设置为0
	int64_t nUniqueKey = pSession->GetUniqueKey();
	auto& iter = m_mapKeyToCtxID.find(nUniqueKey);
	if (iter != m_mapKeyToCtxID.end()) {
		if (pSession == iter->second.m_pSession) {
			uint32_t nCtxID = iter->second.m_nCtxID;
			MACRO_ExeToCtx(pCorutine, m_ctxID, nCtxID, CCoroutineCtx_CCFrameServerSession_SessionRelease, 0, nullptr, nullptr,
						   return false);
			if (m_nDelayDeleteTime == 0) {
				CCtx_ThreadPool::GetThreadPool()->ReleaseObjectCtxByCtxID(nCtxID);
				m_mapKeyToCtxID.erase(iter);
			}
			else {
				//加入延迟删除队列
				m_vtQueue.push_back(nUniqueKey);
			}
		}
		else {
			//如果已经不属于这个session，不析构
		}
	}
	return true;
}

long CCoroutineCtx_CCFrameServerGate::DispathBussinessMsg_3_SessionRelease(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	MACRO_DispatchCheckParam1(CCFrameServerSession* pNotify, (CCFrameServerSession*));
	SessionRelease(pCorutine, pNotify);
	return 0;
}
long CCoroutineCtx_CCFrameServerGate::DispathBussinessMsg_4_ReceiveData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	//不知道怎么做撒都不做
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CCoroutineCtx_CCFrameServerGate::DealWithOnReceiveCtx(CCFrameServerSession* pSession, Net_Int cbData, const char* pszData){
	//默认全部给自己处理
	Ctx_CreateCoroutine(0, [](CCorutinePlus* pCorutine){
		CCorutineStackDataDefault stackData;
		CCoroutineCtx_CCFrameServerGate* pCtx = pCorutine->GetParamPoint<CCoroutineCtx_CCFrameServerGate>(0);
		CCFrameServerSession* pNotify = pCorutine->GetParamPoint<CCFrameServerSession>(1);
		RefCCFrameServerSession pSession = pNotify; //智能指针保证不会析构
		stackData.CopyData(pCorutine->GetParamPoint<const char>(2), pCorutine->GetParam<Net_Int>(3));//拷贝到堆数据
		MACRO_ExeToCtxParam2(pCorutine, 0, pCtx->GetCtxID(), CCoroutineCtx_CCFrameServerGate_ReceiveData, pNotify, &stackData, nullptr,
							 return);
	}, this, pSession, (void*)pszData, &cbData);
}
void CCoroutineCtx_CCFrameServerGate::DealWithOnReceiveToUniqueCtx(CCFrameServerSession* pSession, Net_Int cbData, const char* pszData) {
	Ctx_CreateCoroutine(0, [](CCorutinePlus* pCorutine){
		CCorutineStackDataDefault stackData;
		{
			CCoroutineCtx_CCFrameServerGate* pCtx = pCorutine->GetParamPoint<CCoroutineCtx_CCFrameServerGate>(0);
			CCFrameServerSession* pNotify = pCorutine->GetParamPoint<CCFrameServerSession>(1);
			RefCCFrameServerSession pSession = pNotify; //智能指针保证不会析构
			stackData.CopyData(pCorutine->GetParamPoint<const char>(2), pCorutine->GetParam<Net_Int>(3));//拷贝到堆数据
			MACRO_ExeToCtxParam2(pCorutine, 0, pSession->GetCtxSessionID(), CCoroutineCtx_CCFrameServerSession_ReceiveData, pNotify, &stackData, nullptr,
									return);
		}
	}, this, pSession, (void*)pszData, &cbData);
}
int32_t CCoroutineCtx_CCFrameServerGate::OnReceive(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData) {
	DealWithOnReceiveCtx((CCFrameServerSession*)pNotify, cbData, pszData);
	return BASIC_NET_OK;
}

int32_t CCoroutineCtx_CCFrameServerGate::OnDisconnect(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode) {
	CCFrameServerSession* pSession = (CCFrameServerSession*)pNotify;
	if (pSession->GetCtxSessionID() != 0) {
		Ctx_CreateCoroutine(0, [](CCorutinePlus* pCorutine){
			CCoroutineCtx_CCFrameServerGate* pGate = pCorutine->GetParamPoint<CCoroutineCtx_CCFrameServerGate>(0);
			RefCCFrameServerSession pSession = pCorutine->GetParamPoint<CCFrameServerSession>(1);
			MACRO_ExeToCtxParam1(pCorutine, 0, pGate->GetCtxID(), CCoroutineCtx_CCFrameServerGate_SessionRelease, pSession.GetResFunc(), nullptr,
									return);
		}, this, pSession);
		pSession->ResetSessionID();
	}
	return BASIC_NET_OK;
}