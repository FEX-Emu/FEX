%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "0xFFFFFFFF"
  }
}
%endif

; Test FISTTP with negative value - truncation toward zero
; -1.9 should become -1 (0xFFFFFFFF in 32-bit two's complement), not -2

finit
fld qword [rel .value]

fisttp dword [rel .result]

fstsw ax
and rax, 1

mov ebx, dword [rel .result]

hlt

align 4096
.value: dq -1.9
.result: dd 0