#include "skynetplusprotocal_auto.h"

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const ServerRequestDstData& data){
    ins << data.m_nDstCtxID;
    ins << data.m_nType;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, ServerRequestDstData& data){
    os >> data.m_nDstCtxID;
    os >> data.m_nType;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusPing& data){
    ins << data.m_head;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusPing& data){
    os >> data.m_head;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusServerRequestResponse& data){
    ins << data.m_head;
    ins << data.m_nError;
    data.m_pIns ? ins << (Net_Char)1 << *data.m_pIns : ins << (Net_Char)0;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusServerRequestResponse& data){
    Net_Char cStar = 0;
    os >> data.m_head;
    os >> data.m_nError;
    os >> cStar;if(cStar){if(data.m_pIns)os >> *data.m_pIns;else{Net_CBasicBitstream tmp;os >> tmp;}}
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusRegisterResponse& data){
    ins << data.m_head;
    ins << data.m_nError;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusRegisterResponse& data){
    os >> data.m_head;
    os >> data.m_nError;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusCreateNameToCtxID& data){
    ins << data.m_head;
    ins << data.m_mapNameToCtxID;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusCreateNameToCtxID& data){
    os >> data.m_head;
    os >> data.m_mapNameToCtxID;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusDeleteNameToCtxID& data){
    ins << data.m_head;
    ins << data.m_strName;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusDeleteNameToCtxID& data){
    os >> data.m_head;
    os >> data.m_strName;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusGetCtxIDByNameResponse& data){
    ins << data.m_head;
    ins << data.m_nError;
    ins << data.m_nCtxID;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusGetCtxIDByNameResponse& data){
    os >> data.m_head;
    os >> data.m_nError;
    os >> data.m_nCtxID;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusServerRequest& data){
    ins << data.m_head;
    ins << data.m_request;
    ins << data.m_cParamCount;
    ins << data.m_ins;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusServerRequest& data){
    os >> data.m_head;
    os >> data.m_request;
    os >> data.m_cParamCount;
    os >> data.m_ins;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusGetCtxIDByName& data){
    ins << data.m_head;
    ins << data.m_strName;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusGetCtxIDByName& data){
    os >> data.m_head;
    os >> data.m_strName;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SProtoHead& data){
    ins << data.m_nMethod;
    ins << data.m_nSession;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SProtoHead& data){
    os >> data.m_nMethod;
    os >> data.m_nSession;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusCreateNameToCtxIDResponse& data){
    ins << data.m_head;
    ins << data.m_nError;
    ins << data.m_mapNameToCtxID;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusCreateNameToCtxIDResponse& data){
    os >> data.m_head;
    os >> data.m_nError;
    os >> data.m_mapNameToCtxID;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusServerRequestTransferResponse& data){
    ins << data.m_head;
    ins << data.m_nCtxID;
    ins << data.m_nError;
    ins << data.m_ins;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusServerRequestTransferResponse& data){
    os >> data.m_head;
    os >> data.m_nCtxID;
    os >> data.m_nError;
    os >> data.m_ins;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusServerRequestTransfer& data){
    ins << data.m_head;
    ins << data.m_nCtxID;
    data.m_pIns ? ins << (Net_Char)1 << *data.m_pIns : ins << (Net_Char)0;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusServerRequestTransfer& data){
    Net_Char cStar = 0;
    os >> data.m_head;
    os >> data.m_nCtxID;
    os >> cStar;if(cStar){if(data.m_pIns)os >> *data.m_pIns;else{Net_CBasicBitstream tmp;os >> tmp;}}
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const PublicSprotoResponse& data){
    ins << data.m_head;
    ins << data.m_nError;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, PublicSprotoResponse& data){
    os >> data.m_head;
    os >> data.m_nError;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusRegister& data){
    ins << data.m_head;
    ins << data.m_cIndex;
    ins << data.m_strKeyName;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusRegister& data){
    os >> data.m_head;
    os >> data.m_cIndex;
    os >> data.m_strKeyName;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const PublicSprotoRequest& data){
    ins << data.m_head;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, PublicSprotoRequest& data){
    os >> data.m_head;
    return os;
}

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusDeleteNameToCtxIDResponse& data){
    ins << data.m_head;
    ins << data.m_nError;
    ins << data.m_strName;
    return ins;
}
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusDeleteNameToCtxIDResponse& data){
    os >> data.m_head;
    os >> data.m_nError;
    os >> data.m_strName;
    return os;
}


