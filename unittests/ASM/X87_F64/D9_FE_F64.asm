%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3feaed548f090cee"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rbx, 0xe0000000

lea rdx, [rel data]
fld tword [rdx + 8 * 0]

fsin

fst qword [rbx]
mov rax, [rbx]

hlt

align 8
data:
  dt 1.0
  dq 0
