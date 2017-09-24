#include <stdlib.h>
#include <basic.h>

void TestShowImportDll(){
	HMODULE hFromModule = GetModuleHandle(0);
	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)hFromModule;
	if (IsBadReadPtr(pDosHeader, sizeof(IMAGE_DOS_HEADER)))
		return;
	if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
		return;
	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((PBYTE)hFromModule + pDosHeader->e_lfanew);
	if (IsBadReadPtr(pNTHeader, sizeof(IMAGE_NT_HEADERS)))
		return;
	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
		return;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((PBYTE)hFromModule + pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);
	if (pImportDesc == (PIMAGE_IMPORT_DESCRIPTOR)pNTHeader)
		return;
	while (pImportDesc->OriginalFirstThunk != 0)
	{
		PSTR pszModName = (PSTR)((PBYTE)pDosHeader + pImportDesc->Name);
        basiclib::BasicLogEventV(basiclib::DebugLevel_Info, "LoadModName:%s", pszModName);
		PIMAGE_THUNK_DATA pOrigin = (PIMAGE_THUNK_DATA)((PBYTE)pDosHeader + pImportDesc->OriginalFirstThunk);
		PIMAGE_THUNK_DATA pThunk = (PIMAGE_THUNK_DATA)((PBYTE)pDosHeader + pImportDesc->FirstThunk);
		for (; pOrigin->u1.Ordinal != 0; pOrigin++, pThunk++){
			LPCSTR pFxName = NULL;
			if (pThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
				pFxName = (LPCSTR)IMAGE_ORDINAL(pThunk->u1.Ordinal);
			else
			{
				PIMAGE_IMPORT_BY_NAME piibn = (PIMAGE_IMPORT_BY_NAME)((PBYTE)pDosHeader + pOrigin->u1.AddressOfData);
				pFxName = (LPCSTR)piibn->Name;
			}
            basiclib::BasicLogEventV(basiclib::DebugLevel_Info, "LoadModName:%s Func:%s", pszModName, pFxName);
		}

		pImportDesc++;  // Advance to next imported module descriptor
	}
}

typedef void (*FuncStartKernel)(const char* pConfig);
int main(int argc, char* argv[])
{
	if(argc <= 1){
		printf("ParamError, Need Config\n");
		getchar();
		return 1;
	}
	basiclib::BasicSetExceptionMode();

	basiclib::CBasicLoadDll dllFile;
	if (dllFile.LoadLibrary("kernel.dll") == 0) {
		FuncStartKernel pFunc = (FuncStartKernel)dllFile.GetProcAddress("StartKernel");
		if (pFunc) {
			pFunc(argv[1]);
		}
		else {
			printf("No StartKernel func find!\r\n");
			getchar();
		}
	}
	else {
		printf("loadl kernel.dll fail!\r\n");
		getchar();
	}

	return 0;
}


