#include "kerneltolua.h"
#include "../scbasic/lua/ccbasiclib_lua.h"
#include "ccframe/log/ctx_log.h"
#include "ccframe/net/ccframeserver_frame.h"
#include "kernel_server.h"
#include "../scbasic/sproto/lsproto.h"
#include "../scbasic/sproto/sproto.h"

static int lLogger(lua_State *L){
	size_t sz;
	const char* pLog = lua_tolstring(L, -1, &sz);
    if (CCoroutineCtx_Log::GetLogCtxID() != 0)
        CCFrameSCBasicLogEventV("lua(%s)", pLog);
    else
        basiclib::BasicLogEventV(basiclib::DebugLevel_Info, "lua(%s)", pLog);
	return 0;
}
static int lError(lua_State *L){
	size_t sz;
	const char* pLog = lua_tolstring(L, -1, &sz);
	if (CCoroutineCtx_Log::GetLogCtxID() != 0)
        CCFrameSCBasicLogEventErrorV("lua(%s)", pLog);
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
    const char* pString = lua_tolstring(L, 1, &sz);
    const char* pDefaultString = lua_tolstring(L, 2, &sz);
    lua_pushstring(L, CKernelServer::GetKernelServer()->GetEnvString(pString, pDefaultString));
    return 1;
}
static int lGetConfigInt(lua_State *L){
    size_t sz;
    const char* pString = lua_tolstring(L, 1, &sz);
    int nDefault = lua_tointeger(L, 2);
    lua_pushnumber(L, CKernelServer::GetKernelServer()->GetEnv(pString, nDefault));
    return 1;
}

int luaopen_kernel(lua_State *L)
{
	ExportBasiclibClassToLua(L);
	kaguya::State state(L);
	
	state["CMoniFrameServerSessionLua"].setClass(kaguya::UserdataMetatable<CMoniFrameServerSessionLua>()
		.setConstructors<CMoniFrameServerSessionLua()>()
		.addFunction("SendForLua", &CMoniFrameServerSessionLua::SendForLua)
		.addFunction("GetUniqueID", &CMoniFrameServerSessionLua::GetUniqueID)
		.addFunction("BindSendCallback", &CMoniFrameServerSessionLua::BindSendCallback)
		.addStaticFunction("SendBuffer", [](CMoniFrameServerSessionLua* pSession, std::string& strData) { {
				return pSession->SendData((void*)strData.c_str(), strData.length());
			}})
	);
	
	luaL_Reg l[] = {
		{ "log", lLogger },
		{ "err", lError },
		{ "modulepath", lModulePath },
        { "GetConfigString", lGetConfigString },
        { "GetConfigInt", lGetConfigInt },
        { "DebugInterfaceByName", [](lua_State *L)->int{
			bool bSuccess = false; 
			const char* pName = lua_tostring(L, 1);
			const char* pDebugName = lua_tostring(L, 2);
			uint32_t nType = lua_tointeger(L, 3);
			CCoroutineCtxHandle* pCtxHandle = CCoroutineCtxHandle::GetInstance();
			uint32_t nCtxID = pCtxHandle->GetCtxIDByName(pName, nullptr, 0);
			if (nCtxID != 0) {
				CRefCoroutineCtx pCtx = pCtxHandle->GetContextByHandleID(nCtxID);
				if (pCtx != nullptr) {
					bSuccess = pCtx->DebugInterface(pDebugName, nType, L);
				}
			}
			lua_pushboolean(L, bSuccess);
			return 1;
        } },
        { "DebugInterfaceByID", [](lua_State *L)->int{
			bool bSuccess = false;
			uint32_t nCtxID = lua_tointeger(L, 1);
			const char* pDebugName = lua_tostring(L, 2);
			uint32_t nType = lua_tointeger(L, 3);
			CCoroutineCtxHandle* pCtxHandle = CCoroutineCtxHandle::GetInstance();
			if (nCtxID != 0) {
				CRefCoroutineCtx pCtx = pCtxHandle->GetContextByHandleID(nCtxID);
				if (pCtx != nullptr) {
					bSuccess = pCtx->DebugInterface(pDebugName, nType, L);
				}
			}
			lua_pushboolean(L, bSuccess);
			return 1;
        } },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}