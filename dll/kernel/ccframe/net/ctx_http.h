#ifndef SCBASIC_CCFRAME_HTTP_H
#define SCBASIC_CCFRAME_HTTP_H

#include "../dllmodule.h"
#include "../public/kernelrequeststoredata.h"
#include "../scbasic/http/httpsession.h"
#include "../scbasic/json/scbasicjson.h"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define Define_Http_Ctx_Bussiness_AddFunc		0
#define Define_Http_Ctx_Bussiness_DelFunc		1
#define DEFINEDISPATCH_CORUTINE_SQLPING     2
#define DEFINEDISPATCH_CORUTINE_SQLMULTI    3
///////////////////////////////////////////////////////////////////////
#define Http_Json_KeyDefine_RetNum	"RetNum"		//错误码的返回，0代表成功
#define Http_Json_KeyDefine_String	"RetString"		//附加的信息
//////////////////////////////////////////////////////////////////////
#define Http_Json_KeyDefine_RetNum_Success			0	//成功
#define Http_Json_KeyDefine_RetNum_NoFunction		-1	//参数错误
#define Http_Json_KeyDefine_RetNum_NoRegFunction	-2	//没有注册对应的函数
#define Http_Json_KeyDefine_RetNum_NoReLoadDllError	-3	//重新加载dll错误
#define Http_Json_KeyDefine_RetNum_RunError         -4  //执行出现错误
#define Http_Json_KeyDefine_RetNum_NoTrust          -5  //不信任的地址
//////////////////////////////////////////////////////////////////////
#define Http_Json_Number_Set(doc, Key, Number) \
	rapidjson::Value retNumber(Number);\
	doc.AddMember(Key, retNumber, doc.GetAllocator());
#define Http_Json_String_Set(doc, Key, str)\
	rapidjson::Value retStr;\
	retStr.SetString(str, doc.GetAllocator());\
	doc.AddMember(Key, retStr, doc.GetAllocator());

class CCoroutineCtx_Http;
typedef long(*RegisterDealWithHttpFunction)(CCoroutineCtx_Http*, RefHttpSession, SCBasicDocument&, HttpRequest*, HttpResponse&, CCorutinePlus*, void*);
struct MapHttpToFunc {
	RegisterDealWithHttpFunction        m_func;
	CIpVerify*                          m_pIpVerify;
	void*								m_pUD;
	MapHttpToFunc() {
		m_func = nullptr;
		m_pIpVerify = nullptr;
		m_pUD = nullptr;
	}
	~MapHttpToFunc() {
		if (m_pIpVerify) {
			delete m_pIpVerify;
		}
	}
	void SetIPTrust(const char* pTrust) {
		if (m_pIpVerify) {
			delete m_pIpVerify;
		}
		m_pIpVerify = new CIpVerify();
		m_pIpVerify->SetIPRuler(pTrust);
	}
};
typedef basiclib::basic_map<basiclib::CBasicString, MapHttpToFunc>			MapFunctionToPointFunc;

class _SKYNET_KERNEL_DLL_API CCoroutineCtx_Http : public CCoroutineCtx
{
public:
	CCoroutineCtx_Http(const char* pKeyName = "http", const char* pClassName = GlobalGetClassName(CCoroutineCtx_Http));
    virtual ~CCoroutineCtx_Http();

    CreateTemplateHeader(CCoroutineCtx_Http);

    //! 初始化0代表成功
    virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

    //! 不需要自己delete，只要调用release
    virtual void ReleaseCtx();

	lua_State* GetLuaState() { return m_L; }

    //! 协程里面调用Bussiness消息
    virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket);
    ////////////////////////////////////////////////////////////////////////////////////////
    //业务类, 全部使用静态函数, 这样可以保证动态库函数可以替换,做到动态更新
    static void OnTimer(CCoroutineCtx* pCtx);
protected:
    ////////////////////DispathBussinessMsg
    //! 发送请求
    long DispathBussinessMsg_0_AddFunc(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
    long DispathBussinessMsg_1_DelFunc(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
	long OnHttpAsk(RefHttpSession pSession, HttpRequest* pRequest, HttpResponse& response);
protected:
	long OnCtxHttpAsk(RefHttpSession pSession, HttpRequest* pRequest, HttpResponse& response, CCorutinePlus*);
	long DispathHttpAsk(RefHttpSession pSession, SCBasicDocument&, HttpRequest*, HttpResponse&, CCorutinePlus*);
protected:
    const char*                             m_pAddress;
    const char*                             m_pTrust;

	CHttpSessionServer*						m_pHttpServer;
	MapFunctionToPointFunc					m_mapHttpFunctionToPointFunc;
	//启动一个LUA STATE
	lua_State*								m_L;
};

//实现全局安全的函数封装类似select和query
_SKYNET_KERNEL_DLL_API bool CCFrameHttpRegister(uint32_t nSrcCtxID, uint32_t nHttpCtxID, CCorutinePlus* pCorutine, const char* pFunction, RegisterDealWithHttpFunction func, const char* pTrustIP, void* pUD);
_SKYNET_KERNEL_DLL_API bool CCFrameHttpUnRegister(uint32_t nSrcCtxID, uint32_t nHttpCtxID, CCorutinePlus* pCorutine, const char* pFunction);

#pragma warning (pop)

#endif