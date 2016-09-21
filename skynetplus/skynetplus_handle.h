#ifndef SKYNETPLUA_CONTEXT_HANDLE_H
#define SKYNETPLUA_CONTEXT_HANDLE_H

#include <stdint.h>
#include <basic.h>
#include "rwlock.h"

class CSkynetContext;
// reserve high 8 bits for remote id
//
#define HANDLE_ID_BEGIN	0xff
#define HANDLE_ID_ALLOCATE_LOG 0xffffff

class CSkynetContextHandle : public basiclib::CBasicObject
{
public:
	CSkynetContextHandle();
	virtual ~CSkynetContextHandle();

	void Init();
	uint32_t Register(CSkynetContext* ctx);
	int UnRegister(CSkynetContext* ctx);
	int UnRegister(uint32_t handleID);
	
	bool GrabContext(uint32_t handle, const std::function<void(CSkynetContext*)>& func);
	uint32_t GetHandleIDByName(const char* pName);
	bool GrabContextByName(const char* pName, const std::function<void(CSkynetContext*)>& func);
protected:
	uint32_t GetNewHandleID();
public:
	rwlock 					m_lock;
	uint32_t				m_handle_index;
	
	typedef basiclib::basic_vector<uint32_t>::type ReleaseNodeHandleID;
	ReleaseNodeHandleID		m_releaseHandleID;

	typedef basiclib::basic_map<basiclib::CBasicString, CSkynetContext*>::type MapNameToCtx;
	typedef basiclib::basic_map<uint32_t, CSkynetContext*>::type MapHandleIDToCtx;
	MapNameToCtx		m_mapNameToCtx;
	MapHandleIDToCtx	m_mapIDToCtx;

};

#endif
