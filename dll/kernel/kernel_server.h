#ifndef SKYNETPLUS_KERNEL_SERVERMODULE_H
#define SKYNETPLUS_KERNEL_SERVERMODULE_H

#include <basic.h>
#include "../scbasic/http/httpsession.h"
#include "../scbasic/json/scbasicjson.h"
#include "ccframe/ctx_threadpool.h"
#include "ccframe/dllmodule.h"
#include "kernel_head.h"
extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

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
//system module
//////////////////////////////////////////////////////////////////////
struct ServerModuleConfig
{
	basiclib::CBasicString		m_iniFile;
	basiclib::CBasicString		m_daemon;
	basiclib::CBasicString		m_httpAddress;
	ServerModuleConfig()
	{
	}
};

struct WorkThreadSelfData{
    lua_State*					m_L;
    WorkThreadSelfData(){
        m_L = nullptr;
    }
    ~WorkThreadSelfData(){
        if (m_L)
            lua_close(m_L);
    }
    void Init();
};

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

typedef fastdelegate::FastDelegate4<RefHttpSession, SCBasicDocument&, HttpRequest*, HttpResponse&, long>	RegisterDealWithHttpFunction;
struct MapHttpToFunc{
    RegisterDealWithHttpFunction        m_func;
    CIpVerify*                          m_pIpVerify;
    MapHttpToFunc(){
        m_func = nullptr;
        m_pIpVerify = nullptr;
    }
    ~MapHttpToFunc(){
        if (m_pIpVerify){
            delete m_pIpVerify;
        }
    }
    void SetIPTrust(const char* pTrust){
        if (m_pIpVerify){
            delete m_pIpVerify;
        }
        m_pIpVerify = new CIpVerify();
        m_pIpVerify->SetIPRuler(pTrust);
    }
};
typedef basiclib::basic_map<basiclib::CBasicString, MapHttpToFunc>			MapFunctionToPointFunc;
class _SKYNET_KERNEL_DLL_API CKernelServer : public basiclib::CBasicObject
{
public:
	CKernelServer();
	virtual ~CKernelServer();
public:
	static CKernelServer* CreateKernelServer();
	static void ReleaseKernelServer();
	static CKernelServer* GetKernelServer();
public:
	void KernelServerStart(const char* pConfigFileName);

	bool& IsRunning(){ return m_bRunning; }
	ServerModuleConfig& GetConfig(){ return m_config; }

	const char* GetEnv(const char* key);
	int GetEnv(const char* key, int opt);
	const char* GetEnvString(const char* key, const char* opt);
	void SetEnv(const char* key, const char* value);
	///////////////////////////////////////////////////////////////////////////////////
	bool ReplaceDllList(const char* pReplaceDllList);
protected:
	bool ReadConfig();
	///////////////////////////////////////////////////////////////////////////////////
	//http
	void RegisterHttp();
public:
	bool RegisterHttpFunc(const char* pFunction, RegisterDealWithHttpFunction pFunc, const char* pIpTrust = nullptr);
	void UnRegisterHttpFunc(const char* pFunction);
    //发送http结果
    static void SendHttpResponseAsyn(RefHttpSession pSession, HttpRequest*& pRequest, SCBasicDocument& doc);
protected:
	long OnHttpAsk(RefHttpSession pSession, HttpRequest* pRequest, HttpResponse& response);
    long DispathHttpAsk(RefHttpSession pSession, SCBasicDocument&, HttpRequest*, HttpResponse&);
    long ReLoadDll(RefHttpSession pSession, SCBasicDocument&, HttpRequest*, HttpResponse&);
    long ListCtx(RefHttpSession pSession, SCBasicDocument&, HttpRequest*, HttpResponse&);
	///////////////////////////////////////////////////////////////////////////////////
public:
	bool						m_bRunning;
	ServerModuleConfig			m_config;

	basiclib::SpinLock			m_skynetenv;
	lua_State*					m_L;

	CKernelLoadDllMgr			m_mgtDllFile;
	CHttpSessionServer*			m_pHttpServer;

	basiclib::SpinLock			m_lockFunctionToPointFunc;
	MapFunctionToPointFunc		m_mapHttpFunctionToPointFunc;

	CCtx_ThreadPool*			m_pCtxThreadPool;

    typedef basiclib::basic_map<basiclib::CBasicString, basiclib::CBasicString> MapConfig;
    MapConfig                   m_mapConfig;
};
#pragma warning (pop)

extern "C" void _SKYNET_KERNEL_DLL_API StartKernel(const char* pConfig);

#endif
