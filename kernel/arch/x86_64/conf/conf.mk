ARCH_CFLAGS := \
    -m64 \
    -march=x86-64 \
    -mno-mmx \
    -mno-sse \
    -mno-red-zone \
    -mcmodel=kernel
ARCH_LDFLAGS := -Wl,-m,elf_x86_64
ARCH_NASMFLAGS := -f elf64
LINKER_SCRIPT := arch/$(ARCH)/conf/linker.lds

ifeq ($(call CC_HAS_COMPILE_FLAG,-fcf-protection=none),1)
    ARCH_CFLAGS += -fcf-protection=none
endif

ifeq ($(call CC_HAS_COMPILE_FLAG,-mno-80387),1)
    ARCH_CFLAGS += -mno-80387
endif
