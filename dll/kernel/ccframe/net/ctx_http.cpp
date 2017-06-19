#include "ctx_http.h"
#include "../log/ctx_log.h"
#include "../ctx_threadpool.h"
#include "../coroutinedata/coroutinestackdata.h"
#include "../scbasic/http/httprequest.h"
#include "../scbasic/http/httpresponse.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "../scbasic/http/httpdefine.h"

using namespace basiclib;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CCoroutineCtx_Http)
CCoroutineCtx_Http::CCoroutineCtx_Http(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName){
	m_pHttpServer = nullptr;
	m_L = nullptr;
}

CCoroutineCtx_Http::~CCoroutineCtx_Http(){
}

int CCoroutineCtx_Http::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
    IsErrorHapper(nRet == 0, return nRet);
	char szBuf[64] = { 0 };
	sprintf(szBuf, "%s_Address", m_pCtxName);
    m_pAddress = func(InitGetParamType_Config, szBuf, "");
    if (m_pAddress == nullptr || strlen(m_pAddress) == 0){
        return 2;
    }
	sprintf(szBuf, "%s_iptrust", m_pCtxName);
	m_pTrust = func(InitGetParamType_Config, szBuf, "*");

	lua_State *L = luaL_newstate();
	luaL_openlibs(L);   // link lua lib
	m_L = L;

	m_pHttpServer = CHttpSessionServer::CreateHttpServer(0);
	m_pHttpServer->bind_onhttpask(MakeFastFunction(this, &CCoroutineCtx_Http::OnHttpAsk));
	m_pHttpServer->SetIpTrust(m_pTrust);
	Net_Int nHttpRet = m_pHttpServer->StartServer(m_pAddress);
	if (nHttpRet != BASIC_NET_OK) {
		//10s检查一次
		CoroutineCtxAddOnTimer(m_ctxID, 1000, OnTimer);
	}
    return 0;
}

//! 不需要自己delete，只要调用release
void CCoroutineCtx_Http::ReleaseCtx(){
	if (m_pHttpServer)
		m_pHttpServer->Release();
	if(m_L)
		lua_close(m_L);
    CoroutineCtxDelTimer(OnTimer);
    CCoroutineCtx::ReleaseCtx();
}

long CCoroutineCtx_Http::OnHttpAsk(RefHttpSession pSession, HttpRequest* pRequest, HttpResponse& response){
	const int nSetParam = 3;
	void* pSetParam[nSetParam] = { this, pSession.GetResFunc(), pRequest };
	CreateResumeCoroutineCtx([](CCorutinePlus* pCorutine)->void {
		basiclib::CBasicSmartBuffer smCacheBuf;
		int nGetParamCount = 0;
		void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
		CCoroutineCtx_Http* pCtx = (CCoroutineCtx_Http*)pGetParam[0];
		RefHttpSession pSession = RefHttpSession((CHttpSession*)pGetParam[1]);//智能指针保证不会析构
		HttpRequest* pRequest = (HttpRequest*)pGetParam[2];
		HttpResponse response;

		CCoroutineCtx* pCurrentCtx = nullptr;
		ctx_message* pCurrentMsg = nullptr;
		if (!YieldCorutineToCtx(pCorutine, pCtx->GetCtxID(), pCurrentCtx, pCurrentMsg)) {
			pSession->Close();
			return;
		}
		long lRet = pCtx->OnCtxHttpAsk(pSession, pRequest, response, pCorutine);
		if (lRet == HTTP_SUCC) {
			pSession->AsynSendResponse(pRequest, response, smCacheBuf);
		}
		else if (lRet == HTTP_INTERNAL_ERROR) {
			pSession->Close();
		}
		else {
			pSession->Close();
			ASSERT(0);
		}
	}, 0, nSetParam, pSetParam);
	return HTTP_ASYNC;
}

long CCoroutineCtx_Http::OnCtxHttpAsk(RefHttpSession pSession, HttpRequest* pRequest, HttpResponse& response, CCorutinePlus* pCorutine) {
	SCBasicDocument doc(rapidjson::kObjectType);
	long lRet = DispathHttpAsk(pSession, doc, pRequest, response, pCorutine);
	if (lRet == HTTP_SUCC) {
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
		response.AppendContent(buffer.GetString(), buffer.GetLength());
	};
	return lRet;
}

long CCoroutineCtx_Http::DispathHttpAsk(RefHttpSession pSession, SCBasicDocument& doc, HttpRequest* pRequest, HttpResponse& response, CCorutinePlus* pCorutine){
	const char* pFunction = pRequest->GetParamValue("function");
	if (pFunction == nullptr) {
		Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_NoFunction);
	}
	else {
		MapFunctionToPointFunc::iterator iter = m_mapHttpFunctionToPointFunc.find(pFunction);
		if (iter != m_mapHttpFunctionToPointFunc.end()) {
			if (iter->second.m_pIpVerify) {
				if (!iter->second.m_pIpVerify->IsIpTrust(pSession->GetNetAddress())) {
					Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_NoTrust);
					return HTTP_SUCC;
				}
			}
			return iter->second.m_func(this, pSession, doc, pRequest, response, pCorutine, iter->second.m_pUD);
		}
		else {
			Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_NoRegFunction);
		}
	}
	return HTTP_SUCC;
}

void CCoroutineCtx_Http::OnTimer(CCoroutineCtx* pCtx){
	CCoroutineCtx_Http* pHttpCtx = (CCoroutineCtx_Http*)pCtx;
	if(pHttpCtx->m_pHttpServer->IsListen()){
		CoroutineCtxDelTimer(OnTimer);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! 协程里面调用Bussiness消息
int CCoroutineCtx_Http::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    switch (nType){
    case Define_Http_Ctx_Bussiness_AddFunc:
        return DispathBussinessMsg_0_AddFunc(pCorutine, nParam, pParam, pRetPacket);
	case Define_Http_Ctx_Bussiness_DelFunc:
		return DispathBussinessMsg_1_DelFunc(pCorutine, nParam, pParam, pRetPacket);
    default:
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
        break;
    }
    return -99;
}

long CCoroutineCtx_Http::DispathBussinessMsg_0_AddFunc(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 4, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
	const char* pFunction = (const char*)pParam[0];
	RegisterDealWithHttpFunction pFunc = (RegisterDealWithHttpFunction)pParam[1];
	const char* pTrustIP = (const char*)pParam[2];
	void* pUD = pParam[3];

	MapFunctionToPointFunc::iterator iter = m_mapHttpFunctionToPointFunc.find(pFunction);
	if (iter != m_mapHttpFunctionToPointFunc.end()) {
		ASSERT(0);
		return 0;
	}
	m_mapHttpFunctionToPointFunc[pFunction].m_func = pFunc;
	m_mapHttpFunctionToPointFunc[pFunction].m_pUD = pUD;
	if (pTrustIP) {
		m_mapHttpFunctionToPointFunc[pFunction].SetIPTrust(pTrustIP);
	}
	return 0;
}

long CCoroutineCtx_Http::DispathBussinessMsg_1_DelFunc(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    IsErrorHapper(nParam == 1, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) paramerror(%d)", __FUNCTION__, __FILE__, __LINE__, nParam); return -1);
	const char* pFunction = (const char*)pParam[0];
	m_mapHttpFunctionToPointFunc.erase(pFunction);
	return 0;
}

bool CCFrameHttpRegister(uint32_t nSrcCtxID, uint32_t nHttpCtxID, CCorutinePlus* pCorutine, const char* pFunction, RegisterDealWithHttpFunction func, const char* pTrustIP, void* pUD) {
	const int nSetParam = 4;
	void* pSetParam[nSetParam] = { (void*)pFunction, func, (void*)pTrustIP, pUD };
	int nRetValue = 0;
	if (!WaitExeCoroutineToCtxBussiness(pCorutine, nSrcCtxID, nHttpCtxID, Define_Http_Ctx_Bussiness_AddFunc, nSetParam, pSetParam, nullptr, nRetValue)) {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) error(%s)", __FUNCTION__, __FILE__, __LINE__, pFunction);
		return false;
	}
	return true;
}
bool CCFrameHttpUnRegister(uint32_t nSrcCtxID, uint32_t nHttpCtxID, CCorutinePlus* pCorutine, const char* pFunction) {
	const int nSetParam = 1;
	void* pSetParam[nSetParam] = { (void*)pFunction };
	int nRetValue = 0;
	if (!WaitExeCoroutineToCtxBussiness(pCorutine, nSrcCtxID, nHttpCtxID, Define_Http_Ctx_Bussiness_DelFunc, nSetParam, pSetParam, nullptr, nRetValue)) {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) error(%s)", __FUNCTION__, __FILE__, __LINE__, pFunction);
		return false;
	}
	return true;
}
