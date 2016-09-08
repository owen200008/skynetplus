#include "coroutineplus/coroutineplus.h"
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "public/public_threadlock.h"

void Foo(CCorutinePlus* pCorutine)
{
	int nThread = pCorutine->GetResumeParam<int>(0);
	for(int i = 0;i < 10;i++)
	{
		printf("Thread(%d) %d\n", nThread, i);
		pCorutine->Yield();
	}
}
static int g_i = 0;
void Test()
{
	//malloc(1);
	//for(int i = 0;i < 100;i++)
		g_i++;	
}
void Test2(CCorutinePlus* pCorutine)
{
	//malloc(1);
	//for(int i = 0;i < 100;i++)
		g_i++;
	//pCorutine->Yield();
}

static void* Thread_Work(void* p)
{
	int nThread = ++(*(int*)p);
	CCorutinePlusPool* S = new CCorutinePlusPool();
	S->InitCorutine();
	while(true)
	{
		CCorutinePlus* pCorutine = S->GetCorutine();
		pCorutine->ReInit(Foo);
		pCorutine->Resume(S, nThread);
		printf("Thread(%d) Pool(%d/%d)\n", nThread, S->GetVTCorutineSize(), S->GetCreateCorutineTimes());
		sleep(10);
	}
	delete S;
}

int main()
{
	CCorutinePlusPool* S = new CCorutinePlusPool();
	S->InitCorutine(1);
	for(int i = 0;i < 10;i++)
	{
		//CCorutinePlus* pCorutine = S->GetCorutine();
		//pCorutine->ReInit(Test2);
		//S->ReleaseCorutine(pCorutine);
		clock_t ulBegin = clock();
		for(int j = 0;j < 10000000;j++)
		{
			CCorutinePlus* pCorutine = S->GetCorutine();
			pCorutine->ReInit(Test2);
			pCorutine->Resume(S);
			//pCorutine->Resume(S);
		}
		clock_t ulEnd = clock();
		printf("%d:UseTime(%f) Pool(%d/%d)\n", g_i, (double)(ulEnd - ulBegin) / CLOCKS_PER_SEC, S->GetVTCorutineSize(), S->GetCreateCorutineTimes());
	}
	g_i = 0;
	clock_t ulBegin = clock();
	for(int i = 0;i < 10;i++)
	{
		clock_t ulBegin = clock();
		for(int j = 0;j < 10000000;j++)
		{
			Test();
		}
		clock_t ulEnd = clock();
		printf("%d:UseTime(%f)\n", g_i, (double)(ulEnd - ulBegin) / CLOCKS_PER_SEC);
	}
	g_i = 0;
	/*pthread_mutex_t lock;
	pthread_mutex_init(&lock, NULL);
	for(int i = 0;i < 10;i++)
	{
		clock_t ulBegin = clock();
		for(int j = 0;j < 10000000;j++)
		{
			pthread_mutex_lock(&lock);
			pthread_mutex_unlock(&lock);
			//malloc(1);
		}
		clock_t ulEnd = clock();
		printf("%d:UseTime(%f)\n", g_i, (double)(ulEnd - ulBegin) / CLOCKS_PER_SEC);
	}*/
	return 0;

	const int nThreadCount = 5;
	int nIndex = 0;
	for(int i = 0;i < nThreadCount;i++)
	{
		pthread_t pthreadid;
		pthread_create(&pthreadid, nullptr, Thread_Work, &nIndex);
	}
	getchar();
	return 0;
}


