%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4010000000000000",
    "RBX": "0x4020000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fmul st1, st0

fstp qword [rdx]
mov rax, [rdx]
fstp qword [rdx]
mov rbx, [rdx]

hlt

align 4096
data:
  dt 2.0
  dq 0
data2:
  dt 4.0
  dq 0
