%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0", "0x0"],
    "XMM1": ["0x0101010101010101", "0x0101010101010101"],
    "XMM2": ["0x0101010101010101", "0x0101010101010101"],
    "XMM3": ["0x0100010001010100", "0x0100010001010100"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0000000000000000
mov [rdx + 8 * 0], rax
mov [rdx + 8 * 1], rax

mov rax, 0xFFFFFFFFFFFFFFFF
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax

mov rax, 0x0101010101010101
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

mov rax, 0xFF000100FF01FF00
mov [rdx + 8 * 6], rax
mov [rdx + 8 * 7], rax

; Test with full zero
pabsb xmm0, [rdx + 8 * 0]

; Test with full negative
pabsb xmm1, [rdx + 8 * 2]

; Test with full positive
pabsb xmm2, [rdx + 8 * 4]

; Test a mix
pabsb xmm3, [rdx + 8 * 6]

hlt
