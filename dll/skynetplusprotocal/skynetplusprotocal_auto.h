#ifndef AUTO_SPROTOPROTOCAL_H
#define AUTO_SPROTOPROTOCAL_H

#include "skynetplusprotocal.h"

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const ServerRequestDstData& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, ServerRequestDstData& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusRegisterResponse& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusRegisterResponse& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusServerRequestTransfer& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusServerRequestTransfer& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusCreateNameToCtxID& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusCreateNameToCtxID& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusServerRequestTransferResponse& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusServerRequestTransferResponse& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusServerRequest& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusServerRequest& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusRegister& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusRegister& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusServerRequestResponse& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusServerRequestResponse& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusCreateNameToCtxIDResponse& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusCreateNameToCtxIDResponse& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusDeleteNameToCtxID& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusDeleteNameToCtxID& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusGetCtxIDByNameResponse& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusGetCtxIDByNameResponse& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusGetCtxIDByName& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusGetCtxIDByName& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusDeleteNameToCtxIDResponse& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusDeleteNameToCtxIDResponse& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SProtoHead& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SProtoHead& data);

basiclib::CBasicBitstream& operator<<(basiclib::CBasicBitstream& ins, const SkynetPlusPing& data);
basiclib::CBasicBitstream& operator>>(basiclib::CBasicBitstream& os, SkynetPlusPing& data);


#endif
