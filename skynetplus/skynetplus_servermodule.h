#ifndef SKYNETPLUA_SERVERMODULE_H
#define SKYNETPLUA_SERVERMODULE_H

#include "../public/malloc_3rd.h"
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

#define THREAD_WORKER 0
#define THREAD_MAIN 1
#define THREAD_SOCKET 2
#define THREAD_TIMER 3
#define THREAD_MONITOR 4

struct ServerModuleConfig
{
	int				m_nThread;
	int				m_nHarbor;
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
	void Start();

	void InitThreadHandle(int m);
	uint32_t GetCurrentHandle();

	int GetTotalContext(){return m_nTotal;}
	int TotalContextInc();
	void TotalContextDec();

	const char* GetEnv(const char* key);
	int GetEnv(const char* key, int opt);
	const char* GetEnvString(const char* key, const char* opt);
	void SetEnv(const char* key, const char* value);
protected:
	int DaemonInit();
protected:
	ServerModuleConfig	m_config;

	int 				m_nTotal;
	bool				m_bInit;
	uint32_t 			m_monitor_exit;
	pthread_key_t		m_handle_key;

	SpinLock			m_skynetenv;
	lua_State*			m_L;
};

void id_to_hex(char * str, uint32_t id);
void skynet_initthread(int m);
void SystemLogError(const char* msg, ...);
void SystemLog(const char* msg, ...);
uint32_t SystemGetAllocHandleID();


#endif
