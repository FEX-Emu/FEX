%ifdef CONFIG
{
  "RegData": {
    "RAX":  "0x3ff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f834241 ; 1.02546
mov [rdx + 8 * 0], eax

fld dword [rdx + 8 * 0]

frndint

fst qword [rdx]
mov rax, [rdx]

hlt
