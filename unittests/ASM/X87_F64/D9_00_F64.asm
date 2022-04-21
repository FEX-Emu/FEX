%ifdef CONFIG
{
  "RegData": {
    "RAX": "0x3f800000"
  },
  "X86ReducedPrecision": "1"
}
%endif

mov rdx, 0xe0000000

mov eax, 0x3f800000 ; 1.0
mov [rdx + 8 * 0], eax

fld dword [rdx + 8 * 0]
fst dword [rdx]

xor eax, eax
mov eax, [rdx]

hlt
