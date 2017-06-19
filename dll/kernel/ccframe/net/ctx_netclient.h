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

    //! 初始化0代表成功
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! 不需要自己delete，只要调用release
    virtual void ReleaseCtx();

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg);

    //! 判断是否连接，如果没有连接发起连接
    void CheckConnect();
    //! 错误关闭函数
    void ErrorClose();
    //! 判断是否已经认证成功
    bool IsTransmit();
    //! 发送数据
    void SendData(const char* pData, int nLength);
    ////////////////////////////////////////////////////////////////////////////////////////
    //业务类, 全部使用静态函数, 这样可以保证动态库函数可以替换,做到动态更新
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
    //! 创建协程
    static void Corutine_OnReceiveData(CCorutinePlus* pCorutine);
protected:
    virtual CCommonClientSession* CreateCommonClientSession(const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);
protected:
    const char*                             m_pAddress;

    CCommonClientSession*	                m_pClient;
};

#pragma warning (pop)

#endif