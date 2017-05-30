#ifndef INTERATIONCENTERSERVERSESSION_CTX
#define INTERATIONCENTERSERVERSESSION_CTX

#include "../kernel/ccframe/initbussinesshead.h"
#include "../kernel/ccframe/net/ctx_ccframeserversession.h"
#include "../kernel/ccframe/public/kernelrequeststoredata.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

#define DEFINEDISPATCH_CORUTINE_InterationCenterServerSessionCtx_RequestPacket      0 

//revert
#define ServerSessionRevert_Key_RemoteID            0
#define ServerSessionRevert_Key_RemoteKey           1
class _SKYNET_BUSSINESS_DLL_API CInterationCenterServerSessionCtx : public CCoroutineCtx_CCFrameServerSession
{
public:
    CInterationCenterServerSessionCtx(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CInterationCenterServerSessionCtx));
    virtual ~CInterationCenterServerSessionCtx();
    CreateTemplateHeader(CInterationCenterServerSessionCtx);

    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! ����Ҫ�Լ�delete��ֻҪ����release
    virtual void ReleaseCtx();

    //! Э���������Bussiness��Ϣ
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

    ////////////////////////////////////////////////////////////////////////////////////////
    //ҵ����, ȫ��ʹ�þ�̬����
    static void OnTimer(CCoroutineCtx* pCtx, CCtx_CorutinePlusThreadData* pData);
protected:
    long DispathBussinessMsg_RequestPacket(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, int nParam, void** pParam, void* pRetPacket);
protected:
    //ctx�ڲ���ȫ
    basiclib::CBasicBitstream                               m_insCtxSafe;
    //���͵�������������
    MapKernelRequestStoreData                               m_mapRequest;
    int                                                     m_nDefaultTimeoutRequest;
};

#pragma warning (pop)

#endif