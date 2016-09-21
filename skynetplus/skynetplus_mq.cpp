#include "skynetplus_mq.h"

#define DEFAULT_QUEUE_SIZE 64

#define MQ_IN_GLOBAL 1
#define MQ_OVERLOAD 1024
message_queue::message_queue(CSkynetContext* pCtx)
{
	m_pCtx = pCtx;
	m_cap = DEFAULT_QUEUE_SIZE;
	m_head = 0;
	m_tail = 0;
	m_in_global = MQ_IN_GLOBAL;
	m_overload = 0;
	m_overload_threshold = 1024;
	m_queue = (skynet_message*)basiclib::BasicAllocate(m_cap * sizeof(skynet_message));
	m_next = nullptr;
}

message_queue::~message_queue()
{
	assert(m_next == nullptr);
	assert(m_pCtx == nullptr);
	basiclib::BasicDeallocate(m_queue);
}

CRefSkynetContext& message_queue::GetContext()
{
	return m_pCtx;
}

int message_queue::GetMQLength()
{
	int head, tail,cap;
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	head = m_head;
	tail = m_tail;
	cap = m_cap;
	lock.UnLock();
	if(head <= tail)
		return tail - head;
	return tail + cap - head;
}

int message_queue::Overload()
{
	if(m_overload)
	{
		int overload = m_overload;
		m_overload = 0;
		return overload;
	}
	return 0;
}

void message_queue::MQPush(skynet_message* message)
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	m_queue[m_tail] = *message;
	if (++m_tail >= m_cap)
	{
		m_tail = 0;
	}
	if (m_head == m_tail)
	{
		expand_queue();
	}
	if (m_in_global == 0)
	{
		m_in_global = MQ_IN_GLOBAL;
		lock.UnLock();
		CSingletonSkynetMQ::Instance().GlobalMQPush(this, true);
	}
}

void message_queue::ReadyToGlobalMQ()
{
	CSingletonSkynetMQ::Instance().GlobalMQPush(this, true);
}

void message_queue::PushToGlobalMQ()
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	if (m_in_global == 0)
	{
		m_in_global = MQ_IN_GLOBAL;
		CSingletonSkynetMQ::Instance().GlobalMQPush(this, true);
	}
}

void message_queue::expand_queue()
{
	skynet_message* new_queue = (skynet_message*)basiclib::BasicAllocate(m_cap * 2 * sizeof(skynet_message));
	int i;
	for (i=0;i<m_cap;i++)
	{
		new_queue[i] = m_queue[(m_head + i) % m_cap];
	}
	m_head = 0;
	m_tail = m_cap;
	m_cap *= 2;
	basiclib::BasicDeallocate(m_queue);
	m_queue = new_queue;
}

int message_queue::MQPop(skynet_message* message)
{
	int ret = 1;
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	if (m_head != m_tail)
	{
		*message = m_queue[m_head++];
		ret = 0;
		int head = m_head;
		int tail = m_tail;
		int cap = m_cap;

		if (head >= cap)
		{
			m_head = head = 0;
		}
		int length = tail - head;
		if (length < 0)
		{
			length += cap;
		}
		while (length > m_overload_threshold)
		{
			m_overload = length;
			m_overload_threshold *= 2;
		}
	}
	else
	{
		// reset overload_threshold when queue is empty
		m_overload_threshold = MQ_OVERLOAD;
	}
	if (ret)
	{
		m_in_global = 0;
	}
	return ret;
}

void message_queue::ContextRelease()
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	if(m_pCtx != nullptr)
	{
		m_pCtx = nullptr;
	}
}

void message_queue::_drop_queue(const std::function<void(skynet_message *, void *)>& func, void *ud)
{
	skynet_message msg;
	while(!MQPop(&msg))
	{
		func(&msg, ud);
	}
}

/////////////////////////////////////////////////////////
CSkynetMQ::CSkynetMQ()
{
	m_head = nullptr;
	m_tail = nullptr;
}

CSkynetMQ::~CSkynetMQ()
{
}

void CSkynetMQ::GlobalMQPush(message_queue* queue, bool bSetEvent)
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	assert(queue->m_next == NULL);
	if(m_tail)
	{
		m_tail->m_next = queue;
		m_tail = queue;
	}
	else
	{
		m_head = m_tail = queue;
	}
	lock.UnLock();
	if(bSetEvent)
		m_event.SetEvent();
}

void CSkynetMQ::WaitForGlobalMQ(unsigned int nTimeout)
{
	basiclib::BasicWaitForSingleObject(m_event, nTimeout);
}

message_queue* CSkynetMQ::GlobalMQPop()
{
	basiclib::CSpinLockFunc lock(&m_lock, TRUE);
	message_queue *mq = m_head;
	if(mq)
	{
		m_head = mq->m_next;
		if(m_head == nullptr)
		{
			assert(mq == m_tail);
			m_tail = nullptr;
		}
		mq->m_next = nullptr;
	}
	lock.UnLock();
	return mq;
}


