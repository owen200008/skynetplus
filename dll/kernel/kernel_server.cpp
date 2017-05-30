#include <basic.h>
#include "../scbasic/http/httprequest.h"
#include "../scbasic/http/httpresponse.h"
#include "kernel_server.h"
#include "ccframe/ctx_threadpool.h"

using namespace basiclib;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CCtx_KernelThreadPool : public CCtx_ThreadPool
{
public:
	CCtx_KernelThreadPool(){

	}
	virtual ~CCtx_KernelThreadPool(){
	}
	//����ʵ�ֵ��麯��
	virtual const char* GetCtxInitString(InitGetParamType nType, const char* pParam, const char* pDefault){
		switch (nType){
		case InitGetParamType_Config:
			return CKernelServer::GetKernelServer()->GetEnvString(pParam, pDefault);
		}
		return nullptr;
	}
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define Config_Ini_MainSection "Main"

CKernelServer::CKernelServer()
{
	m_L = nullptr;
	m_pHttpServer = nullptr;
	m_bRunning = true;
	m_pCtxThreadPool = nullptr;
}

CKernelServer::~CKernelServer()
{
	if (m_L)
		lua_close(m_L);
	if (m_pHttpServer)
		m_pHttpServer->Release();
}

CKernelServer* g_pServer = nullptr;
//���캯��
CKernelServer* CKernelServer::CreateKernelServer()
{
	if (g_pServer != nullptr)
	{
		printf("CKernelServer InitKernelServer�Ѿ����ڣ�");
		return nullptr;
	}
	g_pServer = new CKernelServer();
	
	return g_pServer;
}
//��������
void CKernelServer::ReleaseKernelServer()
{
	delete g_pServer;
	g_pServer = nullptr;
}

const char* CKernelServer::GetEnv(const char* key)
{
    auto& iter = m_mapConfig.find(key);
    if (iter != m_mapConfig.end())
        return iter->second.c_str();
    return nullptr;
}

void TestShowImportDll(){
	HMODULE hFromModule = GetModuleHandle(0);
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hFromModule;
	if (IsBadReadPtr(pDosHeader, sizeof(IMAGE_DOS_HEADER)))
		return;
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)hFromModule + pDosHeader->e_lfanew);
	if (IsBadReadPtr(pNTHeader, sizeof(IMAGE_NT_HEADERS)))
		return;
	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
		return;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)hFromModule + pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	if (pImportDesc == (PIMAGE_IMPORT_DESCRIPTOR)pNTHeader)
		return;
	while (pImportDesc->OriginalFirstThunk != 0)
	{
		PSTR pszModName = (PSTR)((PBYTE)pDosHeader + pImportDesc->Name);
        CCFrameSCBasicLogEventV(nullptr, "LoadModName:%s", pszModName);
		PIMAGE_THUNK_DATA pOrigin = (PIMAGE_THUNK_DATA)((PBYTE)pDosHeader + pImportDesc->OriginalFirstThunk);
		PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((PBYTE)pDosHeader + pImportDesc->FirstThunk);
		for (; pOrigin->u1.Ordinal != 0; pOrigin++, pThunk++){
			LPCSTR pFxName = NULL;
			if (pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
				pFxName = (LPCSTR)IMAGE_ORDINAL(pThunk->u1.Ordinal);
			else
			{
				PIMAGE_IMPORT_BY_NAME piibn = (PIMAGE_IMPORT_BY_NAME)((PBYTE)pDosHeader + pOrigin->u1.AddressOfData);
				pFxName = (LPCSTR)piibn->Name;
			}
            CCFrameSCBasicLogEventV(nullptr, "LoadModName:%s Func:%s", pszModName, pFxName);
		}

		pImportDesc++;  // Advance to next imported module descriptor
	}
}

#ifndef __BASICWINDOWS
int sigign()
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);
	return 0;
}
#endif


static void _init_env(lua_State *L)
{
	lua_pushnil(L);  /* first key */
	while (lua_next(L, -2) != 0) {
		int keyt = lua_type(L, -2);
		if (keyt != LUA_TSTRING) {
			printf("Invalid config table");
			exit(1);
		}
		const char * key = lua_tostring(L, -2);
		if (lua_type(L, -1) == LUA_TBOOLEAN) {
			int b = lua_toboolean(L, -1);
			CKernelServer::GetKernelServer()->SetEnv(key, b ? "1" : "0");
		}
		else {
			const char * value = lua_tostring(L, -1);
			if (value == NULL) {
				printf("Invalid config table key = %s", key);
				exit(1);
			}
			CKernelServer::GetKernelServer()->SetEnv(key, value);
		}
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
}

bool CKernelServer::ReadConfig()
{
	lua_State *L = luaL_newstate();
    luaL_openlibs(L);   // link lua lib

    basiclib::CBasicString strConfigFileName = basiclib::BasicGetModulePath() + "lua/loadconfig.lua";
    int err = luaL_loadfile(L, strConfigFileName.c_str());
    if (err){
        printf(lua_tostring(L, -1));
        lua_close(L);
        return false;
    }
    lua_pushstring(L, m_config.m_iniFile.c_str());
    basiclib::CBasicString strModule = basiclib::BasicGetModulePath();
    strModule.Replace("\\", "/");
    lua_pushstring(L, strModule.c_str());
    err = lua_pcall(L, 2, 1, 0);
    if (err){
        printf(lua_tostring(L, -1));
        lua_close(L);
        return false;
    }

    m_L = L;
    _init_env(L);
	//��ʼ��
	strConfigFileName = basiclib::BasicGetModulePath() + "lua/main.lua";
	err = luaL_loadfile(L, strConfigFileName.c_str());
	if (err){
		printf(lua_tostring(L, -1));
		return false;
	}
	err = lua_pcall(L, 0, 0, 0);
	if (err){
		printf(lua_tostring(L, -1));
		return false;
	}

	m_config.m_daemon = GetEnvString("daemon", NULL);
	m_config.m_httpAddress = GetEnvString("httpaddress", "0.0.0.0:9080");
	return true;
}

void CKernelServer::KernelServerStart(const char* pConfigFileName)
{
    //��־�ɿ����־ctx����
    basiclib::InitBasicLog(false);

	//Ŀǰû��ʹ���Ʒ��lua������
	//luaS_initshr();
#ifndef __BASICWINDOWS
	sigign();
#else
    //���ñ���
    basiclib::CWBasicString strTitle = basiclib::Basic_MultiStringToWideString(pConfigFileName, strlen(pConfigFileName));
    SetConsoleTitle(strTitle.c_str());
#endif
	const char * config_file = pConfigFileName;
	m_config.m_iniFile = basiclib::BasicGetModulePath() + "conf/" + pConfigFileName;
	if (basiclib::Basic_GetFileAttributes(m_config.m_iniFile.c_str()) == 0) {
		m_config.m_iniFile = basiclib::BasicGetModulePath() + pConfigFileName;
	}
	if (!ReadConfig()){
		printf("��ȡ�����ļ�ʧ��(%s)", m_config.m_iniFile.c_str());
		return;
	}
    if (GetEnv("sprotofilename")){
		printf("Э��ת�����(%s)", GetEnv("sprotofilename"));
		return;
	}
	//����ccframekey
	SetEnv("ccframekey", config_file);

	m_pCtxThreadPool = new CCtx_KernelThreadPool();
	CCtx_ThreadPool::CreateThreadPool(m_pCtxThreadPool);

	//��ʼ����̬��
	basiclib::CBasicString strLoadDllList = GetEnvString("dlllist", "");
	if (!m_mgtDllFile.LoadAllDll(strLoadDllList.c_str())){
		printf("����ʧ�ܶ�̬���ʼ��ʧ��(%s)", strLoadDllList.c_str());
		return;
	}
	basiclib::CBasicString strhttpReplaceList = GetEnvString("replacedlllist", "");
	if (!ReplaceDllList(strhttpReplaceList.c_str())){
		printf("��̬���滻ʧ��(%s)", strhttpReplaceList.c_str());
		return;
	}

	//���������߳�
    m_pCtxThreadPool->Init([&](CCorutinePlusThreadData* pThreadData)->void*{
		WorkThreadSelfData* pRet = new WorkThreadSelfData();
        pRet->Init();
		return pRet;
	}, [&](void* pUD)->void{
		delete (WorkThreadSelfData*)pUD;
	});

	//����httpģ��
	RegisterHttp();
	m_pHttpServer = CHttpSessionServer::CreateHttpServer(0);
	m_pHttpServer->bind_onhttpask(MakeFastFunction(this, &CKernelServer::OnHttpAsk));
    m_pHttpServer->SetIpTrust(GetEnvString("httpiptrust", "*"));
	Net_Int nHttpRet = m_pHttpServer->StartServer(m_config.m_httpAddress.c_str());
	if (nHttpRet != BASIC_NET_OK){
		printf("����httpʧ��%d", nHttpRet);
	}

	m_pCtxThreadPool->Wait();
}
void WorkThreadSelfData::Init(){
    if (m_L == nullptr){
        lua_State *L = luaL_newstate();
        luaL_openlibs(L);   // link lua lib
        m_L = L;
    }
}

//�ȳ�ʼ�������ȡ��
CKernelServer* CKernelServer::GetKernelServer()
{
	return g_pServer;
}

bool CKernelServer::ReplaceDllList(const char* pReplaceDllList)
{
	basiclib::CBasicKey2Value keyToVal;
	keyToVal.ParseText(pReplaceDllList, ">|");
	bool bSuccess = true;
	keyToVal.ForEach([&](const char* pKey, const char* pValue)->void{
		//�����ж�value�Ƿ��Ѿ����ع�
		if (m_mgtDllFile.IsDllLoaded(pValue)){
			//����Ѿ����ع�������Ҫ����
			return;
		}
		//����value
		m_mgtDllFile.ReplaceLoadDll(pValue);

		int nRet = m_mgtDllFile.ReplaceDllFunc(pKey, pValue);
		if (nRet != 0)
		{
			switch (nRet)
			{
			case -1:
				basiclib::BasicLogEventErrorV("ReplaceDllListʧ�ܣ�%sû�м���(%s -> %s)", pKey, pKey, pValue);
				break;
			case -2:
				basiclib::BasicLogEventErrorV("ReplaceDllListʧ�ܣ�%sû�м���(%s -> %s)", pValue, pKey, pValue);
				break;
			case -3:
				basiclib::BasicLogEventError("ReplaceDllList����Foreach��������!");
				break;
			case -4:
				basiclib::BasicLogEventErrorV("ReplaceDllList�������replace����ʧ��(%s -> %s)", pKey, pValue);
				break;
			}
			bSuccess = false;
		}
	});
	return bSuccess;
}

int CKernelServer::GetEnv(const char* key, int opt)
{
	const char* pKey = GetEnv(key);
    if (pKey == nullptr)
	{
		return opt;
	}
    return strtol(pKey, NULL, 10);
}

const char* CKernelServer::GetEnvString(const char* key, const char* opt)
{
	const char* str = GetEnv(key);
    if (str == nullptr){
		return opt;
	}
	return str;
}

void CKernelServer::SetEnv(const char* key, const char* value)
{
    //ֻ������������һ��
    m_mapConfig[key] = value;
    basiclib::CSpinLockFunc lock(&m_skynetenv, TRUE);
    //LҲҪ����һ��
    lua_State *L = m_L;
    lua_getglobal(L, key);
    assert(lua_isnil(L, -1));
    lua_pop(L, 1);
    lua_pushstring(L, value);
    lua_setglobal(L, key);
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void StartKernel(const char* pConfig){
    CKernelServer* pServer = CKernelServer::CreateKernelServer();
    if (pServer)
        pServer->KernelServerStart(pConfig);
    //basiclib::BasicLogEvent("-------------------------------------------");
    //TestShowImportDll();
    CKernelServer::ReleaseKernelServer();
}