#include "kerneltolua.h"
#include <basic.h>
#include "ccframe/log/ctx_log.h"
#include "ccframe/net/ccframeserver_frame.h"
#include "kernel_server.h"
#include "../scbasic/sproto/lsproto.h"
#include "../scbasic/sproto/sproto.h"
#include "../scbasic/lua/sol2.h"

static int lLogger(lua_State *L){
	size_t sz;
	const char* pLog = lua_tolstring(L, -1, &sz);
    if (CCoroutineCtx_Log::IsExist())
        CCFrameSCBasicLogEventV(nullptr, "lua(%s)", pLog);
    else
        basiclib::BasicLogEventV(basiclib::DebugLevel_Info, "lua(%s)", pLog);
	return 0;
}
static int lError(lua_State *L){
	size_t sz;
	const char* pLog = lua_tolstring(L, -1, &sz);
    if (CCoroutineCtx_Log::IsExist())
        CCFrameSCBasicLogEventErrorV(nullptr, "lua(%s)", pLog);
    else
        basiclib::BasicLogEventErrorV("lua(%s)", pLog);
	return 0;
}
static int lModulePath(lua_State *L){
	lua_pushstring(L, basiclib::BasicGetModulePath().c_str());
	return 1;
}
static int lGetConfigString(lua_State *L){
    size_t sz;
    const char* pString = lua_tolstring(L, -1, &sz);
    const char* pDefaultString = lua_tolstring(L, -2, &sz);
    lua_pushstring(L, CKernelServer::GetKernelServer()->GetEnvString(pString, pDefaultString));
    return 1;
}
static int lGetConfigInt(lua_State *L){
    size_t sz;
    const char* pString = lua_tolstring(L, -1, &sz);
    int nDefault = lua_tointeger(L, -2);
    lua_pushnumber(L, CKernelServer::GetKernelServer()->GetEnv(pString, nDefault));
    return 1;
}

int luaopen_kernel(lua_State *L)
{
	luaL_Reg l[] = {
		{ "log", lLogger },
		{ "err", lError },
		{ "modulepath", lModulePath },
        { "GetConfigString", lGetConfigString },
        { "GetConfigInt", lGetConfigInt },
        { "CreateMoniClient", [](lua_State *L)->int{
            lua_pushlightuserdata(L, new CMoniFrameServerSession());
            return 1;
        }},
        { "DeleteMoniClient", [](lua_State *L)->int{
            delete (CMoniFrameServerSession*)lua_touserdata(L, -1);
            return 0;
        } },
        { "ClearMoniClient", [](lua_State *L)->int{
            CMoniFrameServerSession* pSession = (CMoniFrameServerSession*)lua_touserdata(L, -1);
            pSession->ClearSendBuffer();
            return 0;
        } },
        { "ParseMoniData", [](lua_State *L)->int{
            CMoniFrameServerSession* pSession = (CMoniFrameServerSession*)lua_touserdata(L, -1);
            struct sproto_type* st = (struct sproto_type*)lua_touserdata(L, -2);
            SprotoDecodeFunc(L, pSession->GetSendBuffer(), st);
            return 0;
        } },
        { "DebugSocketFunc", [](lua_State *L)->int{
            bool bSuccess = false;
            const char* pName = lua_tostring(L, -1);
            CMoniFrameServerSession* pSession = (CMoniFrameServerSession*)lua_touserdata(L, -2);
            basiclib::CBasicSmartBuffer* pSMBuf = (basiclib::CBasicSmartBuffer*)lua_touserdata(L, -3);
            CCFrameServer_Frame* pFrame = CCtx_ThreadPool::GetThreadPool()->GetRegisterServer(pName);
            if (pFrame){
                bSuccess = true;
                CCFrameServer* pServer = pFrame->GetServer();
                pServer->DebugReceiveData(pSession, pSMBuf->GetDataLength(), pSMBuf->GetDataBuffer());
            }
            lua_pushboolean(L, bSuccess);
            return 1;
        } },
        { "DebugCtxFunc", [](lua_State *L)->int{
            return 0;
        } },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}