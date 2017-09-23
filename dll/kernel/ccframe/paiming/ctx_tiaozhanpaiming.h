#ifndef SKYNETPLUS_COROUTINE_CTX_TIAOZHANPAIMING_H
#define SKYNETPLUS_COROUTINE_CTX_TIAOZHANPAIMING_H

#include "../dllmodule.h"

#pragma warning (push)
#pragma warning (disable: 4251)
#pragma warning (disable: 4275)
struct BasePaiMingData{
	Net_UInt		m_numKeyID;
	Net_UShort		m_nMingCi;			//0代表第一名
	BasePaiMingData(){
		m_numKeyID = 0;
		m_nMingCi = 0;
	}
};
//! 数据库提供挑战名次和时间和同时间的tick
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

//! 必须是按挑战排名和更新时间排序
typedef basiclib::basic_vector<TiaoZhanPaiMingReadFromDB> VTDBReadData;
typedef basiclib::basic_map<Net_UInt, VTDBReadData>	MapDBReadData;				//挑战ID 到 数据的对应

//! 返回可挑战的数据
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

	//! 协程里面调用Bussiness消息
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
	//! 加入排名数据
	void AddData(BasePaiMingData* pData);

	//! 获取排名数据
	Net_UShort GetPaiMing(Net_UInt key);

	//! 获取名次
	void GetMingCiID(Net_UShort nBegin, Net_UShort nEnd, const std::function<void(BasePaiMingData*)>& func);

	//! 初始化读取
	void ReadFromDBData(time_t* pMinTime, VTDBReadData& dbData);
protected:
	//! 创建排名数据
	virtual void CreatePaiMingData();
	//! 拷贝数据
	virtual void CopyPaiMingData(BasePaiMingData* pSelf, BasePaiMingData* pData);
protected:
	//! 用户id到用户数据的映射
	typedef basiclib::basic_map<Net_UInt, BasePaiMingData*>	MapBufferData;
	MapBufferData			m_mapData;

	Net_UInt				m_nPaiMingCount;		//总排名数
	BasePaiMingData**		m_pPaiMing;				//排名
	void*					m_pAllocateBuf;			//分配的个数

	Net_UInt				m_nTiaoZhanMingCiNum;	//一次最大挑战超过自己多少位

	Net_UInt				m_nSameTimeTick;		//
	time_t					m_nLastAddTime;
};

//! 读取启动的挑战ctx
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
typedef basiclib::basic_vector<StartTiaoZhanData>									VTStartTiaoZhanCtx;			//用来读取启动的时候启动多个ctx
typedef basiclib::basic_map<Net_UInt, basiclib::basic_map<Net_UInt, uint32_t>>		MapTiaoZhanIDToCtxID;		//挑战类型->挑战ID->ctxid

#pragma warning (pop)

#endif