#ifndef SKYNETPLUA_SERVERMODULE_H
#define SKYNETPLUA_SERVERMODULE_H

#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <string.h>
extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <lstring.h>
}

#include "skynetplus_handle.h"
#include "skynetplus_module.h"
#include "skynetplus_monitor.h"
#include "skynetplus_timer.h"
#include "skynetplus_context.h"

#define THREAD_WORKER 0
#define THREAD_MAIN 1
#define THREAD_SOCKET 2
#define THREAD_TIMER 3
#define THREAD_MONITOR 4

//////////////////////////////////////////////////////////////////////
//system module
//////////////////////////////////////////////////////////////////////

struct ServerModuleConfig
{
	int				m_nThread;
	const char*		m_daemon;
	const char*		m_module_path;
	const char*		m_bootstrap;
	const char*		m_logger;
	const char*		m_logservice;
	ServerModuleConfig()
	{
		memset(this, 0, sizeof(ServerModuleConfig));
	}
};

class CSkynetPlus_ServerModule
{
public:
	CSkynetPlus_ServerModule();
	virtual ~CSkynetPlus_ServerModule();
	
	static CSkynetPlus_ServerModule& GetServerModule();
public:
	void Init();
	void ReadConfig();
	void Start(const char* pConfigFileName);

	void InitThreadHandle(int m);
	uint32_t GetCurrentHandle();

	CSkynetContext* NewContext(const char* pName, const char* pParam);
	void DeleteContextByHandle(uint32_t handle);

	message_queue* DispatchMsgQueue(skynet_monitor* sm, message_queue *q);
	void DispatchMsg(CSkynetContext* pCtx, skynet_message *msg);
	void DispathAllContext(CSkynetContext* pCtx);
public:
	//context size
	int GetTotalContext();
	
	//get lua config
	const char* GetEnv(const char* key);
	int GetEnv(const char* key, int opt);
	const char* GetEnvString(const char* key, const char* opt);
	void SetEnv(const char* key, const char* value);
protected:
	void ShowState();
	int DaemonInit();
	void BussinessStart();
	void StartThread();
public:
	void MainRun();
	void TimerRun();
	void WorkRun();
protected:
	ServerModuleConfig			m_config;
	unsigned short				m_usExitThreadCount;
	bool						m_bTimerExit;

	int 						m_nTotal;
	bool						m_bInit;
	bool						m_bExit;
	uint32_t 					m_monitor_exit;
	pthread_key_t				m_handle_key;
	
	//harborid
	CSkynetModules				m_skynetModules;
	CMonitor					m_monitor;

	basiclib::SpinLock			m_skynetenv;
	lua_State*					m_L;
};

void id_to_hex(char * str, uint32_t id);
uint32_t SystemGetAllocHandleID();


#endif
