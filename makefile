CC := gcc
CXX := g++
PLAT := linux

release := 0
ifeq ($(release),1)
RELEASEFLAG := -O2
else
RELEASEFLAG := -gstabs+
endif

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

CCBASIC_PATH := 3rd/ccbasic
CCBASIC_STATICLIB := $(CCBASIC_PATH)/lib/basiclib.a
CCBASIC_INC := $(CCBASIC_PATH)/src/inc
$(CCBASIC_STATICLIB) : 
	cd $(CCBASIC_PATH)/lib/linux && $(MAKE)
	

JEMALLOC_PATH := 3rd/jemalloc
JEMALLOC_STATICLIB := $(JEMALLOC_PATH)/lib/libjemalloc_pic.a
JEMALLOC_INC := $(JEMALLOC_PATH)/include/jemalloc
$(JEMALLOC_STATICLIB) : $(JEMALLOC_PATH)/Makefile
	cd $(JEMALLOC_PATH) && $(MAKE) CC=$(CC)
$(JEMALLOC_PATH)/Makefile :
	cd $(JEMALLOC_PATH) && ./autogen.sh --with-jemalloc-prefix=je_ --disable-valgrind

SRCEXTS :=.cpp .c .S
CFLAGS := $(RELEASEFLAG) -Wall -DUSE_3RRD_MALLOC -D__LINUX -I$(JEMALLOC_INC) -I$(LUA_INC)
CPPFLAGS := $(RELEASEFLAG) -Wall -std=c++11 -DUSE_3RRD_MALLOC -D__LINUX -I$(JEMALLOC_INC) -I$(LUA_INC)
INLIBS := $(JEMALLOC_STATICLIB) $(LUA_STATICLIB) -pthread -lrt -ldl
OUTPUT_PATH := ./
COMPILEOBJDIR := ./obj/
COMPILESOURCE := coctx_swap.S libco_coroutine.cpp coroutineplus.cpp malloc_hook.cpp malloc_3rd.cpp skynetplus_servermodule.cpp skynetplusmain.cpp
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

compileall : UpdateSubModule $(JEMALLOC_STATICLIB) $(LUA_STATICLIB) $(CCBASIC_STATICLIB)  $(OUTPUT_PATH)/skynetpp
compileskynet : $(OUTPUT_PATH)/skynetpp

clean :
	rm -rf obj
	rm $(OUTPUT_PATH)/skynetpp

cleanall: 
ifneq (,$(wildcard $(JEMALLOC_PATH)/Makefile))
	cd $(JEMALLOC_PATH) && $(MAKE) clean
endif
	cd $(LUA_PATH) && $(MAKE) clean
	rm -f $(LUA_STATICLIB)

	

