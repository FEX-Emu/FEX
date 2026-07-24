%ifdef CONFIG
{
  "RegData": {
    "RAX": "1",
    "RBX": "0x80000000"
  }
}
%endif

; Test FIST with 32-bit overflow = Invalid Operation (should set bit 0 of status word)
; Create a very large number that will overflow int32

; Load 2^50 (larger than int32 range)
finit
fild dword [rel .fifty]
fld1
fscale

; Try to convert to int32 - this should overflow and be invalid
fistp dword [rel .dummy]

fstsw ax
and rax, 1

mov ebx, dword [rel .dummy]

hlt

align 4096
.fifty: dq 50
.dummy: dd 0
