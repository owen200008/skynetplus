#include "ctx_mysql.h"
#include "../log/ctx_log.h"
#include "../ctx_threadpool.h"
#include "../coroutinedata/coroutinestackdata.h"

using namespace basiclib;

#define SERVER_MORE_RESULTS_EXISTS  8
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CreateTemplateSrc(CCoroutineCtx_Mysql)
CCoroutineCtx_Mysql::CCoroutineCtx_Mysql(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName){
    m_pAddress = nullptr;
    m_pPwd = nullptr;
    m_pDB = nullptr;
    m_pClient = nullptr;
    m_cProtocal_Version = 0;
    m_nThread_id = 0;
    m_maxPacketSize = 1024 * 1024;
    m_nDefaultTimeoutRequest = 5;
}

CCoroutineCtx_Mysql::~CCoroutineCtx_Mysql(){
    if (m_pClient){
        m_pClient->SafeDelete();
    }
}

int CCoroutineCtx_Mysql::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
    IsErrorHapper(nRet == 0, return nRet);
    char szBuf[64] = { 0 };
    sprintf(szBuf, "%s_Address", m_pCtxName);
    m_pAddress = func(InitGetParamType_Config, szBuf, "");
    if (m_pAddress == nullptr || strlen(m_pAddress) == 0){
        return 2;
    }
    sprintf(szBuf, "%s_AddressUserName", m_pCtxName);
    m_pUserName = func(InitGetParamType_Config, szBuf, "");
    sprintf(szBuf, "%s_AddressPwd", m_pCtxName);
    m_pPwd = func(InitGetParamType_Config, szBuf, "");
    sprintf(szBuf, "%s_AddressDB", m_pCtxName);
    m_pDB = func(InitGetParamType_Config, szBuf, "");
    sprintf(szBuf, "%s_timeout", m_pCtxName);
    m_nDefaultTimeoutRequest = atol(func(InitGetParamType_Config, szBuf, "5"));
    m_pClient = CCommonClientSession::CreateNetClient();
    m_pClient->bind_connect(MakeFastFunction(this, &CCoroutineCtx_Mysql::OnConnect));
    m_pClient->bind_disconnect(MakeFastFunction(this, &CCoroutineCtx_Mysql::OnDisconnect));
    m_pClient->bind_idle(MakeFastFunction(this, &CCoroutineCtx_Mysql::OnIdle));
    m_pClient->Connect(m_pAddress);
    //1s检查一次
    CoroutineCtxAddOnTimer(m_ctxID, 100, OnTimer);
    return 0;
}

//! 不需要自己delete，只要调用release
void CCoroutineCtx_Mysql::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimer);
    CCoroutineCtx::ReleaseCtx();
}


int32_t CCoroutineCtx_Mysql::OnConnect(CBasicSessionNetNotify* pClient, uint32_t nCode){
    m_pClient->bind_rece(MakeFastFunction(this, &CCoroutineCtx_Mysql::OnReceiveVerify));
    //执行
    ctx_message ctxMsg(0, CCoroutineCtx_Mysql::Func_ReceiveMysqlConnect);
    PushMessage(ctxMsg);
    return BASIC_NET_HC_RET_HANDSHAKE;
}
void CCoroutineCtx_Mysql::Func_ReceiveMysqlConnect(CCoroutineCtx* pCtx, ctx_message* pMsg){
    CCoroutineCtx_Mysql* pMySql = (CCoroutineCtx_Mysql*)pCtx;
    pMySql->ReceiveMysqlConnect(pMsg);
}
void CCoroutineCtx_Mysql::ReceiveMysqlConnect(ctx_message* pMsg){
    m_smRecvData.SetDataLength(0);
}

int32_t CCoroutineCtx_Mysql::OnDisconnect(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode){
    ctx_message ctxMsg(0, CCoroutineCtx_Mysql::Func_ReceiveMysqlDisconnect);
    PushMessage(ctxMsg);
    return BASIC_NET_OK;
}

void CCoroutineCtx_Mysql::Func_ReceiveMysqlDisconnect(CCoroutineCtx* pCtx, ctx_message* pMsg){
    CCoroutineCtx_Mysql* pMySql = (CCoroutineCtx_Mysql*)pCtx;
    pMySql->ReceiveMysqlDisconnect(pMsg);
}
void CCoroutineCtx_Mysql::ReceiveMysqlDisconnect(ctx_message* pMsg){
    //发送所有mysql消息断开了,不能保存指针不然指向同一个(最后一个)
	basiclib::basic_vector<KernelRequestStoreData> vtDeleteRequest;
    m_vtRequest.ForeachData([&](KernelRequestStoreData* pRequest)->void{
		vtDeleteRequest.push_back(*pRequest);
    });
	for (auto& deldata : vtDeleteRequest) {
		Ctx_ResumeCoroutine_Fail(deldata.m_pCorutine);
	}
    if (m_vtRequest.GetMQLength() != 0){
        ASSERT(0);
        CCFrameSCBasicLogEventErrorV("%s(%s:%d) vtrequest no empty", __FUNCTION__, __FILE__, __LINE__);
        m_vtRequest.Drop_Queue([&](KernelRequestStoreData* pRequest)->void{});
    }
}

int32_t CCoroutineCtx_Mysql::OnIdle(basiclib::CBasicSessionNetNotify*, uint32_t nIdle){
    if (nIdle % 30 == 29){
        ctx_message ctxMsg(0, CCoroutineCtx_Mysql::Func_ReceiveMysqlIdle);
        PushMessage(ctxMsg);
    }
    return BASIC_NET_OK;
}
void CCoroutineCtx_Mysql::Func_ReceiveMysqlIdle(CCoroutineCtx* pCtx, ctx_message* pMsg){
	Ctx_CreateCoroutine(0, CCoroutineCtx_Mysql::Corutine_OnIdleSendPing, pCtx);
}

//! 创建协程
void CCoroutineCtx_Mysql::Corutine_OnIdleSendPing(CCorutinePlus* pCorutine){
    //外部变量一定要谨慎，线程安全问题
    {
		CCoroutineCtx_Mysql* pMysql = pCorutine->GetParamPoint<CCoroutineCtx_Mysql>(0);
        MysqlReplyExec replyPacket;
		MACRO_ExeToCtx(pCorutine, 0, pMysql->GetCtxID(), DEFINEDISPATCH_CORUTINE_SQLPING, 0, nullptr, &replyPacket,
					   return);
        IsErrorHapper(replyPacket.m_cFieldCount == 0x00, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) Receive No OK Packet, Close", __FUNCTION__, __FILE__, __LINE__); pMysql->m_pClient->Close(0);return);
    }
}

int32_t CCoroutineCtx_Mysql::OnReceiveVerify(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    if (cbData < 5){
        return BASIC_NET_OK;
    }
    unsigned char* pDealData = (unsigned char*)pszData;
    int nLength = cbData;
    Net_UChar cField = 0, cPacketNo = 0;
    if (Parse_PacketHead(pDealData, cbData, cField, cPacketNo) < 0){
        CCFrameSCBasicLogEventErrorV("Mysql OnReceiveVerify解析出现问题,Close!");
		m_pClient->Close(0);
        return BASIC_NET_OK;
    }
    pDealData += 4;
    nLength -= 4;
    do{
        if (nLength <= 1)
            break;
        basiclib::UnSerializeUChar(pDealData, m_cProtocal_Version);
        pDealData += 1;
        nLength -= 1;
        //服务器版本
        int i = 0;
        for (; i < nLength; i++){
            if (pDealData[i] == '\0')
                break;
        }
        m_strServerVersion.assign((char*)pDealData, i);
        pDealData += i + 1;
        nLength -= i + 1;
        if (nLength < 4)
            break;
        basiclib::UnSerializeUInt(pDealData, m_nThread_id);
        pDealData += 4;
        nLength -= 4;
        //skip filter + 1 byte
        if (nLength <= 9 + 2 + 1 + 2 + 2 + 11 + 13){
            //??error
            break;
        }
        char szScramble[60] = { 0 };
        memcpy(szScramble, pDealData, 8);
        pDealData += 9;
        nLength -= 9;
        //_server_capabilities
        pDealData += 2;
        nLength -= 2;
        //_server_lang
        pDealData += 1;
        nLength -= 1;
        //_server_status
        pDealData += 2;
        nLength -= 2;
        //more_capabilities
        pDealData += 2;
        nLength -= 2;
        //no CLIENT_PLUGIN_AUTH
        pDealData += 11;
        nLength -= 11;

        memcpy(szScramble + 8, pDealData, 12);
        pDealData += 13;
        nLength -= 13;
        //计算
        BYTE szTokenBuffer1[20] = { 0 };
        BYTE szTokenBuffer2[20] = { 0 };
        BYTE szTokenBuffer3[20] = { 0 };
        basiclib::SHA1_Perform((BYTE*)m_pPwd, strlen(m_pPwd), szTokenBuffer1);
        basiclib::SHA1_Perform(szTokenBuffer1, 20, szTokenBuffer2);
        basiclib::SHA1_Perform2((BYTE*)szScramble, 20, szTokenBuffer2, 20, szTokenBuffer3);

        for (int i = 0; i < 20; i++){
            szTokenBuffer3[i] ^= szTokenBuffer1[i];
        }

        unsigned char szAuthBuffer[512] = { 0 };
        Net_UInt nClientFlag = 260047;
        int nTotalLength = 0;
        nTotalLength += 4;	//头部的大小和no描述
        nTotalLength += basiclib::SerializeUInt(szAuthBuffer + nTotalLength, nClientFlag);
        nTotalLength += basiclib::SerializeUInt(szAuthBuffer + nTotalLength, m_maxPacketSize);//默认1M
        nTotalLength += 24;
        memcpy(szAuthBuffer + nTotalLength, m_pUserName, strlen(m_pUserName));
        nTotalLength += strlen(m_pUserName) + 1;//最后一位0
        szAuthBuffer[nTotalLength] = 20;
        nTotalLength += 1;
        memcpy(szAuthBuffer + nTotalLength, szTokenBuffer3, 20);
        nTotalLength += 20;
        memcpy(szAuthBuffer + nTotalLength, m_pDB, strlen(m_pDB));
        nTotalLength += strlen(m_pDB) + 1;//最后一位0

        Compose_PacketHead(szAuthBuffer, nTotalLength - 4, cPacketNo + 1);

        m_pClient->bind_rece(MakeFastFunction(this, &CCoroutineCtx_Mysql::OnReceiveVerifyResult));
        m_pClient->Send(szAuthBuffer, nTotalLength);
        return BASIC_NET_OK;
    } while (false);
    CCFrameSCBasicLogEventErrorV("Mysql OnReceiveVerify解析出错, 长度不够,Close!");
	m_pClient->Close(0);
    return BASIC_NET_OK;
}
int32_t CCoroutineCtx_Mysql::OnReceiveVerifyResult(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    if (cbData < 5){
        return BASIC_NET_OK;
    }
    unsigned char* pDealData = (unsigned char*)pszData;
    int nLength = cbData;
    Net_UChar cField = 0, cPacketNo;
    if (Parse_PacketHead(pDealData, cbData, cField, cPacketNo) < 0){
        CCFrameSCBasicLogEventErrorV("Mysql OnReceiveVerifyResult解析出现问题,Close!");
		m_pClient->Close(0);
        return BASIC_NET_OK;
    }
    pDealData += 5;
    nLength -= 5;
    if (cField == 0xff){
        MysqlResultError error;
        ParseErrorPacket(pDealData, nLength, error);
        CCFrameSCBasicLogEventErrorV("OnReceiveVerifyResult error %d %s %s!", error.m_usErrorID, error.m_strError.c_str(), error.m_szSQLState);
        return BASIC_NET_OK;
    }
    else if (cField == 0xfe){
        CCFrameSCBasicLogEventError("old pre-4.1 authentication protocol not supported!");
		m_pClient->Close(0);
        return BASIC_NET_OK;
    }
    else if (cField != 0x00){
		m_pClient->Close(0);
        char szBuf[64] = { 0 };
        sprintf(szBuf, "bad packet type: %d", cField);
        CCFrameSCBasicLogEventErrorV(szBuf);
        return BASIC_NET_OK;
    }
    //认证成功
    m_pClient->bind_rece(MakeFastFunction(this, &CCoroutineCtx_Mysql::OnReceive));
	//认证成功之后将等待队列的发送
	ctx_message msg(0, [](CCoroutineCtx* pCtx, ctx_message* pMsg){
		CCoroutineCtx_Mysql* pRedisCtx = (CCoroutineCtx_Mysql*)pCtx;
		KernelRequestStoreData* pRequestStore = nullptr;
		do{
			pRequestStore = pRedisCtx->m_vtWaitRequest.FrontData();
			if(pRequestStore){
				MACRO_ResumeToCtx(pRequestStore->m_pCorutine, pRedisCtx, CCtx_ThreadPool::GetOrCreateSelfThreadData(),
								  CCFrameSCBasicLogEventErrorV("%s(%s:%d) OnTimerMysql wait queue wakeup", __FUNCTION__, __FILE__, __LINE__); break);
			}
		} while(pRequestStore);
	});
	PushMessage(msg);
    return BASIC_NET_HR_RET_HANDSHAKE;
}

int32_t CCoroutineCtx_Mysql::OnReceive(basiclib::CBasicSessionNetNotify* pNotify, Net_UInt dwNetCode, Net_Int cbData, const char* pszData){
    if (cbData > 0){
		Ctx_CreateCoroutine(0, [](CCorutinePlus* pCorutine){
			CCorutineStackDataDefault stackData;
			CCoroutineCtx_Mysql* pMysql = pCorutine->GetParamPoint<CCoroutineCtx_Mysql>(0);
			stackData.CopyData(pCorutine->GetParamPoint<const char>(1), pCorutine->GetParam<Net_Int>(2));//拷贝到堆数据

			MACRO_YieldToCtx(pCorutine, pMysql->GetCtxID(),
							 return;);
			int nStackDataLength = 0;
			const char* pStack = stackData.GetData(nStackDataLength);
			pMysql->m_smRecvData.AppendData(pStack, nStackDataLength);
			//notify
			do{
				KernelRequestStoreData* pRequest = pMysql->m_vtRequest.FrontData();
				IsErrorHapper(nullptr != pRequest, ASSERT(0); CCFrameSCBasicLogEventErrorV("%s(%s:%d) NoRequest Error, Close", __FUNCTION__, __FILE__, __LINE__); pMysql->m_pClient->Close(0); return;);
				MACRO_ResumeToCtx(pRequest->m_pCorutine, pMysql, CCtx_ThreadPool::GetOrCreateSelfThreadData(), return;);
				KernelRequestStoreData* pNextRequest = pMysql->m_vtRequest.FrontData();
				if(pNextRequest == pRequest || pNextRequest == nullptr)
					break;
			} while(true);
		}, this, (void*)pszData, &cbData);
    }
    return BASIC_NET_OK;
}

void CCoroutineCtx_Mysql::OnTimer(CCoroutineCtx* pCtx){
    CCoroutineCtx_Mysql* pRedisCtx = (CCoroutineCtx_Mysql*)pCtx;
    time_t tmNow = time(NULL);

    KernelRequestStoreData* pRequestStore = pRedisCtx->m_vtRequest.FrontData();
    IsErrorHapper(pRequestStore == nullptr || !pRequestStore->IsTimeOut(tmNow, pRedisCtx->m_nDefaultTimeoutRequest), CCFrameSCBasicLogEventErrorV("%s(%s:%d) OnTimerMysql Timeout, Close", __FUNCTION__, __FILE__, __LINE__); pRedisCtx->m_pClient->Close(0); return);
    //默认一直重连
    pRedisCtx->m_pClient->DoConnect();

	do{
		pRequestStore = pRedisCtx->m_vtWaitRequest.FrontData();
		if(pRequestStore && pRequestStore->IsTimeOut(tmNow, pRedisCtx->m_nDefaultTimeoutRequest)){
			MACRO_ResumeToCtx(pRequestStore->m_pCorutine, pRedisCtx, CCtx_ThreadPool::GetOrCreateSelfThreadData(),
							  CCFrameSCBasicLogEventErrorV("%s(%s:%d) OnTimerMysql timeout wait", __FUNCTION__, __FILE__, __LINE__); break);
		}
		else{
			break;
		}
	} while(pRequestStore);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//解析头
int CCoroutineCtx_Mysql::Parse_PacketHead(unsigned char* pData, int nLength, Net_UChar& cField_count, Net_UChar& packetNO){
    if (nLength < 5){
        CCFrameSCBasicLogEventErrorV("Mysql Parse_PacketHead解析出错, 长度不够,Close!");
		m_pClient->Close(0);
        return -2;
    }
    Net_UChar cPacketNO = 0;
    basiclib::UnSerializeUChar(pData + 3, packetNO);
    pData += 4;

    basiclib::UnSerializeUChar(pData, cField_count);
    return 5;
}
void CCoroutineCtx_Mysql::Compose_PacketHead(unsigned char* pData, Net_UInt nLength, Net_UChar packetNO){
    pData += basiclib::SerializeUInt3Bit(pData, nLength);
    basiclib::SerializeUChar(pData, packetNO);
}

bool CCoroutineCtx_Mysql::FromLengthCodedBin(unsigned char* pData, int nLength, int& nPos, int& nRet){
    if (nPos >= nLength){
        return false;
    }
    Net_UChar cFirst = 0;
    basiclib::UnSerializeUChar(pData + nPos, cFirst);
    if (cFirst >= 0 && cFirst <= 250){
        nPos = nPos + 1;
        nRet = cFirst;
        return true;
    }
    else if (cFirst == 251){
        nPos = nPos + 1;
        nRet = 0;
        return true;
    }
    else if (cFirst == 252){
        nPos = nPos + 1;
        if (nPos + 2 >= nLength){
            basiclib::BasicLogEventErrorV("Mysql FromLengthCodedBin解析出错, 长度不够,Close!");
            return false;
        }
        Net_UShort nValue = 0;
        nPos += basiclib::UnSerializeUShort(pData + nPos, nValue);
        nRet = nValue;
        return true;
    }
    else if (cFirst == 253){
        nPos = nPos + 1;
        if (nPos + 3 >= nLength){
            basiclib::BasicLogEventErrorV("Mysql FromLengthCodedBin解析出错, 长度不够,Close!");
            return false;
        }
        uint32_t nValue = 0;
        nPos += basiclib::UnSerializeUInt3Bit(pData + nPos, nValue);
        nRet = nValue;
        return true;
    }
    else if (cFirst == 254){
        nPos = nPos + 1;
        if (nPos + 4 >= nLength){
            basiclib::BasicLogEventErrorV("Mysql FromLengthCodedBin解析出错, 长度不够,Close!");
            return false;
        }
        uint32_t nValue = 0;
        nPos += basiclib::UnSerializeUInt(pData + nPos, nValue);
        nRet = nValue;
        return true;
    }
    nPos = nPos + 1;
    return false;
}
bool CCoroutineCtx_Mysql::FromLengthCodedStr(unsigned char* pData, int nLength, int& nPos, basiclib::CBasicString* pStrRet){
    int nGetLength = 0;
    if (!FromLengthCodedBin(pData, nLength, nPos, nGetLength)){
        return false;
    }
    if (nGetLength > 0){
        if (pStrRet)
            pStrRet->assign((char*)pData + nPos, nGetLength);
        nPos += nGetLength;
    }
    return true;
}
bool CCoroutineCtx_Mysql::FromLengthCodedStr(unsigned char* pData, int nLength, int& nPos, const std::function<void(int nPos, int nLength)>& func){
    int nGetLength = 0;
    if (!FromLengthCodedBin(pData, nLength, nPos, nGetLength)){
        return false;
    }
    func(nPos, nGetLength);
    if (nGetLength > 0){
        nPos += nGetLength;
    }
    return true;
}

void CCoroutineCtx_Mysql::ParseErrorPacket(unsigned char* pData, int nLength, MysqlResultError& error){
    int nReadLength = basiclib::UnSerializeUShort(pData, error.m_usErrorID);
    pData += nReadLength;
    nLength -= nReadLength;

    Net_UChar marker = *pData;
    basiclib::CBasicString sqlstate;
    if (marker == '#'){
        //with sqlstate
        memcpy(error.m_szSQLState, pData, 4);
        pData += 5;
        nLength -= 5;
    }

    error.m_strError.assign((char*)pData, nLength);
}
void CCoroutineCtx_Mysql::ParseOKPacket(unsigned char* pData, int nLength, MysqlResultOK& result){
    int nPos = 0;
    if (!FromLengthCodedBin(pData, nLength, nPos, result.m_nAffectRow)){
		m_pClient->Close(0);
        return;
    }
    if (!FromLengthCodedBin(pData, nLength, nPos, result.m_nLastInsertID)){
		m_pClient->Close(0);
        return;
    }
    nPos += basiclib::UnSerializeUShort(pData + nPos, result.m_usServerState);
    nPos += basiclib::UnSerializeUShort(pData + nPos, result.m_usWarnCount);
    if (nLength > nPos){
        result.m_strInfo.assign((const char*)pData + nPos, nLength - nPos);
    }
}
bool CCoroutineCtx_Mysql::ParseDataPacket(unsigned char* pData, int nLength, MysqlResultData& data){
    int nPos = 0;
    if (!FromLengthCodedBin(pData, nLength, nPos, data.m_nFieldCount)){
        return false;
    }
    //可选字段
    FromLengthCodedBin(pData, nLength, nPos, data.m_nExtraCount);
    return true;
}
bool CCoroutineCtx_Mysql::ParseFieldPacket(unsigned char* pData, int nLength, MysqlResultData& data, int nIndex){
    MysqlColData colData;
    basiclib::CBasicString strColName;
    int nPos = 0;
    if (!FromLengthCodedStr(pData, nLength, nPos, nullptr)){
        return false;
    }
    if (!FromLengthCodedStr(pData, nLength, nPos, nullptr)){
        return false;
    }
    if (!FromLengthCodedStr(pData, nLength, nPos, nullptr)){
        return false;
    }
    if (!FromLengthCodedStr(pData, nLength, nPos, nullptr)){
        return false;
    }
    if (!FromLengthCodedStr(pData, nLength, nPos, &strColName)){
        return false;
    }
    if (!FromLengthCodedStr(pData, nLength, nPos, nullptr)){
        return false;
    }
    //填充
    nPos += 1;
    nPos += basiclib::UnSerializeUShort(pData + nPos, colData.m_usCharSet);
    nPos += basiclib::UnSerializeUInt(pData + nPos, colData.m_unColLength);
    nPos += basiclib::UnSerializeUChar(pData + nPos, colData.m_nColType);

    data.m_mapCol[strColName] = nIndex;
    data.m_colData.push_back(colData);
    return true;
}
bool CCoroutineCtx_Mysql::ParseRowDataPacket(unsigned char* pData, int nDataLength, MysqlResultData& data){
    MysqlRowData* pRowData = new MysqlRowData();
    int nPos = data.m_smRowData.GetDataLength();
    data.m_smRowData.AppendData((char*)pData, nDataLength);
    unsigned char* pDealData = (unsigned char*)data.m_smRowData.GetDataBuffer();
    int nLength = data.m_smRowData.GetDataLength();
    for (int i = 0; i < data.m_nFieldCount; i++){
        if (!FromLengthCodedStr(pDealData, nLength, nPos, [&](int nRowColPos, int nLength)->void{
            MysqlRowColData rowData;
            rowData.m_nPos = nRowColPos;
            rowData.m_nLength = nLength;
            pRowData->m_vtRowCol.push_back(rowData);
            
        })){
            delete pRowData;
            return false;
        }
    }
    if (data.m_pAddEnd){
        data.m_pAddEnd->m_pNextRowData = pRowData;
    }
    else{
        data.m_pRowData = pRowData;
    }
    data.m_pAddEnd = pRowData;
    return true;
}
void CCoroutineCtx_Mysql::ParseEOFPacket(unsigned char* pData, int nLength, MysqlResultEOF& eofData){
    basiclib::UnSerializeUShort(pData, eofData.m_usWarnCount);
    basiclib::UnSerializeUShort(pData + 2, eofData.m_usState);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//! 协程里面调用Bussiness消息
int CCoroutineCtx_Mysql::DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket){
    switch (nType){
    case DEFINEDISPATCH_CORUTINE_SQLEXEC:
        return DispathBussinessMsg_0_Exec(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_SQLQUERY:
        return DispathBussinessMsg_1_Query(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_SQLPING:
        return DispathBussinessMsg_2_Ping(pCorutine, nParam, pParam, pRetPacket);
    case DEFINEDISPATCH_CORUTINE_SQLMULTI:
        return DispathBussinessMsg_3_Multi(pCorutine, nParam, pParam, pRetPacket);
    default:
		CCFrameSCBasicLogEventErrorV("%s(%s:%d) unknow type(%d) nParam(%d)", __FUNCTION__, __FILE__, __LINE__, nType, nParam);
        break;
    }
    return -99;
}

int CCoroutineCtx_Mysql::DispathBussinessMsg_0_Exec(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam1(const char* pSQL, (const char*));
	if(!m_pClient->IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_pClient->IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}
    //! 保存
    CKernelMsgQueueRequestMgr requestMgr(m_vtRequest, pCorutine);
    //! 发送请求
    SendRequestQuery(pSQL);
	//发送完成，直接进入yield，等待唤醒
	MACRO_YieldToCtx(pCorutine, 0,
					 return DEFINECTX_RET_TYPE_COROUTINEERR);
    return ExecReply(pCorutine, (MysqlReplyExec*)pRetPacket);
}

int CCoroutineCtx_Mysql::ExecReply(CCorutinePlus* pCorutine, MysqlReplyExec* pReplyPacket){
    //! 获取包
    int nRetGet = GetPacketYield(pCorutine, [&](unsigned char* pPacket, int nLength)->int{
        //OK 或者 error
        Net_UChar cPacketNO = 0;
        int nRet = Parse_PacketHead(pPacket, nLength, pReplyPacket->m_cFieldCount, cPacketNO);
        if (pReplyPacket->m_cFieldCount == 0xff){
            ParseErrorPacket(pPacket + 5, nLength - 5, pReplyPacket->m_error);
            return 0;
        }
        else if (pReplyPacket->m_cFieldCount == 0x00){
            ParseOKPacket(pPacket + 5, nLength - 5, pReplyPacket->m_ok);
            if (pReplyPacket->m_ok.m_usServerState & SERVER_MORE_RESULTS_EXISTS){
                return 1;
            }
            return 0;
        }
        return 0;
    });

    if (nRetGet < 0){
        return DEFINECTX_RET_TYPE_ReceDataErr;
    }
    else if (nRetGet == 1){
        pReplyPacket->m_pNext = new MysqlReplyExec();
        return ExecReply(pCorutine, pReplyPacket->m_pNext);
    }
    return DEFINECTX_RET_TYPE_SUCCESS;
}

int CCoroutineCtx_Mysql::DispathBussinessMsg_1_Query(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam1(const char* pSQL, (const char*));
	if(!m_pClient->IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_pClient->IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}
    //! 保存
    CKernelMsgQueueRequestMgr requestMgr(m_vtRequest, pCorutine);

    //! 发送请求
    SendRequestQuery(pSQL);

	//发送完成，直接进入yield，等待唤醒
	MACRO_YieldToCtx(pCorutine, 0,
					 return DEFINECTX_RET_TYPE_COROUTINEERR);
    return QueryReply(pCorutine, (MysqlReplyQuery*)pRetPacket);
}

int CCoroutineCtx_Mysql::QueryReply(CCorutinePlus* pCorutine, MysqlReplyQuery* pReplyPacket){
    //! 获取包
    int nRetGet = GetPacketYield(pCorutine, [&](unsigned char* pPacket, int nLength)->int{
        //data
        Net_UChar cPacketNO = 0;
        int nRet = Parse_PacketHead(pPacket, nLength, pReplyPacket->m_cFieldCount, cPacketNO);
        if (pReplyPacket->m_cFieldCount == 0xff){
            ParseErrorPacket(pPacket + 5, nLength - 5, pReplyPacket->m_error);
            return 1;
        }
        else if (pReplyPacket->m_cFieldCount <= 250){
            if (ParseDataPacket(pPacket + 4, nLength - 4, pReplyPacket->m_data)){
                return 0;
            }
            return 2;
        }
        return 0;
    });
    if (nRetGet < 0)
        return DEFINECTX_RET_TYPE_ReceDataErr;
    else if (nRetGet == 1)
        //error
        return DEFINECTX_RET_TYPE_ReceDataErr;
    else if (nRetGet == 2){
        CCFrameSCBasicLogEventError("CCoroutineCtx_Mysql::DispathBussinessMsg_1_Query parse data error, close");
		m_pClient->Close(0);
        return DEFINECTX_RET_TYPE_ReceDataErr;
    }
    if (pReplyPacket->m_data.m_nFieldCount > 0){
        pReplyPacket->m_data.m_colData.reserve(pReplyPacket->m_data.m_nFieldCount);
    }
    for (int i = 0; i < pReplyPacket->m_data.m_nFieldCount; i++){
        nRetGet = GetPacketYield(pCorutine, [&](unsigned char* pPacket, int nLength)->int{
            //field
            Net_UChar cPacketNO = 0;
            int nRet = Parse_PacketHead(pPacket, nLength, pReplyPacket->m_cFieldCount, cPacketNO);
            if (pReplyPacket->m_cFieldCount == 0xff){
                ParseErrorPacket(pPacket + 5, nLength - 5, pReplyPacket->m_error);
                return 1;
            }
            else if (pReplyPacket->m_cFieldCount <= 250){
                if (ParseFieldPacket(pPacket + 4, nLength - 4, pReplyPacket->m_data, i)){
                    return 0;
                }
                return 2;
            }
            return 3;
        });
        if (nRetGet < 0)
            return DEFINECTX_RET_TYPE_ReceDataErr;
        else if (nRetGet == 1 || nRetGet == 3)
            //error
            return DEFINECTX_RET_TYPE_ReceDataErr;
    }
    nRetGet = GetPacketYield(pCorutine, [&](unsigned char* pPacket, int nLength)->int{
        //eof
        Net_UChar cPacketNO = 0;
        int nRet = Parse_PacketHead(pPacket, nLength, pReplyPacket->m_cFieldCount, cPacketNO);
        if (pReplyPacket->m_cFieldCount == 0xff){
            ParseErrorPacket(pPacket + 5, nLength - 5, pReplyPacket->m_error);
            return 1;
        }
        else if (pReplyPacket->m_cFieldCount == 0xfe){
            return 0;
        }
        return 3;
    });
    if (nRetGet < 0)
        return DEFINECTX_RET_TYPE_ReceDataErr;
    else if (nRetGet == 1 || nRetGet == 3)
        //error
        return DEFINECTX_RET_TYPE_ReceDataErr;

    MysqlResultEOF eofResult;
    while (true){
        nRetGet = GetPacketYield(pCorutine, [&](unsigned char* pPacket, int nLength)->int{
            //data
            Net_UChar cPacketNO = 0;
            int nRet = Parse_PacketHead(pPacket, nLength, pReplyPacket->m_cFieldCount, cPacketNO);
            if (pReplyPacket->m_cFieldCount == 0xff){
                ParseErrorPacket(pPacket + 5, nLength - 5, pReplyPacket->m_error);
                return 1;
            }
            else if (pReplyPacket->m_cFieldCount <= 250){
                if (ParseRowDataPacket(pPacket + 4, nLength - 4, pReplyPacket->m_data)){
                    return 0;
                }
                return 2;
            }
            else if (pReplyPacket->m_cFieldCount == 0xfe){
                ParseEOFPacket(pPacket + 5, nLength - 5, eofResult);
                if (eofResult.m_usState & SERVER_MORE_RESULTS_EXISTS){
                    //多结果集
                    return 5;
                }
                return 4;
            }
            return 3;
        });
        if (nRetGet < 0)
            return DEFINECTX_RET_TYPE_ReceDataErr;
        else if (nRetGet == 1 || nRetGet == 3)
            //error
            return DEFINECTX_RET_TYPE_ReceDataErr;
        else if (nRetGet == 4)
            break;
        else if (nRetGet == 5){
            pReplyPacket->m_pNext = new MysqlReplyQuery();
            return QueryReply(pCorutine, pReplyPacket->m_pNext);
        }
    }
    return DEFINECTX_RET_TYPE_SUCCESS;
}

int CCoroutineCtx_Mysql::DispathBussinessMsg_2_Ping(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
    if (!m_pClient->IsTransmit())
        return DEFINECTX_RET_TYPE_NoNet;
    //! 保存
    CKernelMsgQueueRequestMgr requestMgr(m_vtRequest, pCorutine);

    //! 发送请求
    SendRequestPing();
	//发送完成，直接进入yield，等待唤醒
	MACRO_YieldToCtx(pCorutine, 0,
					 return -1;);
    return ExecReply(pCorutine, (MysqlReplyExec*)pRetPacket);
}

int CCoroutineCtx_Mysql::DispathBussinessMsg_3_Multi(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket){
	MACRO_DispatchCheckParam1(MysqlMultiRequest* pRequest, (MysqlMultiRequest*));
	if(!m_pClient->IsTransmit()){
		CKernelMsgQueueRequestMgr requestMgr(m_vtWaitRequest, pCorutine);
		//发送完成，直接进入yield，等待唤醒
		MACRO_YieldToCtx(pCorutine, 0,
						 return DEFINECTX_RET_TYPE_COROUTINEERR);
		if(!m_pClient->IsTransmit()){
			return DEFINECTX_RET_TYPE_NoNet;
		}
	}

    long lRet = DEFINECTX_RET_TYPE_SUCCESS;
    do{
        if (pRequest->m_nType == DEFINEDISPATCH_CORUTINE_SQLEXEC){
            const int nSetParam = 1;
            void* pSetParam[nSetParam] = { (void*)pRequest->m_pSQL };
            lRet = DispathBussinessMsg_0_Exec(pCorutine, nSetParam, pSetParam, &((MysqlMultiRequestExec*)pRequest)->m_reply);
        }
        else if (pRequest->m_nType == DEFINEDISPATCH_CORUTINE_SQLQUERY){
            const int nSetParam = 1;
            void* pSetParam[nSetParam] = { (void*)pRequest->m_pSQL };
            lRet = DispathBussinessMsg_1_Query(pCorutine, nSetParam, pSetParam, &((MysqlMultiRequestQuery*)pRequest)->m_reply);
        }
        else{
            return DEFINECTX_RET_TYPE_ParamErr;
        }
        if (lRet != 0)
            return lRet;
        pRequest = pRequest->m_pNext;
    } while (pRequest);
    return lRet;
}

//发送执行sql
void CCoroutineCtx_Mysql::SendRequestQuery(const char* pSQL){
    char szBuf[10240] = { 0 };
    int nLength = strlen(pSQL);
    memcpy(szBuf + 5, pSQL, nLength);
    szBuf[4] = 0x03;
    nLength += 1;
    Compose_PacketHead((unsigned char*)szBuf, nLength, 0);
    nLength += 4;
    m_pClient->Send(szBuf, nLength);
}
void CCoroutineCtx_Mysql::SendRequestPing(){
    static char szBuf[] = { 1, 0, 0, 0, 14 };
    m_pClient->Send(szBuf, 5);
}
//获取一个packet
int CCoroutineCtx_Mysql::GetPacketYield(CCorutinePlus* pCorutine, const std::function<int(unsigned char* pPacket, int nLength)>& func){
    int nRet = 0;
    do{
        long lRet = m_smRecvData.IsPacketFull(m_maxPacketSize);
        if (lRet == MYSQLCONNECT_LOCATION_HEADER_FIND){
            //需要加上头部大小
            nRet = func((unsigned char*)m_smRecvData.GetDataBuffer(), m_smRecvData.GetTotalPacketLength());
            m_smRecvData.ResetHeader();
            break;
        }
        else if (lRet == MYSQLCONNECT_LOCATION_HEADER_NOTALLOWED){
            return -2;
        }
        else{
            if (!YieldAndResumeSelf(pCorutine)){
                //代表进来的时候sourcectxid已经不存在了
                return -1;
            }
        }
    } while (true);
    return nRet;
}

bool CCFrameMysqlExec(uint32_t nResCtxID, uint32_t nMysqlCtxID, CCorutinePlus* pCorutine, const char* pSQL, MysqlReplyExec& replyPacket){
	MACRO_ExeToCtxParam1(pCorutine, nResCtxID, nMysqlCtxID, DEFINEDISPATCH_CORUTINE_SQLEXEC, (void*)pSQL, &replyPacket,
						 CCFrameSCBasicLogEventErrorV("CCFrameMysqlExec error %s", pSQL); return false);
    //判断是否执行成功
    MysqlReplyExec* pCheck = &replyPacket;
    do{
        if (pCheck->m_cFieldCount != 0x00){
			CCFrameSCBasicLogEventErrorV("CCFrameMysqlQuery error %s", pSQL);
            return false;
        }
        pCheck = pCheck->m_pNext;
    } while (pCheck);
    return true;
}
bool CCFrameMysqlQuery(uint32_t nResCtxID, uint32_t nMysqlCtxID, CCorutinePlus* pCorutine, const char* pSQL, MysqlReplyQuery& replyPacket){
	MACRO_ExeToCtxParam1(pCorutine, nResCtxID, nMysqlCtxID, DEFINEDISPATCH_CORUTINE_SQLQUERY, (void*)pSQL, &replyPacket,
						 CCFrameSCBasicLogEventErrorV("CCFrameMysqlExec error %s", pSQL); return false);
    MysqlReplyQuery* pCheck = &replyPacket;
    do{
        if (pCheck->m_cFieldCount == 0xff){
			CCFrameSCBasicLogEventErrorV("CCFrameMysqlQuery error %s", pSQL);
            return false;
        }
        pCheck = pCheck->m_pNext;
    } while (pCheck);
    return true;
}
bool CCFrameMysqlMulty(uint32_t nResCtxID, uint32_t nMysqlCtxID, CCorutinePlus* pCorutine, MysqlMultiRequest* pRequest){
	bool bError = false;
	MACRO_ExeToCtxParam1(pCorutine, nResCtxID, nMysqlCtxID, DEFINEDISPATCH_CORUTINE_SQLMULTI, (void*)pRequest, nullptr,
						 bError = true);
    if (bError){
		Net_CBasicBitstream ins;
		char szBuf[2048] = { 0 };
		while (pRequest) {
			sprintf(szBuf, "SQL:(%d)%s\r\n", pRequest->m_nType, pRequest->m_pSQL);
			ins.AppendString(szBuf);
			pRequest = pRequest->m_pNext;
		}
        CCFrameSCBasicLogEventErrorV("CCFrameMysqlMulty error (%s)", ins.GetDataBuffer());
        return false;
    }
	else{
		while(pRequest){
			if(pRequest->m_nType == DEFINEDISPATCH_CORUTINE_SQLEXEC){
				MysqlMultiRequestExec* pReply = (MysqlMultiRequestExec*)pRequest;
				MysqlReplyExec* pCheck = &pReply->m_reply;
				do{
					if(pCheck->m_cFieldCount != 0x00){
						CCFrameSCBasicLogEventErrorV("CCFrameMysqlMulty error %s", pRequest->m_pSQL);
						return false;
					}
					pCheck = pCheck->m_pNext;
				} while(pCheck);
			}
			else if(pRequest->m_nType == DEFINEDISPATCH_CORUTINE_SQLQUERY){
				MysqlMultiRequestQuery* pReply = (MysqlMultiRequestQuery*)pRequest;
				MysqlReplyQuery* pCheck = &pReply->m_reply;
				do{
					if(pCheck->m_cFieldCount == 0xff){
						CCFrameSCBasicLogEventErrorV("CCFrameMysqlMulty error %s", pRequest->m_pSQL);
						return false;
					}
					pCheck = pCheck->m_pNext;
				} while(pCheck);
			}
			else{
				return false;
			}
			pRequest = pRequest->m_pNext;
		}
	}
    return true;
}
