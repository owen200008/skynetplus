#ifndef SKYNETPLUS_SPROTOPROTOCAL_FUNCTION_H
#define SKYNETPLUS_SPROTOPROTOCAL_FUNCTION_H

#include "skynetplusprotocal_auto.h"

template<class SprotoStruct>
void AddToInsWithSprotoStruct(SprotoStruct& data, basiclib::CBasicBitstream& smThreadSafeBuffer){
    smThreadSafeBuffer.SetDataLength(0);
    smThreadSafeBuffer << data;
}
template<class SendSession, class SprotoStruct>
void SendWithSprotoStruct(SendSession& pSession, SprotoStruct& data, basiclib::CBasicBitstream& smThreadSafeBuffer){
    smThreadSafeBuffer.SetDataLength(0);
    smThreadSafeBuffer << data;
    pSession->Send(smThreadSafeBuffer);
}
template<class SprotoStruct>
bool InsToSprotoStruct(SprotoStruct& data, char* pData, int nLength){
    basiclib::CBasicBitstream os;
    os.BindOutData(pData, nLength);
    os >> data;
    if (os.IsReadError()){
        os.ResetReadError();
        return false;
    }
    return true;
}

#endif
