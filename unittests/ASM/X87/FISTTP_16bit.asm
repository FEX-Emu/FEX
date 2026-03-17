%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "1"
  }
}
%endif

; Test FISTTP with 16-bit integer store
; FISTTP always truncates toward zero, ignoring rounding control word

finit
fld qword [rel .value]

; Convert to int16 using truncation - 1.9 should become 1, not 2
fisttp word [rel .result]

fstsw ax
and rax, 1

; Load result to verify truncation worked
movzx rbx, word [rel .result]

hlt

align 4096
.value: dq 1.9
.result: dw 0