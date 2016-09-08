#ifndef skynet_malloc_h
#define skynet_malloc_h

#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include <new>

//#ifdef NOUSE_JEMALLOC
#define malloc3rd malloc
#define calloc3rd calloc
#define realloc3rd realloc
#define free3rd free
#define malloc3rd_res malloc
#define free3rd_res free
//#else
void* malloc3rd(size_t sz);
void* calloc3rd(size_t nmemb,size_t size);
void* realloc3rd(void *ptr, size_t size);
void free3rd(void *ptr);
void* malloc3rd_res(size_t sz);
void free3rd_res(void *ptr);
//#endif
char* strdup3rd(const char *str);
void* lalloc3rd(void *ptr, size_t osize, size_t nsize);	// use for lua
void debug_memory(const char *info);

template<class T>
T* NewObject3rd()
{
	void* p = malloc3rd(sizeof(T));
	::new (p)T;
	return (T*)p;
}

template<class T>
void DelObject3rd(T* obj)
{
	obj->~T();
	free3rd(obj);
}
#ifdef USE_3RRD_MALLOC
void* s_3rd_malloc(size_t size);
void s_3rd_free(void* ptr);
void* s_3rd_malloc_res(size_t size);
void s_3rd_free_res(void* ptr);
#endif

#endif
