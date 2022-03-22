%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x4090000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov eax, 1024
mov [rdx + 8 * 0], eax
mov eax, -1
mov [rdx + 8 * 0 + 2], eax

fild word [rdx + 8 * 0]
fst qword [rdx + 8 * 0]
mov rax, [rdx + 8 * 0]

hlt
