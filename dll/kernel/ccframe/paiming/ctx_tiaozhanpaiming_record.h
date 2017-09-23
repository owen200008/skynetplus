#ifndef SKYNETPLUS_COROUTINE_CTX_TIAOZHANPAIMING_RECORD_H
#define SKYNETPLUS_COROUTINE_CTX_TIAOZHANPAIMING_RECORD_H

#include "../dllmodule.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)
struct TiaoZhanPaiMingStoreData{
	Net_UInt					m_nUniqueKey;
	basiclib::CBasicString		m_strRecord;
	time_t						m_tmGetTime;
	TiaoZhanPaiMingStoreData(){
		m_nUniqueKey = 0;
		m_tmGetTime = time(nullptr);
	}
	void ResetGetTime(){
		m_tmGetTime = time(nullptr);
	}
	bool IsTimeout(time_t tmNow){
		return tmNow - m_tmGetTime > 7200;
	}
};
typedef basiclib::basic_map<Net_UInt, TiaoZhanPaiMingStoreData>		MapUseIDToStoreTiaoZhanData;
typedef basiclib::basic_map<Net_UInt, MapUseIDToStoreTiaoZhanData>	MapTiaoZhanIDToMapData;
typedef basiclib::basic_map<Net_UInt, MapTiaoZhanIDToMapData>		MapTiaoZhanTypeToMapData;

class _SKYNET_KERNEL_DLL_API CCtx_TiaoZhanPaiMingStoreData : public CCoroutineCtx{
public:
	CCtx_TiaoZhanPaiMingStoreData(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCtx_TiaoZhanPaiMingStoreData));
	virtual ~CCtx_TiaoZhanPaiMingStoreData();

	CreateTemplateHeader(CCtx_TiaoZhanPaiMingStoreData);

	virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

	//! 不需要自己delete，只要调用release
	virtual void ReleaseCtx();

	//! 绑定读取record数据的函数
	static void BindReadFromDBFunc(const std::function<bool(CCorutinePlus*, uint32_t, Net_UInt, Net_UInt, Net_UInt, Net_UInt, basiclib::CBasicString&)>& func){
		m_readFromDBFunc = func;
	}

	TiaoZhanPaiMingStoreData* GetTiaoZhanStoreData(CCorutinePlus* pCorutine, Net_UInt nType, Net_UInt nTiaoZhanID, Net_UInt nKeyID, Net_UInt nUniqueKey);

	static void OnTimer(CCoroutineCtx* pCtx);
protected:
	bool ReadFromDB(CCorutinePlus* pCorutine, uint32_t nResCtxID, Net_UInt nType, Net_UInt nTiaoZhanID, Net_UInt nKeyID, Net_UInt nUniqueKey, basiclib::CBasicString& strRecord){
		return m_readFromDBFunc(pCorutine, nResCtxID, nType, nTiaoZhanID, nKeyID, nUniqueKey, strRecord);
	}
protected:
	MapTiaoZhanTypeToMapData						m_mapTiaoZhan;
	//! 直接使用静态函数
	static std::function<bool(CCorutinePlus*, uint32_t, Net_UInt, Net_UInt, Net_UInt, Net_UInt, basiclib::CBasicString&)>	m_readFromDBFunc;
};

_SKYNET_KERNEL_DLL_API bool Ctx_GetTiaoZhanStoreData(CCorutinePlus* pCorutine, uint32_t nResCtxID, uint32_t nPaiMingStoreCtxID, Net_UChar nType, Net_UInt nGuanQiaID, Net_UInt nKeyID, Net_UInt nUniqueKey, TiaoZhanPaiMingStoreData& storeData);

#pragma warning (pop)

#endif