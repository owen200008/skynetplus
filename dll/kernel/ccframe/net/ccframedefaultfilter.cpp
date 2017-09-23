#include "ccframedefaultfilter.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CParseNetClientPacketBuffer::CParseNetClientPacketBuffer(){
    m_nPacketLength = 0;
}
CParseNetClientPacketBuffer::~CParseNetClientPacketBuffer(){

}

//定位数据头
long CParseNetClientPacketBuffer::IsPacketFull(long lMaxPacketSize){
    if (m_cbBuffer < 4)
        return NETCLIENT_LOCATION_HEADER_NOENOUGH;
    if (m_nPacketLength == 0)
        basiclib::UnSerializeUInt((unsigned char*)m_pszBuffer, m_nPacketLength);
    if (m_nPacketLength > lMaxPacketSize || m_nPacketLength == 0){
        return NETCLIENT_LOCATION_HEADER_NOTALLOWED;
    }
    if (m_cbBuffer >= 4 + m_nPacketLength){
        return NETCLIENT_LOCATION_HEADER_FIND;
    }
    return NETCLIENT_LOCATION_HEADER_NOENOUGH;
}

/*下一个头*/
bool CParseNetClientPacketBuffer::ResetHeader(){
    if (0 == m_nPacketLength){
        return false;
    }
    int nHeadSize = 4;

    char* pTail = m_pszBuffer + m_cbBuffer;
    char* pNext = m_pszBuffer + m_nPacketLength + nHeadSize;
    if (pNext == pTail){
        SetDataLength(0);
        m_nPacketLength = 0;
        return true;
    }
    if (pNext < pTail){
        m_cbBuffer = pTail - pNext;
        memmove(m_pszBuffer, pNext, m_cbBuffer);
        //
        char* pLeft = m_pszBuffer + m_cbBuffer;
        int nLeft = pTail - pLeft;
        memset(pLeft, 0, nLeft);
        //reset
        m_nPacketLength = 0;
        return true;
    }
    ASSERT(0);
    return false;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CCCFrameDefaultFilter::CCCFrameDefaultFilter(DWORD dwMaxLength){
    m_dwLength = dwMaxLength;
}

CCCFrameDefaultFilter::~CCCFrameDefaultFilter(){
}

Net_Int CCCFrameDefaultFilter::OnPreReceive(const char *pszData, Net_Int cbData, basiclib::CBasicBitstream& buf, basiclib::CBasicSessionNetNotify* pNetSession){
    if (0 == cbData || NULL == pszData){
    }
    else{
        m_data.AppendDataEx(pszData, cbData);
    }
    long lRet = NETCLIENT_LOCATION_HEADER_FIND;
    do{
        lRet = m_data.IsPacketFull(m_dwLength);
        switch (lRet)
        {
        case NETCLIENT_LOCATION_HEADER_NOTALLOWED:
            return PACK_FILTER_FAIL;
        case NETCLIENT_LOCATION_HEADER_FIND:{
            buf.AppendData(m_data.GetDataBuffer() + 4, m_data.GetTotalPacketLength() - 4);
            m_data.ResetHeader();
        }
        break;
        }
    } while (false);
    return lRet == NETCLIENT_LOCATION_HEADER_FIND ? PACK_FILTER_NEXT : PACK_FILTER_HANDLED;
}

Net_Int CCCFrameDefaultFilter::OnPreSend(const char *pszData, Net_Int cbData, Net_UInt dwFlag, basiclib::SendBufferCacheMgr& buf){
    uint32_t nLength = cbData + 4;
    char* pBuf = buf.ResetDataLength(nLength);
    char* pPoint = pBuf;
    pPoint += basiclib::SerializeUInt((unsigned char*)pPoint, cbData);
    memcpy(pPoint, pszData, cbData);
    return PACK_FILTER_SEARCH;
}

basiclib::CBasicPreSend* CCCFrameDefaultFilter::Construct(){
    CCCFrameDefaultFilter* pNewFilter = new CCCFrameDefaultFilter(m_dwLength);
    return (basiclib::CBasicPreSend*)pNewFilter;
}

void CCCFrameDefaultFilter::ResetPreSend(){
    m_data.Free();
}

