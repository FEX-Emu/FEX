%ifdef CONFIG
{
  "RegData": {
    "RAX": "0",
    "RBX": "12346"
  },
  "Mode": "32BIT"
}
%endif

; Test FIST with valid 16-bit conversion
; Load a value that fits in int16 range

finit
fld qword [rel .value]

; Convert to int16 - this should work without overflow
fistp word [rel .result]

fstsw ax
and eax, 1

; Load the result to verify conversion worked
movzx ebx, word [rel .result]

hlt

align 4096
.value: dq 12345.75
.result: dw 0
