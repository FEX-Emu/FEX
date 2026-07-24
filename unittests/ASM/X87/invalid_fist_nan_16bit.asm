%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "0x8000"
  }
}
%endif

fldz
fldz
fdiv

fistp word [rel .dummy]

fstsw ax
and rax, 1

movzx ebx, word [rel .dummy]

hlt

align 4096
.dummy: dw 0
