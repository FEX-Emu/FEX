%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "0xFFFFFFFFFFFFFFFF"
  }
}
%endif

; Test FISTTP with negative value - truncation toward zero
; -1.9 should become -1 (0xFFFFFFFFFFFFFFFF in 64-bit two's complement), not -2

finit
fld qword [rel .value]

fisttp qword [rel .result]

fstsw ax
and rax, 1

mov rbx, qword [rel .result]

hlt

align 4096
.value: dq -1.9
.result: dq 0