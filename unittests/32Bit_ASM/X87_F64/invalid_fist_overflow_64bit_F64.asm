%ifdef CONFIG
{
  "RegData": {
    "RBX": "1",
    "RCX": "1"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" },
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
mov bx, ax
and ebx, 1

; Also test that a valid conversion after the 1st is ok as well.
finit
fld1
fistp qword [.dummy]

fstsw ax
mov cx, ax
and ecx, 1
test cl, cl
sete cl

hlt

align 4096
.seventyfive: dd 75
.dummy: dq 0
