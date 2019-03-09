CFLAGS	  := -Wall -Wextra -Wdeclaration-after-statement -O2

ifeq ("$(KNX_ARCH)","")
KNX_ARCH	:= x86
endif

#justmiosixthings
ifeq ("$(KNX_ARCH)", "arm-miosix-eabi")
MAKEFILE_VERSION := 1.08
KPATH := ../miosix-kernel/miosix
CONFPATH := ../../
include $(CONFPATH)/config/Makefile.inc
MIOSIX_INCLUDE := -I$(CONFPATH) -I$(CONFPATH)/config/$(BOARD_INC)          \
 				  -I$(KPATH) -I$(KPATH)/arch/common -I$(KPATH)/$(ARCH_INC) \
				  -I$(KPATH)/$(BOARD_INC) $(INCLUDE_DIRS) $(LFLAGS_BASE)
CFLAGS   += $(CFLAGS_BASE) $(MIOSIX_INCLUDE) -D_MIOSIX=1
CXXFLAGS += $(CXXFLAGS_BASE) $(MIOSIX_INCLUDE)
endif

CFLAGS    += -Iinclude/
CXXFLAGS  += -Iinclude/

BINDIR  := bin/$(KNX_ARCH)
OBJDIR  := obj/$(KNX_ARCH)
SRCS	:= $(wildcard src/*.c)
OBJS 	:= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(notdir $(basename $(SRCS)))))

# If arch != x86, remove some files and change compilers
ifneq ("$(KNX_ARCH)","x86")
CC 		:= $(KNX_ARCH)-gcc
CXX		:= $(KNX_ARCH)-g++
AR 		:= $(KNX_ARCH)-ar
LD 		:= $(KNX_ARCH)-ld
CFLAGS  += -std=c99
OBJS    := $(filter-out $(OBJDIR)/timing.o,$(OBJS))
OBJS    := $(filter-out $(OBJDIR)/net.o,$(OBJS))
endif

ifeq ("$(KNX_ARCH)","arm-miosix-eabi")
OBJS    += $(OBJDIR)/miosix_poll.o
endif


all: $(BINDIR)/libpolarssl.a

$(OBJDIR)/%.o: src/%.c
	@echo "[CC]  $<"
	@mkdir -p $(dir $@)
	@$(CC) -c $(CFLAGS) $< -o $@

$(OBJDIR)/%.o: src/%.cc
	@echo "[CXX] $<"
	@mkdir -p $(dir $@)
	@$(CXX) -c $(CXXFLAGS) $< -o $@

$(BINDIR)/%.a: $(OBJS)
	@echo "[AR]  $@"
	@mkdir -p $(BINDIR)
	@$(AR) rcs $@ $^

clean:
	-rm -rf $(OBJS) $(BINDIR)/*.a

.PHONY: clean
