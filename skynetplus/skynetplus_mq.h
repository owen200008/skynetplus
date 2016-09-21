#ifndef SKYNETPLUS_MQ_H
#define SKYNETPLUS_MQ_H

#include <stdint.h>  
#include <basic.h>
#include "skynetplus_context.h"


struct skynet_message
{
	uint32_t 	source;
	int			session;
	void*		data;
	size_t		sz;
	void ReleaseData()
	{
		if(data)
			basiclib::BasicDeallocate(data);
	}
};
// type is encoding in skynet_message.sz high 8bit
#define MESSAGE_TYPE_MASK (SIZE_MAX >> 8)
#define MESSAGE_TYPE_SHIFT ((sizeof(size_t)-1) * 8)

class message_queue : public basiclib::CBasicObject
{
public:
	message_queue(CSkynetContext* pCtx);
	virtual ~message_queue();
	
	//just call in init
	void ReadyToGlobalMQ();
	//push to globalmq
	void PushToGlobalMQ();

	CRefSkynetContext& GetContext();
	void ContextRelease();

	int GetMQLength();
	int Overload();

	void MQPush(skynet_message* message);
	int MQPop(skynet_message* message);

	void _drop_queue(const std::function<void(skynet_message *, void *)>& func, void *ud);
protected:
	void expand_queue();
public:
	basiclib::SpinLock		m_lock;
	int						m_cap;
	int						m_head;
	int						m_tail;
	int 					m_in_global;
	int						m_overload;
	int 					m_overload_threshold;
	skynet_message*			m_queue;
	message_queue*			m_next;

	CRefSkynetContext		m_pCtx;
};

class CSkynetMQ : public basiclib::CBasicObject
{
public:
	CSkynetMQ();
	virtual ~CSkynetMQ();
	
	void GlobalMQPush(message_queue* queue, bool bSetEvent = false);
	message_queue* GlobalMQPop();

	//wait timeout ms
	void WaitForGlobalMQ(unsigned int nTimeout);
protected:
	message_queue*			m_head;
	message_queue*			m_tail;
	basiclib::SpinLock		m_lock;
	basiclib::CEvent		m_event;
};

typedef basiclib::CBasicSingleton<CSkynetMQ> CSingletonSkynetMQ;


#endif
