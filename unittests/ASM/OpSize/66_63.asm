%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x00807F4100807F41", "0x00FF7F4100FF7F41"],
    "XMM1": ["0x00807F4100807F41", "0x00FF7F4100FF7F41"],
    "XMM2": ["0x0000FFFF007F0041", "0x0000FFFF007F0041"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

; 16bit signed -> 8bit signed (saturated)
; input > 0x7F(SCHAR_MAX, 127) = 0x7F(SCHAR_MAX, 127)
; input < 0x80(-127) = 0x80

mov rax, 0x00008000007F0041
mov [rdx + 8 * 0], rax
mov rax, 0x00008000007F0041
mov [rdx + 8 * 1], rax

mov rax, 0x0000FFFF007F0041
mov [rdx + 8 * 2], rax
mov rax, 0x0000FFFF007F0041
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
packsswb xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
packsswb xmm1, xmm2

hlt
