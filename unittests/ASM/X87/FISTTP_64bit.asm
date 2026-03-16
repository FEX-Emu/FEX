%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "1"
  }
}
%endif

; Test FISTTP with 64-bit integer store
; FISTTP always truncates toward zero, ignoring rounding control word

finit
fld qword [rel .value]

; Convert to int64 using truncation - 1.9 should become 1, not 2
fisttp qword [rel .result]

fstsw ax
and rax, 1

; Load result to verify truncation worked
mov rbx, qword [rel .result]

hlt

align 4096
.value: dq 1.9
.result: dq 0