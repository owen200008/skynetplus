#ifndef SCBASIC_CCFRAME_MYSQL_H
#define SCBASIC_CCFRAME_MYSQL_H

#include "../dllmodule.h"
#include "../scbasic/commu/basicclient.h"
#include "../scbasic/mysql/mysqlconnector.h"
#include "../public/kernelrequeststoredata.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

struct MysqlExecReplyPacket{
    MysqlResultOK       m_ok;
    MysqlResultError    m_error;
};

class _SKYNET_KERNEL_DLL_API CCoroutineCtx_Mysql : public CCoroutineCtx
{
public:
    CCoroutineCtx_Mysql(const char* pKeyName = "mysql", const char* pClassName = GlobalGetClassName(CCoroutineCtx_Mysql));
    virtual ~CCoroutineCtx_Mysql();

    CreateTemplateHeader(CCoroutineCtx_Mysql);

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
    ////////////////////////////////////////////////////////////////////////////////////////
    //ҵ����, ȫ��ʹ�þ�̬����, �������Ա�֤��̬�⺯�������滻,������̬����
    static void OnTimer(CCoroutineCtx* pCtx);
protected:
    ////////////////////DispathBussinessMsg
    //! ��������
    long DispathBussinessMsg_0_Exec(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    long ExecReply(CCorutinePlus* pCorutine, MysqlReplyExec* pRetPacket);

    long DispathBussinessMsg_1_Query(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    long QueryReply(CCorutinePlus* pCorutine, MysqlReplyQuery* pRetPacket);

    long DispathBussinessMsg_2_Ping(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_3_Multi(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    //����ִ��sql
    void SendRequestQuery(const char* pSQL);
    void SendRequestPing();
    //��ȡһ��packet
    int GetPacketYield(CCorutinePlus* pCorutine, const std::function<int(unsigned char* pPacket, int nLength)>& func);
protected:
    int32_t OnConnect(basiclib::CBasicSessionNetClient* pClient, uint32_t nCode);
    int32_t OnReceiveVerify(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
    int32_t OnReceiveVerifyResult(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
    int32_t OnReceive(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData);
    int32_t OnDisconnect(basiclib::CBasicSessionNetClient* pNotify, Net_UInt dwNetCode);
    int32_t OnIdle(basiclib::CBasicSessionNetClient*, uint32_t);
protected:
    //! ����Э��
    static void Corutine_OnIdleSendPing(CCorutinePlus* pCorutine);
    //! ����Э��
    static void Corutine_OnReceiveData(CCorutinePlus* pCorutine);
protected:
    static void Func_ReceiveMysqlConnect(CCoroutineCtx* pCtx, ctx_message* pMsg);
    static void Func_ReceiveMysqlDisconnect(CCoroutineCtx* pCtx, ctx_message* pMsg);
    static void Func_ReceiveMysqlIdle(CCoroutineCtx* pCtx, ctx_message* pMsg);
protected:
    void ReceiveMysqlConnect(ctx_message* pMsg);
    void ReceiveMysqlDisconnect(ctx_message* pMsg);
protected:
    //����ͷ
    int Parse_PacketHead(unsigned char* pData, int nLength, Net_UChar& cField_count, Net_UChar& packetNO);
    void Compose_PacketHead(unsigned char* pData, Net_UInt nLength, Net_UChar packetNO);
    bool FromLengthCodedBin(unsigned char* pData, int nLength, int& nPos, int& nRet);
    bool FromLengthCodedStr(unsigned char* pData, int nLength, int& nPos, basiclib::CBasicString* strRet);
    bool FromLengthCodedStr(unsigned char* pData, int nLength, int& nPos, const std::function<void(int nPos, int nLength)>& func);

    void ParseErrorPacket(unsigned char* pData, int nLength, MysqlResultError& error);
    void ParseOKPacket(unsigned char* pData, int nLength, MysqlResultOK& error);
    bool ParseDataPacket(unsigned char* pData, int nLength, MysqlResultData& data);
    bool ParseFieldPacket(unsigned char* pData, int nLength, MysqlResultData& data, int nIndex);
    bool ParseRowDataPacket(unsigned char* pData, int nLength, MysqlResultData& data);
    void ParseEOFPacket(unsigned char* pData, int nLength, MysqlResultEOF& eofData);
protected:
    const char*                             m_pAddress;
    const char*                             m_pUserName;
    const char*                             m_pPwd;
    const char*                             m_pDB;

    CCommonClientSession*	                            m_pClient;
    CParseMySQLPacketBuffer		                        m_smRecvData;
    MsgQueueRequestStoreData                            m_vtRequest;
    int                                                 m_nDefaultTimeoutRequest;
    Net_UChar                                           m_cProtocal_Version;
    basiclib::CBasicString                              m_strServerVersion;
    uint32_t                                            m_nThread_id;
    Net_UInt					                        m_maxPacketSize;
};

//ʵ��ȫ�ְ�ȫ�ĺ�����װ����select��query
_SKYNET_KERNEL_DLL_API bool CCFrameMysqlExec(uint32_t nResCtxID, uint32_t nMysqlCtxID, CCorutinePlus* pCorutine, const char* pSQL, MysqlReplyExec& replyPacket);
_SKYNET_KERNEL_DLL_API bool CCFrameMysqlQuery(uint32_t nResCtxID, uint32_t nMysqlCtxID, CCorutinePlus* pCorutine, const char* pSQL, MysqlReplyQuery& replyPacket);
_SKYNET_KERNEL_DLL_API bool CCFrameMysqlMulty(uint32_t nResCtxID, uint32_t nMysqlCtxID, CCorutinePlus* pCorutine, MysqlMultiRequest* pRequest);

#pragma warning (pop)

#endif