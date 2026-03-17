%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "0xFFFF"
  }
}
%endif

; Test FISTTP with negative value - truncation toward zero
; -1.9 should become -1 (0xFFFF in 16-bit two's complement), not -2

finit
fld qword [rel .value]

fisttp word [rel .result]

fstsw ax
and rax, 1

movzx rbx, word [rel .result]

hlt

align 4096
.value: dq -1.9
.result: dw 0