#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "malloc_hook.h"
#include "atomic.h"

static size_t _used_memory = 0;
static size_t _memory_block = 0;
typedef struct _mem_data {
	uint32_t handle;
	ssize_t allocated;
} mem_data;

#define SLOT_SIZE 0x10000
#define PREFIX_SIZE sizeof(uint32_t)

static mem_data mem_stats[SLOT_SIZE];

extern void SystemLogError(const char* msg, ...);
extern void SystemLog(const char* msg, ...);
extern uint32_t SystemGetAllocHandleID();

#ifndef NOUSE_JEMALLOC

#include "jemalloc.h"

// for lalloc3rd use
#define raw_realloc je_realloc
#define raw_free je_free

static ssize_t*
get_allocated_field(uint32_t handle) {
	int h = (int)(handle & (SLOT_SIZE - 1));
	mem_data *data = &mem_stats[h];
	uint32_t old_handle = data->handle;
	ssize_t old_alloc = data->allocated;
	if(old_handle == 0 || old_alloc <= 0) {
		// data->allocated may less than zero, because it may not count at start.
		if(!ATOM_CAS(&data->handle, old_handle, handle)) {
			return 0;
		}
		if (old_alloc < 0) {
			ATOM_CAS(&data->allocated, old_alloc, 0);
		}
	}
	if(data->handle != handle) {
		return 0;
	}
	return &data->allocated;
}

inline static void 
update_xmalloc_stat_alloc(uint32_t handle, size_t __n) {
	ATOM_ADD(&_used_memory, __n);
	ATOM_INC(&_memory_block); 
	ssize_t* allocated = get_allocated_field(handle);
	if(allocated) {
		ATOM_ADD(allocated, __n);
	}
}

inline static void
update_xmalloc_stat_free(uint32_t handle, size_t __n) {
	ATOM_SUB(&_used_memory, __n);
	ATOM_DEC(&_memory_block);
	ssize_t* allocated = get_allocated_field(handle);
	if(allocated) {
		ATOM_SUB(allocated, __n);
	}
}

inline static void*
fill_prefix(char* ptr) {
	uint32_t handle = SystemGetAllocHandleID();
	size_t size = je_malloc_usable_size(ptr);
	uint32_t *p = (uint32_t *)(ptr + size - sizeof(uint32_t));
	memcpy(p, &handle, sizeof(handle));

	update_xmalloc_stat_alloc(handle, size);
	return ptr;
}

inline static void*
clean_prefix(char* ptr) {
	size_t size = je_malloc_usable_size(ptr);
	uint32_t *p = (uint32_t *)(ptr + size - sizeof(uint32_t));
	uint32_t handle;
	memcpy(&handle, p, sizeof(handle));
	update_xmalloc_stat_free(handle, size);
	return ptr;
}

static void malloc_oom(size_t size) {
	fprintf(stderr, "xmalloc: Out of memory trying to allocate %zu bytes\n",
		size);
	fflush(stderr);
	abort();
}

void 
memory_info_dump(void) {
	je_malloc_stats_print(0,0,0);
}

size_t 
mallctl_int64(const char* name, size_t* newval) {
	size_t v = 0;
	size_t len = sizeof(v);
	if(newval) {
		je_mallctl(name, &v, &len, newval, sizeof(size_t));
	} else {
		je_mallctl(name, &v, &len, NULL, 0);
	}
	//SystemLog("name: %s, value: %zd\n", name, v);
	return v;
}

int 
mallctl_opt(const char* name, int* newval) {
	int v = 0;
	size_t len = sizeof(v);
	if(newval) {
		int ret = je_mallctl(name, &v, &len, newval, sizeof(int));
		if(ret == 0) {
			SystemLog("set new value(%d) for (%s) succeed\n", *newval, name);
		} else {
			SystemLogError("set new value(%d) for (%s) failed: error -> %d\n", *newval, name, ret);
		}
	} else {
		je_mallctl(name, &v, &len, NULL, 0);
	}

	return v;
}

// hook : malloc, realloc, free, calloc
void* malloc3rd_res(size_t size) {
	return je_malloc(size);
}
void free3rd_res(void *ptr) {
	je_free(ptr);
}

void * malloc3rd(size_t size) {
	void* ptr = je_malloc(size + PREFIX_SIZE);
	if(!ptr) malloc_oom(size);
	return fill_prefix((char*)ptr);
}

void * realloc3rd(void *ptr, size_t size) {
	if (ptr == NULL) return malloc3rd(size);

	void* rawptr = clean_prefix((char*)ptr);
	void *newptr = je_realloc(rawptr, size+PREFIX_SIZE);
	if(!newptr) malloc_oom(size);
	return fill_prefix((char*)newptr);
}

void free3rd(void *ptr) {
	if (ptr == NULL) return;
	void* rawptr = clean_prefix((char*)ptr);
	je_free(rawptr);
}

void * calloc3rd(size_t nmemb,size_t size) {
	void* ptr = je_calloc(nmemb + ((PREFIX_SIZE+size-1)/size), size );
	if(!ptr) malloc_oom(size);
	return fill_prefix((char*)ptr);
}

#else

// for lalloc3rd use
#define raw_realloc realloc
#define raw_free free

void memory_info_dump(void) {
	SystemLogError("No jemalloc");
}

size_t mallctl_int64(const char* name, size_t* newval) {
	SystemLogError("No jemalloc : mallctl_int64 %s.", name);
	return 0;
}

int mallctl_opt(const char* name, int* newval) {
	SystemLogError("No jemalloc : mallctl_opt %s.", name);
	return 0;
}

#endif

size_t malloc_used_memory(void) {
	return _used_memory;
}

size_t malloc_memory_block(void) {
	return _memory_block;
}

void dump_c_mem() {
	int i;
	size_t total = 0;
	SystemLog("dump all service mem:");
	for(i=0; i<SLOT_SIZE; i++) {
		mem_data* data = &mem_stats[i];
		if(data->handle != 0 && data->allocated != 0) {
			total += data->allocated;
			SystemLog("0x%x -> %zdkb", data->handle, data->allocated >> 10);
		}
	}
	SystemLog("+total: %zdkb",total >> 10);
}

char* strdup3rd(const char *str) {
	size_t sz = strlen(str);
	char * ret = (char*)malloc3rd(sz+1);
	memcpy(ret, str, sz+1);
	return ret;
}

void* lalloc3rd(void *ptr, size_t osize, size_t nsize) {
	if (nsize == 0) {
		raw_free(ptr);
		return NULL;
	} else {
		return raw_realloc(ptr, nsize);
	}
}

int dump_mem_lua(lua_State *L) {
	int i;
	lua_newtable(L);
	for(i=0; i<SLOT_SIZE; i++) {
		mem_data* data = &mem_stats[i];
		if(data->handle != 0 && data->allocated != 0) {
			lua_pushinteger(L, data->allocated);
			lua_rawseti(L, -2, (lua_Integer)data->handle);
		}
	}
	return 1;
}

size_t malloc_current_memory(void) {
	uint32_t handle = SystemGetAllocHandleID();
	int i;
	for(i=0; i<SLOT_SIZE; i++) {
		mem_data* data = &mem_stats[i];
		if(data->handle == (uint32_t)handle && data->allocated != 0) {
			return (size_t) data->allocated;
		}
	}
	return 0;
}

void debug_memory(const char *info) {
	// for debug use
	uint32_t handle = SystemGetAllocHandleID();
	size_t mem = malloc_current_memory();
	fprintf(stderr, "[:%08x] %s %p\n", handle, info, (void *)mem);
}
