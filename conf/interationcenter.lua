--ָ���Լ���Զ��id
--serverid=1
--servername="interationcenterserver1"

--��־�ļ�Ŀ¼
logpath="$GetModulePath/log/"

--�����������߳�
NetThreadCount=2

--���������߳���
workthread=2

--http������ַ
httpaddress="0.0.0.0:19080"

--���صĶ�̬��
dlllist="kernel.dll;interationcenter.dll"
--�滻�Ķ�̬��
--replacedlllist="skynettest.dll>skynettest_V2.dll"
--Ĭ����������ģ����
maintemplate="CInterationCenterCtx"

--����
server_serversessionctx="CInterationCenterServerSessionCtx"
server_Address="0.0.0.0:6666"
server_serverlistentimeout=45
server_serverlisteniptrust="*"
--
requesttimeout=5

