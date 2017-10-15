#include "ctx_tiaozhanpaiming.h"
#include "../log/ctx_log.h"
#include "../ctx_threadpool.h"

CreateTemplateSrc(CCtx_TiaoZhanPaiMing)

CCtx_TiaoZhanPaiMing::CCtx_TiaoZhanPaiMing(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName){
	m_nPaiMingCount = 100;
	m_pAllocateBuf = nullptr;
	m_pPaiMing = nullptr;
	m_nSameTimeTick = 0;
	m_nLastAddTime = 0;
	m_nTiaoZhanMingCiNum = 10;
}

CCtx_TiaoZhanPaiMing::~CCtx_TiaoZhanPaiMing(){
	if(m_pAllocateBuf){
		basiclib::BasicDeallocate(m_pAllocateBuf);
	}
	if(m_pPaiMing){
		delete[]m_pPaiMing;
	}
}

int CCtx_TiaoZhanPaiMing::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
	int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
	if(nRet != 0)
		return nRet;
	char szBuf[64] = { 0 };
	//先启动网关
	sprintf(szBuf, "%s_PaiMingNum", m_pCtxName);
	m_nPaiMingCount = atol(func(InitGetParamType_Config, szBuf, "100"));
	sprintf(szBuf, "%s_TiaoZhanPaiMingNum", m_pCtxName);
	m_nTiaoZhanMingCiNum = atol(func(InitGetParamType_Config, szBuf, "10"));
	CreatePaiMingData();
	return 0;
}

//! 加入排名数据
void CCtx_TiaoZhanPaiMing::AddData(BasePaiMingData* pData){
	BasePaiMingData* pStoreData = nullptr;
	auto& iter = m_mapData.find(pData->m_numKeyID);
	if(iter == m_mapData.end()){
		if(m_pPaiMing[pData->m_nMingCi]->m_numKeyID != 0){
			//拿最后一个
			pStoreData = m_pPaiMing[m_nPaiMingCount - 1];
			//删除key对应
			if(pStoreData->m_numKeyID != 0)
				m_mapData.erase(pStoreData->m_numKeyID);

			CopyPaiMingData(pStoreData, pData);
			//将后面的数据拷贝
			if(pData->m_nMingCi < m_nPaiMingCount - 1){
				memmove(&m_pPaiMing[pData->m_nMingCi + 1] , &m_pPaiMing[pData->m_nMingCi], (m_nPaiMingCount - pData->m_nMingCi - 1) * sizeof(BasePaiMingData*));
				for(int nBegin = pData->m_nMingCi + 1; nBegin < m_nPaiMingCount; nBegin++){
					m_pPaiMing[nBegin]->m_nMingCi += 1;
					ASSERT(nBegin == m_pPaiMing[nBegin]->m_nMingCi);
				}
			}
			m_pPaiMing[pData->m_nMingCi] = pStoreData;
			m_mapData[pData->m_numKeyID] = pStoreData;
		}
		else{
			pStoreData = m_pPaiMing[pData->m_nMingCi];
			CopyPaiMingData(pStoreData, pData);
			m_mapData[pData->m_numKeyID] = pStoreData;
		}
	}
	else{
		pStoreData = m_pPaiMing[iter->second->m_nMingCi];
		if(pStoreData->m_numKeyID != pData->m_numKeyID){
			ASSERT(0);
			return;
		}
		if(pData->m_nMingCi < iter->second->m_nMingCi){
			memmove(&m_pPaiMing[pData->m_nMingCi + 1], &m_pPaiMing[pData->m_nMingCi], (iter->second->m_nMingCi - pData->m_nMingCi) * sizeof(BasePaiMingData*));
			for(int nBegin = pData->m_nMingCi + 1; nBegin <= iter->second->m_nMingCi; nBegin++){
				m_pPaiMing[nBegin]->m_nMingCi += 1;
				ASSERT(nBegin == m_pPaiMing[nBegin]->m_nMingCi);
			}
			m_pPaiMing[pData->m_nMingCi] = pStoreData;
		}
		CopyPaiMingData(pStoreData, pData);
	}
}

//! 获取排名数据
Net_UShort CCtx_TiaoZhanPaiMing::GetPaiMing(Net_UInt key){
	auto& iter = m_mapData.find(key);
	if(iter == m_mapData.end()){
		return 0xFFFF;
	}
	return iter->second->m_nMingCi;
}

//! 获取名次
void CCtx_TiaoZhanPaiMing::GetMingCiID(Net_UShort nBegin, Net_UShort nEnd, const std::function<void(BasePaiMingData*)>& func){
	if(nEnd == 0xFFFF || nEnd > m_nPaiMingCount - 1)
		nEnd = m_nPaiMingCount - 1;
	if(nBegin > nEnd)
		return;

	int nIndex = 0;
	for(int i = nBegin; i <= nEnd; i++, nIndex++){
		if(m_pPaiMing[i]){
			func(m_pPaiMing[i]);
		}
	}
}

//! 初始化读取
void CCtx_TiaoZhanPaiMing::ReadFromDBData(time_t* pMinTime, VTDBReadData& dbData){
	BasePaiMingData** pPaiMing = new BasePaiMingData*[m_nPaiMingCount];
	for(int i = 0; i < m_nPaiMingCount; i++){
		pPaiMing[i] = nullptr;
	}
	//! 已经是排序好的队列
	for(auto& data : dbData){
		if(data.m_nTiaoZhanMingCi >= m_nPaiMingCount)
			continue;
		Net_UShort nFindMingCi = data.m_nTiaoZhanMingCi;
		do{
			if(pPaiMing[nFindMingCi] == nullptr){
				pPaiMing[nFindMingCi] = data.m_pBaseData;
				if(data.m_tmUpdateTime < *pMinTime){
					*pMinTime = data.m_tmUpdateTime;
				}
				break;
			}
			nFindMingCi++;
		} while(nFindMingCi < m_nPaiMingCount);
	}
	for(int i = 0; i < m_nPaiMingCount; i++){
		if(pPaiMing[i]){
			pPaiMing[i]->m_nMingCi = i;
			AddData(pPaiMing[i]);
		}
	}
	delete [] pPaiMing;
}

void CCtx_TiaoZhanPaiMing::CreatePaiMingData(){
	m_pAllocateBuf = basiclib::BasicAllocate(sizeof(BasePaiMingData) * m_nPaiMingCount);
	//真正的类型
	BasePaiMingData* pPoint = (BasePaiMingData*)m_pAllocateBuf;
	m_pPaiMing = new BasePaiMingData*[m_nPaiMingCount];
	for(int i = 0; i < m_nPaiMingCount; i++){
		m_pPaiMing[i] = new (pPoint + i)BasePaiMingData;
		m_pPaiMing[i]->m_nMingCi = i;
	}
}
//! 拷贝数据
void CCtx_TiaoZhanPaiMing::CopyPaiMingData(BasePaiMingData* pSelf, BasePaiMingData* pData){
	*pSelf = *pData;
}

//! 协程里面调用Bussiness消息
int CCtx_TiaoZhanPaiMing::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket){
	switch(nType){
	case CCtx_TiaoZhanPaiMing_CORUTINE_GetMingCiData:
		return DispathBussinessMsg_GetMingCiData(pCorutine, nParam, pParam, pRetPacket);
	case CCtx_TiaoZhanPaiMing_CORUTINE_GetPaiMingSelf:
		return DispathBussinessMsg_GetSelfPaiMing(pCorutine, nParam, pParam, pRetPacket);
	case CCtx_TiaoZhanPaiMing_CORUTINE_GetPaiMing:
		return DispathBussinessMsg_GetPaiMing(pCorutine, nParam, pParam, pRetPacket);
	case CCtx_TiaoZhanPaiMing_CORUTINE_InitFromDB:
		return DispathBussinessMsg_InitFromDB(pCorutine, nParam, pParam, pRetPacket);
	case CCtx_TiaoZhanPaiMing_CORUTINE_AddDataByUseID:
		return DispathBussinessMsg_AddDataByUseID(pCorutine, nParam, pParam, pRetPacket);
	case CCtx_TiaoZhanPaiMing_CORUTINE_GetMoreThanSelf:
		return DispathBussinessMsg_GetMoreThanSelf(pCorutine, nParam, pParam, pRetPacket);
	case CCtx_TiaoZhanPaiMing_CORUTINE_TiaoZhan:
		return DispathBussinessMsg_TiaoZhan(pCorutine, nParam, pParam, pRetPacket);
	default:
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
		break;
	}
	return -99;
}

int CCtx_TiaoZhanPaiMing::DispathBussinessMsg_GetMingCiData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam1(Net_UShort nMingCi, *(Net_UShort*));
	if(nMingCi < m_nPaiMingCount){
		BasePaiMingData* pRet = (BasePaiMingData*)pRetPacket;
		CopyPaiMingData(pRet, m_pPaiMing[nMingCi]);
	}
	else{
		ASSERT(0);
	}
	return 0;
}

int CCtx_TiaoZhanPaiMing::DispathBussinessMsg_GetSelfPaiMing(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam1(Net_UInt nKey, *(Net_UInt*));
	Net_UShort* pPaiMing = (Net_UShort*)pRetPacket;
	*pPaiMing = GetPaiMing(nKey);
	return 0;
}
int CCtx_TiaoZhanPaiMing::DispathBussinessMsg_GetPaiMing(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam2(Net_UShort nBegin, *(Net_UShort*), Net_UShort nEnd, *(Net_UShort*));
	std::function<void(BasePaiMingData*)>* pFunc = (std::function<void(BasePaiMingData*)>*)pRetPacket;
	GetMingCiID(nBegin, nEnd, *pFunc);
	return 0;
}
int CCtx_TiaoZhanPaiMing::DispathBussinessMsg_InitFromDB(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam1(VTDBReadData* pDBData, (VTDBReadData*));
	time_t* pMinTime = (time_t*)pRetPacket;
	ReadFromDBData(pMinTime, *pDBData);
	return 0;
}
int CCtx_TiaoZhanPaiMing::DispathBussinessMsg_AddDataByUseID(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam3(Net_UInt nKeyID, *(Net_UInt*), Net_UInt nDstKeyID, *(Net_UInt*), Net_UShort nTiaoZhanPaiMing, *(Net_UShort*));
	time_t tmNow = time(nullptr);
	if(tmNow == m_nLastAddTime)
		m_nSameTimeTick++;
	else{
		m_nSameTimeTick = 0;
		m_nLastAddTime = tmNow;
	}
	TiaoZhanPaiMingReadFromDB* pResult = (TiaoZhanPaiMingReadFromDB*)pRetPacket;
	pResult->m_pBaseData->m_numKeyID = nKeyID;
	//首先获取dstkey的排名
	if(nDstKeyID != 0)
		pResult->m_pBaseData->m_nMingCi = GetPaiMing(nDstKeyID);
	else
		pResult->m_pBaseData->m_nMingCi = nTiaoZhanPaiMing;
	if(pResult->m_pBaseData->m_nMingCi < m_nPaiMingCount){
		AddData(pResult->m_pBaseData);
		pResult->m_nTiaoZhanMingCi = pResult->m_pBaseData->m_nMingCi;
		pResult->m_tmUpdateTime = tmNow;
		pResult->m_nSameTimeTick = m_nSameTimeTick;
	}
	return 0;
}

int CCtx_TiaoZhanPaiMing::DispathBussinessMsg_GetMoreThanSelf(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam1(Net_UInt nKey, *(Net_UInt*));
	CanTiaoZhanData* pCanTiaoZhanRet = (CanTiaoZhanData*)pRetPacket;
	if(pCanTiaoZhanRet->m_nNum < m_nTiaoZhanMingCiNum * 2 || m_nTiaoZhanMingCiNum < 5)
		return DEFINECTX_RET_TYPE_RETPACKETTYPERROR;
	pCanTiaoZhanRet->m_nSelfPaiMing = GetPaiMing(nKey);
	Net_UShort nMingCiBegin = 0;
	if(pCanTiaoZhanRet->m_nSelfPaiMing >= m_nPaiMingCount){
		nMingCiBegin = m_nPaiMingCount - m_nTiaoZhanMingCiNum - 1;
	}
	else{
		if(pCanTiaoZhanRet->m_nSelfPaiMing >= m_nTiaoZhanMingCiNum){
			nMingCiBegin = pCanTiaoZhanRet->m_nSelfPaiMing - m_nTiaoZhanMingCiNum;
		}
		else{
			nMingCiBegin = 0;
		}
	}
	Net_UShort nMingCiEnd = nMingCiBegin + m_nTiaoZhanMingCiNum * 1.2;
	if(nMingCiEnd >= m_nPaiMingCount)
		nMingCiEnd = m_nPaiMingCount - 1;
	//先加入前三名
	int nAddNumber = 0;
	for(Net_UShort i = 0; i < 3; i++){
		CopyPaiMingData(pCanTiaoZhanRet->m_pBaseData[nAddNumber], m_pPaiMing[i]);
		nAddNumber++;
	}
	if(nMingCiBegin < 3){
		nMingCiBegin = 3;
	}
	for(Net_UShort i = nMingCiBegin; i <= nMingCiEnd; i++){
		CopyPaiMingData(pCanTiaoZhanRet->m_pBaseData[nAddNumber], m_pPaiMing[i]);
		nAddNumber++;
	}
	pCanTiaoZhanRet->m_nNum = nAddNumber;
	return 0;
}

int CCtx_TiaoZhanPaiMing::DispathBussinessMsg_TiaoZhan(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam3(Net_UInt nKey, *(Net_UInt*), Net_UInt nDstKey, *(Net_UInt*), Net_UShort nDstMingCi, *(Net_UShort*));
	Net_UShort nSelfPaiMing = GetPaiMing(nKey);
	if(nSelfPaiMing == 0xFFFF)
		nSelfPaiMing = m_nPaiMingCount - 1;
	if(nDstKey != 0)
		nDstMingCi = GetPaiMing(nDstKey);
	if(nDstMingCi >= m_nPaiMingCount || nSelfPaiMing < nDstMingCi){
		return 1001;
	}

	if(nDstMingCi + m_nTiaoZhanMingCiNum < nSelfPaiMing){
		return 1002;
	}
	BasePaiMingData* pRet = (BasePaiMingData*)pRetPacket;
	CopyPaiMingData(pRet, m_pPaiMing[nDstMingCi]);
	return 0;
}
