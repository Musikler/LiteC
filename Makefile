# ============================================================
#  LiteC (Bumblebee) — Makefile
# ============================================================
KERNEL_SECTORS = 128
CFLAGS        += -DKERNEL_SECTORS=$(KERNEL_SECTORS)
ASM      = nasm
CC       = gcc
LD       = ld
OBJCOPY  = objcopy

CFLAGS   = -m32 -ffreestanding -fno-pie -fno-stack-protector -fno-builtin \
           -nostdlib -Wall -Wextra -O2 -I.
LDFLAGS  = -m elf_i386 -T linker.ld

# ------------------------------------------------------------
# Objects that go into the kernel binary
# ------------------------------------------------------------
KOBJS = kernel/entry.o \
        kernel/kernel.o \
		kernel/sound.o \
        kernel/commands/beep.o \
        kernel/commands/shutdown.o \
        kernel/commands/calc.o \
        kernel/commands/writer.o \
        kernel/vga.o \
        kernel/serial.o \
        kernel/keyboard.o \
        kernel/string.o \
        kernel/fs.o \
        kernel/installer.o \
        kernel/commands/install.o \
        kernel/commands/reboot.o \
        kernel/commands/panic.o \
        kernel/panic.o \
        kernel/fb.o \
        kernel/gfx.o \
        kernel/console.o \
        kernel/mouse.o \
        kernel/desktop.o \
        kernel/lcs.o \
        kernel/commands/graphics-launch.o \
        kernel/commands/lcsm.o \
		kernel/commands/fsck.o \
        kernel/shell.o \
        kernel/ata.o \
        kernel/litefs.o \
        kernel/commands/help.o \
        kernel/commands/clear.o \
        kernel/commands/echo.o \
        kernel/commands/ls.o \
        kernel/commands/echof.o \
        kernel/commands/touch.o \
        kernel/commands/write.o \
        kernel/commands/rm.o \
        kernel/commands/sync.o \
        kernel/commands/format.o

# ------------------------------------------------------------
# Default target
# ------------------------------------------------------------
all: check litec.img

# ------------------------------------------------------------
# Sanity check: every .o must have a matching .c (or .asm)
# ------------------------------------------------------------
# ------------------------------------------------------------
# Sanity check: every .o must have a matching .c (or .asm)
# ------------------------------------------------------------
check:
	@missing=0; \
	for o in $(KOBJS); do \
	    src_c=$${o%.o}.c; \
	    src_s=$${o%.o}.asm; \
	    if [ ! -f "$$src_c" ] && [ ! -f "$$src_s" ]; then \
	        echo "MISSING SOURCE: neither $$src_c nor $$src_s exists"; \
	        missing=1; \
	    fi; \
	done; \
	if [ $$missing -eq 1 ]; then \
	    echo "Fix the missing files above, then re-run make."; \
	    exit 1; \
	fi; \
	echo "check: all sources present"

# ------------------------------------------------------------
# Font header must exist before the kernel can compile
# ------------------------------------------------------------
check-font:
	@[ -f kernel/font.h ] || { \
	    echo "kernel/font.h missing. Run:"; \
	    echo "  sudo apt install -y console-setup"; \
	    echo "  gunzip -c /usr/share/consolefonts/Lat15-Terminus16.psf.gz > /tmp/t.psf"; \
	    echo "  python3 tools/psf2inc.py /tmp/t.psf > kernel/font.h"; \
	    exit 1; \
	}

# ------------------------------------------------------------
# Bootloader
# ------------------------------------------------------------
boot/boot.bin: boot/boot.asm
	$(ASM) -f bin -o $@ $<

boot/stage2.bin: boot/stage2.asm
	$(ASM) -f bin -o $@ $<

# ------------------------------------------------------------
# Pattern rules
# ------------------------------------------------------------
kernel/%.o: kernel/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

kernel/%.o: kernel/%.asm
	$(ASM) -f elf32 -o $@ $<

kernel/commands/%.o: kernel/commands/%.c
	$(CC) $(CFLAGS) -c -o $@ $<

# ------------------------------------------------------------
# Kernel binary
# ------------------------------------------------------------
kernel.bin: $(KOBJS) linker.ld
	$(LD) $(LDFLAGS) -o kernel.elf $(KOBJS)
	$(OBJCOPY) -O binary kernel.elf kernel.bin
	@sz=$$(stat -c%s kernel.bin); rem=$$((sz % 512)); \
	 if [ $$rem -ne 0 ]; then \
	    dd if=/dev/zero bs=1 count=$$((512 - rem)) >> kernel.bin 2>/dev/null; \
	 fi

# ------------------------------------------------------------
# Floppy image (boot sector + stage2 + kernel)
# ------------------------------------------------------------
litec.img: boot/boot.bin boot/stage2.bin kernel.bin
	dd if=/dev/zero   of=litec.img bs=512 count=2880 status=none
	dd if=boot/boot.bin    of=litec.img conv=notrunc              status=none
	dd if=boot/stage2.bin  of=litec.img bs=512 seek=1 conv=notrunc status=none
	dd if=kernel.bin       of=litec.img bs=512 seek=5 conv=notrunc status=none

# ------------------------------------------------------------
# Data disk (2 MB, holds LiteFS)
# ------------------------------------------------------------
data.img:
	dd if=/dev/zero of=data.img bs=1M count=2 status=none

# ------------------------------------------------------------
# Run targets
# ------------------------------------------------------------
run: litec.img data.img
	qemu-system-i386 \
	    -drive format=raw,file=litec.img,if=floppy \
	    -drive format=raw,file=data.img,if=ide \
	    -nographic -serial mon:stdio
update-wipe:
	rm disk.img
	dd if=/dev/zero of=disk.img bs=1M count=128 status=none
	make clean
	make
	make run-gui-floppy
	make run-gui
run-gui-slow: litec.img data.img
	qemu-system-i386 \
	    -drive format=raw,file=litec.img,if=floppy \
	    -drive format=raw,file=data.img,if=ide \
	    -display curses -monitor none
run-gui: litec.img data.img
	qemu-system-i386 \
		-smp 4 \
		-m 2048 \
		-device VGA,vgamem_mb=64 \
	    -drive format=raw,file=data.img,if=ide \
	    -display curses -monitor none
check-kernel-size: kernel.bin
	@sz=$$(stat -c%s kernel.bin); cap=$$((KERNEL_SECTORS * 512)); \
	if [ $$sz -gt $$cap ]; then \
	    echo "ERROR: kernel.bin ($$sz bytes) exceeds $$cap (KERNEL_SECTORS=$(KERNEL_SECTORS))"; \
	    echo "Increase KERNEL_SECTORS in the Makefile."; \
	    exit 1; \
	fi
# Reset the data disk (erases all files)
wipe-data:
	rm -f data.img
boot/boothdd.bin: boot/boothdd.asm
	$(ASM) -f bin -o $@ $<

boot/stage2hdd.bin: boot/stage2hdd.asm
	$(ASM) -f bin -D KERNEL_SECTORS=$(KERNEL_SECTORS) -o $@ $<

kernel/boothdd_bytes.h: boot/boothdd.bin
	xxd -i -n boot_boothdd_bin $< > $@
kernel/installer.o: kernel/boothdd_bytes.h kernel/stage2hdd_bytes.h
kernel/stage2hdd_bytes.h: boot/stage2hdd.bin
	xxd -i -n boot_stage2hdd_bin $< > $@

kernel/installer.o: kernel/boothdd_bytes.h kernel/stage2hdd_bytes.h
formatDisk:
	rm -f disk.img
	rm -f data.img
	dd if=/dev/zero of=disk.img bs=1M count=128 status=none
# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------
clean:
	rm -f kernel.bin kernel.elf boot/*.bin litec.img data.img
	rm -f kernel/*.o kernel/commands/*.o

# Rebuild from scratch
rebuild: clean all
run-gui-floppy: litec.img disk.img
	GDK_BACKEND=x11 LIBGL_ALWAYS_SOFTWARE=1 \
	qemu-system-i386 \
	    -drive format=raw,file=litec.img,if=floppy,cache=writeback \
	    -drive format=raw,file=disk.img,if=ide,cache=writeback \
	    -display gtk
run-gui: disk.img
	GDK_BACKEND=x11 LIBGL_ALWAYS_SOFTWARE=1 \
	qemu-system-i386 \
	    -drive format=raw,file=disk.img,if=ide,cache=writeback \
	    -display gtk
.PHONY: all check run run-gui wipe-data clean rebuild