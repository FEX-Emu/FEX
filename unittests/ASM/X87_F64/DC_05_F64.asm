%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3ff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fsubr qword [rdx + 8 * 0]

mov rdx, 0xe0000000
fstp qword [rdx]
mov rax, [rdx]

hlt

align 8
data:
  dt 1.0
  dq 0
data2:
  dq 2.0
