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

SRCEXTS :=.cpp .c .S
CFLAGS := $(RELEASEFLAG) -Wall -D__LINUX -I$(LUA_INC) -I$(CCBASIC_INC)
CPPFLAGS := $(RELEASEFLAG) -Wall -std=c++11 -D__LINUX -I$(LUA_INC) -I$(CCBASIC_INC)
INLIBS := $(LUA_STATICLIB) $(CCBASIC_STATICLIB) -pthread -lrt -ldl $(CCBASIC_PATH)/3rd/jemalloc/lib/libjemalloc_pic.a
OUTPUT_PATH := ./
COMPILEOBJDIR := ./obj/
COMPILESOURCE := coctx_swap.S libco_coroutine.cpp coroutineplus.cpp skynetplus_servermodule.cpp skynetplus_context.cpp skynetplus_handle.cpp skynetplus_module.cpp skynetplus_monitor.cpp skynetplus_mq.cpp skynetplus_timer.cpp skynetplusmain.cpp
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

compileall : UpdateSubModule $(LUA_STATICLIB) $(CCBASIC_STATICLIB)  $(OUTPUT_PATH)/skynetpp
compileskynet : $(OUTPUT_PATH)/skynetpp

clean :
	rm -rf obj
	rm $(OUTPUT_PATH)/skynetpp

cleanall: 
	cd $(LUA_PATH) && $(MAKE) clean
	rm -f $(LUA_STATICLIB)

	

