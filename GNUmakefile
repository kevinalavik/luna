.SUFFIXES:
.DELETE_ON_ERROR:

ARCH ?= x86_64

ifneq ($(ARCH),x86_64)
    $(error Only the x86_64 kernel port is currently available)
endif

QEMUFLAGS := -m 2G -debugcon stdio

override QEMU_MACHINE_FLAGS := -M q35
override LIMINE_EFI := BOOTX64.EFI BOOTIA32.EFI
override XORRISO_BIOS_FLAGS := -b boot/limine/limine-bios-cd.bin -no-emul-boot -boot-load-size 4 -boot-info-table
override SGDISK_MBR_FLAGS := -m 1
override QEMU_UEFI_FLAGS := \
    -drive if=pflash,unit=0,format=raw,file=edk2-ovmf-bins/ovmf-code-$(ARCH).fd,readonly=on

override IMAGE_NAME := luna-$(ARCH)

HDD_SIZE := 64

override HDD_HEADS := 64
override HDD_SECTORS_PER_TRACK := 32
override HDD_CYLINDER_SECTORS := $(shell echo $$(( $(HDD_HEADS) * $(HDD_SECTORS_PER_TRACK) )))

override HDD_PART_START := $(HDD_CYLINDER_SECTORS)
override HDD_PART_SECTORS := $(shell echo $$(( ($(HDD_SIZE) - 2) * $(HDD_CYLINDER_SECTORS) )))
override HDD_PART_END := $(shell echo $$(( $(HDD_PART_START) + $(HDD_PART_SECTORS) - 1 )))
override HDD_PART_OFFSET := $(shell echo $$(( $(HDD_PART_START) * 512 )))

HOST_CC := cc
HOST_CFLAGS := -g -O2 -pipe
HOST_CPPFLAGS :=
HOST_LDFLAGS :=
HOST_LIBS :=

.PHONY: all
all: $(IMAGE_NAME).iso

.PHONY: all-hdd
all-hdd: $(IMAGE_NAME).hdd

.PHONY: run
run: edk2-ovmf-bins $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		$(QEMU_MACHINE_FLAGS) \
		$(QEMU_UEFI_FLAGS) \
		-cdrom $(IMAGE_NAME).iso \
		$(QEMUFLAGS)

.PHONY: run-hdd
run-hdd: edk2-ovmf-bins $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		$(QEMU_MACHINE_FLAGS) \
		$(QEMU_UEFI_FLAGS) \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.PHONY: run-bios
run-bios: $(IMAGE_NAME).iso
	qemu-system-$(ARCH) \
		$(QEMU_MACHINE_FLAGS) \
		-cdrom $(IMAGE_NAME).iso \
		-boot d \
		$(QEMUFLAGS)

.PHONY: run-hdd-bios
run-hdd-bios: $(IMAGE_NAME).hdd
	qemu-system-$(ARCH) \
		$(QEMU_MACHINE_FLAGS) \
		-hda $(IMAGE_NAME).hdd \
		$(QEMUFLAGS)

.INTERMEDIATE: edk2-ovmf-bins.tar.gz
edk2-ovmf-bins.tar.gz:
	curl -fL -o $@ https://github.com/osdev0/edk2-ovmf-stable-bins/releases/latest/download/edk2-ovmf-bins.tar.gz

edk2-ovmf-bins: edk2-ovmf-bins.tar.gz
	rm -rf edk2-ovmf-bins
	gunzip < edk2-ovmf-bins.tar.gz | tar -xf -

.INTERMEDIATE: limine-binary.tar.gz
limine-binary.tar.gz:
	curl -fL -o $@ https://github.com/Limine-Bootloader/Limine/releases/latest/download/limine-binary.tar.gz

limine-binary/limine.c: limine-binary.tar.gz
	rm -rf limine-binary
	gunzip < limine-binary.tar.gz | tar -xf -
	touch -r limine-binary.tar.gz $@

override VARSFILES := $(addprefix limine-binary/,HOST_CC HOST_CFLAGS HOST_CPPFLAGS HOST_LDFLAGS HOST_LIBS)
override CHANGED_VARSFILES := $(foreach f,$(VARSFILES),$(shell printf '%s\n' '$(subst ','\'',$($(notdir $(f))))' | cmp -s - $(f) 2>/dev/null || echo $(f)))
.PHONY: FORCE
FORCE:
$(CHANGED_VARSFILES): FORCE
$(VARSFILES): limine-binary/limine.c
	@printf '%s\n' '$(subst ','\'',$($(notdir $@)))' > $@

limine-binary/limine: limine-binary/limine.c $(VARSFILES)
	rm -f $@
	$(MAKE) -C limine-binary \
		CC="$(HOST_CC)" \
		CFLAGS="$(HOST_CFLAGS)" \
		CPPFLAGS="$(HOST_CPPFLAGS)" \
		LDFLAGS="$(HOST_LDFLAGS)" \
		LIBS="$(HOST_LIBS)"

kernel/.deps-obtained:
	./kernel/get-deps

.PHONY: kernel
kernel: kernel/.deps-obtained
	$(MAKE) -C kernel

$(IMAGE_NAME).iso: limine-binary/limine kernel boot/limine.conf
	rm -rf iso_root
	mkdir -p iso_root/boot
	cp kernel/bin-$(ARCH)/kernel iso_root/boot/
	mkdir -p iso_root/boot/limine
	cp boot/limine.conf limine-binary/limine-uefi-cd.bin iso_root/boot/limine/
	cp limine-binary/limine-bios.sys limine-binary/limine-bios-cd.bin iso_root/boot/limine/
	mkdir -p iso_root/EFI/BOOT
	cp $(addprefix limine-binary/,$(LIMINE_EFI)) iso_root/EFI/BOOT/
	xorriso -as mkisofs -R -r -J $(XORRISO_BIOS_FLAGS) \
		-hfsplus -apm-block-size 2048 \
		--efi-boot boot/limine/limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		iso_root -o $(IMAGE_NAME).iso
	./limine-binary/limine bios-install $(IMAGE_NAME).iso
	rm -rf iso_root

$(IMAGE_NAME).hdd: limine-binary/limine kernel boot/limine.conf
	rm -f $(IMAGE_NAME).hdd
	dd if=/dev/null bs=1024k seek=$(HDD_SIZE) of=$(IMAGE_NAME).hdd
	PATH=$$PATH:/usr/sbin:/sbin sgdisk $(IMAGE_NAME).hdd -n 1:$(HDD_PART_START):$(HDD_PART_END) -t 1:ef00 $(SGDISK_MBR_FLAGS)
	./limine-binary/limine bios-install $(IMAGE_NAME).hdd
	mformat -i $(IMAGE_NAME).hdd@@$(HDD_PART_OFFSET) -T $(HDD_PART_SECTORS) -h $(HDD_HEADS) -s $(HDD_SECTORS_PER_TRACK) ::
	mmd -i $(IMAGE_NAME).hdd@@$(HDD_PART_OFFSET) ::/EFI ::/EFI/BOOT ::/boot ::/boot/limine
	mcopy -i $(IMAGE_NAME).hdd@@$(HDD_PART_OFFSET) kernel/bin-$(ARCH)/kernel ::/boot
	mcopy -i $(IMAGE_NAME).hdd@@$(HDD_PART_OFFSET) boot/limine.conf ::/boot/limine/limine.conf
	mcopy -i $(IMAGE_NAME).hdd@@$(HDD_PART_OFFSET) limine-binary/limine-bios.sys ::/boot/limine
	mcopy -i $(IMAGE_NAME).hdd@@$(HDD_PART_OFFSET) $(addprefix limine-binary/,$(LIMINE_EFI)) ::/EFI/BOOT

.PHONY: clean
clean:
	$(MAKE) -C kernel clean
	rm -rf iso_root $(IMAGE_NAME).iso $(IMAGE_NAME).hdd

.PHONY: distclean
distclean:
	$(MAKE) -C kernel distclean
	rm -rf iso_root *.iso *.hdd limine-binary limine-binary.tar.gz edk2-ovmf-bins edk2-ovmf-bins.tar.gz
