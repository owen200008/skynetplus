#include "ctx_tiaozhanpaiming_record.h"
#include "../log/ctx_log.h"
#include "../ctx_threadpool.h"

CreateTemplateSrc(CCtx_TiaoZhanPaiMingStoreData)

std::function<bool(CCorutinePlus*, uint32_t, Net_UInt, Net_UInt, Net_UInt, Net_UInt, basiclib::CBasicString&)> CCtx_TiaoZhanPaiMingStoreData::m_readFromDBFunc;
CCtx_TiaoZhanPaiMingStoreData::CCtx_TiaoZhanPaiMingStoreData(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName){

}

CCtx_TiaoZhanPaiMingStoreData::~CCtx_TiaoZhanPaiMingStoreData(){

}

int CCtx_TiaoZhanPaiMingStoreData::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
	int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
	if(nRet != 0)
		return nRet;
	//600s检查一次
	CoroutineCtxAddOnTimer(m_ctxID, 60000, OnTimer);
	return 0;
}

//! 不需要自己delete，只要调用release
void CCtx_TiaoZhanPaiMingStoreData::ReleaseCtx(){
	CoroutineCtxDelTimer(OnTimer);
	CCoroutineCtx::ReleaseCtx();
}

void CCtx_TiaoZhanPaiMingStoreData::OnTimer(CCoroutineCtx* pCtx){
	time_t tmNow = time(nullptr);
	{
		basiclib::basic_vector<Net_UInt> vtDeleteKey;
		CCtx_TiaoZhanPaiMingStoreData* pMyCtx = (CCtx_TiaoZhanPaiMingStoreData*)pCtx;
		for(auto& tiaozhanidmap : pMyCtx->m_mapTiaoZhan){
			for(auto& useridmap : tiaozhanidmap.second){
				for(auto& userdata : useridmap.second){
					if(userdata.second.IsTimeout(tmNow)){
						vtDeleteKey.push_back(userdata.first);
					}
				}
				for(auto& key : vtDeleteKey){
					useridmap.second.erase(key);
				}
				vtDeleteKey.clear();
			}
		}
	}
}

TiaoZhanPaiMingStoreData* CCtx_TiaoZhanPaiMingStoreData::GetTiaoZhanStoreData(CCorutinePlus* pCorutine, Net_UInt nType, Net_UInt nTiaoZhanID, Net_UInt nKeyID, Net_UInt nUniqueKey){
	MapUseIDToStoreTiaoZhanData& data = m_mapTiaoZhan[nType][nTiaoZhanID];
	auto& userIter = data[nKeyID];
	if(userIter.m_nUniqueKey == nUniqueKey)
		return &userIter;
	//重新读取一次
	if(!ReadFromDB(pCorutine, m_ctxID, nKeyID, nType, nTiaoZhanID, nUniqueKey, userIter.m_strRecord)){
		data.erase(nKeyID);
		return nullptr;
	}
	userIter.m_nUniqueKey = nUniqueKey;
	return &userIter;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Ctx_GetTiaoZhanStoreData(CCorutinePlus* pCorutine, uint32_t nResCtxID, uint32_t nPaiMingStoreCtxID, Net_UChar nType, Net_UInt nGuanQiaID, Net_UInt nKeyID, Net_UInt nUniqueKey, TiaoZhanPaiMingStoreData& storeData){
	MACRO_YieldToCtx(pCorutine, nPaiMingStoreCtxID,
					 return false);
	TiaoZhanPaiMingStoreData* pRet = ((CCtx_TiaoZhanPaiMingStoreData*)pCorutine->GetCurrentCtx())->GetTiaoZhanStoreData(pCorutine, nType, nGuanQiaID, nKeyID, nUniqueKey);
	if(pRet)
		storeData = *pRet;
	if(nResCtxID != 0){
		MACRO_YieldToCtx(pCorutine, nResCtxID,
						 return false);
	}
	return true;

}
