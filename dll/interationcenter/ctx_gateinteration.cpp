#include "ctx_gateinteration.h"
#include "../kernel/ccframe/net/ccframeserver.h"
#include "../kernel/ccframe/coroutinedata/coroutinestackdata.h"
#include "../skynetplusprotocal/skynetplusprotocal_function.h"
#include "../skynetplusprotocal/skynetplusprotocal_error.h"

CreateTemplateSrc(CCoroutineCtx_GateInteration)
CCoroutineCtx_GateInteration::CCoroutineCtx_GateInteration(const char* pKeyName, const char* pClassName) : CCoroutineCtx_CCFrameServerGate(pKeyName, pClassName) {
}
CCoroutineCtx_GateInteration::~CCoroutineCtx_GateInteration() {

}


//! 协程里面调用Bussiness消息
int CCoroutineCtx_GateInteration::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket) {
	switch (nType) {
	case CCoroutineCtx_GateInteration_GetByID:
		return DispathBussinessMsg_GetByID(pCorutine, nParam, pParam, pRetPacket);
	case CCoroutineCtx_GateInteration_SelfCtxRegister:
		return DispathBussinessMsg_SelfCtxRegister(pCorutine, nParam, pParam, pRetPacket);
	case CCoroutineCtx_GateInteration_SelfCtxDelete:
		return DispathBussinessMsg_SelfCtxDelete(pCorutine, nParam, pParam, pRetPacket);
	}
	return CCoroutineCtx_CCFrameServerGate::DispathBussinessMsg(pCorutine, nType, nParam, pParam, pRetPacket);
}

long CCoroutineCtx_GateInteration::DispathBussinessMsg_GetByID(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	MACRO_DispatchCheckParam1(Net_UChar cIndex, *(Net_UChar*));
	auto& iter = m_mapGlobalKeyToSelfCtxID.find(cIndex);
	if (iter == m_mapGlobalKeyToSelfCtxID.end())
		return DEFINECTX_RET_TYPE_NoCtxID;
	InterationCenterCtxStoreInfo* pCtxStoreInfo = (InterationCenterCtxStoreInfo*)pRetPacket;
	*pCtxStoreInfo = iter->second;
	return DEFINECTX_RET_TYPE_SUCCESS;
}

long CCoroutineCtx_GateInteration::DispathBussinessMsg_SelfCtxRegister(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	MACRO_DispatchCheckParam2(const char* pCtxName, (const char*), uint32_t uCtxID, *(uint32_t*));
	auto& iter = m_mapNameToCtxID.find(pCtxName);
	if (iter != m_mapNameToCtxID.end()) {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) CreateNameToCtxID exist name %s(%d:%d)", __FUNCTION__, __FILE__, __LINE__, pCtxName, uCtxID, iter->second);
	}
	m_mapNameToCtxID[pCtxName] = (0xFF000000 | uCtxID);
	return 0;
}

long CCoroutineCtx_GateInteration::DispathBussinessMsg_SelfCtxDelete(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	MACRO_DispatchCheckParam1(const char* pCtxName, (const char*));
	auto& iter = m_mapNameToCtxID.find(pCtxName);
	if (iter == m_mapNameToCtxID.end()) {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) DeleteNameToCtxID no exist name %s", __FUNCTION__, __FILE__, __LINE__, pCtxName);
		return 0;
	}
	m_mapNameToCtxID.erase(iter);
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t CCoroutineCtx_GateInteration::OnReceive(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData) {
	CCFrameServerSession* pSession = (CCFrameServerSession*)pNotify;
	uint32_t nCtxID = pSession->GetCtxSessionID();
	if (cbData < sizeof(SProtoHead)) {
		CCFrameSCBasicLogEventErrorV("CCoroutineCtx_XKServer CtxID(%d)收到的包大小错误%d,断开连接", nCtxID, cbData);
		pSession->Close();
		return BASIC_NET_GENERIC_ERROR;
	}
	Net_UInt nMethod = 0;
	basiclib::UnSerializeUInt((unsigned char*)pszData, nMethod);
	Net_UInt nFieldType = nMethod & SPROTO_METHOD_FIELD_MASK;
	if (nFieldType == SPROTO_METHOD_FIELD_SKYNET) {
		if (nMethod == SPROTO_METHOD_SKYNET_Ping)
			return BASIC_NET_OK;
		DealWithOnReceiveCtx(pSession, cbData, pszData);
	}
	else {
#ifdef _DEBUG
		CCFrameSCBasicLogEventErrorV("%x:%d", nMethod, cbData);
#endif
		if (nCtxID != 0) {
			DealWithOnReceiveToUniqueCtx(pSession, cbData, pszData);
		}
		else {
			CCFrameSCBasicLogEventErrorV("%s(%s:%d) method error no ctxid method(%x)", __FUNCTION__, __FILE__, __LINE__, nMethod);
			pSession->Close(0);
		}
	}
	return BASIC_NET_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
long CCoroutineCtx_GateInteration::DispathBussinessMsg_4_ReceiveData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket) {
	MACRO_DispatchCheckParam2(CCFrameServerSession* pSession, (CCFrameServerSession*), CCorutineStackDataDefault* pDefaultStack, (CCorutineStackDataDefault*))
	Net_UInt nMethod = GetDefaultStackMethod(pDefaultStack);
	switch (nMethod) {
	case SPROTO_METHOD_SKYNET_Register: {
		SkynetPlusRegister request;
		SkynetPlusRegisterResponse response;
		do {
			IsErrorHapper(InsToSprotoStructDefaultStack(request, pDefaultStack, response),
				response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
			if (request.m_cIndex == 0 || request.m_cIndex == 0xFF) {
				//0代表自己，FF代表中心服务器
				ASSERT(0);
				CCFrameSCBasicLogEventErrorV("%s(%s:%d) method error %x", __FUNCTION__, __FILE__, __LINE__, request.m_head.m_nMethod);
				response.m_nError = SPROTO_ERRORID_SKYNETPLUS_REGISTER_ERRORREGISTERKEY;
				break;
			}
			auto& iter = m_mapGlobalKeyToSelfCtxID.find(request.m_cIndex);
			if (iter != m_mapGlobalKeyToSelfCtxID.end()) {
				if (iter->second.m_strServerKey.Compare(request.m_strKeyName.c_str()) != 0) {
					//已经存在，并且key不相同
					CCFrameSCBasicLogEventErrorV("%s(%s:%d) exist key no same (%d:%s:%s)", __FUNCTION__, __FILE__, __LINE__, request.m_cIndex, iter->second.m_strServerKey.c_str(), request.m_strKeyName.c_str());
					response.m_nError = SPROTO_ERRORID_SKYNETPLUS_REGISTER_KEYEXIST;
					break;
				}
			}
			IsErrorHapper(Create(pCorutine, pSession, request.m_cIndex, [&]() {
				SendWithSprotoStruct(pSession, response, m_insCtxSafe);
			}), response.m_nError = SPROTO_ERRORID_SKYNETPLUS_REGISTER_CREATECTXERR; break)
			InterationCenterCtxStoreInfo& storeInfo = m_mapGlobalKeyToSelfCtxID[request.m_cIndex];
			storeInfo.m_strServerKey = request.m_strKeyName;
			storeInfo.m_nSelfCtxID = pSession->GetCtxSessionID();
			storeInfo.m_nSessionID = pSession->GetSessionID();
			CCFrameSCBasicLogEventV("RegisterChildServer %d %s", request.m_cIndex, request.m_strKeyName.c_str());
		} while (false);
		//正确的情况在create里面的init前必须发送
		if (response.m_nError != 0) {
			SendWithSprotoStruct(pSession, response, m_insCtxSafe);
			pSession->Close();
		}
	}
	break;
	case SPROTO_METHOD_SKYNET_CreateNameToCtxID: {
		SkynetPlusCreateNameToCtxID request;
		SkynetPlusCreateNameToCtxIDResponse response;
		do {
			IsErrorHapper(pSession->GetCtxSessionID() != 0, response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_NoRegister; break)
			Net_UChar cIndex = (Net_UChar)pSession->GetUniqueKey();
			IsErrorHapper(InsToSprotoStructDefaultStack(request, pDefaultStack, response), response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
			for (auto& key : request.m_mapNameToCtxID) {
				uint32_t nSetCtxID = cIndex << 24 | key.second;
				auto& iter = m_mapNameToCtxID.find(key.first);
				if (iter != m_mapNameToCtxID.end()) {
					if (iter->second != nSetCtxID) {
						CCFrameSCBasicLogEventErrorV("%s(%s:%d) CreateNameToCtxID exist name %s(%d:%d)", __FUNCTION__, __FILE__, __LINE__, key.first.c_str(), nSetCtxID, iter->second);
					}
				}
				m_mapNameToCtxID[key.first] = nSetCtxID;
			}
			response.m_mapNameToCtxID = request.m_mapNameToCtxID;
		} while (false);
		SendWithSprotoStruct(pSession, response, m_insCtxSafe);
	}
		break;
	case SPROTO_METHOD_SKYNET_DeleteNameToCtxID: {
		SkynetPlusDeleteNameToCtxID request;
		SkynetPlusDeleteNameToCtxIDResponse response;
		do {
			IsErrorHapper(pSession->GetCtxSessionID() != 0, response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_NoRegister; break)
			IsErrorHapper(InsToSprotoStructDefaultStack(request, pDefaultStack, response),
				response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
			m_mapNameToCtxID.erase(request.m_strName);
			response.m_strName = request.m_strName;
		} while (false);
		SendWithSprotoStruct(pSession, response, m_insCtxSafe);
	}
		break;
	case SPROTO_METHOD_SKYNET_GetCtxIDByName: {
		SkynetPlusGetCtxIDByName request;
		SkynetPlusGetCtxIDByNameResponse response;
		do {
			IsErrorHapper(pSession->GetCtxSessionID() != 0, response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_NoRegister; break)
			IsErrorHapper(InsToSprotoStructDefaultStack(request, pDefaultStack, response),
				response.m_nError = SPROTO_ERRORID_SKYNETPLUS_Global_ParseError; break);
			auto& iter = m_mapNameToCtxID.find(request.m_strName);
			IsErrorHapper(iter != m_mapNameToCtxID.end(), response.m_nError = SPROTO_ERRORID_SKYNETPLUS_GetNameToCtx_NoName; break);
			response.m_nCtxID = iter->second;
		} while (false);
		SendWithSprotoStruct(pSession, response, m_insCtxSafe);
	}
		break;
	default: {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow method %x", __FUNCTION__, __FILE__, __LINE__, nMethod);
		ASSERT(0);
	}
		break;
	}
	return 0;
}

bool CCoroutineCtx_GateInteration::SessionRelease(CCorutinePlus* pCorutine, CCFrameServerSession* pSession) {
	Net_UChar cIndex = (Net_UChar)pSession->GetUniqueKey();
	auto& iter = m_mapGlobalKeyToSelfCtxID.find(cIndex);
	if (iter == m_mapGlobalKeyToSelfCtxID.end()) {
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) disconnect no find cindex(%d)", __FUNCTION__, __FILE__, __LINE__, cIndex);
		return false;
	}
	m_mapGlobalKeyToSelfCtxID.erase(iter);
	return CCoroutineCtx_CCFrameServerGate::SessionRelease(pCorutine, pSession);
}