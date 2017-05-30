#ifndef SKYNETPLUS_CCFRAMEDEFAULTFILTER_H
#define SKYNETPLUS_CCFRAMEDEFAULTFILTER_H

#include <basic.h>
#include "../scbasic/net/net.h"
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

class _SKYNET_KERNEL_DLL_API CCCFrameDefaultFilter : public basiclib::CBasicPreSend
{
public:
    CCCFrameDefaultFilter(DWORD dwMaxLength = CCFRAMENETPROTOCAL_PACKET_SIZE_CLIENT);
    virtual ~CCCFrameDefaultFilter();

    /*\brief �����յ������� */
    // �������ɵ����ݷ���buf
    virtual Net_Int OnPreReceive(const char *pszData, Net_Int cbData, basiclib::CBasicBitstream& buf, basiclib::CBasicSessionNetClient* pNetSession);

    /*\brief ���˷��͵����� */
    // �������ɵ����ݷ���buf
    virtual Net_Int OnPreSend(const char *pszData, Net_Int cbData, Net_UInt dwFlag, basiclib::SendDataToSendThread& buf);

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

#endif