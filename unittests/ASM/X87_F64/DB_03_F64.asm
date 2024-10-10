%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x400",
    "RBX": ["0x3ff0000000000000"]
  }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x44800000 ; 1024.0
mov [rdx + 8 * 0], eax
mov eax, 0
mov [rdx + 8 * 1], eax

fld dword [rdx + 8 * 0]

fistp dword [rdx + 8 * 1]

fld1

mov eax, [rdx + 8 * 1]

fst qword [rdx]
mov rbx, [rdx]

hlt
