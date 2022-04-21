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

fild dword [rdx + 8 * 0]

fstp qword [rdx]
mov rax, [rdx]

hlt
