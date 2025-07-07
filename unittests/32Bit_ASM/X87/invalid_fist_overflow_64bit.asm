%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test FIST with 64-bit overflow = Invalid Operation (should set bit 0 of status word)
; Create a very large number that will overflow int64

; Load 2^75 (larger than int64 range)
finit
fild dword [.seventyfive]
fld1
fscale

; Try to convert to int64 - this should overflow and be invalid
fistp qword [.dummy]

fstsw ax
and eax, 1

hlt

.seventyfive: dd 75
.dummy: dq 0
