%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x400",
    "RBX": "0x3ff0000000000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x44800000 ; 1024.0
mov [rdx + 8 * 0], eax
mov rax, -1
mov [rdx + 8 * 1], rax

fld dword [rdx + 8 * 0]

fistp qword [rdx + 8 * 1]

fld1

mov rax, qword [rdx + 8 * 1]

fstp qword [rdx + 8 * 1]

mov rbx, qword [rdx + 8 * 1]

hlt
