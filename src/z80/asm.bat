gcc -DX86_ASM -DLSB_FIRST -DNEW_INTERRUPT_SYSTEM=0 -I.. -I..\msdos -O3 -mpentium -fomit-frame-pointer -S z80.c
