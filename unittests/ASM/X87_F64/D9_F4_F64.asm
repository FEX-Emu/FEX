%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0xbffe000000000000",
    "RBX":  "0x4008000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fxtract

fstp qword [rcx]
mov rax, [rcx]
fst qword [rcx]
mov rbx, [rcx]

hlt

align 8
data:
  dt -15.0
  dq 0
