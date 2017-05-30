local configfilename, modulepath = ...
kernel_modulepath = modulepath
local config_name = configfilename
local f = assert(io.open(config_name))
local code = assert(f:read "*a")
local function getenv(name) 
    if name == "GetModulePath" then
        return kernel_modulepath
    end
    return assert(os.getenv(name), "os.getenv() failed:"..name) 
end
code = string.gsub(code, "%$([%w_%d]+)", getenv)
f:close()
local result = {}
assert(load(code,"=(load)","t",result))()
return result