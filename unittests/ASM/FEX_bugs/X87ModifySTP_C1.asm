%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000000",
    "RBX": "0x0000000000000000"
  }
}
%endif


fninit

fldenv [rel env]
fdecstp
fnstsw ax
and rax, 0x0200
mov r8, rax

fldenv [rel env]
fincstp
fnstsw ax
and rax, 0x0200 
mov rbx, rax
mov rax, r8

hlt

env:
    dd 0x037F ; FCW
    dd 0x3200 ; FSW: C1=1
    dd 0x0000 ; FTW
