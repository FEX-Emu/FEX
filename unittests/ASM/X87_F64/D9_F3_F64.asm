%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3ff0000000000000",
    "RBX":  "0x3ff921fb54442d18"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fpatan
fld1

fstp qword [rcx]
mov rax, [rcx]

fstp qword [rcx]
mov rbx, [rcx]

hlt

align 8
data:
  dt 7.0
  dq 0
data2:
  dt 0.0
  dq 0
