#include "libco_coroutine.h"
#include <string.h>
#include <stdint.h>

#define ESP 0
#define EIP 1
// -----------
#define RSP 0
#define RIP 1
#define RBX 2
#define RDI 3
#define RSI 4

extern "C"
{
	extern void coctx_swap( coctx_t *,coctx_t* ) asm("coctx_swap");
}

void coctx_init(coctx_t *ctx)
{
	memset(ctx,0,sizeof(*ctx));
}
#if defined(__i386__)
void coctx_make(coctx_t *ctx,coctx_pfn_t pfn, const void* s1)
{
	char *sp = ctx->ss_sp + ctx->ss_size;
	sp = (char*)((unsigned long)sp & -16L);

	int len = sizeof(coctx_param_t) + 64;
	memset( sp - len,0,len );
	ctx->routine = pfn;
	ctx->s1 = s1;
	ctx->s2 = s2;

	ctx->param = (coctx_param_t*)sp ;
	ctx->param->f = pfn;
	ctx->param->f_link = 0;
	ctx->param->s1 = s1;

	ctx->regs[ ESP ] = (char*)(ctx->param) + sizeof(void*);
	ctx->regs[ EIP ] = (char*)pfn;
}
#elif defined(__x86_64__)
void coctx_make(coctx_t *ctx, coctx_pfn_t pfn, const void *s1)
{
	char *stack = ctx->ss_sp;

	long long int *sp = (long long int*)((uintptr_t)stack + ctx->ss_size);
	sp -= 1;
	sp = (long long int*)((((uintptr_t)sp) & - 16L ) - 8);

	ctx->regs[ RBX ] = &sp[1];
	ctx->regs[ RSP ] = (char*)sp;
	ctx->regs[ RIP ] = (char*)pfn;
	
	sp[1] = 0;
	
	ctx->regs[ RDI ] = (char*)s1;
}
#endif

void coctx_swapcontext(coctx_t *ctx1, coctx_t *ctx2)
{
	coctx_swap(ctx1, ctx2);
}
