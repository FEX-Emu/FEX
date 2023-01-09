%ifdef CONFIG
{
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" },
  "RegData": {
    "RAX": "0"
  }
}
%endif

mov rbx, 0xe0000000
o32 fstenv [rbx]
mov dword [rbx+4], 0xFFFFFFFF ; set status word to all one
o32 fldenv [rbx]

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

lea rdx, [rel data2]
fld tword [rdx + 8 * 0]

fprem

xor rax, rax
fstsw ax
and rax, 0x400 ; C2 should be set to zero

hlt

align 8
data:
  dt 3.0
  dq 0
data2:
  dt 5.1
  dq 0
