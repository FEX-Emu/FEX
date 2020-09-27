%ifdef CONFIG
{
  "RegData": {
    "XMM0": ["0x0", "0x0"],
    "XMM1": ["0xFE02FE02FE02FE02", "0xFE02FE02FE02FE02"],
    "XMM2": ["0x7F7F7F7F7F7F7F7F", "0x7F7F7F7F7F7F7F7F"],
    "XMM3": ["0x7FFF7FFF7FFF7FFF", "0x7FFF7FFF7FFF7FFF"],
    "XMM4": ["0x057306BC07B808B8", "0xBC53BC0EBAE5BA2E"],
    "XMM5": ["0xA473A5BCA6B8A7B8", "0x0553070E07E5092E"]
  }
}
%endif

mov rdx, 0xe0000000

mov rax, 0x0
mov [rdx + 8 * 0], rax
mov [rdx + 8 * 1], rax

mov rax, -1
mov [rdx + 8 * 2], rax
mov [rdx + 8 * 3], rax

mov rax, 0x7F7F7F7F7F7F7F7F
mov [rdx + 8 * 4], rax
mov [rdx + 8 * 5], rax

mov rax, 0x8141824383448445
mov [rdx + 8 * 6], rax
mov rax, 0x21F223F323F424F5
mov [rdx + 8 * 7], rax

mov rax, 0xE251E352E453E554
mov [rdx + 8 * 8], rax
mov rax, 0x71A972A873A774A6
mov [rdx + 8 * 9], rax

; Zero
movaps xmm0, [rdx + 8 * 0]
pmaddubsw xmm0, [rdx + 8 * 0]

; -1
movaps xmm1, [rdx + 8 * 2]
pmaddubsw xmm1, [rdx + 8 * 2]

; 127
movaps xmm2, [rdx + 8 * 4]
pmaddubsw mm2, [rdx + 8 * 4]

; 255 and 127
movaps xmm3, [rdx + 8 * 2]
pmaddubsw xmm3, [rdx + 8 * 4]

; Mixture
movaps xmm4, [rdx + 8 * 6]
pmaddubsw xmm4, [rdx + 8 * 8]

movaps xmm5, [rdx + 8 * 8]
pmaddubsw xmm5, [rdx + 8 * 6]

hlt
