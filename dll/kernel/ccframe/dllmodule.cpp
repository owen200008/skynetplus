#include "dllmodule.h"
#include "log/ctx_log.h"

CInheritGlobalParam::CInheritGlobalParam(const char* pVersion)
{
	m_strVersion = pVersion;
}

CInheritGlobalParam::~CInheritGlobalParam()
{

}
////////////////////////////////////////////////////////////////////////////////////
CKernelLoadDll::CKernelLoadDll()
{
	m_pDllGlobalParam = nullptr;
}

CKernelLoadDll::~CKernelLoadDll()
{

}

bool CKernelLoadDll::LoadKernelDll(const char* pLoadDll, bool bReplace)
{
	if (m_dll.LoadLibrary(pLoadDll) != 0)
	{
		CCFrameSCBasicLogEventErrorV("���ض�̬��ʧ��%s", pLoadDll);
		return false;
	}

	KernelLoadCreate pCreate = (KernelLoadCreate)m_dll.GetProcAddress("InitForKernel");
	if (pCreate)
	{
		return pCreate(this, bReplace, m_pDllGlobalParam);
	}
	return true;
}
bool CKernelLoadDll::ReplaceDll(CKernelLoadDll& dll)
{
	bool bRet = m_dll.ReplaceDll(dll.GetDll(), [](const char* pLog)->void{
		CCFrameSCBasicLogEvent(pLog);
	});
	//�ϲ�
	if (m_pDllGlobalParam)
		m_pDllGlobalParam->InheritParamTo(dll.GetInheritGlobalParam());
	//�޸�ָ��ָ���µ�ȫ�ֱ���
	m_pDllGlobalParam = dll.GetInheritGlobalParam();
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
bool CKernelLoadDllMgr::LoadAllDll(const char* pDlls)
{
	basiclib::CBasicStringArray ayItems;
	basiclib::BasicSpliteString(pDlls, ';', basiclib::IntoContainer_s<basiclib::CBasicStringArray>(ayItems));
	int nSize = ayItems.GetSize();
	for (int i = 0; i < nSize; i++)
	{
		if (!RegisterDll(ayItems[i].c_str()))
			return false;
	}
	return true;
}

bool CKernelLoadDllMgr::ReplaceLoadDll(const char* pLoadDll)
{
	return RegisterDll(pLoadDll, true);
}

bool CKernelLoadDllMgr::RegisterDll(const char* pLoadDll, bool bReplace)
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	//�Ѿ����ز���Ҫ�ظ�����
	if (m_mapDll.find(pLoadDll) != m_mapDll.end())
		return true;
	CKernelLoadDll* pDll = new CKernelLoadDll();
	if (!pDll->LoadKernelDll(pLoadDll, bReplace))
	{
		delete pDll;
		return false;
	}
	m_mapDll[pLoadDll] = pDll;
	return true;
}
int CKernelLoadDllMgr::ReplaceDllFunc(const char* pLoadOldDll, const char* pLoadNewDll)
{
	if (pLoadOldDll == nullptr || pLoadNewDll == nullptr)
		return -3;
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	if (m_mapDll.find(pLoadOldDll) == m_mapDll.end())
		return -1;
	if (m_mapDll.find(pLoadNewDll) == m_mapDll.end())
		return -2;
	return m_mapDll[pLoadOldDll]->ReplaceDll(*m_mapDll[pLoadNewDll]) ? 0 : -4;
}
bool CKernelLoadDllMgr::IsDllLoaded(const char* pDll)
{
	if (pDll == nullptr)
	{
		return false;
	}
	if (m_mapDll.find(pDll) == m_mapDll.end())
		return false;
	return true;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
CCoroutineCtxTemplate::CCoroutineCtxTemplate()
{
}
CCoroutineCtxTemplate::~CCoroutineCtxTemplate()
{

}

void CCoroutineCtxTemplate::SetCreateCtx(KernelCtxCreate pCreate)
{
	m_create = pCreate;
}
void CCoroutineCtxTemplate::SetReleaseCtx(KernelCtxRelease pRelease)
{
	m_release = pRelease;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
CDllRegisterCtxTemplateMgr::CDllRegisterCtxTemplateMgr()
{

}

CDllRegisterCtxTemplateMgr::~CDllRegisterCtxTemplateMgr()
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	m_queue.Drop_Queue([](CCoroutineCtxTemplateMessage* pTemplateMsg)->void{
		pTemplateMsg->Release();
	});
}

bool CDllRegisterCtxTemplateMgr::Register(CCoroutineCtxTemplateCreateFunc pCreate, CCoroutineCtxTemplateReleaseFunc pRelease)
{
	if (nullptr == pCreate || nullptr == pRelease)
	{
		CCFrameSCBasicLogEventError("ע��������ʧ�ܴ������ͷź����ǿ�");
#ifdef _DEBUG
		stacktrace::call_stack stack(0);
        CCFrameSCBasicLogEventErrorV("%s", stack.to_string());
#endif
		return false;
	}
	CCoroutineCtxTemplate* pTemplate = pCreate();
	if (pTemplate == nullptr){
		CCFrameSCBasicLogEventError("ע�������Ĵ���ģ��ʧ��");
#ifdef _DEBUG
		stacktrace::call_stack stack(0);
        CCFrameSCBasicLogEventErrorV("%s", stack.to_string());
#endif
		return false;
	}
	if (!pTemplate->IsValid()){
		CCFrameSCBasicLogEventError("ע��������ģ�崴��������쳣");
#ifdef _DEBUG
		stacktrace::call_stack stack(0);
        CCFrameSCBasicLogEventErrorV("%s", stack.to_string());
#endif
		return false;
	}
	CCoroutineCtxTemplateMessage msg(pTemplate, pRelease);

	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	m_queue.MQPush(&msg);
	m_mgrTemplate[pTemplate->GetTemplateName()] = (CCoroutineCtxTemplate*)(pTemplate);
	lock.UnLock();
	return true;
}

CCoroutineCtxTemplate* CDllRegisterCtxTemplateMgr::GetCtxTemplate(const char* pName)
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	MapTemplateMgr::iterator iter = m_mgrTemplate.find(pName);
	if (iter != m_mgrTemplate.end())
	{
		return iter->second;
	}
	return nullptr;
}

void CCoroutineCtxTemplateMessage::Release(){
	if (m_pTemplate && m_pReleaseFunc){
		m_pReleaseFunc(m_pTemplate);
	}
	else{
		CCFrameSCBasicLogEventErrorV("�ͷ�������ģ��������(%s)", m_pTemplate->GetTemplateName().c_str());
	}
}
///////////////////////////////////////////////////////////////////////////////////////////
void ReleaseTemplate(CCoroutineCtxTemplate* pTemplate){
	delete pTemplate;
}





