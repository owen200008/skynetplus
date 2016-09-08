#include "coroutineplus/coroutineplus.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "public/public_threadlock.h"

#ifdef USE_3RRD_MALLOC
extern "C"{
#include "jemalloc.h"
}

void* s_3rd_malloc(size_t size)
{
	return je_malloc(size);	
}
void s_3rd_free(void* ptr)
{
	je_free(ptr);
}
#endif

static int g_i = 0;
void Foo(CCorutinePlus* pCorutine)
{
	int nThread = pCorutine->GetResumeParam<int>(0);
	for(int i = 0;i < 10000000;i++)
	{
		g_i++;
		//printf("Thread(%d) %d\n", nThread, i);
		pCorutine->Yield();
	}
}

static void* Thread_Work(void* p)
{
	int nThread = ++(*(int*)p);
	CCorutinePlusPool* S = new CCorutinePlusPool();
	S->InitCorutine();
	CCorutinePlus* pCorutine = S->GetCorutine();
	pCorutine->ReInit(Foo);
	pCorutine->Resume(S, nThread);
	clock_t ulBegin = clock();
	for(int i = 0;i < 10000000;i++)
	{
		pCorutine->Resume(S, nThread);
	}
	clock_t ulEnd = clock();
	printf("%d:UseTime(%f) Pool(%d/%d)\n", g_i, (double)(ulEnd - ulBegin) / CLOCKS_PER_SEC, S->GetVTCorutineSize(), S->GetCreateCorutineTimes());
	delete S;
}

void Test()
{
	g_i++;	
}
void Test2(CCorutinePlus* pCorutine)
{
	g_i++;
}
void DoTest1(const std::function<void()>& func)
{
	g_i = 0;
	for(int i = 0;i < 5;i++)
	{
		clock_t ulBegin = clock();
		for(int j = 0;j < 10000000;j++)
		{
			func();
		}
		clock_t ulEnd = clock();
		printf("%d:UseTime(%f)\n", g_i, (double)(ulEnd - ulBegin) / CLOCKS_PER_SEC);
	}
}

int main()
{
	const int nThreadCount = 2;
	int nIndex = 0;
	for(int i = 0;i < nThreadCount; i++)
	{
		pthread_t pthreadid;
		pthread_create(&pthreadid, nullptr, Thread_Work, &nIndex);
	}
	/*
	CCorutinePlusPool* S = new CCorutinePlusPool();
	S->InitCorutine();
	DoTest1([&]()->void{
		CCorutinePlus* pCorutine = S->GetCorutine();
		pCorutine->ReInit(Test2);
		pCorutine->Resume(S);
	});

	DoTest1([]()->void{
		Test();
	});

	DoTest1([]()->void{
		void* pRet = malloc(1);
		free(pRet);
	});

	pthread_mutex_t lock;
	pthread_mutex_init(&lock, NULL);
	DoTest1([&]()->void{
		pthread_mutex_lock(&lock);
		pthread_mutex_unlock(&lock);
	});
	*/

	getchar();
	return 0;
}


