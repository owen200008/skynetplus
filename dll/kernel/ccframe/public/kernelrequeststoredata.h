#ifndef SKYNETPLUS_KERNELREQUESTSTOREDATA_H
#define SKYNETPLUS_KERNELREQUESTSTOREDATA_H

#include <basic.h>
#include "../../kernel_head.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

struct _SKYNET_KERNEL_DLL_API KernelRequestStoreData{
    CCorutinePlus*      m_pCorutine;
    time_t              m_tmRequestTime;
    KernelRequestStoreData(){
        memset(this, 0, sizeof(KernelRequestStoreData));
    }
    KernelRequestStoreData(CCorutinePlus* pCorutine){
        Reset(pCorutine);
    }
    void Reset(CCorutinePlus* pCorutine){
        m_pCorutine = pCorutine;
        m_tmRequestTime = time(NULL);
    }
    bool IsTimeOut(time_t dwNowTick, int nTimeOut){
        return dwNowTick >= m_tmRequestTime + nTimeOut;
    }
};
typedef basiclib::basic_map<uint32_t, KernelRequestStoreData> MapKernelRequestStoreData;
class _SKYNET_KERNEL_DLL_API CKernelMapRequestMgr{
public:
    CKernelMapRequestMgr(MapKernelRequestStoreData& mapRequest, CCorutinePlus* pCorutine, uint32_t nSessionID);
    ~CKernelMapRequestMgr();
protected:
    uint32_t                                            m_nSessionID;
    MapKernelRequestStoreData&                          m_mapRequest;
};

typedef basiclib::CMessageQueue<KernelRequestStoreData> MsgQueueRequestStoreData;
class _SKYNET_KERNEL_DLL_API CKernelMsgQueueRequestMgr{
public:
    CKernelMsgQueueRequestMgr(MsgQueueRequestStoreData& vtRequest, CCorutinePlus* pCorutine);
    ~CKernelMsgQueueRequestMgr();
protected:
    CCorutinePlus*                                      m_pCorutine;
    MsgQueueRequestStoreData&                           m_vtRequest;
};

#pragma warning (pop)

#endif