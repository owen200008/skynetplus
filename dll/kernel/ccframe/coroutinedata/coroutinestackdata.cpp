#include "coroutinestackdata.h"

Net_UInt GetDefaultStackMethod(CCorutineStackDataDefault* pDefaultStack) {
	Net_UInt nMethod = 0;
	int nStackDataLength = 0;
	const char* pStackData = pDefaultStack->GetData(nStackDataLength);
	if (nStackDataLength >= sizeof(Net_UInt))
		basiclib::UnSerializeUInt((unsigned char*)pStackData, nMethod);
	return nMethod;
}
