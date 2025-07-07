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
fld qword [.value]

; Convert to int16 - this should work without overflow
fistp word [.result]

fstsw ax
and eax, 1

; Load the result to verify conversion worked
movzx ebx, word [.result]

hlt

.value: dq 12345.75
.result: dw 0
