CFLAGS	:= -Wall -Wextra -Wdeclaration-after-statement -Werror -pedantic -O2
CFLAGS  += -Iinclude -I../polarssl/include 

#justmiosixthings
ifeq ("$(KNX_ARCH)", "arm-miosix-eabi")
MAKEFILE_VERSION := 1.08
include ../../config/Makefile.inc
CFLAGS  += $(CFLAGS_BASE) 
endif

ifeq ("$(KNX_ARCH)","")
KNX_ARCH	:= x86
endif

ifneq ("$(KNX_ARCH)","x86")
CC 		:= $(KNX_ARCH)-gcc
CXX		:= $(KNX_ARCH)-g++
AR 		:= $(KNX_ARCH)-ar
LD 		:= $(KNX_ARCH)-ld
CFLAGS  += -std=c99
endif

BINDIR  := bin/$(KNX_ARCH)
OBJDIR  := obj/$(KNX_ARCH)
SRCS	:= $(wildcard src/*.c)
OBJS 	:= $(addprefix $(OBJDIR)/, \
				$(addsuffix .o, $(notdir $(basename $(SRCS)))))

all: $(BINDIR)/libsban.a

$(OBJDIR)/%.o: src/%.c
	@echo "[CC]  $<"
	@mkdir -p $(dir $@)
	@$(CC) -c $(CFLAGS) $< -o $@

$(BINDIR)/%.a: $(OBJS)
	@echo "[AR]  $@"
	@mkdir -p $(BINDIR)
	$(AR) rcs $@ $(OBJS)

clean:
	-rm -rf $(OBJS) $(BINDIR)/*.a

.PHONY: clean
