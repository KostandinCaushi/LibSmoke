all:
ifeq ($(KNX_ARCH),x86)
	$(MAKE) -C x86
endif

ifeq ($(KNX_ARCH),arm-miosix-eabi)
	$(MAKE) -C miosix
endif

clean:
ifeq ($(KNX_ARCH),x86)
	$(MAKE) -C x86 clean
endif

ifeq ($(KNX_ARCH),arm-miosix-eabi)
	$(MAKE) -C miosix clean
endif
	
.PHONY: clean
