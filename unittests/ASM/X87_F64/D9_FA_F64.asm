%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x4010000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rbx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fsqrt

fst qword [rbx]
mov rax, [rbx]

hlt

align 8
data:
  dt 16.0
  dq 0
