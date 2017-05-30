#ifndef SKYNETPLUS_INITBUSSINESSHEAD_H
#define SKYNETPLUS_INITBUSSINESSHEAD_H

#include "dllmodule.h"

#ifdef __BASICWINDOWS
#define _SKYNET_BUSSINESS_DLL_API 	__declspec(dllexport)
#endif

extern "C" _SKYNET_BUSSINESS_DLL_API bool InitForKernel(CKernelLoadDll* pDll, bool bReplace, CInheritGlobalParam*& pInitParam);

#endif