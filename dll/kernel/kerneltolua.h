#ifndef SKYNETPLUS_KERNEL_TOLUA_H
#define SKYNETPLUS_KERNEL_TOLUA_H

#include "kernel_head.h"
extern "C"{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

extern "C"  int _SKYNET_KERNEL_DLL_API luaopen_kernel(lua_State *L);

#endif