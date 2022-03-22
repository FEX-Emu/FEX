%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3F800000",
    "RBX": "0x40000000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f800000 ; 1.0
mov [rdx + 8 * 0], eax
mov eax, 0x40000000 ; 2.0
mov [rdx + 8 * 1], eax
mov eax, 0x0 ; 1.0
mov [rdx + 8 * 2], eax

fld dword [rdx + 8 * 0]
fstp dword [rdx + 8 * 2]
fld dword [rdx + 8 * 1]

mov eax, [rdx + 8 * 2]
fst dword [rdx + 8 * 2]
mov ebx, [rdx + 8 * 2]

hlt
