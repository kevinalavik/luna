.SUFFIXES:

.DELETE_ON_ERROR:

override OUTPUT := kernel

ARCH ?= x86_64

PREFIX := /usr/local

TOOLCHAIN :=
TOOLCHAIN_PREFIX :=
ifneq ($(TOOLCHAIN),)
    ifeq ($(TOOLCHAIN_PREFIX),)
        TOOLCHAIN_PREFIX := $(TOOLCHAIN)-
    endif
endif

ifneq ($(TOOLCHAIN_PREFIX),)
    CC := $(TOOLCHAIN_PREFIX)gcc
else
    CC := cc
endif

ifeq ($(TOOLCHAIN),llvm)
    CC := clang
endif

CFLAGS := -g -O2 -pipe

CPPFLAGS :=

NASMFLAGS := -g

LDFLAGS :=

ifneq ($(filter-out clean distclean,$(or $(MAKECMDGOALS),default)),)
    ifeq ($(wildcard .deps-obtained),)
        $(error Please run the ./get-deps script first)
    endif
endif

ifneq ($(filter-out clean distclean,$(or $(MAKECMDGOALS),default)),)
    override TOOLCHAIN_ERROR = $(error $(1))
else
    override TOOLCHAIN_ERROR =
endif

override CC_IS_CLANG := $(shell ! $(CC) --version 2>/dev/null | grep -q '^Target: '; echo $$?)

override FLAG_NAME = -$(subst -,-?,$(patsubst -%,%,$(firstword $(subst =, ,$(1)))))
override FLAG_IS_NAMED = grep -qE '(error|warning|note):.*[^[:alnum:]]($(call FLAG_NAME,$(1))$(if $(2),|$(call FLAG_NAME,$(2))))([^-[:alnum:]]|$$)'

override CC_HAS_COMPILE_FLAG = $(shell LC_ALL=C $(CC) $(CFLAGS) $(1) -c -x c /dev/null -o /dev/null 2>&1 >/dev/null | $(call FLAG_IS_NAMED,$(1),$(2)); echo $$?)
override CC_HAS_LINK_FLAG = $(shell LC_ALL=C $(CC) $(CFLAGS) $(LDFLAGS) $(1) -Wl,--version 2>&1 >/dev/null | $(call FLAG_IS_NAMED,$(1),$(2)); echo $$?)

override LD_HAS_FLAG = $(shell ! LC_ALL=C $(CC) $(CFLAGS) $(LDFLAGS) -Wl,--help 2>/dev/null | grep -qE '(^|[[:space:]])-?$(1)([[:space:],=[]|$$)'; echo $$?)

ARCH_CONF := arch/$(ARCH)/conf/conf.mk
ifeq ($(wildcard $(ARCH_CONF)),)
    $(error Architecture $(ARCH) is not configured (expected $(ARCH_CONF)))
endif
include $(ARCH_CONF)

override CFLAGS += \
    -Wall \
    -Wextra \
    -std=gnu11 \
    -ffreestanding \
    -fno-common \
    -fno-stack-protector \
    -fno-stack-check \
    -fno-PIC \
    -ffunction-sections \
    -fdata-sections

override CPPFLAGS := \
    -nostdinc \
    -I include \
    -I arch/$(ARCH)/include \
    -I limine-protocol/include \
    -isystem freestanding-c-hdrs/include \
    $(CPPFLAGS) \
    -MMD \
    -MP

override CFLAGS += $(ARCH_CFLAGS)
override NASMFLAGS := $(ARCH_NASMFLAGS) $(patsubst -g,-g -F dwarf,$(NASMFLAGS)) -i include/ -Wall

override LDFLAGS := \
    -Wl,--build-id=sha1 \
    $(LDFLAGS)

ifeq ($(CC_IS_CLANG),1)
    override CC += \
        -target $(ARCH)-unknown-none-elf
    override LDFLAGS := \
        -fuse-ld=lld \
        $(LDFLAGS)
endif

override LDFLAGS += \
    -nostdlib \
    -static \
    -Wl,-z,max-page-size=0x1000 \
    -Wl,-z,noexecstack \
    -Wl,--gc-sections \
    -Wl,-T,$(LINKER_SCRIPT) \
    $(ARCH_LDFLAGS)

override CC_COMPILE_ERROR := $(shell LC_ALL=C $(CC) $(CFLAGS) -c -x c /dev/null -o /dev/null 2>&1 >/dev/null | grep -i 'error:' | head -n 1)
ifneq ($(CC_COMPILE_ERROR),)
    $(call TOOLCHAIN_ERROR,The compiler does not work with the given CFLAGS: $(CC_COMPILE_ERROR))
endif

ifneq ($(CC_IS_CLANG),1)
    override CC_EMITS_ELF := $(shell ! $(CC) $(CFLAGS) -dM -E -x c /dev/null 2>/dev/null | grep -q 'define __ELF__ 1'; echo $$?)
    ifneq ($(CC_EMITS_ELF),1)
        $(call TOOLCHAIN_ERROR,The compiler does not emit ELF objects)
    endif
endif

ifeq ($(CC_IS_CLANG),1)
    override CLANG_PRINT_COMMANDS := -\#\#\#
    override CC_LINKS_VIA_DRIVER = $(shell ! $(CC) $(CFLAGS) $(LDFLAGS) -Wl,--version $(CLANG_PRINT_COMMANDS) 2>&1 | grep '^ "' | tail -n 1 | grep -q -- '-Wl,'; echo $$?)

    ifeq ($(CC_LINKS_VIA_DRIVER),1)
        override LDFLAGS += \
            -target $(ARCH)-linux-gnu
        ifeq ($(call CC_HAS_COMPILE_FLAG,--no-default-config),1)
            override LDFLAGS += \
                --no-default-config
        endif

        ifeq ($(CC_LINKS_VIA_DRIVER),1)
            $(call TOOLCHAIN_ERROR,The compiler still hands the link over to another compiler driver)
        endif
    endif
endif

override CC_LINK_ERROR := $(shell LC_ALL=C $(CC) $(CFLAGS) $(LDFLAGS) -Wl,--version 2>&1 >/dev/null | grep -i 'error:' | head -n 1)
ifneq ($(CC_LINK_ERROR),)
    $(call TOOLCHAIN_ERROR,The linker does not work with the given LDFLAGS: $(CC_LINK_ERROR))
endif

override CC_FUSE_LD := $(lastword $(filter -fuse-ld=%,$(CFLAGS) $(LDFLAGS)))
ifneq ($(CC_FUSE_LD),)
    override CC_FUSE_LD_NAME := $(firstword $(subst -, ,$(patsubst ld.%,%,$(notdir $(patsubst -fuse-ld=%,%,$(CC_FUSE_LD))))))
    override CC_LD_BANNER_NAME := $(patsubst bfd,ld,$(CC_FUSE_LD_NAME))
    override CC_LD_VERSION := $(shell LC_ALL=C $(CC) $(CFLAGS) $(LDFLAGS) -Wl,--version 2>/dev/null | head -n 1)
    override CC_RUNS_FUSE_LD := $(shell ! printf '%s\n' '$(subst ',,$(CC_LD_VERSION))' | grep -qiE '(^|[^[:alnum:]])$(CC_LD_BANNER_NAME)([^[:alnum:]]|$$)'; echo $$?)
    ifneq ($(CC_RUNS_FUSE_LD),1)
        $(call TOOLCHAIN_ERROR,The compiler ignores $(CC_FUSE_LD) and runs another linker: $(CC_LD_VERSION))
    endif
endif

ifeq ($(call CC_HAS_COMPILE_FLAG,-fno-stack-clash-protection),1)
    override CFLAGS += \
        -fno-stack-clash-protection
endif

ifeq ($(call CC_HAS_LINK_FLAG,-no-pie),1)
    override LDFLAGS += \
        -no-pie
endif

ifeq ($(call LD_HAS_FLAG,-no-pie),1)
    override LDFLAGS += \
        -Wl,-no-pie
endif

override SRCFILES := $(shell find -L boot lib sys cc-runtime/src -type f 2>/dev/null | LC_ALL=C sort)
override SRCFILES += $(shell find -L arch/$(ARCH) -type f \( -name '*.c' -o -name '*.S' -o -name '*.asm' \) 2>/dev/null | LC_ALL=C sort)
override CFILES := $(filter %.c,$(SRCFILES))
override ASFILES := $(filter %.S,$(SRCFILES))
override OBJ := $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.o) $(ASFILES:.S=.S.o))
override HEADER_DEPS := $(addprefix obj-$(ARCH)/,$(CFILES:.c=.c.d) $(ASFILES:.S=.S.d))
override NASMFILES := $(filter %.asm,$(SRCFILES))
override OBJ += $(addprefix obj-$(ARCH)/,$(NASMFILES:.asm=.asm.o))
override HEADER_DEPS += $(addprefix obj-$(ARCH)/,$(NASMFILES:.asm=.asm.d))

.PHONY: all
all: bin-$(ARCH)/$(OUTPUT)

-include $(HEADER_DEPS)

override VARSFILES := $(addprefix obj-$(ARCH)/,CC CFLAGS CPPFLAGS LDFLAGS)
override VARSFILES += obj-$(ARCH)/NASMFLAGS
$(foreach f,$(VARSFILES),$(eval $(f): override $(notdir $(f)) := $$($(notdir $(f)))))

override CHANGED_VARSFILES := $(foreach f,$(VARSFILES),$(shell printf '%s\n' '$(subst ','\'',$($(notdir $(f))))' | cmp -s - $(f) 2>/dev/null || echo $(f)))
.PHONY: FORCE
FORCE:
$(CHANGED_VARSFILES): FORCE
$(VARSFILES):
	@mkdir -p "$(dir $@)"
	@printf '%s\n' '$(subst ','\'',$($(notdir $@)))' > $@

bin-$(ARCH)/$(OUTPUT): GNUmakefile $(ARCH_CONF) $(LINKER_SCRIPT) obj-$(ARCH)/CC obj-$(ARCH)/CFLAGS obj-$(ARCH)/LDFLAGS $(OBJ)
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJ) -o $@

obj-$(ARCH)/lib/memory.c.o obj-$(ARCH)/cc-runtime/src/cc-runtime.c.o: override CFLAGS += -fno-lto

obj-$(ARCH)/%.c.o: %.c GNUmakefile obj-$(ARCH)/CC obj-$(ARCH)/CFLAGS obj-$(ARCH)/CPPFLAGS
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj-$(ARCH)/%.S.o: %.S GNUmakefile obj-$(ARCH)/CC obj-$(ARCH)/CFLAGS obj-$(ARCH)/CPPFLAGS
	mkdir -p "$(dir $@)"
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

obj-$(ARCH)/%.asm.o: %.asm GNUmakefile obj-$(ARCH)/NASMFLAGS
	mkdir -p "$(dir $@)"
	nasm $(NASMFLAGS) -MD $(@:.o=.d) -MP $< -o $@

.PHONY: clean
clean:
	rm -rf bin-$(ARCH) obj-$(ARCH)

.PHONY: distclean
distclean:
	rm -rf bin-* obj-* .deps-obtained .cache compile_commands.json freestanding-c-hdrs cc-runtime limine-protocol

.PHONY: install
install: all
	mkdir -p "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)"
	cp bin-$(ARCH)/$(OUTPUT) "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH)"
	chmod 644 "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH)"

.PHONY: uninstall
uninstall:
	rm -f "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)/$(OUTPUT)-$(ARCH)"
	-rmdir "$(DESTDIR)$(PREFIX)/share/$(OUTPUT)"
