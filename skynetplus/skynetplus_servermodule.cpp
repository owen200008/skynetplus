#include "skynetplus_servermodule.h"
#include "../public/atomic.h"
#include <assert.h>

static int SIG = 0;
static void handle_hup(int signal) 
{
	if (signal == SIGHUP) 
	{
		signal = 1;
	}
}
////////////////////////////////////////////////////////////////////////////////
static CSkynetPlus_ServerModule g_Node;
CSkynetPlus_ServerModule& CSkynetPlus_ServerModule::GetServerModule()
{
	return g_Node;	
}

CSkynetPlus_ServerModule::CSkynetPlus_ServerModule()
{
	m_nTotal = 0;
	m_bInit = false;
	m_monitor_exit = 0;

	m_L = nullptr;
}

CSkynetPlus_ServerModule::~CSkynetPlus_ServerModule()
{
	if(m_bInit)
	{
		pthread_key_delete(m_handle_key);
	}
}

int CSkynetPlus_ServerModule::TotalContextInc()
{
	ATOM_INC(&m_nTotal);	
}

void CSkynetPlus_ServerModule::TotalContextDec()
{
	ATOM_DEC(&m_nTotal);
}

void CSkynetPlus_ServerModule::Init()
{
	m_bInit = true;
	if (pthread_key_create(&m_handle_key, NULL)) 
	{
		printf("pthread_key_create failed");
		exit(1);
	}
	InitThreadHandle(THREAD_MAIN);

	m_L = luaL_newstate();
}

void CSkynetPlus_ServerModule::ReadConfig()
{
	m_config.m_nThread = GetEnv("thread", 8);
	m_config.m_module_path = GetEnvString("cpath", "./cservice/?.so");
	m_config.m_nHarbor = GetEnv("harbor", 1);
	m_config.m_bootstrap = GetEnvString("bootstrap","snlua bootstrap");
	m_config.m_daemon = GetEnvString("daemon", NULL);
	m_config.m_logger = GetEnvString("logger", NULL);
	m_config.m_logservice = GetEnvString("logservice", "logger");
}

void CSkynetPlus_ServerModule::Start()
{
	// register SIGHUP for log file reopen
	struct sigaction sa;
	sa.sa_handler = &handle_hup;
	sa.sa_flags = SA_RESTART;
	sigfillset(&sa.sa_mask);
	sigaction(SIGHUP, &sa, NULL);
	
	if(DaemonInit())
	{
		exit(1);
	}
}

int CSkynetPlus_ServerModule::DaemonInit()
{
	if(m_config.m_daemon)
	{
		const char* pidfile = m_config.m_daemon;
		int pid = check_pid(pidfile);
		if (pid) 
		{
			printf("Skynet is already running, pid = %d.\n", pid);
			return 1;
		}
		if (daemon(1,0)) 
		{
			printf("")
			return 1;
		}
	}
	return 0;
}

uint32_t CSkynetPlus_ServerModule::GetCurrentHandle()
{
	if(m_bInit)
	{
		void * handle = pthread_getspecific(m_handle_key);
		return (uint32_t)(uintptr_t)handle;
	}
	uint32_t v = (uint32_t)(-THREAD_MAIN);
	return v;
}

void CSkynetPlus_ServerModule::InitThreadHandle(int m) 
{
	uintptr_t v = (uint32_t)(-m);
	pthread_setspecific(m_handle_key, (void *)v);
}

const char* CSkynetPlus_ServerModule::GetEnv(const char* key)
{
	CSpinLockFunc lock(m_skynetenv);
	lock.Lock();
	lua_State *L = m_L;
	lua_getglobal(L, key);
	const char * result = lua_tostring(L, -1);
	lua_pop(L, 1);
	return result;
}
int  CSkynetPlus_ServerModule::GetEnv(const char* key, int opt)
{
	const char* str = GetEnv(key);
	if (str == NULL) 
	{
		char tmp[20];
		sprintf(tmp,"%d",opt);
		SetEnv(key, tmp);
		return opt;
	}
	return strtol(str, NULL, 10);
}

const char* CSkynetPlus_ServerModule::GetEnvString(const char* key, const char* opt)
{
	const char* str = GetEnv(key);
	if (str == NULL)
	{
		if (opt) 
		{
			SetEnv(key, opt);
			opt = GetEnv(key);
		}
		return opt;
	}
	return str;
}

void CSkynetPlus_ServerModule::SetEnv(const char* key, const char* value)
{
	CSpinLockFunc lock(m_skynetenv);
	lock.Lock();
	lua_State *L = m_L;
	lua_getglobal(L, key);
	assert(lua_isnil(L, -1));
	lua_pop(L,1);
	lua_pushstring(L,value);
	lua_setglobal(L,key);
}
////////////////////////////////////////////////////////////
void id_to_hex(char * str, uint32_t id) 
{
	int i;
	static char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F' };
	str[0] = ':';
	for (i=0;i<8;i++) 
	{
		str[i+1] = hex[(id >> ((7-i) * 4))&0xf];
	}
	str[9] = '\0';
}


void SystemLogError(const char* msg, ...)
{
	
}

void SystemLog(const char* msg, ...)
{

}

uint32_t SystemGetAllocHandleID()
{
	return g_Node.GetCurrentHandle();
}
