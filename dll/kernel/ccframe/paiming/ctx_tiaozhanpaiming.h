#ifndef SKYNETPLUS_COROUTINE_CTX_TIAOZHANPAIMING_H
#define SKYNETPLUS_COROUTINE_CTX_TIAOZHANPAIMING_H

#include "../dllmodule.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)
struct BasePaiMingData{
	Net_UInt		m_numKeyID;
	Net_UShort		m_nMingCi;			//0�����һ��
	BasePaiMingData(){
		m_numKeyID = 0;
		m_nMingCi = 0;
	}
};
//! ���ݿ��ṩ��ս���κ�ʱ���ͬʱ���tick
struct TiaoZhanPaiMingReadFromDB{
	Net_UShort			m_nTiaoZhanMingCi;
	time_t				m_tmUpdateTime;
	Net_UInt			m_nSameTimeTick;
	BasePaiMingData*	m_pBaseData;
	char*				m_pStrRecord;
	TiaoZhanPaiMingReadFromDB(){
		m_nTiaoZhanMingCi = 0;
		m_tmUpdateTime = 0;
		m_nSameTimeTick = 0;
		m_pBaseData = nullptr;
		m_pStrRecord = nullptr;
	}
};

//! �����ǰ���ս�����͸���ʱ������
typedef basiclib::basic_vector<TiaoZhanPaiMingReadFromDB> VTDBReadData;
typedef basiclib::basic_map<Net_UInt, VTDBReadData>	MapDBReadData;				//��սID �� ���ݵĶ�Ӧ

//! ���ؿ���ս������
struct CanTiaoZhanData{
	Net_UShort			m_nSelfPaiMing;
	Net_UShort			m_nNum;
	BasePaiMingData**	m_pBaseData;
	CanTiaoZhanData(){
		m_nSelfPaiMing = 0;
		m_nNum = 0;
		m_pBaseData = nullptr;
	}
	CanTiaoZhanData(BasePaiMingData** pData, Net_UShort nNum){
		m_nSelfPaiMing = 0;
		m_nNum = nNum;
		m_pBaseData = pData;
	}
};

#define CCtx_TiaoZhanPaiMing_CORUTINE_GetMingCiData		0
#define CCtx_TiaoZhanPaiMing_CORUTINE_GetPaiMingSelf	1
#define CCtx_TiaoZhanPaiMing_CORUTINE_GetPaiMing		2
#define CCtx_TiaoZhanPaiMing_CORUTINE_InitFromDB		3
#define CCtx_TiaoZhanPaiMing_CORUTINE_AddDataByUseID	4
#define CCtx_TiaoZhanPaiMing_CORUTINE_GetMoreThanSelf	5
#define CCtx_TiaoZhanPaiMing_CORUTINE_TiaoZhan			6
#define CCtx_TiaoZhanPaiMing_CORUTINE_UserDefine		1000
class _SKYNET_KERNEL_DLL_API CCtx_TiaoZhanPaiMing : public CCoroutineCtx{
public:
	CCtx_TiaoZhanPaiMing(const char* pKeyName = nullptr, const char* pClassName = GlobalGetClassName(CCtx_TiaoZhanPaiMing));
	virtual ~CCtx_TiaoZhanPaiMing();

	CreateTemplateHeader(CCtx_TiaoZhanPaiMing);

	virtual int InitCtx(CMQMgr* pMQMgr, const std::function<const char*(InitGetParamType, const char* pKey, const char* pDefault)>& func);

	//! Э���������Bussiness��Ϣ
	virtual int DispathBussinessMsg(CCorutinePlus* pCorutine, uint32_t nType, int nParam, void** pParam, void* pRetPacket);
protected:
	int DispathBussinessMsg_GetMingCiData(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	int DispathBussinessMsg_GetSelfPaiMing(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	int DispathBussinessMsg_GetPaiMing(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	int DispathBussinessMsg_InitFromDB(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	int DispathBussinessMsg_AddDataByUseID(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	int DispathBussinessMsg_GetMoreThanSelf(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
	int DispathBussinessMsg_TiaoZhan(CCorutinePlus* pCorutine, int nParam, void** pParam, void* pRetPacket);
protected:
	//! ������������
	void AddData(BasePaiMingData* pData);

	//! ��ȡ��������
	Net_UShort GetPaiMing(Net_UInt key);

	//! ��ȡ����
	void GetMingCiID(Net_UShort nBegin, Net_UShort nEnd, const std::function<void(BasePaiMingData*)>& func);

	//! ��ʼ����ȡ
	void ReadFromDBData(time_t* pMinTime, VTDBReadData& dbData);
protected:
	//! ������������
	virtual void CreatePaiMingData();
	//! ��������
	virtual void CopyPaiMingData(BasePaiMingData* pSelf, BasePaiMingData* pData);
protected:
	//! �û�id���û����ݵ�ӳ��
	typedef basiclib::basic_map<Net_UInt, BasePaiMingData*>	MapBufferData;
	MapBufferData			m_mapData;

	Net_UInt				m_nPaiMingCount;		//��������
	BasePaiMingData**		m_pPaiMing;				//����
	void*					m_pAllocateBuf;			//����ĸ���

	Net_UInt				m_nTiaoZhanMingCiNum;	//һ�������ս�����Լ�����λ

	Net_UInt				m_nSameTimeTick;		//
	time_t					m_nLastAddTime;
};

//! ��ȡ��������սctx
struct StartTiaoZhanData{
	Net_UInt		m_nTiaoZhanType;
	Net_UInt		m_nTiaoZhanID;
	StartTiaoZhanData(){
		m_nTiaoZhanType = 0;
		m_nTiaoZhanID = 0;
	}
	StartTiaoZhanData(Net_UInt nType, Net_UInt nID){
		m_nTiaoZhanType = nType;
		m_nTiaoZhanID = nID;
	}
};
typedef basiclib::basic_vector<StartTiaoZhanData>									VTStartTiaoZhanCtx;			//������ȡ������ʱ���������ctx
typedef basiclib::basic_map<Net_UInt, basiclib::basic_map<Net_UInt, uint32_t>>		MapTiaoZhanIDToCtxID;		//��ս����->��սID->ctxid

#pragma warning (pop)

#endif