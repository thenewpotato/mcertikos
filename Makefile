#
# Top-level Makefile for certikos64
#

ifndef V
V		:= @
else
V		:=
endif

ARCH		:= i386

# Directories
TOP		:= .
SRCDIR		:= $(TOP)
OBJDIR		:= $(TOP)/obj
UTILSDIR	:= $(TOP)/misc
TESTDIR		:= $(TOP)/test
OBJDIRS		:=

# Compiler and Linker
CC		:= gcc
LD		:= ld
CFLAGS		:= -MD -Wall -Werror -Wno-strict-aliasing -Wno-unused-function -pipe -fno-builtin -nostdinc -fno-stack-protector
LDFLAGS		:= -nostdlib

ifdef ENABLE_CCOMP
CCOMP		:= ccomp
CCOMP_CFLAGS	:= -finline-asm -fpacked-structs -flonglong -D_CCOMP_

# Uncomment following two lines when you suspect differences between gcc and
# compcert cause problems.

#CCOMP		:= gcc
#CCOMP_CFLAGS	:= -MD -Wall -Werror -Wno-strict-aliasing -Wno-unused-function -pipe -fno-builtin -nostdinc -fno-stack-protector -m32 -D_CCOMP_
endif

# other tools
PERL		:= perl
OBJDUMP		:= objdump
OBJCOPY		:= objcopy
DD		:= dd
NM		:= nm
CSCOPE		:= cscope

# others
GCC_LIB32 := $(shell $(CC) $(CFLAGS) -m32 -print-libgcc-file-name)
ifeq ($(ARCH), amd64)
GCC_LIB64 := $(shell $(CC) $(CFLAGS) -m64 -print-libgcc-file-name)
endif

# If this is the first time building CertiKOS64, please follow the instructions
# in HOW_TO_MAKE_DISK_IMAGE to create a disk image file manually and put it in
# directory $(OBJDIR)/ (default: obj/)
CERTIKOS_IMG	:= $(OBJDIR)/certikos.img

# bochs
BOCHS		:= bochs
BOCHS_OPT	:= -q

# qemu
QEMU		:= qemu-system-x86_64
QEMUOPTS	:= -smp 4 -hda $(CERTIKOS_IMG) -serial mon:stdio -m 256 -k en-us
QEMUOPTS_KVM	:= -cpu host -enable-kvm
QEMUOPTS_BIOS	:= -L $(UTILSDIR)/qemu/

# Targets

.PHONY: all boot dev kern lib sys user deps


ifndef ENABLE_CCOMP
all: boot sys user
	@echo "All targets are done."
else
# Compcert dones't support -MD, so we have to clean all the already generated
# files.
all: clean boot sys user
	@echo "All targets are done."
endif

install_img: install_boot install_sys install_user
	@echo "CertiKOS is installed on the disk image."

bochs: $(CERTIKOS_IMG) .bochsrc
	@echo + start bochs
	$(V)$(BOCHS) $(BOCHS_OPT)

qemu: $(CERTIKOS_IMG)
	$(V)$(QEMU) $(QEMUOPTS)

qemu-kvm: $(CERTIKOS_IMG)
	$(V)$(QEMU) $(QEMUOPTS) $(QEMUOPTS_KVM)

qemu-bios: $(CERTIKOS_IMG)
	$(V)$(QEMU) $(QEMUOPTS) $(QEMUOPTS_BIOS)

iso: all
	$(V)cp $(OBJDIR)/sys/kernel $(UTILSDIR)/iso/boot/kernel
	$(V)grub-mkrescue -o $(CERTIKOS_IMG) $(UTILSDIR)/iso

package:
	$(V)tar czf ../certikos.tar.gz --exclude=obj --exclude=cscope.* .

cscope:
	$(V)rm -rf cscope.*
	$(V)find . -name "*.[chsS]" > cscope.files
	$(V)cscope -bkq -i cscope.files

# Sub-makefiles
include boot/Makefile.inc
include user/Makefile.inc
include sys/Makefile.inc

deps: $(OBJDIR)/.deps

$(OBJDIR)/.deps: $(foreach dir, $(OBJDIRS), $(wildcard $(dir)/*.d))
	$(V)mkdir -p $(@D)
	$(V)$(PERL) $(UTILSDIR)/mergedep.pl $@ $^

-include $(OBJDIR)/.deps

clean:
	$(V)rm -rf $(OBJDIR)
