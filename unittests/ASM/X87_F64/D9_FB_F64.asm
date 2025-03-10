%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3fe14a280fb5068c",
    "RBX":  "0x3feaed548f090cee"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif
mov rcx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fsincos

fstp qword [rcx]
mov rax, [rcx]
fstp qword [rcx]
mov rbx, [rcx]

hlt

align 8
data:
  dt 1.0
  dq 0
