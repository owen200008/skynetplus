#ifndef SKYNETPLUA_CONTEXT_H
#define SKYNETPLUA_CONTEXT_H

#include <stdint.h>
#include <basic.h>
#include "skynetplus_module.h"
#include "skynetplus_handle.h"

#define PTYPE_TEXT 0
#define PTYPE_RESPONSE 1
#define PTYPE_MULTICAST 2
#define PTYPE_CLIENT 3
#define PTYPE_SYSTEM 4
#define PTYPE_HARBOR 5
#define PTYPE_SOCKET 6
// read lualib/skynet.lua examples/simplemonitor.lua
#define PTYPE_ERROR 7	
// read lualib/skynet.lua lualib/mqueue.lua lualib/snax.lua
#define PTYPE_RESERVED_QUEUE 8
#define PTYPE_RESERVED_DEBUG 9
#define PTYPE_RESERVED_LUA 10
#define PTYPE_RESERVED_SNAX 11
#define PTYPE_ONTIMER	101
#define PTYPE_MAXDEFINE	256
//
#define PTYPE_TAG_DONTCOPY 0x10000
#define PTYPE_TAG_ALLOCSESSION 0x20000

struct ServerModuleConfig;
class CSkynetContext;
class message_queue;
struct skynet_message;

typedef int (*skynet_cb)(CSkynetContext* pCtx, void *ud, int type, int session, uint32_t source , const void * msg, size_t sz);
class CSkynetContext : public basiclib::CBasicObject, public basiclib::EnableRefPtr<CSkynetContext>
{
public:
	CSkynetContext(const char* pName, CRefSkynetLoadDll pDll, void* pObject);
	virtual ~CSkynetContext();
	//global init
	static void InitGlobal(ServerModuleConfig* pConfig);
	
	//init self & delete
	int InitSkynetContext(const char* pParam);
	void ReleaseContext();
	bool IsReleaseContext(){return m_bRelease;}
	/////////////////////////////////////////////////////////////////////////////
	FILE* LogOpen();
	void LogClose();
	void LogOutput(uint32_t source, int type, int session, void* buffer, size_t sz);


	basiclib::CBasicString& GetContextName(){ return m_strName; }
	bool IsSameContextName(const char* pName){ return strcmp(pName, m_strName.c_str()) == 0; }

	uint32_t GetContextHandleID(){ return handle; }
	void SetContextHandleID(uint32_t uHandleID){ handle = uHandleID; }

	int GetNewSessionID();

	//query the handleid
	static uint32_t QueryHandleIDByName(const char* pName);
	int SendByName(uint32_t source, const char* pDst, int type, int session, void * data, size_t sz);
	int SendByHandleID(uint32_t source, uint32_t destination , int type, int session, void * data, size_t sz);

public:
	void _filter_args(int type, int *session, void** data, size_t* sz);
	static int PushToHandleMQ(uint32_t handle, skynet_message *message);

public:
	basiclib::CBasicString		m_strName;
	uint32_t					handle;

	CRefSkynetLoadDll			m_dll;
	void*						m_pObject;
	skynet_cb					m_cbFunc;
	void*						m_cb_ud;
	int 						m_session_id;
	FILE* 						m_pLogfile;
	bool						m_bEndLess;

	message_queue*				m_pQueue;

	bool						m_bRelease;

	static unsigned int			m_bTotalCtx;
	static CSkynetContextHandle m_skynetHandle;
};
typedef basiclib::CBasicRefPtr<CSkynetContext> CRefSkynetContext;

#endif
