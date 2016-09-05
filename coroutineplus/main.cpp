#include "coroutineplus.h"
#include <stdio.h>
#include <pthread.h>

void Foo(CCorutinePlus* pCorutine)
{
	for(int i = 0;i < 5;i++)
	{
		printf("%d\n", i);
		pCorutine->Yield();
	}
}

void test(CCorutinePlusPool* S)
{
	CCorutinePlus* pCorutine = S->GetCorutine();
	printf("Test Begin\n");
	pCorutine->ReInit(Foo);
	printf("Test Start Resume Pool: %d/%d\n", S->GetVTCorutineSize(), S->GetCreateCorutineTimes());
	pCorutine->Resume(S);
	printf("Test Before Resume Pool: %d/%d\n", S->GetVTCorutineSize(), S->GetCreateCorutineTimes());
	for(int i = 0;i < 10;i++)
	{
		pCorutine->Resume(S);
		printf("Test After Resume Pool: %d/%d\n", S->GetVTCorutineSize(), S->GetCreateCorutineTimes());
	}
	printf("Test End\n");
}

int main()
{
	CCorutinePlusPool* S = new CCorutinePlusPool();
	S->InitCorutine();
	for(int i = 0;i < 10;i++)
	{
		test(S);
		printf("Pool: %d/%d\n", S->GetVTCorutineSize(), S->GetCreateCorutineTimes());
	}
	getchar();

	delete S;
	return 0;
}


