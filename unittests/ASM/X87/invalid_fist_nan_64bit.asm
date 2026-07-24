%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "0x8000000000000000"
  }
}
%endif

fldz
fldz
fdiv

fistp qword [rel .dummy]

fstsw ax
and rax, 1

mov rbx, qword [rel .dummy]

hlt

align 4096
.dummy: dq 0
