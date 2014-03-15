
TOP=../../workspace/devicedb-ng

OUTPUT_DIR=.

# replace with your own lib dirs... 
# twlib is here: https://github.com/WigWagCo/twlib
# libkj/capnp is here: http://kentonv.github.io/capnproto/install.html
# libuv is here: https://github.com/joyent/libuv

PREREQ_HEADERS=$(TOP)/expanded-prereqs/include
PREREQ_LIBS=$(TOP)/expanded-prereqs/lib
PREREQ_BIN=$(TOP)/expanded-prereqs/bin

CC=g++
GLIBCFLAG=-D_USING_GLIBC_  -std=c++11

ifdef RELEASE
	GEN_OPTIONS= -O2 
else
	GEN_OPTIONS= -g3 -O0 -fno-inline 
endif

LDFLAGS= -lkj -lkj-async -lcapnp -lcapnp-rpc -luv
LDFLAGS+= -lpthread -lTW  -L$(PREREQ_LIBS) $(TGT_LDFLAGS) # -ljansson

CFLAGS= $(GEN_OPTIONS) $(BUILD_OPTIONS) $(GLIBCFLAG) -DIZ_LINUX -D_TW_DEBUG -DZDB_JSONPARSER_DEBUG \
-DZDB_DEBUG_JSON -DZDB_DEBUG_DATATREE \
-I. \
-I$(PREREQ_HEADERS) -I/usr/include -D__DEBUG \
-I$(CPP_LIBS)

SRCS=

OBJS1= $(SRCS:%.cpp=$(OUTPUT_DIR)/%.o)
OBJS= $(OBJS1:%.cxx=$(OUTPUT_DIR)/%.o)

DEPENDS=$(SRCS:%.cpp=$(OUTPUT_DIR)/%.d) $(SRCS:%.cxx=$(OUTPUT_DIR)/%.d)

# look for source files using these extensions:
SRC_EXT= cxx cpp cc

define compile_rule
$(OUTPUT_DIR)/%.o: %.$1
	$(CC) $(CFLAGS) $(TGT_CFLAGS) $(LDFLAGS) -c $$< -o $$@
endef
$(foreach EXT,$(SRC_EXT),$(eval $(call compile_rule,$(EXT))))


testPool: testPool.o poolthread.h
	$(CC) $(CFLAGS) -I$(d) -o $(OUTPUT_DIR)/$@ testPool.o poolthread.h $(LDFLAGS)
