%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test FIST with value too large for 32-bit integer = Invalid Operation
; Load a large floating point value that exceeds INT32_MAX
lea edx, [.large_value]
fld tword [edx]

; Try to convert to 32-bit integer - should set Invalid Operation
lea ebx, [.data]
fist dword [ebx]

fstsw ax
and eax, 1

hlt

align 4096
.large_value:
  dt 1e20
.data:
  dd 0
