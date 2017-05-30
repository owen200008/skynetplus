#ifndef SKYNETPLUS_SPROTO_PROTOCAL_DEFINE_H
#define SKYNETPLUS_SPROTO_PROTOCAL_DEFINE_H

/////////////////////////////////////////////////////////////////////////////////////////
/*method ����*/
//���Ͷ���
#define SPROTO_METHOD_TYPE_MASK			    0x80000000
#define SPROTO_METHOD_TYPE_RESPONSE		    0x80000000          //�����Ƿ񷵻ذ�
//��Ļ���
#define SPROTO_METHOD_FIELD_MASK		    0x7F000000
//ϵͳ����(0x70000000 - 0x7F000000)
#define SPROTO_METHOD_FIELD_SKYNETCHILD     0x7E000000          //������������
#define SPROTO_METHOD_FIELD_SKYNET          0x7F000000          //������ͨ����
//�û��Զ�����
#define SPROTO_USERDEFINE_FIELD_MASK        0x00FF0000          //�û������Զ�����
//��ϢID�Ķ���
#define SPROTO_METHOD_ID_MASK               0x0000FFFF          //��ϢID������

//!ȫ������Ϣ����

//!ϵͳ��SPROTO_METHOD_FIELD_SKYNETCHILD ��Ϣ����
#define SPROTO_METHOD_SKYNETCHILD_Request           (SPROTO_METHOD_FIELD_SKYNETCHILD | 0x00000000)      //��������
#define SPROTO_METHOD_SKYNETCHILD_ServerRequest     (SPROTO_METHOD_FIELD_SKYNETCHILD | 0x00000001)      //�������󷵻�

//!ϵͳ��SPROTO_METHOD_FIELD_SKYNET ��Ϣ����
#define SPROTO_METHOD_SKYNET_Ping                   (SPROTO_METHOD_FIELD_SKYNET | 0x00000000)
#define SPROTO_METHOD_SKYNET_Register               (SPROTO_METHOD_FIELD_SKYNET | 0x00000001)           //ע��
#define SPROTO_METHOD_SKYNET_CreateNameToCtxID      (SPROTO_METHOD_FIELD_SKYNET | 0x00000002)           //�������ֵ�id�Ķ�Ӧ
#define SPROTO_METHOD_SKYNET_DeleteNameToCtxID      (SPROTO_METHOD_FIELD_SKYNET | 0x00000003)           //ɾ�����ֵ�id�Ķ�Ӧ
#define SPROTO_METHOD_SKYNET_GetCtxIDByName         (SPROTO_METHOD_FIELD_SKYNET | 0x00000004)           //���Ҷ�Ӧ
/////////////////////////////////////////////////////////////////////////////////////////

#endif