%ifdef CONFIG
{
  "RegData": {
    "RAX": "1"
  }
}
%endif

; Test FIST with 16-bit overflow = Invalid Operation (should set bit 0 of status word)
; Create a large number that will overflow int16

; Load 2^30 (larger than int16 range: max int16 = 32767, 2^30 = 1073741824)
finit
fild dword [rel .thirty]
fld1
fscale

; Try to convert to int16 - this should overflow and be invalid
fistp word [rel .dummy]

fstsw ax
and rax, 1

hlt

align 4096
.thirty: dq 30
.dummy: dw 0
