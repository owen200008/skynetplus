#include "skynetplus_module.h"

CSkynetLoadDll::CSkynetLoadDll()
{
	create = nullptr;
	init = nullptr;
	release = nullptr;
	signalfunc = nullptr;
}

CSkynetLoadDll::~CSkynetLoadDll()
{
}

int CSkynetLoadDll::OpenSym()
{
	int name_size = m_strLoadFileName.GetLength();
	char tmp[name_size + 9];
	memcpy(tmp, m_strLoadFileName.c_str(), name_size);
	strcpy(tmp+name_size, "_create");
	create = (skynet_dl_create)GetProcAddress(tmp);
	strcpy(tmp+name_size, "_init");
	init = (skynet_dl_init)GetProcAddress(tmp);
	strcpy(tmp+name_size, "_release");
	release = (skynet_dl_release)GetProcAddress(tmp);
	strcpy(tmp+name_size, "_signal");
	signalfunc = (skynet_dl_signal)GetProcAddress(tmp);

	return init == nullptr;
}

void* CSkynetLoadDll::Create()
{
	if(create)
		return create();
	return nullptr;
}
int CSkynetLoadDll::Init(void* inst, CSkynetContext* pCtx, const char* param)
{
	return init(inst, pCtx, param);
}
void CSkynetLoadDll::Release(void* inst)
{
	if(release)
		release(inst);
}
void CSkynetLoadDll::Signal(void* inst, int signal)
{
	if(signalfunc)
		signalfunc(inst, signal);
}
/////////////////////////////////////////////////////////////////////////
CSkynetModules::CSkynetModules()
{
}

CSkynetModules::~CSkynetModules()
{
}

void CSkynetModules::Init(const char* pPath)
{
	basiclib::BasicSpliteString(pPath, ";", basiclib::IntoContainer_s<basiclib::CBasicStringArray>(m_ayPath));
}

CRefSkynetLoadDll CSkynetModules::GetSkynetModule(const char* pName)
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	MapSkynetModule::iterator iter = m_modules.find(pName);
	if(iter != m_modules.end())
	{
		return iter->second;
	}
	int nNameSize = strlen(pName);
	CSkynetLoadDll* pRet = new CSkynetLoadDll();
	const int nMaxPathLength = 256;
	char tmp[nMaxPathLength];
	int nSize = m_ayPath.size();
	bool bLoadSuccess = false;
	for(int i = 0;i < nSize;i++)
	{
		memset(tmp,0,nMaxPathLength);
		basiclib::CBasicString& strPath = m_ayPath[i];
		int nSubPathLength = strPath.GetLength();
		int j = 0;
		char cChar = 0;
		for(;j < nSubPathLength; j++)
		{
			cChar = strPath.GetAt(j);
			if(cChar == '?')
			{
				break;
			}
			tmp[j] = cChar;
		}
		if(cChar == '?')
		{
			memcpy(tmp + j, pName, nNameSize);
			strcpy(tmp + j + nNameSize, strPath.c_str() + j + 1);
		}
		else
		{
			basiclib::BasicLogEventError("Invalid C service path");
			exit(1);
		}
		long lLoadRet = pRet->LoadLibrary(tmp);
		if(lLoadRet == 0)
		{
			bLoadSuccess = true;
			break;
		}
	}
	if(!bLoadSuccess)
	{
		delete pRet;
		basiclib::BasicLogEventErrorV("try open %s failed",pName);
		return nullptr;
	}
	else
	{
		if(pRet->OpenSym() != 0)
		{
			delete pRet;
			return nullptr;
		}
		m_modules[pName] = pRet;
	}

	return pRet;
}

void CSkynetModules::InsertSkynetModule(CSkynetLoadDll* p)
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	m_modules[p->GetLibName()] = p;

}

void CSkynetModules::ReleaseSkynetModule(const char* pName)
{
	if(pName == nullptr)
		return;
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	m_modules.erase(pName);
}








