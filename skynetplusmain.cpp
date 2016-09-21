#include <stdlib.h>
#include <basic.h>
#include "skynetplus/skynetplus_servermodule.h"
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <lstring.h>
}

int sigign()
{
	struct sigaction sa;
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, 0);
	return 0;
}

static void _init_env(lua_State *L) 
{
	lua_pushnil(L);  /* first key */
	while (lua_next(L, -2) != 0) {
		int keyt = lua_type(L, -2);
		if (keyt != LUA_TSTRING) {
			basiclib::BasicLogEventError("Invalid config table");
			exit(1);
		}
		const char * key = lua_tostring(L,-2);
		if (lua_type(L,-1) == LUA_TBOOLEAN) {
			int b = lua_toboolean(L,-1);
			CSkynetPlus_ServerModule::GetServerModule().SetEnv(key, b ? "true" : "false");
		} else {
			const char * value = lua_tostring(L,-1);
			if (value == NULL) {
				basiclib::BasicLogEventErrorV("Invalid config table key = %s", key);
				exit(1);
			}
			CSkynetPlus_ServerModule::GetServerModule().SetEnv(key, value);
		}
		lua_pop(L,1);
	}
	lua_pop(L,1);
}

static const char * load_config = "\
	local config_name = ...\
	local f = assert(io.open(config_name))\
	local code = assert(f:read \'*a\')\
	local function getenv(name) return assert(os.getenv(name), \'os.getenv() failed: \' .. name) end\
	code = string.gsub(code, \'%$([%w_%d]+)\', getenv)\
	f:close()\
	local result = {}\
	assert(load(code,\'=(load)\',\'t\',result))()\
	return result\
";

int main(int argc, char* argv[])
{
	if(argc <= 1)
	{
		printf("ParamError, Need Config\n");
		return 1;
	}
	const char * config_file = argv[1] ;
	//default log
	basiclib::CBasicString strDefaultLogFileName = basiclib::BasicGetModulePath() + "/log/" + config_file + ".log";
	basiclib::CBasicString strDefaultErrorFileName = basiclib::BasicGetModulePath() + "/log/" + config_file + ".error";

	basiclib::BasicSetDefaultLogEventMode(LOG_ADD_TIME|LOG_ADD_THREAD, strDefaultLogFileName.c_str());
	basiclib::BasicSetDefaultLogEventErrorMode(LOG_ADD_TIME|LOG_ADD_THREAD, strDefaultErrorFileName.c_str());

	luaS_initshr();
	CSkynetPlus_ServerModule& serverModule = CSkynetPlus_ServerModule::GetServerModule();
	serverModule.Init();
	
	sigign();

	lua_State *L = luaL_newstate();
	luaL_openlibs(L);   // link lua lib

	int err = luaL_loadstring(L, load_config);
	err = lua_pcall(L, 1, 1, 0);
	if (err) 
	{
		basiclib::BasicLogEventErrorV("%s", lua_tostring(L,-1));
		lua_close(L);
		return 1;
	}
	_init_env(L);
	lua_close(L);
	
	serverModule.Start(config_file);
	return 0;
}


