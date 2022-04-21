%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3ff0000000000000",
    "RBX": "0xc01a000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fscale

mov rcx, 0xe0000000
fstp qword [rcx]
mov rax, [rcx]
fstp qword [rcx]
mov rbx, [rcx]

hlt

align 8
data:
  dt 64.0
  dq 0

data2:
  dt -6.5
  dq 0
