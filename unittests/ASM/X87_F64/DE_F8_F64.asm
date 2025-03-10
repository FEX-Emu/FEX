%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x4010000000000000",
    "RBX":  "0x3fe0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

; fdivp 2.0, 4.0
; == st1 = 2.0 / 4.0
fdivp st1, st0

lea rdx, [rel data3]
fld tword [rdx + 8 * 0]

fstp qword [rcx]
mov rax, [rcx]

fst qword [rcx]
mov rbx, [rcx]

hlt

align 8
data:
  dt 2.0
  dq 0
data2:
  dt 4.0
  dq 0
data3:
  dt 4.0
  dq 0
