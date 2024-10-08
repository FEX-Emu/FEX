%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x100",
    "RBX": "0x0",
    "RCX": "0x4000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

fld dword [rel positive]
ftst
fnstsw ax
and rax, 0x4700
mov rbx, rax

fldz
ftst
fnstsw ax
and rax, 0x4700
mov rcx, rax

fld dword [rel negative]
ftst
fnstsw ax
and rax, 0x4700

hlt

align 8
positive: dd 3.14159
negative: dd -2.71828