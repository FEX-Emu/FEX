%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test FIST with 32-bit overflow = Invalid Operation (should set bit 0 of status word)
; Create a large number that will overflow int32

; Load 2^40 (larger than int32 range: max int32 = 2147483647, 2^40 = 1099511627776)
finit
fild dword [.forty]
fld1
fscale

; Try to convert to int32 - this should overflow and be invalid
fistp dword [.dummy]

fstsw ax
and eax, 1

hlt

.forty: dd 40
.dummy: dd 0
