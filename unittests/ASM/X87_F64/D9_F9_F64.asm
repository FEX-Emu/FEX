%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3f800000",
    "RBX": "0x41be320c"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fyl2xp1
fld1

mov rcx, 0xe0000000
fstp dword [rcx]
mov eax, [rcx]
fstp dword [rcx]
mov ebx, [rcx]

hlt

align 8
data:
  dt 15.0
  dq 0

data2:
  dt 2.0
  dq 0
