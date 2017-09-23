#ifndef SKYNETPLUS_CCFRAMEDEFAULTFILTER_H
#define SKYNETPLUS_CCFRAMEDEFAULTFILTER_H

#include <basic.h>
#include "../../kernel_head.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define	NETCLIENT_LOCATION_HEADER_FIND		    0
#define NETCLIENT_LOCATION_HEADER_NOENOUGH	    1
#define NETCLIENT_LOCATION_HEADER_NOTALLOWED    2
class _SKYNET_KERNEL_DLL_API CParseNetClientPacketBuffer : public basiclib::CBasicSmartBuffer
{
public:
    CParseNetClientPacketBuffer();
    virtual ~CParseNetClientPacketBuffer();

    //! 定位数据头
    long IsPacketFull(long lMaxPacketSize);

    //! 获取包长
    Net_UInt GetTotalPacketLength(){ return m_nPacketLength + 4; }

    //! 下一个
    bool ResetHeader();
protected:
    Net_UInt	m_nPacketLength;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//设置最大的缓存包大小
#define CCFRAMENETPROTOCAL_PACKET_SIZE_CLIENT		0xFFFFFF		//最大设置成16M的数据

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)
class _SKYNET_KERNEL_DLL_API CCCFrameDefaultFilter : public basiclib::CBasicPreSend
{
public:
    CCCFrameDefaultFilter(DWORD dwMaxLength = CCFRAMENETPROTOCAL_PACKET_SIZE_CLIENT);
    virtual ~CCCFrameDefaultFilter();

    /*\brief 过滤收到的数据 */
    // 过滤生成的数据放入buf
	virtual int32_t OnPreReceive(const char *pszData, int32_t cbData, basiclib::CBasicBitstream& buf, basiclib::CBasicSessionNetNotify* pNetSession);

    /*\brief 过滤发送的数据 */
    // 过滤生成的数据放入buf
	virtual int32_t OnPreSend(const char *pszData, int32_t cbData, uint32_t dwFlag, basiclib::SendBufferCacheMgr& sendBuf);

    /*\brief 构造新的实例 */
    // 用于Accept
    virtual CBasicPreSend* Construct();

    /*\brief 重置过滤器状态 */
    virtual void ResetPreSend();
protected:
    CParseNetClientPacketBuffer     m_data;
    //最大允许的数据长度
    DWORD				            m_dwLength;
};
#pragma warning (pop)

#endif