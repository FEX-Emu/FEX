%ifdef CONFIG
{
  "RegData": {
    "RAX":  ["0xbff0000000000000"]
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x3ff0000000000000 ; 1.0
mov [rdx + 8 * 0], rax
mov eax, 2
mov [rdx + 8 * 1], eax

fld qword [rdx + 8 * 0]
fisub dword [rdx + 8 * 1]

fst qword [rdx]
mov rax, [rdx]

hlt
