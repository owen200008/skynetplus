local requireModule, bReload = ...

if bReload then
	package.loaded[requireModule] = nil  
end
require(requireModule)
