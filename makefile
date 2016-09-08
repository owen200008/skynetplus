CC := gcc
CXX := g++
PLAT := linux

all : compileskynet

total : compileall

UpdateSubModule : 
	git submodule update --init

SKYNET_PATH := 3rd/skynet
LUA_PATH := 3rd/skynet/3rd/lua
LUA_STATICLIB := $(LUA_PATH)/liblua.a
LUA_INC := $(LUA_PATH)
$(LUA_STATICLIB) :
	cd $(LUA_PATH) && $(MAKE) CC='$(CC) -std=gnu99' $(PLAT)

JEMALLOC_PATH := 3rd/jemalloc
JEMALLOC_STATICLIB := $(JEMALLOC_PATH)/lib/libjemalloc_pic.a
JEMALLOC_INC := $(JEMALLOC_PATH)/include/jemalloc
$(JEMALLOC_STATICLIB) : $(JEMALLOC_PATH)/Makefile
	cd $(JEMALLOC_PATH) && $(MAKE) CC=$(CC)
$(JEMALLOC_PATH)/Makefile :
	cd $(JEMALLOC_PATH) && ./autogen.sh --with-jemalloc-prefix=je_ --disable-valgrind

SRCEXTS :=.cpp .c .S
CFLAGS := -Wall -gstabs+ -DUSE_JEMALLOC -D__LINUX -I$(JEMALLOC_INC) -I$(LUA_INC)
CPPFLAGS := -Wall -std=c++11 -gstabs+ -DUSE_JEMALLOC -D__LINUX -I$(JEMALLOC_INC) -I$(LUA_INC)
INLIBS := $(JEMALLOC_STATICLIB) $(LUA_STATICLIB) -pthread -lrt
OUTPUT_PATH := ./
COMPILEOBJDIR := ./obj/
COMPILESOURCE := coroutineplus.cpp public_threadlock.cpp main.cpp libco_coroutine.cpp coctx_swap.S
COMPILEOBJS := $(foreach x,$(SRCEXTS), $(patsubst %$(x),%.o,$(filter %$(x),$(COMPILESOURCE))))
COMPILEFULLOBJS := $(addprefix $(COMPILEOBJDIR),$(COMPILEOBJS))
vpath %.c ./coroutineplus ./public ./skynetplus
vpath %.cpp ./coroutineplus ./public ./skynetplus
vpath %.S ./coroutineplus
vpath %.o ${COMPILEOBJDIR}
config : 
	mkdir -p $(COMPILEOBJDIR)
%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $(COMPILEOBJDIR)$(notdir $@)
%.o : %.cpp
	$(CXX) -c $(CPPFLAGS) $< -o $(COMPILEOBJDIR)$(notdir $@)
%.o : %.S
	$(CC) -c $< -o $(COMPILEOBJDIR)$(notdir $@)

$(OUTPUT_PATH)/skynetpp : config $(COMPILEOBJS)
	$(CXX) -o $@ $(COMPILEFULLOBJS) $(INLIBS)

compileall : UpdateSubModule $(JEMALLOC_STATICLIB) $(LUA_STATICLIB) $(OUTPUT_PATH)/skynetplus
compileskynet : $(OUTPUT_PATH)/skynetpp

clean :
	rm $(OUTPUT_PATH)/skynetpp

cleanall: 
ifneq (,$(wildcard $(JEMALLOC_PATH)/Makefile))
	cd $(JEMALLOC_PATH) && $(MAKE) clean
endif
	cd $(LUA_PATH) && $(MAKE) clean
	rm -f $(LUA_STATICLIB)

	

