%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3ff0000000000000",
    "RBX":  "0xc000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]
fchs

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]
fchs

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
  dt -1.0
  dq 0
