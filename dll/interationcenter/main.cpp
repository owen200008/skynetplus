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

    //实现继承的函数,考虑多线程操作的问题
    virtual bool InheritParamTo(CInheritGlobalParam* pNew){
        return true;
    }
};
//全局变量
CMainInheritGlobalParam m_globalParam;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
extern "C" _SKYNET_BUSSINESS_DLL_API bool InitForKernel(CKernelLoadDll* pDll, bool bReplace, CInheritGlobalParam*& pInitParam)
{
    //赋值全局参数
    pInitParam = &m_globalParam;

    //注册模板,默认情况下不需要替换模板
    if (!bReplace){
        CDllRegisterCtxTemplateMgr& ctxInterface = CCtx_ThreadPool::GetThreadPool()->GetCtxTemplateRegister();
        ctxInterface.Register(CInterationCenterCtx::CreateTemplate, ReleaseTemplate);
        ctxInterface.Register(CInterationCenterServerSessionCtx::CreateTemplate, ReleaseTemplate);
    }
    return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
