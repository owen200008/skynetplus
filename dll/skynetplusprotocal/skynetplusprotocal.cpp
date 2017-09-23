#include "skynetplusprotocal.h"

PublicSprotoRequest::PublicSprotoRequest(Net_UInt nMethod, Net_UInt nSession) {
	m_head.m_nMethod = nMethod;
	m_head.m_nSession = nSession;
}

//根据请求构造返回
void PublicSprotoResponse::CreateWithRequest(SProtoHead& requestHead) {
	m_head = requestHead;
	m_head.m_nMethod |= SPROTO_METHOD_TYPE_RESPONSE;
}