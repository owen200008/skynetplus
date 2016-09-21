#ifndef SKYNETPLUS_MODULE_H
#define SKYNETPLUS_MODULE_H

#include <stdint.h>
#include <basic.h>

class CSkynetContext;

typedef void * (*skynet_dl_create)(void);
typedef int (*skynet_dl_init)(void * inst, CSkynetContext*, const char * parm);
typedef void (*skynet_dl_release)(void * inst);
typedef void (*skynet_dl_signal)(void * inst, int signal);

class CSkynetLoadDll : public basiclib::CBasicLoadDll, public basiclib::EnableRefPtr<CSkynetLoadDll>
{
public:
	CSkynetLoadDll();
	virtual ~CSkynetLoadDll();

	void* Create();
	int Init(void* inst, CSkynetContext* pCtx, const char* param);
	void Release(void* inst);
	void Signal(void* inst, int signal);

	//use by CSkynetModules, no need to call
	int OpenSym();
protected:
	skynet_dl_create 	create;
	skynet_dl_init 		init;
	skynet_dl_release 	release;
	skynet_dl_signal 	signalfunc;
};
typedef basiclib::CBasicRefPtr<CSkynetLoadDll> CRefSkynetLoadDll;

class CSkynetModules : basiclib::CBasicObject
{
public:
	CSkynetModules();
	virtual ~CSkynetModules();

	void Init(const char* pPath);

	CRefSkynetLoadDll GetSkynetModule(const char* pName);
	void InsertSkynetModule(CSkynetLoadDll* p);

	void ReleaseSkynetModule(const char* pName);
protected:
	basiclib::SpinLock				m_lock;
	typedef basiclib::basic_map<basiclib::CBasicString, CRefSkynetLoadDll>::type MapSkynetModule;
	MapSkynetModule					m_modules;
	basiclib::CBasicStringArray		m_ayPath;
};


#endif
