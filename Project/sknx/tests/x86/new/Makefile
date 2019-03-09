LIBDIR  := ../../../libs
BINDIR  := ../../../bin/x86/tests
OBJDIR  := ../../../obj/x86/tests

CFLAGS 	:= -I$(LIBDIR)/polarssl/include -I$(LIBDIR)/sban/include
CFLAGS  += -Wall -Wextra -Werror -g -I ../../../src/shared
LDFLAGS := -lpthread \
	$(LIBDIR)/sban/bin/x86/libsban.a \
	$(LIBDIR)/polarssl/bin/x86/libpolarssl.a

SRCS	:= $(wildcard *.cc)
OBJS 	:= $(addprefix $(OBJDIR)/, $(addsuffix .o, $(basename $(SRCS))))
BINS 	:= $(addprefix $(BINDIR)/, $(addsuffix .bin, $(basename $(SRCS))))

all: $(BINS)

$(BINDIR)/%.bin: $(OBJDIR)/%.o
	@echo "[LD]  $@"
	@mkdir -p $(BINDIR)
	@$(CXX) -o $@ $< $(LDFLAGS)

$(OBJDIR)/%.o: %.cc
	@echo "[CC]  $<"
	@mkdir -p $(dir $@)
	@$(CXX) -c $(CFLAGS) $< -o $@

clean:
	-rm -rf $(BINDIR)/*.bin $(OBJDIR)/*.o

.PHONY: clean
