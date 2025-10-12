%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  },
  "Mode": "32BIT"
}
%endif

; Test FIST with 16-bit overflow = Invalid Operation (should set bit 0 of status word)
; Create a large number that will overflow int16

; Load 2^30 (larger than int16 range: max int16 = 32767, 2^30 = 1073741824)
finit
fild dword [.thirty]
fld1
fscale

; Try to convert to int16 - this should overflow and be invalid
fistp word [.dummy]

fstsw ax
and eax, 1

hlt

align 4096
.thirty: dd 30
.dummy: dw 0
