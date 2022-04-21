%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4000000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x4000000000000000 ; 2.0
mov [rdx + 8 * 0], rax

fld qword [rdx + 8 * 0]

fstp qword [rdx]
mov rax, [rdx]

hlt
