#include "ctx_log.h"
#include "../ctx_threadpool.h"

using namespace basiclib;

CreateTemplateSrc(CCoroutineCtx_Log)

CCoroutineCtx_Log* m_pLog = nullptr;
//! �Ƿ����
bool CCoroutineCtx_Log::IsExist(){
    return m_pLog != nullptr;
}
CCoroutineCtx_Log::CCoroutineCtx_Log(const char* pKeyName, const char* pClassName) : CCoroutineCtx(pKeyName, pClassName)
{

}

CCoroutineCtx_Log::~CCoroutineCtx_Log(){

}

int CCoroutineCtx_Log::InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func){
	m_pLog = this;
    int nRet = CCoroutineCtx::InitCtx(pMQMgr, func);
    if (nRet != 0)
        return nRet;

    basiclib::CBasicString strDefaultLogPath = basiclib::BasicGetModulePath() + "/log/";
    basiclib::CBasicString strLogPath = func(InitGetParamType_Config, "logpath", strDefaultLogPath.c_str());
    basiclib::CBasicString strDefaultKey = "ccframelog";
    basiclib::CBasicString strKey = func(InitGetParamType_Config, "ccframekey", strDefaultKey.c_str());
    basiclib::CBasicString strDefaultLogFileName = strLogPath + strKey + ".log";
    basiclib::CBasicString strDefaultErrorFileName = strLogPath + strKey + ".error";

    basiclib::BasicSetDefaultLogEventMode(0, strDefaultLogFileName.c_str());
    basiclib::BasicSetDefaultLogEventErrorMode(0, strDefaultErrorFileName.c_str());
    //30s
    CoroutineCtxAddOnTimer(m_ctxID, 3000, OnTimerBasicLog);
    return 0;
}

//! ����Ҫ�Լ�delete��ֻҪ����release
void CCoroutineCtx_Log::ReleaseCtx(){
    CoroutineCtxDelTimer(OnTimerBasicLog);
    CCoroutineCtx::ReleaseCtx();
}

//! Э���������Bussiness��Ϣ
int CCoroutineCtx_Log::DispathBussinessMsg(CCorutinePlus* pCorutine, CCtx_CorutinePlusThreadData* pData, uint32_t nType, int nParam, void** pParam, void* pRetPacket, ctx_message* pCurrentMsg){
    if (nParam != 2){
        ASSERT(0);
        //ʹ���Ϸ���д��
        basiclib::BasicLogEvent(basiclib::DebugLevel_Error, "CCoroutineCtx_Log::DiapathBussinessMsg Param size not 2");
        return -1;
    }
    basiclib::WriteLogDataBuffer* pWriteLog = (basiclib::WriteLogDataBuffer*)pParam[0];
    int nChannel = *(int*)pParam[1];
    //д��־
    TRACE("LOG%d:%s\r\n", nChannel, pWriteLog->m_pText);
    basiclib::BasicWriteByLogDataBuffer(nChannel, *pWriteLog, true);
    return 0;
}

void CCoroutineCtx_Log::LogEvent(CCorutinePlusThreadData* pThreadData, int nChannel, const char* pszLog){
	basiclib::WriteLogDataBuffer logData;
	logData.InitLogData(pszLog);
	logData.m_lCurTime = time(NULL);
	logData.m_dwProcessId = basiclib::Basic_GetCurrentProcessId();
	logData.m_dwThreadId = basiclib::BasicGetCurrentThreadId();
	if (pThreadData == nullptr){
        pThreadData = CCtx_ThreadPool::GetOrCreateSelfThreadData();
    }
    const int nSetParam = 3;
    void* pSetParam[nSetParam] = { this, &logData, &nChannel };
    CreateResumeCoroutineCtx(CCoroutineCtx_Log::OnLogEventCtx, pThreadData, 0, nSetParam, pSetParam);
}

void CCoroutineCtx_Log::OnLogEventCtx(CCorutinePlus* pCorutine)
{
    bool bFree = false;
    const int nDefaultSize = 512;
    char szBuf[nDefaultSize] = { 0 };
    basiclib::WriteLogDataBuffer bufSelf = *pCorutine->GetResumeParamPoint<basiclib::WriteLogDataBuffer>(0);
    int nChannel = 0;
    char* pBuffer = szBuf;
    int nGetParamCount = 0;
    void** pGetParam = GetCoroutineCtxParamInfo(pCorutine, nGetParamCount);
    if (nGetParamCount != 3){
        ASSERT(0);
        //ʹ���Ϸ���д��
        basiclib::BasicLogEvent(basiclib::DebugLevel_Error, "OnLogEventCtx Param size not 3");
        return;
    }
    CCoroutineCtx_Log* pCtx = (CCoroutineCtx_Log*)pGetParam[0];
    bufSelf = *(basiclib::WriteLogDataBuffer*)pGetParam[1];
    nChannel = *(int*)pGetParam[2];

    int nLength = strlen(bufSelf.m_pText);
    if (nLength > nDefaultSize){
        bFree = true;
        pBuffer = (char*)basiclib::BasicAllocate(nLength);
        memcpy(pBuffer, bufSelf.m_pText, nLength);
    }
    else{
        memcpy(szBuf, bufSelf.m_pText, nLength);
    }
    bufSelf.InitLogData(pBuffer);
    const int nSetParam = 2;
    void* pSetParam[nSetParam] = { &bufSelf, &nChannel };
    //��Ϊ�����ܻ���reply���Կ�������Ϊnullptr
    void* pReply = nullptr;
    int nRetValue = 0;
    WaitExeCoroutineToCtxBussiness(pCorutine, 0, pCtx->GetCtxID(), 0, nSetParam, pSetParam, pReply, nRetValue);
    if (bFree){
        basiclib::BasicDeallocate(pBuffer);
    }
}

void CCoroutineCtx_Log::OnTimerBasicLog(CCoroutineCtx* pCtx, CCtx_CorutinePlusThreadData* pData){
	basiclib::OnTimerBasicLog();
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define CCFRAME_LOG_MESSAGE_SIZE 256
void CCFrameSCBasicLogEvent(const char* pszLog){
	m_pLog->LogEvent(nullptr, 0, pszLog);
}
void CCFrameSCBasicLogEventError(const char* pszLog){
	m_pLog->LogEvent(nullptr, 1, pszLog);
}

void CCFrameSCBasicLogEventV(CCorutinePlusThreadData* pThreadData, const char* pszLog, ...){
	char tmp[CCFRAME_LOG_MESSAGE_SIZE];
	va_list argList;
	va_start(argList, pszLog);
	int len = vsnprintf(tmp, CCFRAME_LOG_MESSAGE_SIZE, pszLog, argList);
	va_end(argList);
	if (len >= 0 && len < CCFRAME_LOG_MESSAGE_SIZE){
		m_pLog->LogEvent(pThreadData, 0, tmp);
	}
	else{
		CBasicString strLog;
		va_start(argList, pszLog);
		strLog.FormatV(pszLog, argList);
		va_end(argList);
		m_pLog->LogEvent(pThreadData, 0, strLog.c_str());
	}
}
void CCFrameSCBasicLogEventErrorV(CCorutinePlusThreadData* pThreadData, const char* pszLog, ...){
	char tmp[CCFRAME_LOG_MESSAGE_SIZE];
	va_list argList;
	va_start(argList, pszLog);
	int len = vsnprintf(tmp, CCFRAME_LOG_MESSAGE_SIZE, pszLog, argList);
	va_end(argList);
	if (len >= 0 && len < CCFRAME_LOG_MESSAGE_SIZE){
		m_pLog->LogEvent(pThreadData, 1, tmp);
	}
	else{
		CBasicString strLog;
		va_start(argList, pszLog);
		strLog.FormatV(pszLog, argList);
		va_end(argList);
		m_pLog->LogEvent(pThreadData, 1, strLog.c_str());
	}
}
void CCFrameSCBasicLogEvent(CCorutinePlusThreadData* pThreadData, const char* pszLog){
	m_pLog->LogEvent(pThreadData, 0, pszLog);
}
void CCFrameSCBasicLogEventError(CCorutinePlusThreadData* pThreadData, const char* pszLog){
	m_pLog->LogEvent(pThreadData, 1, pszLog);
}
