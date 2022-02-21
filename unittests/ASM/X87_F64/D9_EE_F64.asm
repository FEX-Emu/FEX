%ifdef CONFIG
{
  "RegData": {
    "RAX": ["0"]
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

fldz

fst qword [rcx]
mov rax, [rcx]

hlt
