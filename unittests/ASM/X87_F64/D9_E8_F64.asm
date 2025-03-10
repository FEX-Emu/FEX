%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3ff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rcx, 0xe0000000

fld1

fst qword [rcx]
mov rax, [rcx]

hlt
