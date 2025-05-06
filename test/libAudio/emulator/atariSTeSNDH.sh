#!/bin/sh

m68k-none-elf-as -pic --pcrel --register-prefix-optional -o atariSTe.sndh.o atariSTeSNDH.s
m68k-none-elf-ld -r -static -o atariSTe.sndh.elf atariSTe.sndh.o
m68k-none-elf-objcopy -Obinary atariSTe.sndh.elf atariSTe.sndh

rm atariSTe.sndh.o atariSTe.sndh.elf
