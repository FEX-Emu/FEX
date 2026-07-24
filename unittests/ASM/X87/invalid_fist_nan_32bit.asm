%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "0x80000000"
  }
}
%endif

fldz
fldz
fdiv

fistp dword [rel .dummy]

fstsw ax
and rax, 1

mov ebx, dword [rel .dummy]

hlt

align 4096
.dummy: dd 0
