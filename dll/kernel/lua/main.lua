local sprotoparser = require "sprotoparser"
local kernelskyentplus = require "kernel"
local sproto = require "sproto"

function string.split(input, delimiter)
    input = tostring(input)
    delimiter = tostring(delimiter)
    if (delimiter=='') then return false end
    local pos,arr = 0, {}
    -- for each divider found
    for st,sp in function() return string.find(input, delimiter, pos, true) end do
        table.insert(arr, string.sub(input, pos, st - 1))
        pos = sp + 1
    end
    table.insert(arr, string.sub(input, pos))
    return arr
end

if sprotofilename then
    kernelskyentplus.log(sprotofilename)
	local ayGroupIDs = string.split(sprotofilename, "&")
	local parseData = ""
	local lastName = ""
	for _, value in ipairs(ayGroupIDs) do
		local opensprotocalfilename = value
        lastName = opensprotocalfilename
		local f = assert(io.open(opensprotocalfilename))
		local sprotocode = assert(f:read "*a")
		parseData = parseData..sprotocode
		f:close()
		--保证统一
		sprotoparser.parsetocpp(parseData, lastName, nil)
	end
    if sprotostoreluafile then
        sprotostoreluafile = kernelskyentplus.modulepath()..sprotostoreluafile
    end
    kernelskyentplus.log(lastName)
    sprotoparser.parsetocpp(parseData, lastName, sprotostoreluafile)
end
