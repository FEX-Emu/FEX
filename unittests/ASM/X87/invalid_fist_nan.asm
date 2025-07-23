%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test FIST with NaN input = Invalid Operation (should set bit 0 of status word)
; Create NaN by computing 0.0 / 0.0
fldz
fldz
fdiv

; Try to convert NaN to integer - this should set Invalid Operation
lea rbx, [rel data]
fist dword [rbx]

fstsw ax
and rax, 1

hlt

align 8
data:
  dd 0
