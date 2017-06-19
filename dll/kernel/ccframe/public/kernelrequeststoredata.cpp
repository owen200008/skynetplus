#include "kernelrequeststoredata.h"

CKernelMapRequestMgr::CKernelMapRequestMgr(MapKernelRequestStoreData& mapRequest, CCorutinePlus* pCorutine, uint32_t nSessionID) : m_mapRequest(mapRequest){
    //! ±£´æ
    m_nSessionID = nSessionID;
    m_mapRequest[m_nSessionID].Reset(pCorutine);
}

CKernelMapRequestMgr::~CKernelMapRequestMgr(){
    m_mapRequest.erase(m_nSessionID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
CKernelMsgQueueRequestMgr::CKernelMsgQueueRequestMgr(MsgQueueRequestStoreData& vtRequest, CCorutinePlus* pCorutine) : m_vtRequest(vtRequest){
    //! ±£´æ
    m_pCorutine = pCorutine;
    KernelRequestStoreData request(pCorutine);
    m_vtRequest.MQPush(&request);
}

CKernelMsgQueueRequestMgr::~CKernelMsgQueueRequestMgr(){
    KernelRequestStoreData request;
    m_vtRequest.MQPop(&request);
	if (request.m_pCorutine != m_pCorutine) {
		ASSERT(0);
	}
    //ASSERT(request.m_pCorutine == m_pCorutine);
}
