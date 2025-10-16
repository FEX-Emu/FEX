%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test FIST with value too large for 32-bit integer = Invalid Operation
; Load a large floating point value that exceeds INT32_MAX
lea rdx, [rel large_value]
fld tword [rdx]

; Try to convert to 32-bit integer - should set Invalid Operation
lea rbx, [rel data]
fist dword [rbx]

fstsw ax
and rax, 1

hlt

align 4096
large_value:
  dt 1e20
data:
  dd 0
