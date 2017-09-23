#ifndef SKYNETPLUS_INITBUSSINESSHEAD_H
#define SKYNETPLUS_INITBUSSINESSHEAD_H

#include "dllmodule.h"

#ifdef __BASICWINDOWS
#define _SKYNET_BUSSINESS_DLL_API 	__declspec(dllexport)
#endif
#define KAGUYA_SUPPORT_MULTIPLE_SHARED_LIBRARY 1

extern "C" _SKYNET_BUSSINESS_DLL_API bool InitForKernel(CKernelLoadDll* pDll, CInheritGlobalParam*& pInitParam);

#endif