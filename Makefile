
CC   := gcc
CXX  := g++
LD   := $(CXX)

MSG_PREFIX ?=

$(info $(MSG_PREFIX)Using CC=$(CC))
$(info $(MSG_PREFIX)Using CXX=$(CXX))
$(info $(MSG_PREFIX)Using LD=$(LD))

PROG := abc

MODULES := \
	$(wildcard src/ext*) \
	src/base/abc src/base/abci src/base/cmd src/base/io src/base/main \
	src/base/ver src/base/wlc src/base/bac src/base/cba src/base/pla src/base/test \
	src/bdd/cudd src/bdd/dsd src/bdd/epd src/bdd/mtr src/bdd/parse \
	src/bdd/reo src/bdd/cas \
	src/map/mapper src/map/mio src/map/super src/map/if \
	src/map/amap src/map/cov src/map/scl src/map/mpm \
	src/misc/extra src/misc/mvc src/misc/st src/misc/util src/misc/nm \
	src/misc/vec src/misc/hash src/misc/tim src/misc/bzlib src/misc/zlib \
	src/misc/mem src/misc/bar src/misc/bbl \
	src/opt/cut src/opt/fxu src/opt/rwr src/opt/mfs src/opt/sim \
	src/opt/ret src/opt/res src/opt/lpk src/opt/nwk src/opt/rwt \
	src/opt/cgt src/opt/csw src/opt/dar src/opt/dau src/opt/sfm src/opt/mig src/opt/abl \
	src/sat/bsat src/sat/csat src/sat/msat src/sat/psat src/sat/cnf src/sat/bmc \
	src/bool/bdc src/bool/deco src/bool/dec src/bool/kit src/bool/lucky \
	src/bool/rsb src/bool/rpo \
	src/proof/pdr src/proof/abs src/proof/bbr src/proof/llb src/proof/live \
	src/proof/cec src/proof/dch src/proof/fraig src/proof/fra src/proof/ssw \
	src/proof/ssc src/proof/int \
	src/aig/aig src/aig/saig src/aig/gia src/aig/ioa src/aig/ivy src/aig/hop \
	src/aig/miniaig \
	src/python 

all: $(PROG)
default: $(PROG)

arch_flags : arch_flags.c
	$(CC) arch_flags.c -o arch_flags

ARCHFLAGS ?= $(shell $(CC) arch_flags.c -o arch_flags && ./arch_flags)
ARCHFLAGS := $(ARCHFLAGS)

OPTFLAGS  ?= -g -O #-DABC_NAMESPACE=xxx

CFLAGS    += -Wall -Wno-unused-function -Wno-write-strings -Wno-sign-compare $(OPTFLAGS) $(ARCHFLAGS) -Isrc
ifneq ($(findstring arm,$(shell uname -m)),)
	CFLAGS += -DABC_MEMALIGN=4
endif

# Set -Wno-unused-bug-set-variable for GCC 4.6.0 and greater only
ifneq ($(or $(findstring gcc,$(CC)),$(findstring g++,$(CC))),)
empty:=
space:=$(empty) $(empty)

GCC_VERSION=$(shell $(CC) -dumpversion)
GCC_MAJOR=$(word 1,$(subst .,$(space),$(GCC_VERSION)))
GCC_MINOR=$(word 2,$(subst .,$(space),$(GCC_VERSION)))

$(info $(MSG_PREFIX)Found GCC_VERSION $(GCC_VERSION))
ifeq ($(findstring $(GCC_MAJOR),0 1 2 3),)
$(info $(MSG_PREFIX)Found GCC_MAJOR>=4)
ifeq ($(findstring $(GCC_MINOR),0 1 2 3 4 5),)
$(info $(MSG_PREFIX)Found GCC_MINOR>=6)
CFLAGS += -Wno-unused-but-set-variable
endif
endif

endif

# LIBS := -ldl -lrt
LIBS += -ldl -lm
ifneq ($(findstring Darwin, $(shell uname)), Darwin)
   LIBS += -lrt
endif

ifneq ($(READLINE),0)
CFLAGS += -DABC_USE_READLINE
LIBS += -lreadline
endif

ifneq ($(PTHREADS),0)
CFLAGS += -DABC_USE_PTHREADS
LIBS += -lpthread
endif

$(info $(MSG_PREFIX)Using CFLAGS=$(CFLAGS))
CXXFLAGS += $(CFLAGS) 

SRC  := 
GARBAGE := core core.* *.stackdump ./tags $(PROG) arch_flags

.PHONY: all default tags clean docs

include $(patsubst %, %/module.make, $(MODULES))

OBJ := \
	$(patsubst %.cc, %.o, $(filter %.cc, $(SRC))) \
	$(patsubst %.cpp, %.o, $(filter %.cpp, $(SRC))) \
	$(patsubst %.c, %.o,  $(filter %.c, $(SRC)))  \
	$(patsubst %.y, %.o,  $(filter %.y, $(SRC))) 

DEP := $(OBJ:.o=.d)

# implicit rules

%.o: %.c
	@echo "$(MSG_PREFIX)\`\` Compiling:" $(LOCAL_PATH)/$<
	@$(CC) -c $(CFLAGS) $< -o $@

%.o: %.cc
	@echo "$(MSG_PREFIX)\`\` Compiling:" $(LOCAL_PATH)/$<
	@$(CXX) -c $(CXXFLAGS) $< -o $@

%.o: %.cpp
	@echo "$(MSG_PREFIX)\`\` Compiling:" $(LOCAL_PATH)/$<
	@$(CXX) -c $(CXXFLAGS) $< -o $@

%.d: %.c
	@echo "$(MSG_PREFIX)\`\` Generating dependency:" $(LOCAL_PATH)/$<
	@./depends.sh $(CC) `dirname $*.c` $(CFLAGS) $*.c > $@

%.d: %.cc
	@echo "$(MSG_PREFIX)\`\` Generating dependency:" $(LOCAL_PATH)/$<
	@./depends.sh $(CXX) `dirname $*.cc` $(CXXFLAGS) $*.cc > $@

%.d: %.cpp
	@echo "$(MSG_PREFIX)\`\` Generating dependency:" $(LOCAL_PATH)/$<
	@./depends.sh $(CXX) `dirname $*.cpp` $(CXXFLAGS) $*.cpp > $@

-include $(DEP)

# Actual targets

depend: $(DEP)

clean: 
	@echo "$(MSG_PREFIX)\`\` Cleaning up..."
	@rm -rvf $(PROG) lib$(PROG).a $(OBJ) $(GARBAGE) $(OBJ:.o=.d) 

tags:
	etags `find . -type f -regex '.*\.\(c\|h\)'`

$(PROG): $(OBJ)
	@echo "$(MSG_PREFIX)\`\` Building binary:" $(notdir $@)
	@$(LD) -o $@ $^ $(LIBS)

lib$(PROG).a: $(OBJ)
	@echo "$(MSG_PREFIX)\`\` Linking:" $(notdir $@)
	@ar rv $@ $?
	@ranlib $@

docs:
	@echo "$(MSG_PREFIX)\`\` Building documentation." $(notdir $@)
	@doxygen doxygen.conf
