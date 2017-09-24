/***********************************************************************************************
// �ļ���:     ctx_msgqueue.h
// ������:     ������
// Email:      zqcai@w.cn
// ��������:   �����ĵ���Ϣ����
// �汾��Ϣ:   1.0V
************************************************************************************************/
#ifndef SCBASIC_CTX_MSGQUEUE_H
#define SCBASIC_CTX_MSGQUEUE_H

#include <basic.h>
#include "../kernel_head.h"

#define CTXMESSAGE_TYPE_NOTHING					0		//��ʵ�ǳ���İ�
#define CTXMESSAGE_TYPE_RUNFUNC					1		//ִ�к���		
#define CTXMESSAGE_TYPE_RUNCOROUTINE			2		//����Э��
#define CTXMESSAGE_TYPE_ONTIMER					3		//ontimer
#define CTXMESSAGE_TYPE_USERTYPEDEFINEBEGIN     100000

//ִ�к����Ķ���
class CCoroutineCtx;
class CCorutinePlus;
struct ctx_message;
typedef void(*pRunFuncCtxMessageCallback)(CCoroutineCtx*, ctx_message*);

#pragma	pack(1)
struct _SKYNET_KERNEL_DLL_API ctx_message
{
	uint32_t		m_nSourceCtxID;
	intptr_t		m_session;
	uint32_t		m_nType;
	void*			m_pFunc;
	void*			m_data;
	size_t			sz;
	ctx_message(){
		memset(this, 0, sizeof(ctx_message));
	}
	ctx_message(uint32_t nCtxID){
		memset(this, 0, sizeof(ctx_message));
		m_nSourceCtxID = nCtxID;
	}
	//! ִ�к���
	ctx_message(uint32_t nCtxID, pRunFuncCtxMessageCallback pFunc, intptr_t nSession = 0){
		memset(this, 0, sizeof(ctx_message));
		m_nType = CTXMESSAGE_TYPE_RUNFUNC;
		m_nSourceCtxID = nCtxID;
		m_pFunc = pFunc;
		m_session = nSession;
	}
	//! ִ��Э��
	ctx_message(uint32_t nCtxID, CCorutinePlus* pCorutine, intptr_t nSession = 0){
		memset(this, 0, sizeof(ctx_message));
		m_nType = CTXMESSAGE_TYPE_RUNCOROUTINE;
		m_nSourceCtxID = nCtxID;
		m_pFunc = pCorutine;
		m_session = nSession;
	}
	//! ontimer
	ctx_message(uint32_t nCtxID, Net_PtrInt nOnTimerKey, intptr_t nSession = 0){
		memset(this, 0, sizeof(ctx_message));
		m_nType = CTXMESSAGE_TYPE_ONTIMER;
		m_nSourceCtxID = nCtxID;
		m_pFunc = (void*)nOnTimerKey;
		m_session = nSession;
	}
	void ExportFromSmartBuffer(basiclib::CBasicSmartBuffer& smBuf){
		basiclib::SmartBufferExportOutData data;
		smBuf.ExportOutData(data);
		m_data = data.m_pExport;
		sz = data.m_nLength;
		data.m_pExport = nullptr;
		data.m_nLength = 0;
	}
	void CloneData(const char* pData, int nLength){
		if (nLength != 0){
			sz = nLength;
			m_data = basiclib::BasicAllocate(sz);
			memcpy(m_data, pData, sz);
		}
	}
	void ReleaseData(){
		if (sz > 0 && m_data){
			basiclib::BasicDeallocate(m_data);
		}
	}
	//! �ɶ����ַ���
	void FormatMsgQueueToString(const std::function<void(const char* pData, int nLength)>& func){
		char szBuf[128] = { 0 };
#pragma warning (push)
#pragma warning (disable: 4293)
		func(szBuf, 
			sprintf(szBuf, "C(%d) S(%x%x) T(%d) SZ(%d) F(%x%x) D(%x%x)", m_nSourceCtxID, 
			(uint32_t)(m_session >> 32), (uint32_t)m_session, m_nType, sz, 
			(uint32_t)((intptr_t)m_pFunc >> 32), (uint32_t)m_pFunc, 
			(uint32_t)((intptr_t)m_data >> 32), (uint32_t)m_data));
#pragma warning (pop)
	}
};
#pragma	pack()

class CMQMgr;
class CCtxMessageQueue : public basiclib::CLockFreeMessageQueue<ctx_message, 32>
{
public:
	CCtxMessageQueue();
	virtual ~CCtxMessageQueue();

	//! ��ʼ������
	void InitCtxMsgQueue(uint32_t nCtxID, CMQMgr* pMgr);
	
	//! ������Ϊһ���ɹ������ش����Ƿ�֪ͨ,���token�������Լ���
	bool MQPush(moodycamel::ProducerToken& token, ctx_message& message);
	bool MQPush(ctx_message& message);
	//! ���������ߣ����̣߳�,�����Ƿ��Ѿ�ȫ��ȡ��
	bool MQPop(moodycamel::ConsumerToken& token, ctx_message& message);
	bool MQPop(ctx_message& message);

	void PushToMQMgr();
	uint32_t GetCtxID(){ return m_nCtxID; }
protected:
	uint32_t									m_nCtxID;
	std::atomic<uint32_t>						m_nAddTimes;
	CMQMgr*										m_pMgr;

	friend class CMQMgr;
	friend class CCoroutineCtx;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CMQMgr : public basiclib::CLockFreeMessageQueue<CCtxMessageQueue*, 1024>
{
public:
	CMQMgr();
	virtual ~CMQMgr();

	void GlobalMQPush(CCtxMessageQueue* queue);
	void GlobalMQPush(moodycamel::ProducerToken& token, CCtxMessageQueue* queue);
	CCtxMessageQueue* GlobalMQPop(moodycamel::ConsumerToken& token);

	//wait timeout ms
	void WaitForGlobalMQ(unsigned int nTimeout);
protected:
	basiclib::CEvent*		m_pEvent;
	std::atomic<uint16_t>	m_nWaitThread;
};
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#endif