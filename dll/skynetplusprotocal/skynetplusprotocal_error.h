#ifndef SKYNETPLUS_SPROTO_PROTOCAL_ERROR_H
#define SKYNETPLUS_SPROTO_PROTOCAL_ERROR_H

//!����Ķ���ϵͳ������0xF0000000-0xFFFFFFFF
#define SPROTO_ERRORID_SKYNETPLUS_Global_ParseError         0xF0000000          //���ݰ���������

#define SPROTO_ERRORID_SKYNETPLUS_REGISTER_METHODIDERR      0xF0001001          //ע���������
#define SPROTO_ERRORID_SKYNETPLUS_REGISTER_KEYEXIST         0xF0001002          //key�Ѿ�����
#define SPROTO_ERRORID_SKYNETPLUS_REGISTER_CREATECTXERR     0xF0001003          //����ctxʧ��
#define SPROTO_ERRORID_SKYNETPLUS_REGISTER_ERRORREGISTERKEY 0xF0001004          //������ע���key

#define SPROTO_ERRORID_SKYNETPLUS_GetNameToCtx_NoName       0xF0004001          //��ȡ�������ֲ�����

#define SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_RoadErr  0xF0005001          //���˴���ķ�����
#define SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_NoSeria  0xF0005002          //ûע�����л�����
#define SPROTO_ERRORID_SKYNETPLUS_ServerRequestCtx_SeriaErr 0xF0005003          //���л�ʧ��

#define SPROTO_ERRORID_SKYNETPLUS_RequestCtx_NoServer       0xF1000001          //�Ҳ���������
#define SPROTO_ERRORID_SKYNETPLUS_RequestCtx_ServerYieldErr 0xF1000002          //����ʧ��
#define SPROTO_ERRORID_SKYNETPLUS_RequestCtx_ResponseErr    0xF1000003          //���󷵻ش���

#endif