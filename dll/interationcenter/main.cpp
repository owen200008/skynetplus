#include "../kernel/ccframe/initbussinesshead.h"
#include "dll/kernel/kernel_server.h"
#include "interationcenter.h"
#include "interationcenterserversessionctx.h"

#define DLL_VERSION			"1.0.0"

class _SKYNET_BUSSINESS_DLL_API CMainInheritGlobalParam : public CInheritGlobalParam
{
public:
    CMainInheritGlobalParam() : CInheritGlobalParam(DLL_VERSION){
    }
    virtual ~CMainInheritGlobalParam(){
    }

    //ʵ�ּ̳еĺ���,���Ƕ��̲߳���������
    virtual bool InheritParamTo(CInheritGlobalParam* pNew){
        return true;
    }
};
//ȫ�ֱ���
CMainInheritGlobalParam m_globalParam;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" _SKYNET_BUSSINESS_DLL_API bool InitForKernel(CKernelLoadDll* pDll, bool bReplace, CInheritGlobalParam*& pInitParam)
{
    //��ֵȫ�ֲ���
    pInitParam = &m_globalParam;

    //ע��ģ��,Ĭ������²���Ҫ�滻ģ��
    if (!bReplace){
        CDllRegisterCtxTemplateMgr& ctxInterface = CCtx_ThreadPool::GetThreadPool()->GetCtxTemplateRegister();
        ctxInterface.Register(CInterationCenterCtx::CreateTemplate, ReleaseTemplate);
        ctxInterface.Register(CInterationCenterServerSessionCtx::CreateTemplate, ReleaseTemplate);
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
