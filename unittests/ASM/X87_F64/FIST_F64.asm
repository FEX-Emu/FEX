%ifdef CONFIG
{
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" },
  "RegData": {
    "RAX": "0xffffffff"
  }
}
%endif

; Test behaviour of overflow
; and storing negative numbers
; to 32-bit registers.

lea rbp, [rel data]
mov rdx, 0xe0000000

fld qword [rbp]
fistp dword [rdx]
mov eax, [rdx]

hlt

align 8
data:
  dq 0xbff0000000000000
