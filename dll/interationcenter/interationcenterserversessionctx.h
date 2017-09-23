#ifndef INTERATIONCENTERSERVERSESSION_CTX
#define INTERATIONCENTERSERVERSESSION_CTX

#include "../kernel/ccframe/initbussinesshead.h"
#include "../kernel/ccframe/net/ctx_ccframeserversession.h"
#include "../kernel/ccframe/public/kernelrequeststoredata.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

#define DEFINEDISPATCH_CORUTINE_InterationCenterServerSessionCtx_WakeUp				1000

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
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket);

    ////////////////////////////////////////////////////////////////////////////////////////
    //ҵ����, ȫ��ʹ�þ�̬����
    static void OnTimer(CCoroutineCtx* pCtx);
protected:
	virtual long DispathBussinessMsg_0_Init(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	virtual long DispathBussinessMsg_1_SessionRelease(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	virtual long DispathBussinessMsg_2_ReceiveData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);

	long DispathBussinessMsg_Wakeup(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
    //ctx�ڲ���ȫ
    basiclib::CBasicBitstream                               m_insCtxSafe;
    //���͵�������������
    MapKernelRequestStoreData                               m_mapRequest;
    int                                                     m_nDefaultTimeoutRequest;
};

#pragma warning (pop)

#endif