#include <basic.h>
#include "skynetplus_servermodule.h"
#include "skynetplus_timer.h"
#include "skynetplus_mq.h"
#include "../public/atomic.h"
#include <assert.h>
#include <sys/file.h>

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
	m_bExit = false;
	m_bTimerExit = false;
	m_usExitThreadCount = 0;
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

void CSkynetPlus_ServerModule::Init()
{
	m_bInit = true;
	if (pthread_key_create(&m_handle_key, NULL)) 
	{
		basiclib::BasicLogEvent("pthread_key_create failed");
		exit(1);
	}
	InitThreadHandle(THREAD_MAIN);

	m_L = luaL_newstate();
}

void CSkynetPlus_ServerModule::ReadConfig()
{
	m_config.m_nThread = GetEnv("thread", 8);
	m_config.m_module_path = GetEnvString("cpath", "./cservice/?.so");
	m_config.m_bootstrap = GetEnvString("bootstrap","snlua bootstrap");
	m_config.m_daemon = GetEnvString("daemon", NULL);
	m_config.m_logger = GetEnvString("logger", NULL);
	m_config.m_logservice = GetEnvString("logservice", "logger");
}

void CSkynetPlus_ServerModule::Start(const char* pConfigFileName)
{
	ReadConfig();
	//default log
	basiclib::CBasicString strLogPath = GetEnvString("logpath", (basiclib::BasicGetModulePath() + "/log/").c_str());
	basiclib::CBasicString strDefaultLogFileName = strLogPath + pConfigFileName + ".log";
	basiclib::CBasicString strDefaultErrorFileName = strLogPath + pConfigFileName + ".error";
	basiclib::BasicSetDefaultLogEventMode(LOG_ADD_TIME|LOG_ADD_THREAD, strDefaultLogFileName.c_str());
	basiclib::BasicSetDefaultLogEventErrorMode(LOG_ADD_TIME|LOG_ADD_THREAD, strDefaultErrorFileName.c_str());

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

	BussinessStart();

	if(m_config.m_daemon)
	{
		unlink(m_config.m_daemon);
	}
}

void CSkynetPlus_ServerModule::BussinessStart()
{
	CSkynetContext::InitGlobal(&m_config);
	m_skynetModules.Init(m_config.m_module_path);
	//??skynet_timer_init();
	//??skynet_socket_init();
	//boot
	int sz = strlen(m_config.m_bootstrap);
	char name[sz+1];
	char args[sz+1];
	sscanf(m_config.m_bootstrap, "%s %s", name, args);
	CSkynetContext* pCtxBoot = NewContext(name, args);
	if(pCtxBoot == nullptr)
	{
		basiclib::BasicLogEventErrorV("Bootstrap error : %s\n", m_config.m_bootstrap);
		exit(1);
	}
	StartThread();
}

THREAD_RETURN Main_Thread(void* pParam)
{
	CSkynetPlus_ServerModule* pServer = (CSkynetPlus_ServerModule*)pParam;
	pServer->MainRun();
	return 0;
}

THREAD_RETURN Timer_Thread(void* pParam)
{
	CSkynetPlus_ServerModule* pServer = (CSkynetPlus_ServerModule*)pParam;
	pServer->TimerRun();
	return 0;
}

THREAD_RETURN Work_Thread(void* pParam)
{
	CSkynetPlus_ServerModule* pServer = (CSkynetPlus_ServerModule*)pParam;
	pServer->WorkRun();
	return 0;
}

void CSkynetPlus_ServerModule::ShowState()
{
	basiclib::BasicLogEventV("TotalCtx:%d", GetTotalContext());
}

void CSkynetPlus_ServerModule::MainRun()
{
	basiclib::BasicLogEventV("StartMainThread:%d", (unsigned int)basiclib::BasicGetCurrentThreadId());
	int nTick = 0;
	InitThreadHandle(THREAD_MONITOR);
	while(!m_bExit)
	{
		if(nTick % 5 == 0)
			m_monitor.Check();
		if(nTick % 30 == 29)
		{
			ShowState();
		}

		basiclib::BasicSleep(1000);
		nTick++;
	}
	//wait for other thread
	while(m_usExitThreadCount != m_config.m_nThread || m_bTimerExit == false)
	{
		basiclib::BasicSleep(100);
	}
}

void CSkynetPlus_ServerModule::TimerRun()
{
	m_bTimerExit = false;
	InitThreadHandle(THREAD_TIMER);
	while(!m_bExit)
	{
		skynet_updatetime();
		usleep(2500);
	}
	m_bTimerExit = true;
}

void CSkynetPlus_ServerModule::WorkRun()
{
	static int nIndex = 0;

	int nThreadIndex = basiclib::BasicInterlockedIncrement((LONG*)&nIndex);
	basiclib::BasicLogEventV("StartWorkThread:%d Index:%d", (unsigned int)basiclib::BasicGetCurrentThreadId(), nThreadIndex);
	InitThreadHandle(THREAD_WORKER);
	skynet_monitor* sm = &(m_monitor.m_pMonitor[nThreadIndex]);
	message_queue * q = nullptr;
	while(!m_bExit)
	{
		q = DispatchMsgQueue(sm, q);
		if(q == nullptr)
		{
			CSingletonSkynetMQ::Instance().WaitForGlobalMQ(5000);
		}
	}
	basiclib::BasicInterlockedIncrement((LONG*)&m_usExitThreadCount);
}

void CSkynetPlus_ServerModule::StartThread()
{
	m_monitor.InitMonitor(m_config.m_nThread);

	DWORD nThreadId = 0;
	HANDLE pMainHandle = basiclib::BasicCreateThread(Main_Thread, this, &nThreadId);
	basiclib::BasicCreateThread(Timer_Thread, this, &nThreadId);

	for(int i = 0;i < m_config.m_nThread;i++)
	{
		basiclib::BasicCreateThread(Work_Thread, this, &nThreadId);
	}
	basiclib::BasicWaitThread(pMainHandle);
}


CSkynetContext* CSkynetPlus_ServerModule::NewContext(const char* pName, const char* pParam)
{
	CRefSkynetLoadDll pDllModule = m_skynetModules.GetSkynetModule(pName);
	if(pDllModule == nullptr)
		return nullptr;
	void* inst = pDllModule->Create();
	if (inst == nullptr)
		return nullptr;
	CSkynetContext* pCtx = new CSkynetContext(pName, pDllModule, inst);

	int r = pCtx->InitSkynetContext(pParam);
	if (r == 0) 
	{
		basiclib::BasicLogEventV("LAUNCH %s %s", pName, pParam ? pParam : "");
		return pCtx;
	}
	basiclib::BasicLogEventErrorV("FAILED launch %s", pName);
	pCtx->ReleaseContext();
	
	return nullptr;
}

void CSkynetPlus_ServerModule::DeleteContextByHandle(uint32_t handle)
{
	bool bRet = CSkynetContext::m_skynetHandle.GrabContext(handle,[&](CSkynetContext* pCtx)->void{
		pCtx->ReleaseContext();
	});
	basiclib::BasicLogEventErrorV("KILL :%0x Ret:%d", handle, bRet);
}

void CSkynetPlus_ServerModule::DispatchMsg(CSkynetContext* pCtx, skynet_message *msg)
{
	pthread_setspecific(m_handle_key, (void*)(uintptr_t)pCtx->handle);
	int nType = msg->sz >> MESSAGE_TYPE_SHIFT;
	size_t sz = msg->sz & MESSAGE_TYPE_MASK;
	pCtx->LogOutput(msg->source, nType,  msg->session, msg->data, sz);

	if(pCtx->m_cbFunc(pCtx, pCtx->m_cb_ud, nType, msg->session, msg->source, msg->data, sz))
	{
		msg->ReleaseData();
	}
}

void CSkynetPlus_ServerModule::DispathAllContext(CSkynetContext* pCtx)
{
	skynet_message msg;
	message_queue *q = pCtx->m_pQueue;
	while (!q->MQPop(&msg)) 
	{
		DispatchMsg(pCtx, &msg);
	}
}

message_queue* CSkynetPlus_ServerModule::DispatchMsgQueue(skynet_monitor* sm, message_queue *q)
{
	CSkynetMQ& globalMQ = CSingletonSkynetMQ::Instance();
	if (q == nullptr) 
	{
		q = globalMQ.GlobalMQPop();
		if (q == nullptr)
		{
			return nullptr;
		}
	}
	CRefSkynetContext& refCtx = q->GetContext();
	if(refCtx->IsReleaseContext())
	{
		q->ContextRelease();
		return globalMQ.GlobalMQPop();
	}
	int i,n=1;
	skynet_message msg;
	for (i=0;i<n;i++)
	{
		if(q->MQPop(&msg))
		{
			return globalMQ.GlobalMQPop();
		}
		int overload = q->Overload();
		if(overload)
		{
			basiclib::BasicLogEventErrorV("May overload, message queue length = %d", overload);
		}
		sm->Trigger(msg.source, refCtx->handle);
		if (refCtx->m_cbFunc == NULL)
		{
			msg.ReleaseData();
		}
		else
		{
			DispatchMsg(refCtx.GetResFunc(), &msg);
		}
		sm->Trigger(0, 0);
	}
	message_queue *nq = globalMQ.GlobalMQPop();
	if(nq)
	{
		globalMQ.GlobalMQPush(q);
		q = nq;
	}
	return q;
}

int CheckPid(const char* pidfile)
{
	int pid = 0;
	FILE *f = fopen(pidfile,"r");
	if (f == NULL)
		return 0;
	int n = fscanf(f,"%d", &pid);
	fclose(f);
	if (n != 1 || pid == 0 || pid == getpid())
	{
		return 0;
	}
	if (kill(pid, 0) && errno == ESRCH)
		return 0;
	return pid;
}
int WritePid(const char* pidfile)
{
	FILE *f = nullptr;
	int pid = 0;
	int fd = open(pidfile, O_RDWR|O_CREAT, 0644);
	if (fd == -1) 
	{
		basiclib::BasicLogEventV("Can't create %s", pidfile);
		return 0;
	}
	f = fdopen(fd, "r+");
	if (f == NULL) 
	{
		basiclib::BasicLogEventV("Can't open %s", pidfile);
		return 0;
	}
	if (flock(fd, LOCK_EX|LOCK_NB) == -1) 
	{
		int n = fscanf(f, "%d", &pid);
		fclose(f);
		if (n != 1)
		{
			basiclib::BasicLogEvent("Can't lock and read pidfile");
		}
		else
		{
			basiclib::BasicLogEventV("Can't lock pidfile, lock is held by pid %d.\n", pid);
		}
		return 0;
	}
	pid = getpid();
	if (!fprintf(f,"%d\n", pid))
	{
		basiclib::BasicLogEvent("Can't write pid");
		close(fd);
		return 0;
	}
	fflush(f);
	return pid;
}

int CSkynetPlus_ServerModule::DaemonInit()
{
	if(m_config.m_daemon)
	{
		const char* pidfile = m_config.m_daemon;
		int pid = CheckPid(pidfile);
		if (pid) 
		{
			basiclib::BasicLogEventV("Skynet is already running, pid = %d", pid);
			return 1;
		}
		if (daemon(1,0)) 
		{
			basiclib::BasicLogEventV("Can't daemonize");
			return 1;
		}
		pid = WritePid(pidfile);
		if(pid == 0)
		{
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

///////////////////////////////////////////////////////////////////////////////
int CSkynetPlus_ServerModule::GetTotalContext()
{
	return CSkynetContext::m_bTotalCtx;
}

const char* CSkynetPlus_ServerModule::GetEnv(const char* key)
{
	CSpinLockFunc lock(&m_skynetenv, TRUE);
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
	CSpinLockFunc lock(&m_skynetenv, TRUE);
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

uint32_t SystemGetAllocHandleID()
{
	return g_Node.GetCurrentHandle();
}
