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

    //! ��λ����ͷ
    long IsPacketFull(long lMaxPacketSize);

    //! ��ȡ����
    Net_UInt GetTotalPacketLength(){ return m_nPacketLength + 4; }

    //! ��һ��
    bool ResetHeader();
protected:
    Net_UInt	m_nPacketLength;
};
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//�������Ļ������С
#define CCFRAMENETPROTOCAL_PACKET_SIZE_CLIENT		0xFFFFFF		//������ó�16M������

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)
class _SKYNET_KERNEL_DLL_API CCCFrameDefaultFilter : public basiclib::CBasicPreSend
{
public:
    CCCFrameDefaultFilter(DWORD dwMaxLength = CCFRAMENETPROTOCAL_PACKET_SIZE_CLIENT);
    virtual ~CCCFrameDefaultFilter();

    /*\brief �����յ������� */
    // �������ɵ����ݷ���buf
	virtual int32_t OnPreReceive(const char *pszData, int32_t cbData, basiclib::CBasicBitstream& buf, basiclib::CBasicSessionNetNotify* pNetSession);

    /*\brief ���˷��͵����� */
    // �������ɵ����ݷ���buf
	virtual int32_t OnPreSend(const char *pszData, int32_t cbData, uint32_t dwFlag, basiclib::SendBufferCacheMgr& sendBuf);

    /*\brief �����µ�ʵ�� */
    // ����Accept
    virtual CBasicPreSend* Construct();

    /*\brief ���ù�����״̬ */
    virtual void ResetPreSend();
protected:
    CParseNetClientPacketBuffer     m_data;
    //�����������ݳ���
    DWORD				            m_dwLength;
};
#pragma warning (pop)

#endif