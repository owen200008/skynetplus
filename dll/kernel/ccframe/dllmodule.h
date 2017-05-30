#ifndef SCBASIC_MODULE_H
#define SCBASIC_MODULE_H

#include "coroutine_ctx.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)

class _SKYNET_KERNEL_DLL_API CInheritGlobalParam : public basiclib::CBasicObject
{
public:
	CInheritGlobalParam(const char* pVersion);
	virtual ~CInheritGlobalParam();

	//ʵ�ּ̳еĺ���,���Ƕ��̲߳���������
	virtual bool InheritParamTo(CInheritGlobalParam* pNew) = 0;
protected:
	basiclib::CBasicString m_strVersion;
};

class CKernelLoadDll;
typedef bool(*KernelLoadCreate)(CKernelLoadDll*, bool bReplace, CInheritGlobalParam*& pInitParam);
class CKernelLoadDll : public basiclib::CBasicObject
{
public:
	CKernelLoadDll();
	virtual ~CKernelLoadDll();

	bool LoadKernelDll(const char* pLoadDll, bool bReplace);

	bool ReplaceDll(CKernelLoadDll& dll);

	basiclib::CBasicLoadDll& GetDll(){ return m_dll; }
	CInheritGlobalParam* GetInheritGlobalParam(){ return m_pDllGlobalParam; }
protected:
	basiclib::CBasicLoadDll			m_dll; 
	CInheritGlobalParam*			m_pDllGlobalParam;
};

class _SKYNET_KERNEL_DLL_API CKernelLoadDllMgr : public basiclib::CBasicObject
{
public:
	CKernelLoadDllMgr(){}
	virtual ~CKernelLoadDllMgr(){}
	
	bool LoadAllDll(const char* pDlls);
	int ReplaceDllFunc(const char* pLoadOldDll, const char* pLoadNewDll);
	bool IsDllLoaded(const char* pDll);
	//replaceload 
	bool ReplaceLoadDll(const char* pDll);
protected:
	bool RegisterDll(const char* pLoadDll, bool bReplace = false);
protected:
	basiclib::SpinLock		m_lock;
	typedef basiclib::basic_map<basiclib::CBasicString, CKernelLoadDll*>	MapDllContain;
	MapDllContain			m_mapDll;
};

typedef CCoroutineCtx*(*KernelCtxCreate)(const char* pKeyName);
typedef void(*KernelCtxRelease)(CCoroutineCtx*&);
//ע��ʵ��
class _SKYNET_KERNEL_DLL_API CCoroutineCtxTemplate : public basiclib::CBasicObject
{
public:
	CCoroutineCtxTemplate();
	virtual ~CCoroutineCtxTemplate();

	//�ж��Ƿ�Ϸ�
	bool IsValid(){ return !m_strTemplateName.IsEmpty() && m_create != nullptr && m_release != nullptr; }

	const basiclib::CBasicString& GetTemplateName(){ return m_strTemplateName; }
	void SetTemplateName(const char* pName){ m_strTemplateName = pName; }

	KernelCtxCreate GetCreate(){ return m_create; }
	void SetCreateCtx(KernelCtxCreate pCreate);
	
	KernelCtxRelease GetRelease(){ return m_release; }
	void SetReleaseCtx(KernelCtxRelease pRelease);
protected:
	basiclib::CBasicString		m_strTemplateName;
	KernelCtxCreate				m_create;
	KernelCtxRelease			m_release;
};

typedef CCoroutineCtxTemplate*(*CCoroutineCtxTemplateCreateFunc)();
typedef void(*CCoroutineCtxTemplateReleaseFunc)(CCoroutineCtxTemplate*);
struct CCoroutineCtxTemplateMessage
{
	CCoroutineCtxTemplate*				m_pTemplate;
	CCoroutineCtxTemplateReleaseFunc	m_pReleaseFunc;
	CCoroutineCtxTemplateMessage(){
		m_pTemplate = nullptr;
		m_pReleaseFunc = nullptr;
	}
	CCoroutineCtxTemplateMessage(CCoroutineCtxTemplate* pTemplate, CCoroutineCtxTemplateReleaseFunc pRelease){
		m_pTemplate = pTemplate;
		m_pReleaseFunc = pRelease;
	}
	void Release();
};
class _SKYNET_KERNEL_DLL_API CDllRegisterCtxTemplateMgr : public basiclib::CBasicObject
{
public:
	CDllRegisterCtxTemplateMgr();
	virtual ~CDllRegisterCtxTemplateMgr();

	//ע������ľͲ���Ҫ�ⲿ�ͷ�
	bool Register(CCoroutineCtxTemplateCreateFunc pCreate, CCoroutineCtxTemplateReleaseFunc pRelease);
	CCoroutineCtxTemplate* GetCtxTemplate(const char* pName);
	//����Ҫ�ͷ�,ע��һ��
protected:
	basiclib::SpinLock				m_lock;
	typedef basiclib::basic_map<basiclib::CBasicString, CCoroutineCtxTemplate*>		MapTemplateMgr;
	MapTemplateMgr											m_mgrTemplate;
	basiclib::CMessageQueue<CCoroutineCtxTemplateMessage>	m_queue;
};
#pragma warning (pop)

#define CreateTemplateHeader(s) \
static CCoroutineCtxTemplate* CreateTemplate();\
static CCoroutineCtx* CreateClass(const char* pKeyName = nullptr);\
static void ReleaseClass(CCoroutineCtx*&)

#define CreateTemplateSrc(s) \
CCoroutineCtxTemplate* s::CreateTemplate(){\
	CCoroutineCtxTemplate* pRet = new CCoroutineCtxTemplate();\
	pRet->SetTemplateName(GlobalGetClassName(s));\
	pRet->SetCreateCtx(s::CreateClass);\
	pRet->SetReleaseCtx(s::ReleaseClass);\
	return pRet;\
}\
CCoroutineCtx* s::CreateClass(const char* pKeyName){\
    if(pKeyName)\
        return new s(pKeyName);\
	return new s();\
}\
void s::ReleaseClass(CCoroutineCtx*& pCtx){\
	delete pCtx;\
	pCtx = nullptr;\
}

void _SKYNET_KERNEL_DLL_API ReleaseTemplate(CCoroutineCtxTemplate* pTemplate);

#endif
