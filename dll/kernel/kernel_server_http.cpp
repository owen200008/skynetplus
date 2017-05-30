#include <basic.h>
#include "../scbasic/http/httprequest.h"
#include "../scbasic/http/httpresponse.h"
#include "kernel_server.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "../scbasic/http/httpdefine.h"


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CKernelServer::RegisterHttpFunc(const char* pFunction, RegisterDealWithHttpFunction pFunc, const char* pIpTrust)
{
	basiclib::CSpinLockFunc lock(&m_lockFunctionToPointFunc, TRUE);
	MapFunctionToPointFunc::iterator iter = m_mapHttpFunctionToPointFunc.find(pFunction);
	if (iter != m_mapHttpFunctionToPointFunc.end()){
		return false;
	}
	m_mapHttpFunctionToPointFunc[pFunction].m_func = pFunc;
    if (pIpTrust){
        m_mapHttpFunctionToPointFunc[pFunction].SetIPTrust(pIpTrust);
    }
	return true;
}

void CKernelServer::UnRegisterHttpFunc(const char* pFunction)
{
	basiclib::CSpinLockFunc lock(&m_lockFunctionToPointFunc, TRUE);
	m_mapHttpFunctionToPointFunc.erase(pFunction);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
long CKernelServer::OnHttpAsk(RefHttpSession pSession, HttpRequest* pRequest, HttpResponse& response)
{
	SCBasicDocument doc(rapidjson::kObjectType);

    long lRet = DispathHttpAsk(pSession, doc, pRequest, response);
	if (lRet == HTTP_SUCC){
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
		response.AppendContent(buffer.GetString(), buffer.GetLength());
	};
	return lRet;
}
void CKernelServer::SendHttpResponseAsyn(RefHttpSession pSession, HttpRequest*& pRequest, SCBasicDocument& doc){
    HttpResponse response;
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);
    response.AppendContent(buffer.GetString(), buffer.GetLength());
    basiclib::CBasicSmartBuffer smBuf;
    pSession->AsynSendResponse(pRequest, response, smBuf);
}

long CKernelServer::DispathHttpAsk(RefHttpSession pSession, SCBasicDocument& doc, HttpRequest* pRequest, HttpResponse& response)
{
	const char* pFunction = pRequest->GetParamValue("function");
	if (pFunction == nullptr){
		Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_NoFunction);
	}
	else{
		basiclib::CSpinLockFunc lock(&m_lockFunctionToPointFunc, TRUE);
		MapFunctionToPointFunc::iterator iter = m_mapHttpFunctionToPointFunc.find(pFunction);
		if (iter != m_mapHttpFunctionToPointFunc.end()){
            if (iter->second.m_pIpVerify){
                if (!iter->second.m_pIpVerify->IsIpTrust(pSession->GetNetAddress())){
                    Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_NoTrust);
                    return HTTP_SUCC;
                }
            }
            return iter->second.m_func(pSession, doc, pRequest, response);
		}
		else{
			Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_NoRegFunction);
		}
	}
	return HTTP_SUCC;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CKernelServer::RegisterHttp()
{
	RegisterHttpFunc("reloaddll", MakeFastFunction(this, &CKernelServer::ReLoadDll));
    RegisterHttpFunc("listctx", MakeFastFunction(this, &CKernelServer::ListCtx));
}
long CKernelServer::ListCtx(RefHttpSession pSession, SCBasicDocument& doc, HttpRequest*, HttpResponse&)
{
    char szBuf[256] = { 0 };
    basiclib::CBasicString strRet;
    CCoroutineCtxHandle::GetInstance()->Foreach([&](CRefCoroutineCtx pCtx)->bool{
        sprintf(szBuf, "%08d: ClassName(%s) CtxName(%s);", pCtx->GetCtxID(), pCtx->GetCtxClassName(), pCtx->GetCtxName() ? pCtx->GetCtxName() : "");
        strRet += szBuf;
        return true;
    });
    Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_Success);
    Http_Json_String_Set(doc, Http_Json_KeyDefine_String, strRet.c_str());
	return HTTP_SUCC;
}

long CKernelServer::ReLoadDll(RefHttpSession pSession, SCBasicDocument& doc, HttpRequest* pRequest, HttpResponse& response)
{
	ReadConfig();
	//Ìæ»»¶¯Ì¬¿â
	basiclib::CBasicString strhttpReplaceList = GetEnvString("replacedlllist", "");
	if (!ReplaceDllList(strhttpReplaceList.c_str()))
	{
		char szBuf[4096] = { 0 };
		sprintf(szBuf, "¶¯Ì¬¿âÌæ»»Ê§°Ü(%s)", strhttpReplaceList.c_str());
		basiclib::BasicLogEventError(szBuf);
		Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_Success);
		Http_Json_String_Set(doc, Http_Json_KeyDefine_String, szBuf);
	}
	else
	{
		Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, Http_Json_KeyDefine_RetNum_Success);
	}
	return HTTP_SUCC;
}


