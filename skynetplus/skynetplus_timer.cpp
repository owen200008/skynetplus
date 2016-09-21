#include <basic.h>

#include "skynetplus_timer.h"
#include "skynetplus_mq.h"

#include <time.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>


typedef void (*timer_execute_func)(void *ud,void *arg);

#define TIME_NEAR_SHIFT 8
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_LEVEL_SHIFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR-1)
#define TIME_LEVEL_MASK (TIME_LEVEL-1)

struct timer_event {
	uint32_t 	handle;
	int 		session;
};

struct timer_node {
	timer_node*			next;
	uint32_t 			expire;
	uint32_t			m_nRepeatTime;
	uint8_t				m_bIsValid;
};

struct link_list {
	timer_node head;
	timer_node *tail;
};


// centisecond: 1/100 second
static void systime(uint32_t *sec, uint32_t *cs) 
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	*sec = tv.tv_sec;
	*cs = tv.tv_usec / 10000;
}


static uint64_t gettime() 
{
	uint64_t t;
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t = (uint64_t)tv.tv_sec * 100;
	t += tv.tv_usec / 10000;
	return t;
}

typedef basiclib::basic_map<int, timer_node*>::type MapTimerNode;
class CTimer
{
public:
	CTimer()
	{
		memset(this, 0, sizeof(*this));
		m_pMapNode = new MapTimerNode();
	}
	~CTimer()
	{
		if(m_pMapNode)
		{
			delete m_pMapNode;
		}
	}
	void Init()
	{
		int i,j;
		for (i=0;i<TIME_NEAR;i++)
		{
			link_clear(&m_near[i]);
		}
		for (i=0;i<4;i++) 
		{
			for (int j=0;j<TIME_LEVEL;j++)
			{
				 link_clear(&m_t[i][j]);
			}
		}
		uint32_t current = 0;
		systime(&starttime, &current);
		m_current = current;
		m_current_point = gettime();
	}
	void timer_add(timer_event& event, int time,int bRepeat)
	{
		int sz = sizeof(event);
		struct timer_node *node = (struct timer_node *)basiclib::BasicAllocate(sizeof(*node)+sz);
		memcpy(node+1,&event,sz);
		node->m_bIsValid = 1;
		node->m_nRepeatTime = bRepeat > 0 ? time : 0;
		basiclib::CSpinLockFunc lock(&m_lock, TRUE);
			node->expire=time + m_time;
			if(bRepeat > 0)
			{
				(*m_pMapNode)[event.session] = node;
			}
			add_node(node);
	}
	void timer_del(int nSession)
	{
		basiclib::CSpinLockFunc lock(&m_lock, TRUE);
			MapTimerNode::iterator iter = (*m_pMapNode).find(nSession);
			if(iter != (*m_pMapNode).end())
			{
				timer_node* pNode = iter->second;
				pNode->m_bIsValid = 0;
				(*m_pMapNode).erase(iter);
			}
	}

	void add_node(timer_node *node)
	{
		uint32_t time = node->expire;
		uint32_t current_time = m_time;
	
		if ((time|TIME_NEAR_MASK) == (current_time|TIME_NEAR_MASK)) 
		{
			linklist(&m_near[time&TIME_NEAR_MASK],node);
		} 
		else 
		{
			int i;
			uint32_t mask=TIME_NEAR << TIME_LEVEL_SHIFT;
			for (i=0;i<3;i++) 
			{
				if ((time|(mask-1))==(current_time|(mask-1))) 
				{
					break;
				}
				mask <<= TIME_LEVEL_SHIFT;
			}

			linklist(&m_t[i][((time>>(TIME_NEAR_SHIFT + i*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)],node);	
		}
	}
	void OnTimer()
	{
		basiclib::CSpinLockFunc lock(&m_lock, TRUE);

		// try to dispatch timeout 0 (rare condition)
		timer_execute(lock);

		// shift time first, and then dispatch timer message
		timer_shift();

		timer_execute(lock);
	}
protected:
	void timer_execute(basiclib::CSpinLockFunc& lock)
	{
		int idx = m_time & TIME_NEAR_MASK;
	
		while (m_near[idx].head.next) 
		{
			timer_node *current = link_clear(&m_near[idx]);

			lock.UnLock();
			// dispatch_list don't need lock T
			dispatch_list(current);
			lock.Lock();
		}
	}
	void timer_shift()
	{
		int mask = TIME_NEAR;
		uint32_t ct = ++m_time;
		if (ct == 0) 
		{
			move_list(3, 0);
		} 
		else 
		{
			uint32_t time = ct >> TIME_NEAR_SHIFT;
			int i=0;

			while ((ct & (mask-1))==0) 
			{
				int idx= time & TIME_LEVEL_MASK;
				if (idx!=0) 
				{
					move_list(i, idx);
					break;				
				}
				mask <<= TIME_LEVEL_SHIFT;
				time >>= TIME_LEVEL_SHIFT;
				++i;
			}
		}
	}
	void move_list(int level, int idx)
	{
		timer_node *current = link_clear(&m_t[level][idx]);
		while (current) 
		{
			timer_node *temp=current->next;
			add_node(current);
			current=temp; 
		}
	}
	void dispatch_list(struct timer_node *current)
	{
		do 
		{
			size_t sType = (size_t)PTYPE_RESPONSE << MESSAGE_TYPE_SHIFT;
			if(current->m_nRepeatTime > 0)
				sType = (size_t)PTYPE_ONTIMER << MESSAGE_TYPE_SHIFT;
			int nRet = 0;
			timer_event * event = (struct timer_event *)(current+1);
			if(current->m_bIsValid)
			{
				skynet_message message;
				message.source = 0;
				message.session = event->session;
				message.data = NULL;
				message.sz = sType;

				nRet = CSkynetContext::PushToHandleMQ(event->handle, &message);
			}
			timer_node* temp = current;
			current=current->next;
			if(temp->m_nRepeatTime > 0 && nRet == 0 && temp->m_bIsValid)
			{
				//spin unlock, so add is valid
				temp->expire += temp->m_nRepeatTime;
				
				basiclib::CSpinLockFunc lock(&m_lock, TRUE);
					add_node(temp);
			}
			else
			{
				basiclib::BasicDeallocate(temp);  
			}
		}while(current);
	}
protected:
	void linklist(link_list *list, timer_node *node)
	{
		list->tail->next = node;
		list->tail = node;
		node->next=0;
	}
	timer_node* link_clear(link_list *list)
	{
		timer_node * ret = list->head.next;
		list->head.next = 0;
		list->tail = &(list->head);

		return ret;
	}
public:
	link_list                		m_near[TIME_NEAR];
	link_list                		m_t[4][TIME_LEVEL];
	SpinLock                 		m_lock;
	uint32_t                        m_time;
	uint32_t                        starttime;
	uint64_t                       	m_current;
	uint64_t                        m_current_point;
	MapTimerNode*					m_pMapNode;
};

static struct CTimer * TI = NULL;

static void timer_update(CTimer *T) 
{
	T->OnTimer();
}

int skynet_timeout(uint32_t handle, int time, int session) 
{
	if (time <= 0) 
	{
		struct skynet_message message;
		message.source = 0;
		message.session = session;
		message.data = NULL;
		message.sz = (size_t)PTYPE_RESPONSE << MESSAGE_TYPE_SHIFT;

		if (CSkynetContext::PushToHandleMQ(handle, &message)) 
		{
			return -1;
		}
	} 
	else 
	{
		struct timer_event event;
		event.handle = handle;
		event.session = session;
		TI->timer_add(event, time, 0);
	}
	return session;
}
int skynet_ontimer(uint32_t handle, int time, int session)
{
	if (time > 0) 
	{
		struct timer_event event;
		event.handle = handle;
		event.session = session;
		TI->timer_add(event, time, 1);
	} 
	return session;
}

void skynet_delontimer(int session)
{
	TI->timer_del(session);
}

void skynet_updatetime(void) 
{
	uint64_t cp = gettime();
	if(cp < TI->m_current_point) 
	{
		basiclib::BasicLogEventErrorV("time diff error: change from %lld to %lld", cp, TI->m_current_point);
		TI->m_current_point = cp;
	}
	else if (cp != TI->m_current_point) 
	{
		uint32_t diff = (uint32_t)(cp - TI->m_current_point);
		TI->m_current_point = cp;
		TI->m_current += diff;
		for (int i=0;i<diff;i++) 
		{
			timer_update(TI);
		}
	}
}

uint32_t skynet_starttime(void) 
{
	return TI->starttime;
}

uint64_t skynet_now(void) 
{
	return TI->m_current;
}

void skynet_timer_init(void) 
{
	CTimer* pTimer = new CTimer();
	pTimer->Init();
	TI = pTimer;
}

