%ifdef CONFIG
{
  "RegData": {
    "RAX": "0xC090000000000000",
    "RBX": "0xC070000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov eax, -1024
mov [rdx + 8 * 0], eax

fild dword [rdx + 8 * 0]

fstp qword [rdx]
mov rax, [rdx]

xor rbx, rbx
mov bx, -256
mov [rdx + 8 * 0], bx
fild word [rdx + 8 * 0]

fstp qword [rdx]
mov rbx, [rdx]

hlt
