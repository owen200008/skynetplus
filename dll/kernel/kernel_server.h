#ifndef SKYNETPLUS_KERNEL_SERVERMODULE_H
#define SKYNETPLUS_KERNEL_SERVERMODULE_H

#include <basic.h>
#include "../scbasic/json/scbasicjson.h"
#include "ccframe/ctx_threadpool.h"
#include "ccframe/dllmodule.h"
#include "kernel_head.h"
extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
//system module
//////////////////////////////////////////////////////////////////////
struct ServerModuleConfig
{
	basiclib::CBasicString		m_iniFile;
	basiclib::CBasicString		m_daemon;
	ServerModuleConfig()
	{
	}
};

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

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
public:
	bool						m_bRunning;
	ServerModuleConfig			m_config;

	basiclib::SpinLock			m_skynetenv;
	lua_State*					m_L;

	CKernelLoadDllMgr			m_mgtDllFile;

	CCtx_ThreadPool*			m_pCtxThreadPool;

    typedef basiclib::basic_map<basiclib::CBasicString, basiclib::CBasicString> MapConfig;
    MapConfig                   m_mapConfig;
};
#pragma warning (pop)

extern "C" void _SKYNET_KERNEL_DLL_API StartKernel(const char* pConfig);

#endif
