#ifndef SCBASIC_CCFRAME_NETCLIENT_H
#define SCBASIC_CCFRAME_NETCLIENT_H

#include "../dllmodule.h"
#include "../scbasic/commu/basicclient.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

#define DEFINEDISPATCH_CORUTINE_NetClient_Receive   0
class _SKYNET_KERNEL_DLL_API CCoroutineCtx_NetClient : public CCoroutineCtx
{
public:
    CCoroutineCtx_NetClient(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCoroutineCtx_Mysql));
    virtual ~CCoroutineCtx_NetClient();

    CreateTemplateHeader(CCoroutineCtx_NetClient);

    //! ��ʼ��0����ɹ�
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! ����Ҫ�Լ�delete��ֻҪ����release
    virtual void ReleaseCtx();

    //! Э���������Bussiness��Ϣ
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

    //! �ж��Ƿ����ӣ����û�����ӷ�������
    void CheckConnect();
    //! ����رպ���
    void ErrorClose();
    //! �ж��Ƿ��Ѿ���֤�ɹ�
    bool IsTransmit();
    //! ��������
    void SendData(const char* pData, int nLength);
    ////////////////////////////////////////////////////////////////////////////////////////
    //ҵ����, ȫ��ʹ�þ�̬����, �������Ա�֤��̬�⺯�������滻,������̬����
    static void OnTimer(CCoroutineCtx* pCtx);
protected:
    virtual long DispathBussinessMsg_Receive(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
        return 0;
    }
protected:
    virtual void OnTimerNetClient();
    virtual int32_t OnConnect(basiclib::CBasicSessionNetClient* pClient, uint32_t nCode);
    virtual int32_t OnReceive(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
    virtual int32_t OnIdle(basiclib::CBasicSessionNetClient*, uint32_t);
    virtual int32_t OnDisconnect(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode);
protected:
    //! ����Э��
    static void Corutine_OnReceiveData(CCorutinePlus* pCorutine);
protected:
    virtual CCommonClientSession* CreateCommonClientSession(const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);
protected:
    const char*                             m_pAddress;

    CCommonClientSession*	                m_pClient;
};

#pragma warning (pop)

#endif