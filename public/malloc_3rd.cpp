#include "malloc_3rd.h"

#ifdef USE_3RRD_MALLOC
void* s_3rd_malloc(size_t size)
{
	return malloc3rd(size);
}
void s_3rd_free(void* ptr)
{
	free3rd(ptr);
}
void* s_3rd_malloc_res(size_t size)
{
	return malloc3rd_res(size);
}
void s_3rd_free_res(void* ptr)
{
	free3rd_res(ptr);
}
#endif

