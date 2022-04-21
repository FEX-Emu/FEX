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
mov eax, -1
mov [rdx + 8 * 1], eax

fld dword [rdx + 8 * 0]

fistp word [rdx + 8 * 1]

fld1

mov eax, 0
mov ax, word [rdx + 8 * 1]

fst qword [rdx + 8 * 0]
mov rbx, [rdx + 8 * 0]

hlt
