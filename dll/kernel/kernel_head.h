#ifndef SKYNETKERNEL_HEAD_H
#define SKYNETKERNEL_HEAD_H

#ifdef __BASICWINDOWS
#ifdef kernel_EXPORTS
#define _SKYNET_KERNEL_DLL_API 	__declspec(dllexport)
#else
#define _SKYNET_KERNEL_DLL_API 	__declspec(dllimport)
#endif
#endif

#ifndef _SKYNET_KERNEL_DLL_API
#define _SKYNET_KERNEL_DLL_API
#endif

#define MUSTBE_EXTERNC		extern "C"

#define GlobalGetClassName(s) #s

//≈–∂œ «∑Ò“Ï≥£break
#define IsErrorHapper(exp, afterexp)\
    if(!(exp)){\
        afterexp;\
    }
#define IsErrorHapper2(exp, afterexp1, afterexp2)\
    if(!(exp)){\
        afterexp1;\
        afterexp2;\
        }
#define IsHttpErrorHapper(exp, doc, value, str, afterexp)\
    if(!(exp)){\
        Http_Json_Number_Set(doc, Http_Json_KeyDefine_RetNum, value);\
        Http_Json_String_Set(doc, Http_Json_KeyDefine_String, str);\
        afterexp;\
    }

#endif