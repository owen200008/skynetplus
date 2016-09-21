#include "skynetplus_context.h"
#include "skynetplus_servermodule.h"
#include "skynetplus_mq.h"

struct drop_t
{
	uint32_t handle;
};
//////////////////////////////////////////////////////////////////////////
unsigned int CSkynetContext::m_bTotalCtx = 0;
CSkynetContextHandle CSkynetContext::m_skynetHandle;


void CSkynetContext::InitGlobal(ServerModuleConfig* pConfig)
{
	 m_skynetHandle.Init();
}

CSkynetContext::CSkynetContext(const char* pName, CRefSkynetLoadDll pDll, void* pObject)
{
	m_strName = pName;
	m_dll = pDll;
	m_pObject = pObject;
	handle = 0;
	m_pObject = nullptr;
	m_cbFunc = nullptr;
	m_cb_ud = nullptr;
	m_session_id = 0;
	m_pLogfile = nullptr;
	m_bEndLess = false;
	m_pQueue = nullptr;
	
	m_bRelease = false;

	m_skynetHandle.Register(this);
	m_pQueue = new message_queue(this);
	basiclib::BasicInterlockedIncrement((LONG*)&m_bTotalCtx);
}

CSkynetContext::~CSkynetContext()
{
	assert(m_bRelease);
	if(m_pLogfile)
	{
		fclose(m_pLogfile);
	}
	if(m_pObject)
	{
		m_dll->Release(m_pObject);
	}
	if(m_pQueue)
	{
		struct drop_t d = { handle };
		m_pQueue->_drop_queue([&](struct skynet_message *msg, void *ud)->void{
			drop_t* d = (drop_t*)ud;
			msg->ReleaseData();
			uint32_t source = d->handle;
			SendByHandleID(source, msg->source, PTYPE_ERROR, 0, nullptr, 0);
		}, &d);
		delete m_pQueue;
	}
	basiclib::BasicInterlockedDecrement((LONG*)&m_bTotalCtx);
}

void CSkynetContext::ReleaseContext()
{
	m_skynetHandle.UnRegister(this);
	m_bRelease = true; 
	//push to global queue
	m_pQueue->PushToGlobalMQ();
}

int CSkynetContext::InitSkynetContext(const char* pParam)
{
	int nRet = m_dll->Init(m_pObject, this, pParam);
	if(nRet == 0)
	{
		m_pQueue->ReadyToGlobalMQ();
	}
	return nRet;
}

int CSkynetContext::GetNewSessionID()
{
	int session = ++m_session_id;
	if(session <= 0)
		session = 1;
	return session;
}

FILE* CSkynetContext::LogOpen()
{
	basiclib::CBasicString strLogPath = CSkynetPlus_ServerModule::GetServerModule().GetEnvString("logpath", (basiclib::BasicGetModulePath() + "/log/").c_str());
	size_t sz = strLogPath.GetLength();
	char tmp[sz + 128] = {0};
	sprintf(tmp, "%s/%s_%08x.log", strLogPath.c_str(), m_strName.c_str(), handle);
	FILE *f = fopen(tmp, "ab");
	if(f)
	{
		m_pLogfile = f;
		basiclib::BasicLogEventV("Open log file %s", tmp);
	}
	else
	{
		basiclib::BasicLogEventErrorV("Open log file %s fail", tmp);
	}
	return f;
}

void CSkynetContext::LogClose()
{
	basiclib::BasicLogEventV("Close log file :%s_%08x", m_strName.c_str(), handle);
	if(m_pLogfile)
	{
		fclose(m_pLogfile);
		m_pLogfile = nullptr;
	}
}

void LogBLOB(basiclib::CBasicSmartBuffer& smBuf, void * buffer, size_t sz)
{
	char szBuf[8] = {0};
	uint8_t* buf = (uint8_t*)buffer;
	for (int i=0;i < sz;i++) 
	{
		sprintf(szBuf, "%02x", buf[i]);
		smBuf.AppendData(szBuf, 2);
	}
}

void CSkynetContext::LogOutput(uint32_t source, int type, int session, void* buffer, size_t sz)
{
	if(m_pLogfile)
	{
		basiclib::CBasicSmartBuffer smBuf;
		smBuf.SetDataLength(1024);
		smBuf.SetDataLength(0);
		char szBuf[64] = {0};
		basiclib::GetTimeYMDHMS(szBuf, 64);
		smBuf.AppendString(szBuf);
		if (type == PTYPE_SOCKET)
		{
			//sprintf(szBuf, " [socket] ", );
		}
		else
		{
			sprintf(szBuf, " :%08x %d %d ", source, type, session);
			smBuf.AppendString(szBuf);
			LogBLOB(smBuf, buffer, sz);
			fwrite(smBuf.GetDataBuffer(), 1, smBuf.GetDataLength(), m_pLogfile);
		}
		smBuf.AppendString("\r\n");
		fflush(m_pLogfile);
	}
}

uint32_t CSkynetContext::QueryHandleIDByName(const char* pName)
{
	switch(pName[0])
	{
	case ':':
		return strtoul(pName + 1,NULL,16);
	case '.':
		return m_skynetHandle.GetHandleIDByName(pName);
	}
	basiclib::BasicLogEventErrorV("Don't support query global name %s", pName);
	return 0;
}

void CSkynetContext::_filter_args(int type, int *session, void** data, size_t* sz)
{
	int needcopy = !(type & PTYPE_TAG_DONTCOPY);
	int allocsession = type & PTYPE_TAG_ALLOCSESSION;
	type &= 0xff;
	if (allocsession)
	{
		assert(*session == 0);
		*session = GetNewSessionID();
	}
	if (needcopy && *data)
	{
		char* msg = (char*)basiclib::BasicAllocate(*sz + 1);
		memcpy(msg, *data, *sz);
		msg[*sz] = '\0';
		*data = msg;
	}
	*sz |= (size_t)type << MESSAGE_TYPE_SHIFT;
}

int CSkynetContext::SendByHandleID(uint32_t source, uint32_t destination , int type, int session, void * data, size_t sz)
{
	if((sz & MESSAGE_TYPE_MASK) != sz)
	{
		basiclib::BasicLogEventErrorV( "The message to %x is too large", destination);
		if(type & PTYPE_TAG_DONTCOPY)
		{
			basiclib::BasicDeallocate(data);
		}
		return -1;
	}
	_filter_args(type, &session, (void **)&data, &sz);
	if (source == 0)
		source = handle;
	if(destination == 0)
		return session;
	skynet_message smsg;
	smsg.source = source;
	smsg.session = session;
	smsg.data = data;
	smsg.sz = sz;
	if(PushToHandleMQ(destination, &smsg))
	{
		if(data)
			basiclib::BasicDeallocate(data);
		return -1;
	}

	return session;
}

int CSkynetContext::SendByName(uint32_t source, const char* pDst , int type, int session, void * data, size_t sz)
{
	uint32_t uDst = QueryHandleIDByName(pDst);
	if(uDst == 0)
	{
		if (type & PTYPE_TAG_DONTCOPY)
		{
			if(data)
				basiclib::BasicDeallocate(data);
		}
		return -1;
	}
	return SendByHandleID(source, uDst, type, session, data, sz);
}

int CSkynetContext::PushToHandleMQ(uint32_t handle, skynet_message *message)
{
	bool bRet = m_skynetHandle.GrabContext(handle, [=](CSkynetContext*pCtx)->void{
		pCtx->m_pQueue->MQPush(message);
	});
	if(bRet == false)
		return -1;
	return 0;
}

