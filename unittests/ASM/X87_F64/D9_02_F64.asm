%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3F800000"
  },
  "Env": { "FEX_X87REDUCEDPRECISION" : "1" }
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f800000 ; 1.0
mov [rdx + 8 * 0], eax
mov eax, 0x0
mov [rdx + 8 * 1], eax

fld dword [rdx + 8 * 0]
fst dword [rdx + 8 * 1]

mov eax, [rdx + 8 * 1]

hlt
