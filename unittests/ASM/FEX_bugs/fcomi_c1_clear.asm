%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x0000000000000000"
  }
}
%endif

fninit

fld qword [rel two]
fld qword [rel one]

fldenv [rel env]

fcomi st1

fnstsw ax
and rax, 0x0200 ; extract C1

hlt

one:
    dq 1.0
two:
    dq 2.0

env:
    dd 0x037F ; FCW
    dd 0x3200 ; FSW: C1=1
    dd 0x0000 ; FTW
