/*
not useable in real mode
lar lldt lslltr sldt str verr verw

registers

cr0 32? bit (machine status word
gdt 32? bit base, 16 bit limit (global descriptor table)
idt interrupt descriptor table
ldt local descriptor table
task register

arpl
 reg,reg
 mem,reg
clts (cr0 &=~8)
lar
 reg,reg
 reg,mem
lgdt mem
 mem points to memory: 2 bytes limit, 4 bytes linear address
lidt mem
lldt mem
lsl 
 reg,reg
 reg,mem
lmsw 
 mem
 reg
ltr
 reg
 mem
sgdt mem
sidt mem
sldt 
 reg
 mem
smsw
 reg
 mem
str
 reg
 mem
verr
 reg
 mem
verw
 reg
 mem
 */
