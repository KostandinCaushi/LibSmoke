KNX_ARCHS := x86 arm-miosix-eabi
SUBPRJ := libs src tests
MIOSIX_AVAILABLE := $(shell command -v arm-miosix-eabi-gcc 2> /dev/null)


# WORST HACK EVER
define \n


endef

all: checkmiosix
	$(foreach i,$(KNX_ARCHS), $(foreach j,$(SUBPRJ), \
		@echo "[MAKE] Jumping into $(j) using KNX_ARCH=$(i)" ${\n} \
		@$(MAKE) KNX_ARCH=$(i) -C $(j) ${\n} \
	))

clean:
	$(foreach i,$(KNX_ARCHS), $(foreach j,$(SUBPRJ), \
		@echo "[MAKE] Jumping into $(j) using KNX_ARCH=$(i)" ${\n} \
		@$(MAKE) KNX_ARCH=$(i) -C $(j) clean ${\n} \
	))

checkmiosix:
ifndef MIOSIX_AVAILABLE
	$(error "Miosix toolchain is not available in this system. Please download it from https://miosix.org/wiki/index.php?title=Miosix_Toolchain")
endif

.PHONY: clean
