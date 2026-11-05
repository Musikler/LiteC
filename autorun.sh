#!/usr/bin/env bash
set -e
exec qemu-system-i386 \
    -drive format=raw,file=litec.img,if=floppy,cache=writeback \
    -drive format=raw,file=disk.img,if=ide,cache=writeback \
    -nographic -serial mon:stdio