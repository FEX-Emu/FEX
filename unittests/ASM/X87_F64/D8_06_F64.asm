%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3fe0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov rax, 0x40000000 ; 2.0
mov [rdx + 8 * 1], rax

fld qword [rdx + 8 * 0]
fdiv dword [rdx + 8 * 1]

fst qword [rdx + 8 * 2]
mov rax, [rdx + 8 * 2]

hlt
