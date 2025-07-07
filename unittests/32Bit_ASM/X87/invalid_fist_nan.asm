%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test FIST with NaN input = Invalid Operation (should set bit 0 of status word)
; Create NaN by computing 0.0 / 0.0
fldz
fldz
fdiv

; Try to convert NaN to integer - this should set Invalid Operation
lea ebx, [.data]
fist dword [ebx]

fstsw ax
and eax, 1

hlt

.data:
  dd 0
