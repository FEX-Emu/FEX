%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x00007F4100007F41", "0x00007F4100007F41"],
    "XMM1": ["0x00007F4100007F41", "0x00007F4100007F41"],
    "XMM2": ["0x0000FFFF007F0041", "0x0000FFFF007F0041"]
  },
  "MemoryRegions": {
    "0x100000000": "4096"
  }
}
%endif

mov rdx, 0xe0000000

; 16bit signed -> 8bit unsigned (saturated)
; input > 0x7F(SCHAR_MAX, 127) = 0xFF(UCHAR_MAX, 255)
; input < 0x00(Negative) = 0x0

mov rax, 0x0000FFFF007F0041
mov [rdx + 8 * 0], rax
mov rax, 0x0000FFFF007F0041
mov [rdx + 8 * 1], rax

mov rax, 0x0000FFFF007F0041
mov [rdx + 8 * 2], rax
mov rax, 0x0000FFFF007F0041
mov [rdx + 8 * 3], rax

movapd xmm0, [rdx]
packuswb xmm0, [rdx + 8 * 2]

movapd xmm1, [rdx]
movapd xmm2, [rdx + 8 * 2]
packuswb xmm1, xmm2

hlt
