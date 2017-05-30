/***********************************************************************************************************
֧�ֵ�����, ��֧�ּ̳нṹ��ֻ֧�����
1�ֽڣ�Net_Char Net_UChar 2�ֽڣ�Net_Short Net_UShort 4�ֽڣ�Net_Int Net_UInt 8�ֽڣ�Net_LONGLONG 8�ֽڣ�Net_Double �ַ�����Net_CBasicString
Net_Vector
Net_Map map��key�����ǻ������ͣ���֧��string
��֧������
//����
#pragma pack(4)
struct TestHead
{
Net_UInt		m_nMethod;
Net_UInt		m_nSession;
};
typedef Net_Vector<Net_UInt>::ContainForNet				VTVector;
typedef Net_Map<Net_LONGLONG, TestHead>::ContainForNet	MapMap;

struct Login
{
TestHead				m_head;
VTVector				m_vt;
MapMap					m_map;
Net_Char				m_cChar;
Net_UChar				m_ucChar;
Net_CBasicString		m_strName;
};
#pragma pack()
************************************************************************************************************/
#ifndef SKYNETPLUS_SPROTO_PROTOCAL_H
#define SKYNETPLUS_SPROTO_PROTOCAL_H

#include <basic.h>
#include "skynetplusprotocal_define.h"

//! ����ͷ
struct SProtoHead{
    Net_UInt		m_nMethod;			//0x0000000
    Net_UInt		m_nSession;
};

////////////////////////////////////////////////////////////////////
//! ϵͳ��
//! skynet������֮�������
struct SkynetPlusPing{
    SProtoHead              m_head;
    SkynetPlusPing(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_Ping;
        m_head.m_nSession = 0;
    }
};

struct SkynetPlusRegister{
    SProtoHead          m_head;
    Net_UChar           m_cIndex;
    Net_CBasicString    m_strKeyName;//���index�Ѿ����ڣ���Ҫ��key���ж��Ƿ��������滻�����key��ͬ���滻��key����ͬ�ʹ������ó���
    SkynetPlusRegister(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_Register;
        m_head.m_nSession = 0;
        m_cIndex = 0;
    }
};
struct SkynetPlusRegisterResponse{
    SProtoHead      m_head;
    Net_UInt        m_nError;
    SkynetPlusRegisterResponse(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_Register | SPROTO_METHOD_TYPE_RESPONSE;
        m_head.m_nSession = 0;
        m_nError = 0;
    }
};

typedef Net_Map<Net_CBasicString, Net_UInt> MapNameToCtxID;
//! create name to ctxid
struct SkynetPlusCreateNameToCtxID{
    SProtoHead      m_head;
    MapNameToCtxID  m_mapNameToCtxID;
    SkynetPlusCreateNameToCtxID(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_CreateNameToCtxID;
        m_head.m_nSession = 0;
    }
};
struct SkynetPlusCreateNameToCtxIDResponse{
    SProtoHead      m_head;
    Net_UInt        m_nError;
    MapNameToCtxID  m_mapNameToCtxID;
    SkynetPlusCreateNameToCtxIDResponse(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_CreateNameToCtxID | SPROTO_METHOD_TYPE_RESPONSE;
        m_head.m_nSession = 0;
        m_nError = 0;
    }
};
//! delete name to ctxid
struct SkynetPlusDeleteNameToCtxID{
    SProtoHead              m_head;
    Net_CBasicString        m_strName;
    SkynetPlusDeleteNameToCtxID(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_DeleteNameToCtxID;
        m_head.m_nSession = 0;
    }
    
};
struct SkynetPlusDeleteNameToCtxIDResponse{
    SProtoHead              m_head;
    Net_UInt                m_nError;
    Net_CBasicString        m_strName;
    SkynetPlusDeleteNameToCtxIDResponse(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_DeleteNameToCtxID | SPROTO_METHOD_TYPE_RESPONSE;
        m_head.m_nSession = 0;
        m_nError = 0;
    }
};
//! get
struct SkynetPlusGetCtxIDByName{
    SProtoHead              m_head;
    Net_CBasicString        m_strName;
    SkynetPlusGetCtxIDByName(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_GetCtxIDByName;
        m_head.m_nSession = 0;
    }
};
struct SkynetPlusGetCtxIDByNameResponse{
    SProtoHead              m_head;
    Net_UInt                m_nError;
    Net_UInt                m_nCtxID;
    SkynetPlusGetCtxIDByNameResponse(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNET_GetCtxIDByName | SPROTO_METHOD_TYPE_RESPONSE;
        m_head.m_nSession = 0;
        m_nError = 0;
        m_nCtxID = 0;
    }
};

struct ServerRequestDstData{
    Net_UInt                m_nDstCtxID;
    Net_UInt                m_nType;
    ServerRequestDstData(){
        m_nDstCtxID = 0;
        m_nType = 0;
    }
};

//! ����
struct SkynetPlusServerRequest{
    SProtoHead              m_head;
    ServerRequestDstData    m_request;//������ΪֹΪֹ���ܱ䶯��serverֻ����������
    Net_UChar               m_cParamCount;
    Net_CBasicBitstream     m_ins;
    SkynetPlusServerRequest(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNETCHILD_Request;
        m_head.m_nSession = 0;
        m_cParamCount = 0;
    }
};
struct SkynetPlusServerRequestResponse{
    SProtoHead              m_head;
    Net_UInt                m_nError;
    Net_CBasicBitstream*    m_pIns;//������SkynetPlusServerRequestTransferResponse���л�
    SkynetPlusServerRequestResponse(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNETCHILD_Request | SPROTO_METHOD_TYPE_RESPONSE;
        m_head.m_nSession = 0;
        m_nError = 0;
    }
};

struct SkynetPlusServerRequestTransfer{
    SProtoHead              m_head;
    Net_UInt                m_nCtxID;
    Net_CBasicBitstream*    m_pIns;//������SkynetPlusServerRequest���л�
    SkynetPlusServerRequestTransfer(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNETCHILD_ServerRequest;
        m_head.m_nSession = 0;
        m_nCtxID = 0;
        m_pIns = nullptr;
    }
};
struct SkynetPlusServerRequestTransferResponse{
    SProtoHead              m_head;
    Net_UInt                m_nCtxID;
    Net_UInt                m_nError;
    Net_CBasicBitstream     m_ins;//���reply
    SkynetPlusServerRequestTransferResponse(){
        m_head.m_nMethod = SPROTO_METHOD_SKYNETCHILD_ServerRequest | SPROTO_METHOD_TYPE_RESPONSE;
        m_head.m_nSession = 0;
        m_nCtxID = 0;
        m_nError = 0;
    }
};
////////////////////////////////////////////////////////////////////
#endif